---
layout: post
title:  "实现 Git 目录权限控制功能"
date:   2019-04-01 10:00:00
published: true
categories: git
---

# 前言
 
 Git 与 Subversion 有诸多不同，最核心的一点是前者属于分布式版本控制工具，后者属于集中式版本控制工具。前者的提交行为是离线的，本地的，后者的提交是在线的，需要与远程中央服务器通信，在线创建提交。基于这种现实，Git 和 Subversion 在原生提供的附加功能也存在很大的差别。比如目录权限控制。Git 原生并不支持目录权限控制，而 Subversion 支持。

 ## Subversion 的目录权限控制

 用户接入远程服务器上的 Subversion 存储库通常可以使用 HTTP 协议 SVN 协议以及 SVN+SSH 协议，HTTP 协议本质上是 HTTP 客户端与 Apache httpd 服务器通信，此时，请求由 `mod_dav_svn.so` 模块处理，然后调用 subversion 的核心模块，包括文件系统和存储库模块。使用 HTTP 访问 Subversion 存储库时，可以如下：

 ```shell
 svn co https://llvm.org/svn/llvm-project/llvm/trunk
 ```

 还可以直接检出存储库的子目录：

 ```shell
 svn co https://llvm.org/svn/llvm-project/llvm/trunk/include/
 ```

使用 SVN 协议与 SVN+SSH 协议本质上都是与远程服务器上的 `svnserve` 通信，前者是 svnserve 监听 3690 TCP 端口，后者是在远程服务器上运行 `svnserve -t`，但内部的细节都是一致的。检出代码如下：

 ```shell
 svn co svn://llvm.org/svn/llvm-project/llvm/trunk
 ```

 还可以直接检出存储库的子目录：

 ```shell
 svn co svn://llvm.org/svn/llvm-project/llvm/trunk/include/
 ```

Subversion 的提交是在线的，以 SVN 协议为例，当用户提交代码时，svn 客户端将发送 `commit` 命令消息给 svnserve，然后在这个命令中将修改的文件发送到远程服务器，远程服务器上作出一些列操作，然后创建提交。在这个过程中，只要对文件相对存储库的路径与目录权限规则作出匹配，就可以知道用户是否由能力修改相应文件。

Subversion 的原理也就使其很容易支持细粒度的权限控制，可以精确到文件。svnserve 可以修改 svnserve.conf 实现，而 HTTP 则与 apache mod_dav_svn 模块有关。

如果要禁止用户读取，则在 checkout 的时候禁止检出即可，实现起来并不困难。

## Git 不支持目录权限控制

像 Git 这样的分布式版本控制系统，获得远程存储库的行为叫做 `clone`，常规情况下获得远程存储库的所有数据，但这种方式毕竟需要获得大量数据，因此 Git 还提供浅表克隆，还有 VFSForGit 这样的方式只获得目录结构，不获取文件内容，还有规划中的部分克隆。

在服务器上，要控制存储库的数据按目录粒度提供给用户读取，通常无法通过 Git 客户端实现，这是由 Git 传输协议决定的，Git 在克隆时先要发现引用列表，然后按需求获得引用列表的提交 ID，服务器将这些 Commit 以及相关的 Tree Blob 等等回溯打包发送给客户端。Git 服务器最多能实现的是在引用发现时隐藏特定的引用实现禁止读取，要做到目录级别的禁止读取，这是无法实现的。

分布式版本控制系统的提交是发生在本地的，当人们运行 `git commit -m "message"` 时，就会在本地创建一个提交。无论是 Subversion 还是 Git，一个提交在一个存储库中是唯一的，比如 Subversion 中占据唯一的一个修订号(Revision)，在 Git 中则是唯一的 commitID (目前为 SHA1，未来计划转变为 SHA-256)。在本地提交后，目录结构已经确定，推送到服务器并不能改变其目录结构。这就意味着，Git 不支持目录级别粒度的写入控制。

综合来看，Git 是完全不支持目录权限控制的。

## Git 实现目录权限控制的一种方法

**Git 的目录级别粒度的禁止读取并不在本文的研究范围，本文所述的目录权限控制仅指 '目录的只读'。**

在设计此功能前，我们需要了解 Git 的一些原理，比如，目录权限控制发生在服务端，因此，处理的时机为 Git 推送代码代远程存储库时，一旦发现推送中修改了只读文件（目录），则拒绝推送。在不修改 Git 源码的基础上要实现这些功能通常可以使用服务端钩子，服务器上的主要的钩子有三种，`pre-receive`，`update`,`post-receive` ，相关钩子的原理在 [Git 原生钩子的深度优化](https://forcemz.net/git/2017/11/22/GitNativeHookDepthOptimization/) 一文中有介绍，要实现本节所述功能首先得排除 `post-receive` 钩子，此钩子的成功与否不会影响引用的更新，所以并不合适，而 `pre-receive` 钩子目前使用了 [环境隔离： Quarantine Environment](https://git-scm.com/docs/git-receive-pack#_quarantine_environment) 机制，目前 libgit2 并不支持此特性，所以要实现目录只读也不能直接使用 `pre-receive` 钩子。

最终，只有 `update` 钩子可用。

在实现目录权限检测时，我们应当先初始化规则，在 [git-analyze](https://gitee.com/oscstudio/git-analyze) 项目中，我添加了一个 `update` 钩子作为目录权限检测的原型，规则文件如下：

```json
{
  "master":{
    "dirs":[
      "lib/",
      "build/"
    ],
    "regex":[
      "build$"
    ]
  }
}

```

规则按分支名划分，其中 `.all` 表示所有分支，`.all` 不能被用于 git 分支或者引用名称，因此代表所有的分支。规则有目录级别匹配和正则匹配，`lib` 表示 `lib` 文件或者目录不可被修改。而 `regex` 则表示目录符合正则表达式的路径不可被修改。


## 反思

虽然本文讲述了实现 Git 目录权限控制的一个实现机制，但笔者比较反对过分使用 Git 服务端钩子实现过多额外的功能。这些功能大多数都能可以通过规范的协作方式实现相同的目的。