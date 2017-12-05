---
layout: post
title:  "基于 Asio 的服务器平滑重启方案"
date:   2017-11-27 10:00:00
published: true
categories: server
---

## 前言

与客户端程序不同的是，服务端程序要尽可能的长时间运行，故障时能够自动恢复，并且更新时不能影响服务正在处理的请求。这就产生了平滑重启的功能需求。实际上，网络上比较流行的 HTTP 服务器 Nginx 就支持平滑重启。而 apache httpd 同样支持。码云分布式以后，很多功能被分解成一个个的服务，比如存储机器上的 git-srv, git-diamond 等等，为了平台的稳定运行，也需要支持平滑重启。

## 平滑重启的原理

平滑重启的原理实际上比较简单，即，当接收到平滑重启后，旧的服务关闭监听套接字，处理完所有请求后便退出。而在旧的服务关闭监听套接字后，启动一个新的服务监听套接字，处理新的请求。

在 POSIX 系统中，我们可以使用 **Signal** 通知服务平滑重启，在 Windows 中，可以使用 **Windows Event Object** 通知服务平滑重启。

平滑重启并没有什么技术难点，但实现的过程中却需要避开一些陷阱。

## Asio 平滑重启

我开发的网络服务大多基于 `Boost.Asio` 实现，线上的服务器也大多运行 Ubuntu 14.04 或者 Ubuntu 16.04。最初的时候，码云的服务器并没有支持平滑重启，这并不是我没有尝试实现平滑重启功能，当时为了加快部署，支持使用 `apt-get` 安装了 `Boost.Asio`，而不是源码编译 `Boost.Asio`。经由 apt-get 安装的库往往比最新版本老很多（可能是 5~6 个版本），也容易出现一些特定的 bug。于是我也没将那些服务有支持平滑重启。后来在我将所有的 Boost.Asio 替换成独立的 `Asio` 就支持平滑重启了。

下面是一个例子：

```c++
///

#ifndef _NET_SERVE_HPP
#define _NET_SERVE_HPP
#include <mutex>
#include <thread> ///std::thread::hardware_concurrency()
#include <utility>
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif
#include <asio.hpp>

namespace net {

class Context {
public:
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;
  explicit Context(std::size_t iocsize) : next_(0) {
    if (iocsize == 0) {
      throw std::runtime_error("io_context size eqaul 0");
    }
    for (std::size_t i = 0; i < iocsize; i++) {
      io_context_t io_context(new asio::io_context);
      work_t work(new io_work_t(asio::make_work_guard(*io_context)));
      io_contexts_.push_back(io_context);
      works_.push_back(work);
    }
  }
  void Execute() {
    std::vector<std::shared_ptr<std::thread>> threads;
    for (auto &ctx : io_contexts_) {
      std::shared_ptr<std::thread> thread(
          new std::thread([ctx]() { ctx->run(); }));
      threads.push_back(thread);
    }
    for (auto &t : threads) {
      t->join();
    }
  }
  void Exit() {
    for (auto &ioc : io_contexts_) {
      ioc->stop();
    }
  }
  void Clear() {
    //
    works_.clear();
  }

  asio::io_context &Next() {
    auto &ioc = *io_contexts_[next_];
    ++next_;
    if (next_ == io_contexts_.size()) {
      next_ = 0;
    }
    return ioc;
  }

private:
  using io_context_t = std::shared_ptr<asio::io_context>;
  using io_work_t = asio::executor_work_guard<asio::io_context::executor_type>;
  using work_t = std::shared_ptr<io_work_t>;
  std::vector<io_context_t> io_contexts_;
  std::vector<work_t> works_;
  std::size_t next_;
};

template <typename ServeVars, typename ServeSession> class Serve {
public:
  Serve(std::size_t n) : context_(n), acceptor_(context_.Next()) {}
  void SetVars(ServeVars &&vars) {
    ///
    vars_ = std::move(vars);
  }
  void Acceptorclose(std::error_code &ec) {
    /// close acceptor
    acceptor_.close(ec);
  }
  void DelayExit() {
    std::error_code ec;
    acceptor_.close(ec);
    context_.Clear();
  }
  void Exit() { context_.Exit(); }
  int ListenAndServe(const std::string &addr, int port) {
    std::error_code ec;
    asio::ip::tcp::endpoint ep(asio::ip::make_address(addr, ec),
                               static_cast<unsigned int>(port));
    if (ec) {
      fprintf(stderr, "make_address: %s\n", ec.message().c_str());
      return -1;
    }
    acceptor_.open(ep.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
/*
When the server needs to be supported for graceful restart, the old sockets
need to be closed, and when invoke the fork and exec* syscall, the file
descriptor is inherited, so the FD_CLOEXEC flag should be set so that the
listener socket can be closed in time.
*/
#ifndef _WIN32
    int oflag = fcntl(acceptor_.native_handle(), F_GETFD, 0);
    if (oflag >= 0) {
      fcntl(acceptor_.native_handle(), F_SETFD, oflag | FD_CLOEXEC);
    }
#endif
    for (int i = 0; i < 10; i++) {
      acceptor_.bind(ep, ec);
      if (!ec) {
        break;
      }
      fprintf(stderr, "bind: %s:%d %s\n", addr.c_str(), port,
              ec.message().c_str());
    }
    if (ec) {
      fprintf(stderr, "bind address failed, exit\n");
      return 1;
    }

    acceptor_.listen();
    Accept();
    context_.Execute();
    return 0;
  }

private:
  void Accept() {
    acceptor_.async_accept(
        context_.Next(),
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
          if (ec) {
            if (socket.is_open()) {
            }
            return;
          }
          std::make_shared<ServeSession>(std::move(socket), vars_)->run();
          Accept();
        });
  }

  Context context_;
  asio::ip::tcp::acceptor acceptor_;
  ServeVars vars_;
};

}; // namespace net

#endif

```

实际上就是在收到信号调用 `DelayExit` ，而在 `DelayExit` 中，实际上是先关闭 acceptor 然后清理 `executor_work_guard` 这样一来处理完 io_context 的任务后，线程就会退出，当所有线程退出后，主线程也就退出了。

完整的例子在：[oscstudio/utilcode@example/asio_server](https://gitee.com/oscstudio/utilcode/tree/master/example/asio_server)

这里值得注意的是，在 Unix 的世界中，子进程会集成父进程的文件描述符，为了避免 acceptor 被继承，需要设置监听套接字的文件描述符为 **FD_CLOEXEC**。如果在平滑重启时，子进程启动时继承了监听套接字的文件描述符，就会导致平滑重启失败，新服务进程无法绑定监听套接字。

## 最后

Utilcode 有个完整的 Linux 方案，支持平滑重启，故障自动重启，等等功能。有兴趣的可以阅读源码。