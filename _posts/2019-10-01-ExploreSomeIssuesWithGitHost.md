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

Git 使用文件快照记录文件变更，当对象存储到松散文件目录时，每一次大小不变的文件修改相当于存储库中增加特定文件的大小，Git 使用 zlib [deflate](https://en.wikipedia.org/wiki/DEFLATE) 压缩对象，对象头包括对象类型，原始大小。

引用来自的 [https://github.com/facebook/zstd](https://github.com/facebook/zstd/tree/2164a130f353e64a1e89c8e60f36cf2498ab1eea#benchmarks) 基准测试，我们发现 zlib(deflate) 在接近的压缩比，无论是压缩还是解压都不如 `brotli`/`zstd`：

| Compressor name         | Ratio | Compression| Decompress.|
| ---------------         | ------| -----------| ---------- |
| zstd 1.4.0 -1       | 2.884 |   530 MB/s |  1360 MB/s |
| **zlib 1.2.11 -1**          | 2.743 |   110 MB/s |   440 MB/s |
| brotli 1.0.7 -0         | 2.701 |   430 MB/s |   470 MB/s |
| quicklz 1.5.0 -1        | 2.238 |   600 MB/s |   800 MB/s |
| lzo1x 2.09 -1           | 2.106 |   680 MB/s |   950 MB/s |
| lz4 1.8.3               | 2.101 |   800 MB/s |  4220 MB/s |
| snappy 1.1.4            | 2.073 |   580 MB/s |  2020 MB/s |
| lzf 3.6 -1              | 2.077 |   440 MB/s |   930 MB/s |



## 不同伸缩性的 Git 代码托管平台

### 基于内置工具搭建 Git 代码托管服务

Git 最初由 Linus Torvalds 开发用来取代 BitKeeper 作为 Linux 内核源码的版本控制工具，所以 Git 一直和 Linux 内核源码托管在同一个服务器上。官方地址是：[https://git.kernel.org/](https://git.kernel.org/)。在 git.kernel.org 上，Git 代码托管功能是由 git 内置的工具实现的。用户使用 HTTPS 协议访问 [https://git.kernel.org/](https://git.kernel.org/) 时，Nginx 会以 CGI 的方式将浏览器的请求转发到 [GitWeb](https://git.wiki.kernel.org/index.php/Gitweb)。GitWeb 是一个使用 Perl 编写的 CGI 程序，为用户提供简单的 git 在线交互图形界面。GitWeb 的源码地址可以在 [Github Git 镜像](https://github.com/git/git/blob/master/gitweb/gitweb.perl) 中查看。GitWeb 界面比较不够精美，相比于 Github 这样的代码托管平台，功能寥寥无几。当用户需要使用 HTTP/HTTPS 协议拉取推送源码时，Nginx 会以 CGI的方式将请求转发给 [git-http-backend](https://git-scm.com/docs/git-http-backend) 处理。git-http-backend 是 **Git Over HTTP** 的服务端实现。当用户 GIT 协议 (`git://`) 在 git.kernel.org 上拉取源码是，请求会被 [git-daemon](https://git-scm.com/docs/git-daemon) 处理。[git-daemon](https://git-scm.com/docs/git-daemon) 默默的监听 9418 端口，静静的等待 git 客户端的访问。

使用 Git 内置的 GitWeb/git-http-backend/git-daemon，我们能够搭建一个简易的 Git 代码托管服务器，但这里没有 SSH 协议支持。而实现 SSH 协议支持也非常简单，只需要在服务器上运行 `sshd` (OpenSSH)，并允许命令 `git-upload-pack/git-receive-pack/git-upload-archive` 命令的运行，对于 SSH 协议的验证，我们则可以使用 `authorized_keys` 机制，将需要允许的用户的 SSH 公钥添加到 `authorized_keys` 文件。

[https://git.kernel.org/](https://git.kernel.org/) 网站托管了 Linux 内核源码，驱动，文档等大概有 1000+ 个存储库，较大的存储库如 Linux 内核源码磁盘占用大概是 2GB，因此在理想情况下，一块 2TB 磁盘的服务器便可支撑 [https://git.kernel.org/](https://git.kernel.org/)  这个网站的运行（实际情况则并不是如此，由于 Linux 内核的流行，git.kernel.org 的请求将比较多，对硬件的需求将更高一点）。基于 Git 内置功能搭建的代码托管服务，麻雀虽小五脏俱全，不过回过头来说，这样的代码托管服务功能有限，可伸缩性和扩展性不佳。

### 小型的 Git 代码托管平台

当用户需要搭建一个几人到几十几百人规模的 Git 代码托管服务，通常有非常多的选择，下面是几个目前仍然比较活跃的小型 Git 代码托管平台。

|名称|平台|语言|技术概述|
|---|---|---|---|
|[Bonobo Git Server](https://github.com/jakubgarfield/Bonobo-Git-Server)|Windows Only|C#|基于 .Net Famework 4.6（迁移到 .Net Core 的建议在 2017 年便被提出，但截至目前仍为迁移到 .Net Core）。使用 [LibGit2Sharp](https://github.com/libgit2/libgit2sharp/) 操作存储库，但版本较老，不支持 SSH 协议访问。|
|[Gogs](https://github.com/gogs/gogs)|Cross Platform|Golang|基于 Golang 编写，Web 读写 Git 存储库由 [git-module](https://github.com/gogs/git-module) 封装 Git 命令实现，SSH 由 Golang [crypto/ssh](https://github.com/golang/crypto/tree/master/ssh) 提供，支持多种数据库，是一个极简的代码托管平台，可以在 Raspberry Pi 上运行|
|[Gitea](https://github.com/go-gitea/gitea)|Cross Platform|Golang|是 Gogs 的开源分叉，Web 读写 Git 存储库使用了 [src-d/go-git](https://github.com/src-d/go-git)，使用 [gliderlabs/ssh](https://github.com/gliderlabs/ssh) 提供 SSH 接入功能，支持多种数据库，可以在 Raspberry Pi 上运行。|
|[GitBucket](https://github.com/gitbucket/gitbucket)|Cross Platform|Scala/Java|使用 [Apache Mina SSHD](https://github.com/apache/mina-sshd) 实现 SSH 功能。Mina SSHD 还专门针对 JGit 实现了一个 [sshd-git](https://github.com/apache/mina-sshd/tree/master/sshd-git) 模块，但 GitBucket 是直接使用 JGit 的 [`transport`](https://github.com/eclipse/jgit/blob/master/org.eclipse.jgit/src/org/eclipse/jgit/transport) 相关类。Eclipse JGit 主要由 Google 开发者参与贡献。|

除了上述定位为代码托管平台的服务，还有像 [Phabricator](https://www.phacility.com/) 这样的 Web 软件也提供 Git 代码托管功能，但 Phabricator 的重点更多是缺陷追踪，代码审核。LLVM [https://reviews.llvm.org/](https://reviews.llvm.org/) 和 libssh  [https://bugs.libssh.org/](https://bugs.libssh.org/) 就是基于 Phabricator。

### 云服务级别的 Git 代码托管平台

随着用户规模和存储库规模的增长，达到一定级别后，上述代码托管平台往往变得力不从心，而下面的代码托管平台却深耕于此，能够支撑巨大规模的用户量和存储库数量。

[Github](https://github.com) 是全球最大的代码托管平台，目前 Github [官方数据](https://github.com/about)显示注册用户数量为 4000万，项目数量为 1亿。Github 网站主要的技术是 [Ruby on Rails](https://rubyonrails.org/) 内部进程名为 `github-unicorn`，最近他们将其升级到了 [Rails 6.0](https://github.blog/2019-09-09-running-github-on-rails-6-0/)。Github 使用 Spokes 负责文件系统上存储库的复制，同步和备份。Github 之前使用 libssh 开发 Git SSH 服务器，目前的 SSH 服务器的标识为 `babeld-*`，但不确定 babeld 是否依然基于 libssh。Git 验证服务为 `github-gitauth`。Github 的大多数服务都是闭源的，因此分析 Github 的技术内幕通常是 Github 官方的一些技术博客， 当然也可以分析 `Github Enterprise` 去窥测 Github 内幕。

关于 Github Spokes 的大致原理可以阅读 [Introducing DGit](https://githubengineering.com/introducing-dgit/) 和 [Building resilience in Spokes](https://github.blog/2016-09-07-building-resilience-in-spokes/)。

在开发 [Gitaly](https://gitlab.com/gitlab-org/gitaly) 之后， [Gitlab](https://gitlab.com) 摆脱了 NFS 的禁锢，在平台的伸缩性方面得到了巨大的提升。要知道 Gitlab 使用 Gitaly 的原因可以阅读 [The road to Gitaly v1.0](https://about.gitlab.com/2018/09/12/the-road-to-gitaly-1-0/)。Gitaly 使用 RPC 将存储服务器上的 git 命令包转成前端服务机器上的 git 命令，并为 gitlab 服务提供存储库的读写。 Gitlab 的 SSH 功能仍然由 OpenSSH 提供，而一些静态资源，文件下载，附件等功能则由 Golang 编写的 [gitlab-workhorse](https://gitlab.com/gitlab-org/gitlab-workhorse) 实现，gitlab-workhorse 需要与 Gitaly 通信。

[Bitbucket](https://bitbucket.org) 是 [Atlassian](https://www.atlassian.com) 开发的代码托管平台，与 Github/Gitlab 不同，Bitbucket 还提供了原生 Mercurial 支持，不过最近，Bitbucket 宣布要逐步关闭 Mercurial 的支持。Atlassian 还开发了 Jira/Sourcetree 这样著名的软件，Bitbucket 源码没有开发，推测主要使用 Java 技术栈（这个从一次 Bitbucket VFSForGit 安装包分析可得）。

[Gitee](https://gitee.com) 是目前国内最大的代码托管平台之一，早在 2015 年便开始了分布式改造，并编写了一系列服务实现分布式架构，编写了 Nginx 路由模块实现动态路由，基于 libssh 开发了 Basalt v1 SSH 服务器，基于 Golang 开发了 Basalt v2 SSH 服务器，还开发了 git-srv 智能服务后端，brzox Git HTTP/Archive 服务。以及 git-diamond git 协议内部传输服务等等。Gitee 最初代码基于 Gitlab，几年之间已经与 Gitlab 有了很大的差异，现在 Gitee 已经逐步将一些功能从 gitlab 中剥离，实现云平台的微服务，比如目前的 git/svn/hook 验证服务是基于 Golang 编写的 banjo。Gitee 使用比较紧张的硬件资源支撑了大量的接入，这与财大气粗的 Github 有着很大的不同。

## Git 代码托管平台服务实现
<!--SSH/HTTP/GIT, LFS, GitVFS....-->
Git 代码托管平台的基本服务应该包括浏览器接入支持和 git 客户端接入支持，前者需要平台开发网页提供若干服务供用户访问。后者需要支持 git 客户端推拉代码。通过网站访问存储库意味着 HTTP 服务需要通过一定的途径读写存储库，在 GitWeb 中，这通常使用 git 命令实现，比如使用 `git tree` 查看 `tree`，使用 `git archive` 打包文件等等。在 Gogs 中，使用的 [git-module](https://github.com/gogs/git-module) 同样使用了命令读写存储库。而 Gogs 的分叉 Gitea 则使用的是 [src-d/go-git](https://github.com/src-d/go-git) 读写存储库。实际上我们常常有那种感觉，使用命令行可能会比直接调用 API 慢，并且错误难以处理，这通常是对的。比如我们查看 `HEAD` 对应的引用，使用命令我们可以运行 `git symbolic-ref HEAD`，运行这个命令我们需要 fork 出一个进程，fork 成功后马上在子进程中执行 exec git symbolic-ref，为了读取 git symbolic-ref 的输出，我们还需要创建几对 Pipe，并检测 git symbolic-ref 的退出值。而使用 libgit2 API 我们只需要调用 `git_repository_open`,`git_reference_open`,`git_reference_symbolic_target` 即可拿到对应的引用。而对于服务程序而言，fork-exec 的代价可能不小。当然你也可以直接使用 `open("/path/to/.git/HEAD")` 然后解析 HEAD 对应的引用。GitBucket 使用 JGit 读写存储库，Gitlab 曾经历了 Grit (Grit 部分命令部分 Git 纯 Ruby 实现，Github 曾经使用)。后来的 Rugged，到现在 Gitaly 的纯命令 + Ruby Repository（Gitlab 现在的架构我对其保留意见，至少 IO 复制将增加多次）。Github 目前使用 Rugged 读写存储库，当然一些更多的细节因为没有源码不得而知。Gitee 目前使用 Rugged，但一部分 libgit2 实现不佳的则直接采用 git 命令实现。

实现 Git Over HTTP，Gitlab 最初采用的是 Grack, 受限于 `unicorn`，Grack 并发有限，而使用 Golang 开发的 Gogs，Gitea 则使用 Golang 原生 HTTP 库编写，这要比 Grack 好一些。目前 Gitee 的 Brzox 服务也是使用 Golang 编写。

实现 Git Over SSH，Gitlab 目前依然使用的是 OpenSSH，而不像 Github/BitBucket/Gitee 直接编写 SSH 服务器，直接编写 SSH 服务器可以禁用 SSH 登录，自定义错误消息，简化验证流程，减少网络拷贝。而 Gogs/Gitea 在虽然使用 Golang SSH，但实现的 SSH 服务器并不是直接运行命令而是增加了中间命令 serv，这种做法也会增加拷贝，这可能是设计不足的妥协吧。

## Git 代码托管平台的伸缩性
<!--存储库分片，大存储库，大文件，分布式文件系统-->

## Git 代码托管平台的附加功能
<!--保护分支，只读目录，安全，两步验证...-->


## 文件服务
<!--附件下载，发布文件，Archive 下载-->

### 附件，Release

### Archive
