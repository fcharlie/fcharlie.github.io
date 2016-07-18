---
layout: post
title:  "Git 传输协议实现"
date:   2016-07-16 20:00:00
published: true
categories: git
---

# GIT 传输协议实现

在 GIT 的三种主流传输协议 HTTP SSH GIT 中，GIT 协议是最少被使用的协议（也就是 URL 以 `git://` 开始的协议）。
这是由于 git 协议的权限控制几乎没有，要么全部可读，要么全部可写，要么全部可读写。所以对于代码托管平台来说，
git 协议的目的仅仅是为了支持 公开项目的只读访问。

在 git 的各种传输协议中，git 协议无疑是最高效的，HTTP 受限于 HTTP 的特性，传输过程需要构造 HTTP 请求和响应。
如果是 HTTPS 还涉及到加密解密。另外 HTTP 的超时设置，以及包体大小限制都会影响用户体验。

而 SSH 协议的性能问题主要集中在加密解密上。当然相对于用户的信息安全来说，这些代价都是可以接受。

git 协议实际上相当于 SSH 无加密无验证，也就无从谈起权限控制，但实际上代码托管平台内部的一些同步服务，如果使用
git 协议实现，将会得到很大的性能提升。

## 传输协议规范

git 协议的技术文档可以从 git 源码目录的 `Documentation/technical` 找到，即 [Packfile transfer protocols](https://github.com/git/git/blob/master/Documentation/technical/pack-protocol.txt)
创建 TCP 连接后，git 客户端率先发送请求体，请求格式基于 BNF 的描述如下：

    git-proto-request = request-command SP pathname NUL [ host-parameter NUL ]
    request-command   = "git-upload-pack" / "git-receive-pack" / "git-upload-archive"   ; case sensitive
    pathname          = *( %x01-ff ) ; exclude NUL
    host-parameter    = "host=" hostname [ ":" port ]

一个例子如下：

`0033git-upload-pack /project.git\0host=myserver.com\0`

在 git 的协议中，pkt-line 是非常有意思的设计，行前 4 个字节表示整个行长，长度包括其前 4 字节，
但是有个特例，`0000` 其代表行长为 0，但其自身长度是 4。

下面是一个关于请求的结构体：

{% highlight cpp %}
struct GitRequest{
    std::string command;
    std::string path;
    std::string host;
};
{% endhighlight %}

git 有自带的 git-daemon 实现，这个服务程序监听 9418 端口，在接收到客户端的请求后，先要判断 command 是
否是被允许的，git 协议中有 fetch 和 push 以及 archive 之类的操作，分别对应的服务器上的命令是 git-upload-pack
git-receive-pack git-upload-archive。HTTP 只会支持前两种，SSH 会支持三种，而 代码托管平台的 git 通常支持的
是 git-upload-pack git-upload-archive。

当不允许的命令被接入时需要发送错误信息给客户端，这个信息在不同的 git-daemon 实现中也不一样，大体
如下所示。

`001bERR service not enabled`

git-daemon 将对请求路径进行转换，以期得到在服务器上的绝对路径，同时可以判断路径是否存在，不存在时
可以给客户端发送 **Repository Not Found**。而 host 可能时域名也可能时 ip 地址，当然也可以包括端口。
服务器可以在这里做进一步的限制，出于安全考虑应当考虑到请求是可以被伪造的。

客户端发送请求过去后，服务器将启动相应的命令，将命令标准错误和标准输出的内容发送给客户端，将客户端
传输过来的数据写入到命令的标准输入中来。

在请求体中，命令为 git-upload-pack /project.git 在服务器上运行时，就会类似 

>git-upload-pack ${RepositoriesRoot}/project.git

出于限制连接的目的，一般还会添加 `--timeout=60` 这样的参数。timeout 并不是整个操作过程的超时。

与 HTTP 不同的是，命令中没有参数 `--stateless-rpc` 和 `--advertise-refs`  两个命令都存在时，只输出
存储库的引用列表与 capabilities ，当只有 --stateless-rpc 时，等待客户端的数据，然后解析发送数据
给客户端。

## 进程输入输出的读写

在 C 语言中，有 popen 函数，可以创建一个进程,并将进程的标准输出或标准输入创建成一个文件指针，即 `FILE*`
其他可以使用 C 函数的语言很多也提供了类似的实现，比如 Ruby，基于 Ruby 的 git HTTP 服务器 grack 正是使用
的 popen，相比与其他语言改造的 popen，C 语言中 popen 存在了一些缺陷，比如无法同时读写，如果要输出标准
错误，需要在命令参数中额外的将标准错误重定向到标准输出。

在 musl libc 的中，popen 的实现如下：

{% highlight cpp %}
FILE *popen(const char *cmd, const char *mode)
{
	int p[2], op, e;
	pid_t pid;
	FILE *f;
	posix_spawn_file_actions_t fa;

	if (*mode == 'r') {
		op = 0;
	} else if (*mode == 'w') {
		op = 1;
	} else {
		errno = EINVAL;
		return 0;
	}
	
	if (pipe2(p, O_CLOEXEC)) return NULL;
	f = fdopen(p[op], mode);
	if (!f) {
		__syscall(SYS_close, p[0]);
		__syscall(SYS_close, p[1]);
		return NULL;
	}
	FLOCK(f);

	/* If the child's end of the pipe happens to already be on the final
	 * fd number to which it will be assigned (either 0 or 1), it must
	 * be moved to a different fd. Otherwise, there is no safe way to
	 * remove the close-on-exec flag in the child without also creating
	 * a file descriptor leak race condition in the parent. */
	if (p[1-op] == 1-op) {
		int tmp = fcntl(1-op, F_DUPFD_CLOEXEC, 0);
		if (tmp < 0) {
			e = errno;
			goto fail;
		}
		__syscall(SYS_close, p[1-op]);
		p[1-op] = tmp;
	}

	e = ENOMEM;
	if (!posix_spawn_file_actions_init(&fa)) {
		if (!posix_spawn_file_actions_adddup2(&fa, p[1-op], 1-op)) {
			if (!(e = posix_spawn(&pid, "/bin/sh", &fa, 0,
			    (char *[]){ "sh", "-c", (char *)cmd, 0 }, __environ))) {
				posix_spawn_file_actions_destroy(&fa);
				f->pipe_pid = pid;
				if (!strchr(mode, 'e'))
					fcntl(p[op], F_SETFD, 0);
				__syscall(SYS_close, p[1-op]);
				FUNLOCK(f);
				return f;
			}
		}
		posix_spawn_file_actions_destroy(&fa);
	}
fail:
	fclose(f);
	__syscall(SYS_close, p[1-op]);

	errno = e;
	return 0;
}
{% endhighlight %}

在 Windows Visual C++ 中，popen 源码在 `C:\Program Files (x86)\Windows Kits\10\Source\${SDKVersion}\ucrt\conio\popen.cpp` ，
按照 MSDN 文档说明，Windows 32 GUI 程序，即 subsystem 是 Windows 的程序，使用 popen 可能导致程序无限失去响应。

所以在笔者实现 git-daemon 及其他 git 服务器时，都不会使用 popen 这个函数。

为了支持跨平台和简化编程，笔者在实现 svn 代理服务器时就使用了 Boost Asio 库，后来也用 Asio 实现过一个 git 远程命令服务，
每一个客户端与服务器连接后，服务器启动程序，需要创建 3 条管道，分别是 子进程的标准输入 输出 错误，即 stdout stdin stderr，
然后注册读写异步事件，将子进程的输出与错误写入到 socket 发送出去，读取 socket 写入到子进程的标准输入中。

在 POSIX 系统中，boost 有一个文件描述符类 `boost::asio::posix::stream_descriptor` 这个类不能是常规文件，以前用 go 做 HTTP 前端
没注意就 coredump 掉。

在 Windows 系统中，boost 有文件句柄类 `boost::asio::windows::stream_handle` 此处的文件应当支持随机读取，比如命名管道（当然
在 Windows 系统的，匿名管道实际上也是命名管道的一种特例实现）。

以上两种类都支持 `async_read` `async_write` ，所以可以很方便的实现异步的读取。


上面的做法，唯一的缺陷是性能并不是非常高，代码逻辑也比较复杂，当然好处是，错误异常可控一些。

在 Linux 网络通信中，类似与 git 协议这样读取子进程输入输出的服务程序的传统做法是，将 子进程的 IO 重定向到 socket，
值得注意的是 boost 中 socket 是异步非阻塞的，然而，git 命令的标准输入标准错误标准输出都是同步的，所以在 fork 子进程之
前，需要将 socket 设置为同步阻塞，当 fork 失败时，要设置回来。

`socket_.native_non_blocking(false);`

另外，为了记录子进程是否异常退出，需要注册信号 SIGCHLD 并且使用 waitpid 函数去等待，boost 就有 `boost::asio::signal_set::async_wait`
当然，如果你开发这样一个服务，会发现，频繁的启动子进程，响应信号，管理连接，这些操作才是性能的短板。

一般而言，Windows 平台的 IO 并不能重定向到 socket，实际上，你如果使用 IOCP 也可以达到相应的效率。还有，Windows 的 socket API WSASocket WSADuplicateSocket 
复制句柄 DuplicateHandle ，这些可以好好利用。

## 其他

对于非代码托管平台的从业者来说，上面的相关内容可能显得无足轻重，不过，网络编程都是殊途同归，最后核心理念都是类似的。关于 git-daemon
如果笔者有时间会实现一个跨平台的简易版并开源。