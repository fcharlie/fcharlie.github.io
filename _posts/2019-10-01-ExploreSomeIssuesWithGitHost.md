---
layout: post
title:  "探讨 GIT 代码托管平台的若干问题"
date:   2019-10-01 10:00:00
published: false
categories: git
---

## 关于 GIT

版本控制软件种类繁多，维基百科上有据可循的最早的版本控制系统是 1972 年贝尔实验室的 [Source Code Control System](https://en.wikipedia.org/wiki/Source_Code_Control_System)。1986 诞生了 [Concurrent Versions System(CVS)](https://en.wikipedia.org/wiki/Concurrent_Versions_System), 目前已经很少软件使用 CVS 进行版本管理，但 OpenBSD 仍然使用。2000 年，CollabNet 创建了 Subversion 项目，2009年，Subversion 被 Apache 基金会接受成为 [Apache Subversion](https://en.wikipedia.org/wiki/Apache_Subversion)。2005 年 [Linus Torvalds](https://en.wikipedia.org/wiki/Git) 创建了 Git，至今已经有 14 年。

与 CVS/Subversion 这种集中式版本控制系统不同的是，Git 的存储库数据会被存储在本地，提交也是发生在本地，远程更像一个镜像。而 CVS/Subversion 的提交都是在线的。这就是 Git 作为分布式版本控制系统的核心特性。

Git 的源码通常托管在 [https://git.kernel.org/pub/scm/git/git.git/](https://git.kernel.org/pub/scm/git/git.git/)，而 `git.kernel.org` 实际上也是使用 git 内置的 git web 进行访问。Github 上也有只读镜像 [https://github.com/git/git](https://github.com/git/git)。[https://git-scm.com](https://git-scm.com) 源码也是托管在 Github 上的。通常给 git 提交 PR 需要注册 [public-inbox.org](https://public-inbox.org) 邮件列表发送补丁，但也可以在 Github 上给 [https://github.com/gitgitgadget/git](https://github.com/gitgitgadget/git) 提交 PR，这是微软 Git 开发者开发的机器人，能够帮助开发者更便利的提交补丁，笔者就有一个[补丁](https://github.com/gitgitgadget/git/pull/69)使用 gitgitgadget 提交。

Git 与远程存储库之间的传输协议有 HTTP, GIT，SSH. 在 [《Pro Git - 2nd Edition》4.1 Git on the Server - The Protocols](https://git-scm.com/book/en/v2/Git-on-the-Server-The-Protocols) 中有介绍。其中 HTTP 协议包括哑协议和智能协议，由于哑协议是只读协议，目前大多数代码托管平台均不再提供支持。HTTP 智能协议和 GIT 协议，SSH 协议类似，都是特定几组 客户端/服务端 git 命令之间的输入输出数据传输和交换。

|Action|Client Side|Server Side|
|---|---|---|
|fetch/clone|git fetch-pack|git upload-pack|
|push|git send-pack|git receive-pack|

## 内置的 Git 托管