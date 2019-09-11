---
layout: post
title:  "探讨 Git 代码托管平台的若干问题 - 2019 版"
date:   2019-10-01 10:00:00 +0800
published: false
categories: git
---

## 关于 Git

版本控制软件种类繁多，维基百科收录的最早的版本控制系统是 1972 年贝尔实验室开发的 [Source Code Control System](https://en.wikipedia.org/wiki/Source_Code_Control_System)。1986 年 [Concurrent Versions System(CVS)](https://en.wikipedia.org/wiki/Concurrent_Versions_System) 诞生，CVS 曾非常流行，但今时用之寥寥无几，不过 OpenBSD 仍在使用 CVS。2000 年 CollabNet 创建了 Subversion 项目，2009年，Subversion 被 Apache 基金会接受成为顶级项目并被命名为 [Apache Subversion](https://en.wikipedia.org/wiki/Apache_Subversion)。2005 年 [Linus Torvalds](https://en.wikipedia.org/wiki/Git) 创建了 Git，2007 Github 诞生后，Git 随着 Github 的发展愈发流行，14 年间，Git 成为了最流行的版本控制系统，无论是 Windows 还是 Linux 或是 Android，MySQL 等等大型软件都使用 git 进行版本控制。纵观版本控制系统流行史，前有 CVS 后有 SVN，今日 Git 更风流。俱往矣，数风流人物，还看今朝，版本控制系统莫不如斯。

与 CVS/Subversion 这种集中式版本控制系统不同的是，Git 的存储库数据会被存储在本地，提交也是发生在本地，远程可以看作是本地存储库的一个镜像。而 CVS/Subversion 的提交都是在线的。这就是分布式版本控制系统的核心特征。（理解这一问题的关联在于区分工作树 `worktree` 和存储库 `repository`。）

Git 的源码托管在 [https://git.kernel.org/pub/scm/git/git.git/](https://git.kernel.org/pub/scm/git/git.git/)，Github 上也有只读镜像 [https://github.com/git/git](https://github.com/git/git)。Git 主页 [https://git-scm.com](https://git-scm.com) 网页源码托管在 Github。通常给 git 提交 PR 需要注册 [public-inbox.org](https://public-inbox.org) 邮件列表然后发送补丁，但你可以在 Github 上给 [https://github.com/gitgitgadget/git](https://github.com/gitgitgadget/git) 提交 PR，这简化了给 Git 贡献的难度。[gitgitgadget](https://github.com/gitgitgadget/gitgitgadget) 是微软开发者 [Johannes Schindelin](https://github.com/dscho) 开发的机器人，用于帮助开发者更简便的向 Git 提交补丁。笔者就有一个[补丁](https://github.com/gitgitgadget/git/pull/69)使用 gitgitgadget 提交。[Johannes Schindelin](https://github.com/dscho) 此人也是 [git-for-windows](https://github.com/git-for-windows/git) 的维护者。Git 的维护者则是 Google 的开发者 [Junio C Hamano](https://github.com/gitster)。

Git 与远程存储库之间的传输协议有 HTTP, GIT(`git://`)，SSH. 在 [《Pro Git - 2nd Edition》4.1 Git on the Server - The Protocols](https://git-scm.com/book/en/v2/Git-on-the-Server-The-Protocols) 中有介绍。其中 HTTP 协议包括哑协议和智能协议，由于哑协议是只读协议，目前大多数代码托管平台均不再提供支持。HTTP 智能协议和 GIT 协议，SSH 协议类似，都是特定几组 客户端/服务端 git 命令之间的输入输出数据传输和交换。如此简单的协议使得实现基于 Git 的代码托管平台尤为容易，针对不同的用户和存储库数量规模，Git 的选择也都比 Subversion，Mercurial 有更多的选择。

**Git 克隆/拉取/推送命令对**：

|Action|Client Side|Server Side|
|---|---|---|
|fetch/clone|git fetch-pack|git upload-pack|
|push|git send-pack|git receive-pack|

## 不同伸缩性的 Git 代码托管平台

### 基于内置工具搭建 Git 代码托管服务

Git 最初由 Linus Torvalds 开发用来取代 BitKeeper 作为 Linux 内核源码的版本控制工具，所以 Git 一直和 Linux 内核源码托管在同一个服务器上。官方地址是：[https://git.kernel.org/](https://git.kernel.org/)。在 git.kernel.org 上，Git 代码托管功能是由 git 内置的工具实现的。用户使用 HTTPS 协议访问 [https://git.kernel.org/](https://git.kernel.org/) 时，Nginx 会以 CGI 的方式将浏览器的请求转发到 [GitWeb](https://git.wiki.kernel.org/index.php/Gitweb)。GitWeb 是一个使用 Perl 编写的 CGI 程序，为用户提供简单的 git 在线交互图形界面。GitWeb 的源码地址可以在 [Github Git 镜像](https://github.com/git/git/blob/master/gitweb/gitweb.perl) 中查看。GitWeb 界面比较不够精美，相比于 Github 这样的代码托管平台，功能寥寥无几。当用户需要使用 HTTP/HTTPS 协议拉取推送源码时，Nginx 会以 CGI的方式将请求转发给 [git-http-backend](https://git-scm.com/docs/git-http-backend) 处理。git-http-backend 是 **Git Over HTTP** 的服务端实现。当用户 GIT 协议 (`git://`) 在 git.kernel.org 上拉取源码是，请求会被 [git-daemon](https://git-scm.com/docs/git-daemon) 处理。[git-daemon](https://git-scm.com/docs/git-daemon) 默默的监听 9418 端口，静静的等待 git 客户端的访问。

使用 Git 内置的 GitWeb/git-http-backend/git-daemon，我们能够搭建一个建议的 Git 代码托管服务器，但这里没有 SSH 协议支持。而实现 SSH 协议支持也非常简单，只需要在服务器上运行 `sshd` (OpenSSH)，并允许命令 `git-upload-pack/git-receive-pack/git-upload-archive` 命令的运行，对于 SSH 协议的验证，我们则可以使用 `authorized_keys` 机制，将需要允许的用户的 SSH 公钥添加到 `authorized_keys` 文件。

[https://git.kernel.org/](https://git.kernel.org/) 包括驱动，文档等项目大概有 1千个存储库，Linux 内核存储库磁盘占用大概是 2GB，属于提交较大的存储库，理想情况下，一块 2TB 磁盘的服务器便可支撑 [https://git.kernel.org/](https://git.kernel.org/)  运行（实际情况下并不会这样，由于 Linux 内核需要提供下载等情况，git.kernel.org 服务器的配置要好的多。）。基于 Git 内置功能搭建的代码托管服务，麻雀虽小五脏俱全，但回过头来说，功能也比较少，服务的可伸缩性不佳。

### 开箱即用的 Git 代码托管平台

|名称|语言|技术概述|
|---|---|---|
|[Bonobo Git Server](https://github.com/jakubgarfield/Bonobo-Git-Server)|C#||
|[Gitea](https://github.com/go-gitea/gitea)|Golang||
|[Gogs](https://github.com/gogs/gogs)|Golang||
|[GitBucket](https://github.com/gitbucket/gitbucket)|Scala/Java|SSH 功能使用 Apache SSHD|


### 云服务级别的 Git 代码托管平台

[Github](https://github.com)

[Introducing DGit](https://githubengineering.com/introducing-dgit/)
[Building resilience in Spokes](https://github.blog/2016-09-07-building-resilience-in-spokes/)

[Gitlab](https://gitlab.com)
[Bitbucket](https://bitbucket.org)
[Gitee](https://gitee.com)

## Git 代码托管平台的伸缩性
<!--存储库分片，大存储库，大文件，分布式文件系统-->

## Git 代码托管平台的附加功能
<!--保护分支，只读目录，安全，两步验证...-->

## Git 代码托管平台服务实现
<!--SSH/HTTP/GIT, LFS, GitVFS....-->


## 文件服务
<!--附件下载，发布文件，Archive 下载-->

### 附件，Release

### Archive
