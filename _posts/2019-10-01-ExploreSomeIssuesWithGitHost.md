---
layout: post
title:  "探讨 Git 代码托管平台的若干问题"
date:   2019-10-01 10:00:00
published: false
categories: git
---

## 关于 Git

版本控制软件种类繁多，维基百科上有据可循的最早的版本控制系统是 1972 年贝尔实验室的 [Source Code Control System](https://en.wikipedia.org/wiki/Source_Code_Control_System)。1986 诞生了 [Concurrent Versions System(CVS)](https://en.wikipedia.org/wiki/Concurrent_Versions_System), 目前已经很少软件使用 CVS 进行版本管理，但 OpenBSD 仍然使用。2000 年，CollabNet 创建了 Subversion 项目，2009年，Subversion 被 Apache 基金会接受成为 [Apache Subversion](https://en.wikipedia.org/wiki/Apache_Subversion)。2005 年 [Linus Torvalds](https://en.wikipedia.org/wiki/Git) 创建了 Git，至今已经有 14 年。

与 CVS/Subversion 这种集中式版本控制系统不同的是，Git 的存储库数据会被存储在本地，提交也是发生在本地，远程更像一个镜像。而 CVS/Subversion 的提交都是在线的。这就是 Git 作为分布式版本控制系统的核心特性。

Git 的源码通常托管在 [https://git.kernel.org/pub/scm/git/git.git/](https://git.kernel.org/pub/scm/git/git.git/)，而 `git.kernel.org` 实际上也是使用 git 内置的 git web 进行访问。Github 上也有只读镜像 [https://github.com/git/git](https://github.com/git/git)。[https://git-scm.com](https://git-scm.com) 源码也是托管在 Github 上的。通常给 git 提交 PR 需要注册 [public-inbox.org](https://public-inbox.org) 邮件列表发送补丁，但也可以在 Github 上给 [https://github.com/gitgitgadget/git](https://github.com/gitgitgadget/git) 提交 PR，这是微软 Git 开发者开发的机器人，能够帮助开发者更便利的提交补丁，笔者就有一个[补丁](https://github.com/gitgitgadget/git/pull/69)使用 gitgitgadget 提交。

Git 与远程存储库之间的传输协议有 HTTP, GIT(`git://`)，SSH. 在 [《Pro Git - 2nd Edition》4.1 Git on the Server - The Protocols](https://git-scm.com/book/en/v2/Git-on-the-Server-The-Protocols) 中有介绍。其中 HTTP 协议包括哑协议和智能协议，由于哑协议是只读协议，目前大多数代码托管平台均不再提供支持。HTTP 智能协议和 GIT 协议，SSH 协议类似，都是特定几组 客户端/服务端 git 命令之间的输入输出数据传输和交换。如此简单的协议使得实现基于 Git 的代码托管平台尤为容易，针对不同的用户和存储库数量规模，Git 的选择也都比 Subversion，Mercurial 有更多的选择。

**Git 克隆/拉取/推送命令对**：

|Action|Client Side|Server Side|
|---|---|---|
|fetch/clone|git fetch-pack|git upload-pack|
|push|git send-pack|git receive-pack|

## 不同规模层次的 Git 代码托管参考

### 内置的托管支持

Git 最初由 Linus Torvalds 开发用来取代 BitKeeper 管理 Linux 内核源码，Linux 内核源码和 Git 源码的官方地址是：[https://git.kernel.org/](https://git.kernel.org/)。在 git.kernel.org 上，Git 代码托管功能都是由 git 内置的工具实现。用户访问 [https://git.kernel.org/](https://git.kernel.org/) 时，Nginx 会以 CGI 的方式将浏览器的请求转发到 [GitWeb](https://git.wiki.kernel.org/index.php/Gitweb) GitWeb 是一个使用 Perl 编写的 CGI 程序，为用户提供简单的 git 在线交互图形界面。GitWeb 的源码地址可以在 [Github Git 镜像](https://github.com/git/git/blob/master/gitweb/gitweb.perl) 中查看，GitWeb 比较简陋，通过浏览器访问此地址就可以看到 GitWeb 的界面：[https://git.kernel.org/pub/scm/git/git.git/](https://git.kernel.org/pub/scm/git/git.git/)。而在 git.kernel.org 用户需要通过 `git-http/https` 方式获取源码时，Nginx 会以 CGI的方式将请求转发给 [git-http-backend](https://git-scm.com/docs/git-http-backend) 处理，git-http-backend 是 **Git Over HTTP** 的服务端实现。如果用户需要使用 GIT 协议 (`git://`)，在 git.kernel.org 上，[git-daemon](https://git-scm.com/docs/git-daemon) 正在监听 9418 端口，默默的等待 git 客户端的访问。如果要实现 Git Over SSH 接入支持，则需要运行 OpenSSH sshd 并允许 git-upload-pack/git-receive-pack/git-upload-archive 命令，在 `authorized_keys` 文件中添加需要被允许的用户的公钥即可。

使用 Git 内置的工具实现 Git 代码托管平台虽然容易，但功能却十分简陋。

### 小型 Git 代码托管平台

[Bonobo Git Server](https://github.com/jakubgarfield/Bonobo-Git-Server)
[Gitea](https://github.com/go-gitea/gitea)
[Gogs](https://github.com/gogs/gogs)
[GitBucket](https://github.com/gitbucket/gitbucket)

### 巨大规模 Git 代码托管平台

[Github](https://github.com)

[Introducing DGit](https://githubengineering.com/introducing-dgit/)
[Building resilience in Spokes](https://github.blog/2016-09-07-building-resilience-in-spokes/)

[Gitlab](https://gitlab.com)
[Bitbucket](https://bitbucket.org)
[Gitee](https://gitee.com)

## Git 代码托管平台的伸缩性


## 大文件大存储库的限制


## Git 代码托管平台服务实现

## 文件服务

### 附件，Release

### Archive
