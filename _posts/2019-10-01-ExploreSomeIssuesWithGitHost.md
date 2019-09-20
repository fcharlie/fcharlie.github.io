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

Git 的源码托管在 [git.kernel.org](https://git.kernel.org/pub/scm/git/git.git/) 上，Github 上也有只读镜像 [github.com/git/git](https://github.com/git/git)。Git 主页 [https://git-scm.com](https://git-scm.com) 的网页源码则托管在 Github 上。通常给 git 提交 PR 需要注册 [public-inbox.org](https://public-inbox.org) 邮件列表，然后发送补丁。者通常比较麻烦，好在有微软开发者[Johannes Schindelin](https://github.com/dscho) 使用 TypeScript 开发 [gitgitgadget](https://github.com/gitgitgadget/gitgitgadget) ，当你在 Github 上像 [gitgitgadget/git](https://github.com/gitgitgadget/git)  提交 PR 时，gitgitgadget 会将你的 PR 发送到 public-inbox，一旦补丁被 git 维护者接受，gitgitgadget 则会关闭那个 PR。gitgitgadget 简化了给 git 贡献代码的难度，省去了注册 Inbox 的麻烦，这年头开发者大多都有 Github 帐号。我就使用 gitgitgadget 给 git 提交了一个[补丁](https://github.com/gitgitgadget/git/pull/69)用于支持 HTTP/2。

[Johannes Schindelin](https://github.com/dscho) 此人也是 [git-for-windows](https://github.com/git-for-windows/git) 的维护者。
Git 的维护者则是 Google 的开发者 [Junio C Hamano](https://github.com/gitster)。大多数 Git 开发者来自于 Google/Microsoft（包括 Github）。libgit2 的开发者主要来自 Microsoft（包括 Github）。而 JGit 的开发者则主要来自 Google。已故 JGit 的创始人 Shawn Pearce 还开发了著名的 [Gerrit Code Review](https://www.gerritcodereview.com/)。这些开发者的无私奉献才能使我们用上这么优秀的版本控制系统，感谢他们的付出。

Git 与远程存储库之间的传输协议有 HTTP, GIT(`git://`)，SSH. 在 [《Pro Git - 2nd Edition》4.1 Git on the Server - The Protocols](https://git-scm.com/book/en/v2/Git-on-the-Server-The-Protocols) 中有介绍。其中 HTTP 协议包括哑协议和智能协议，由于哑协议是只读协议，目前大多数代码托管平台均不再提供支持。HTTP 智能协议和 GIT 协议，SSH 协议类似，都是特定几组 客户端/服务端 git 命令之间的输入输出数据传输和交换。Git 传输协议较为简单，以智能传输协议 v1 为例，基本的 `fetch/push` 流程如下：

Git 拉取流程：

![Fetch Flow](https://github.com/fcharlie/pagesimage/raw/master/images/git-fetch-flow.png)

Git 推送流程：

![Push Flow](https://github.com/fcharlie/pagesimage/raw/master/images/git-push-flow.png)

虽然在 2018 年 5 月，git 推出了 [`Wire Protocol`](https://opensource.googleblog.com/2018/05/introducing-git-protocol-version-2.html)（即 Git v2 协议），增加了 Git 协议的复杂性，但在服务器上支持 git 协议（包括 v2 协议）仍然只需要在服务器上运行 git-upload-pack/git-receive-pack。这使得开发者很容易实现对 git 协议的支持。正因为 Git 协议表征的简单，所以针对不同的用户和存储库数量规模，Git 也都比 Subversion，Mercurial 有更多的选择。

Git 使用文件快照记录文件变更，当对象存储到松散文件目录时，每一次大小不变的文件修改相当于存储库中增加特定文件的大小，Git 使用 zlib [deflate](https://en.wikipedia.org/wiki/DEFLATE) 压缩对象，对象头包括对象类型，原始大小。基于快照的方式使得 Git 在提交代码，检出文件时都比较高效，但存储库的占用缺比较高。但运行 `git gc` 时，Git 会将松散的对象打包到 pack 文件中，这个时候会使用特定的机制存储一部分文件的 `OFS_DELTA`，这样就能节省一部分空间。

zlib（deflate） 压缩算法通常来说除了没有版权限制，无论是压缩比还是速度，CPU 使用率都不是一个最佳的选择，引用来自的 [https://github.com/facebook/zstd](https://github.com/facebook/zstd/tree/2164a130f353e64a1e89c8e60f36cf2498ab1eea#benchmarks) 基准测试，zlib 看起来必后起之秀 `brotli`/`zstd` 差多了：

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

当开发者要将 git 集成到其他软件或者系统中时，可以通过命令行调用 git 命令捕获输出，也可以使用 libgit2/JGit 等库。

libgit2 最初是由 Shawn Pearce 创建了[初始 commit](https://github.com/libgit2/libgit2/tree/c15648cbd059b92c177586ab1701a167222c7681)。目前主要维护者来自微软。libgit2 提供一些基础的 API，功能基本上是完整的，除了一部分实现性能没有 git 那么好，其他方面令人满意，并且有多种语言绑定，包括 C++/D/Golang/Ruby/.NET/Node.js/Perl/Perl6/Ruby/Rust 等等。Gitee 原生钩子就使用了 libgit2，Gitee-gitlab 项目使用了 rugged。

JGit 也是有 Shawn Pearce 创建的，目前属于 Eclipse 基金会，运行在 JVM 上，国内腾讯的工峰的 TGit 也是使用的 JGit。

在 [Git Rev News 第48期](https://git.github.io/rev_news/2019/02/27/edition-48/)，编辑推荐了 [gitbase](https://github.com/src-d/gitbase) 通过 SQL 的方式查询 git 存储库，这个工具基于 [src-d/go-git](https://github.com/src-d/go-git)，go-git 是纯 Golang 实现的，如果基于 Golang 的项目需要简单的读写存储库，可以使用 go-git。与 libgit2 的 Golang 绑定 git2go 相比，不需要使用 CGO。 

当然还有一些其他的 git 实现，大多是实验性的，不建议用于生产环境，比如基于 Rust 的 [git-rs](https://github.com/chrisdickinson/git-rs)。

## 不同伸缩性的 Git 代码托管平台

### 基于内置工具搭建 Git 代码托管服务

Git 最初由 Linus Torvalds 开发用来取代 BitKeeper 作为 Linux 内核源码的版本控制工具，所以 Git 一直和 Linux 内核源码托管在同一个服务器上。官方地址是：[https://git.kernel.org/](https://git.kernel.org/)。在 git.kernel.org 上，Git 代码托管功能是由 git 内置的工具实现的。用户使用 HTTPS 协议访问 [https://git.kernel.org/](https://git.kernel.org/) 时，Nginx 会以 CGI 的方式将浏览器的请求转发到 [GitWeb](https://git.wiki.kernel.org/index.php/Gitweb)。GitWeb 是一个使用 Perl 编写的 CGI 程序，为用户提供简单的 git 在线交互图形界面。GitWeb 的源码地址可以在 [Github Git 镜像](https://github.com/git/git/blob/master/gitweb/gitweb.perl) 中查看。GitWeb 界面比较不够精美，相比于 Github 这样的代码托管平台，功能寥寥无几。当用户需要使用 HTTP/HTTPS 协议拉取推送源码时，Nginx 会以 CGI的方式将请求转发给 [git-http-backend](https://git-scm.com/docs/git-http-backend) 处理。git-http-backend 是 **Git Over HTTP** 的服务端实现。当用户 GIT 协议 (`git://`) 在 git.kernel.org 上拉取源码是，请求会被 [git-daemon](https://git-scm.com/docs/git-daemon) 处理。[git-daemon](https://git-scm.com/docs/git-daemon) 默默的监听 9418 端口，静静的等待 git 客户端的访问。

使用 Git 内置的 GitWeb/git-http-backend/git-daemon，我们能够搭建一个简易的 Git 代码托管服务器，但这里没有 SSH 协议支持。而实现 SSH 协议支持也非常简单，只需要在服务器上运行 `sshd` (OpenSSH)，并允许命令 `git-upload-pack/git-receive-pack/git-upload-archive` 命令的运行，对于 SSH 协议的验证，我们则可以使用 `authorized_keys` 机制，将需要允许的用户的 SSH 公钥添加到 `authorized_keys` 文件。

[https://git.kernel.org/](https://git.kernel.org/) 网站托管了 Linux 内核源码，驱动，文档等大概有 1000 多个存储库，较大的存储库比如 Linux 内核源码磁盘占用大概是 2GB，因此在理想情况下，一块 2TB 磁盘的服务器便可支撑 [https://git.kernel.org/](https://git.kernel.org/)  这个网站的运行（实际情况则并不是如此，由于 Linux 内核的流行，git.kernel.org 的请求将比较多，对硬件的需求将更高一点）。基于 Git 内置功能搭建的代码托管服务，麻雀虽小五脏俱全，不过回过头来说，这样的代码托管服务功能有限，可伸缩性和扩展性不佳。

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

[Gitee](https://gitee.com) 是目前国内最大的代码托管平台之一，早在 2015 年便开始了分布式改造，并编写了一系列服务实现分布式架构，编写了 Nginx 路由模块实现动态路由，基于 libssh 开发了 Basalt v1 SSH 服务器，基于 Golang 开发了 Basalt v2 SSH 服务器，还开发了 git-srv 智能服务后端，brzox Git HTTP/Archive 服务。以及 git-diamond git 协议内部传输服务等等。Gitee 最初代码基于 Gitlab，几年之间已经与 Gitlab 有了很大的差异，现在 Gitee 已经逐步将一些功能从 gitlab 中剥离，实现云平台的微服务，比如目前的 git/svn/hook 验证服务是基于 Golang 编写的 banjo。Gitee 需要以有限的硬件实现更多的用户接入，所以在服务的设计上更倾向于提供资源使用率，对一些比较容易造成计算资源紧张的服务进行降级。

## Git 代码托管平台服务实现
<!--SSH/HTTP/GIT, LFS, GitVFS....-->
Git 代码托管平台的基本服务应该包括浏览器接入支持和 git 客户端接入支持，前者需要平台开发网页提供若干服务供用户访问。后者需要支持 git 客户端推拉代码。通过网站访问存储库意味着 HTTP 服务需要通过一定的途径读写存储库，在 GitWeb 中，这通常使用 git 命令实现，比如使用 `git tree` 查看 `tree`，使用 `git archive` 打包文件等等。在 Gogs 中，使用的 [git-module](https://github.com/gogs/git-module) 同样使用了命令读写存储库。而 Gogs 的分叉 Gitea 则使用的是 [src-d/go-git](https://github.com/src-d/go-git) 读写存储库。实际上我们常常有那种感觉，使用命令行可能会比直接调用 API 慢，并且错误难以处理，这通常是对的。比如我们查看 `HEAD` 对应的引用，使用命令我们可以运行 `git symbolic-ref HEAD`，运行这个命令我们需要 fork 出一个进程，fork 成功后马上在子进程中执行 exec git symbolic-ref，为了读取 git symbolic-ref 的输出，我们还需要创建几对 Pipe，并检测 git symbolic-ref 的退出值。而使用 libgit2 API 我们只需要调用 `git_repository_open`,`git_reference_open`,`git_reference_symbolic_target` 即可拿到对应的引用。而对于服务程序而言，fork-exec 的代价可能不小。当然你也可以直接使用 `open("/path/to/.git/HEAD")` 然后解析 HEAD 对应的引用。GitBucket 使用 JGit 读写存储库，Gitlab 曾经历了 Grit (Grit 部分命令部分 Git 纯 Ruby 实现，Github 曾经使用)。后来的 Rugged，到现在 Gitaly 的纯命令 + Ruby Repository（Gitlab 现在的架构我对其保留意见，至少 IO 复制将增加多次）。Github 目前使用 Rugged 读写存储库，当然一些更多的细节因为没有源码不得而知。Gitee 目前使用 Rugged，但一部分 libgit2 实现不佳的则直接采用 git 命令实现。

实现 Git Over HTTP，Gitlab 最初采用了 Grack, 运行在 `unicorn` 中的 Grack 并发有限且容易影响 Web 访问（即 Git 请求较多时，Web 拒绝服务），而基于 Golang 开发的 Gogs，Gitea 使用 Golang 原生 HTTP 库编写 Git HTTP Server 功能，这要比 Grack 好要好很多，Golang HTTP 模型能够支撑更多的并发。目前 Gitee 的 Git HTTP Server Brzox 也是使用 Golang 编写。

实现 Git Over SSH，Gitlab 目前依然使用的是 OpenSSH，而不像 Github/BitBucket/Gitee 直接编写 SSH 服务器，直接编写 SSH 服务器可以禁用 SSH 登录，自定义错误消息，简化验证流程，减少数据拷贝。Github 早先是基于 libssh 编写的 SSH Server, 目前不得而知。BitBucket 技术上偏向 Java, 则有可能使用 Apache Mina SSHD, GitBucket 使用 Apache Mina SSHD + JGit 实现 Git Over SSH 功能。而 Gogs/Gitea 在虽然使用 Golang crypto/ssh 编写了 SSH 服务，但在实现时仍然使用了中间命令，这就导致数据拷贝次数的增加，观测 Gogs/Gitea 的各种服务实现，这可能是设计不足的妥协吧。

实现 Git Over TCP （git:// 协议）也非常简单，但 Git 协议并不提供验证机制，Git 代码托管平台提不提供 Git 协议支持也无关紧要，但 Git 协议无需加密，协议简单，作为平台内部传输服务倒是可以，目前 Gitee 使用 C++ Asio 编写 git-diamond 支持内部同步，企业存储库备份等功能。

## Git 代码托管平台的伸缩性
<!--存储库分片，分布式文件系统-->

伸缩性是 Git 代码托管能否支撑成千上万用户/存储库的重要指标。像 Gogs/Gitea 这样的代码托管系统尽量认为自身运行在单一服务器上，因此这类 Git 代码托管平台伸缩性非常有限，当然如果使用 NFS/Ceph 这类分布式文件系统能够在单一服务器上支持更多的存储库，但 NFS/Ceph 这种分布式系统的做为 Git 代码托管系统的存储层，除了分布式文件系统带来的性能下降，还会带来内网带宽过高等更多的问题。

我们以使用 NFS 挂载实现伸缩性的平台和 Gitee 分布式模型 git 请求 对比，I/O 细节简化如下：

NFS I/O 细节：

![NFS](https://github.com/fcharlie/pagesimage/raw/master/images/git-on-nfs.png)

Gitee Basalt I/O 细节：

![Basalt](https://github.com/fcharlie/pagesimage/raw/master/images/gitee-distributed.png)

计算机是质朴的，流程的增加往往需要更多的计算资源，与 Basalt-GitSrv 相比，NFS 的 I/O 拷贝要多一些，排除 Git 协议影响我们可能会认为 Basalt 的机制要比 NFS 更节省 I/O。如果考虑到 Git 协议的影响，我们应该确信如此，git 推送或者拉取都需要耗费大量的 CPU 计算资源，而在 NFS 模型中，计算全部都是发生在前端服务器，当请求数量较多时，前端服务器则容易出现 CPU 竞争的局面，这将非常影响服务器性能，另外，对于 NFS 这样的文件系统，读写 Git 松散对象都是不得力的。另外，由于 NFS 的缓存机制，负载较高时会出现 `master.lock` 这样的锁定情况，导致用户使用异常。而对于 Basalt，git 则是在存储服务器上直接操作存储库，打包压缩，解压等对 CPU 需求较高的活动也在存储服务器上，这样意味着，CPU 计算被摊薄到存储服务器上了，另外 basalt-gitsrv 中间传输的是打包后的数据，这与 NFS 读写多个文件相比，网络数据量实际上是下降的。

Gitee 作为国内最早的 Git 代码托管平台之一，最开始使用 NFS 实现伸缩性，随着用户规模增长很快出现了上述所有 NFS 容易遇到的问题，后来尝试切换到 Ceph，git 松散对象给其致命一击，上线便宣告失败，出现了严重的宕机事故，数据被毁，只能从备份恢复。后来迁移到分布式架构后基本稳定运行至今（这种方案基本上增加机器即可，前端负载高加前端，存储满了加存储）。

Github 目前有大约 1亿个项目，我们假设 Github 上存储库大小平均为 10MB，目前 Github 存储库使用三副本机制，大概需要的磁盘容量为 2861 TB，按照硬盘出厂的规则（1000GB=1TB）,则是需要最小 3PB。这么大的磁盘容量并不是一个标准服务器能够提供的，按照目前企业级硬盘容量较大的每个 16TB, 则需要硬盘大概 188 块。你能想象到这样大的规模能够简单的运行在分布式文件系统上吗？目前的技术基本上不太现实。

实现 Git 代码托管平台的可伸缩性重要的是实现资源的分片，最开始 Gitee 分布式时使用的是基于用户（namespace）的资源分片，也就是存储库所在的机器与 namespace 所属的机器像匹配，这实际上是一种先入为主的设计，在使用 NFS 挂载的时代，Gitee 的存储库就是按照 namespace 的前两个字母分片存储到不同服务器上，挂载到前端服务器上。因此，基于 namespace 的分片带来了一些问题，比如用户转移存储库可能需要跨机器，fork 存储库也可能需要跨机器，这就无法实现高效的轻量级 fork 功能。从去年开始迁移到基于存储库的分片，基于存储库分片基本上可以解决这些问题，但由于历史原因，轻量级 fork 等功能道阻且长。

资源的分片和请求的路由相伴而生，将存储库存储到不同服务器上后，则需要在这些服务器上实现对应的服务支持前端的请求，而前端也需要实现特定的路由机制，关于 Gitee 的路由机制架构，可以参考相关演讲或者博客。Gitee 存储服务器上使用了 git-srv 作为 Git 传输协议后端服务，而 Github 则使用了 DGit/Spokes，Gitlab 使用了 Gitaly。不同平台的技术各有侧重，比如 Gitlab Gitaly 侧重兼容旧的 OpenSSH，而 Gitee 的 Basalt-GitSrv 针对实际情况优化，与 Gitaly 相比要少一次 I/O 拷贝。 Gitee 目前不足之处是存没有完全剥离 Web(基于早期 Gitlab 发展而来)，而 Gitaly 也有 Ruby 代码实现存储库读写（这块代码用 Golang 封装 I/O 多了一次拷贝）。与 Gitee 类似，Gitea 还有另一种方案，即将 Gitea 部署到多个服务器上共用 DB 支持分片，比如 [gitea.com](https://gitea.com) 便是这样的平台，但 gitea.com 似乎并不支持 SSH，因此并不能算有效的分片。

前端服务器的扩展性实际上要比存储服务器好，前端服务器的迁移一般不需要像存储服务器那样转移存储库，服务也一般更简单。

存储库分片之后还是无法避免特定存储库请求过多的问题，Github 的解决方案是使用三副本读写分离的 Spokes 机制，这一方案最多能够提供 3倍于单一服务器的并发读取能力，但不支持并发写入存储库。三副本机制需要解决分布式系统常见的一致性问题，引入并发写入可能会带来更多的数据冲突，破坏一致性，因此 Github 完全禁止并发写入存储库副本（即同时有不同的写存储库请求）。Gitlab 没有实现这样的技术，BitBucket 则没有披露相关资讯，Gitee 受限与硬件限制和开发资源限制，也没有实施。

github-dfs：

![DGIT](https://github.blog/wp-content/uploads/2016/04/architecture.png)

除了存储库的分片，代码托管平台还需要考虑数据库 SQL/NoSQL 能否支撑大规模并发，数据库的分布式集群是一个比较成熟的方案，而 Redis 最新的版本也支持集群，因此数据库的伸缩性一般不会存在太大问题，增加机器搭建集群即可。选择关系性数据库时还需要考虑许可证，数据库自身的功能等，比如 Gitlab 目前已经放弃对 MySQL 的支持，而是选择了 PostgreSQL，不过 Gitlab 的选择对于其他代码托管平台来说，也只能算作**仅作参考**。MariaDB 是 MySQL 的分支版本，随着 MySQL 被 Oracle 收购，开源社区渐渐丧失了对 MySQL 的兴趣，虽然 MySQL 8.0 发布已经很久，但采用 MySQL 8.0 的发行版本寥寥无几，很多还停留在 MySQL 5.X，有些发行版还使用 [mariadb-connector-c](https://github.com/MariaDB/mariadb-connector-c) 替代 `libmysqlclient` 作为数据库连接器，使用 MySQL 的平台很容易迁移到 MariaDB 而不用修改客户端数据库连接代码 ，MariaDB 支持线程池，而 MySQL 仅在企业版中支持线程池。一些 MariaDB 与 MySQL 的对比这里不赘述了。Gogs/Gitea 还支持使用 SQLite，但其使用 SQLite 时，基本上是放弃了伸缩性，不过目前有一个使用 Raft+libuv 实现的分布式 SQLite [canonical/dqlite](https://github.com/canonical/dqlite)，可以尝试一下。Redis 一般可以作为 Web 缓存或者任务队列的中间件，目前 Redis 虽然支持集群，但就单机 Redis 而言，由于它是单线程的服务，在将内存数据持久化到磁盘是还是可能出现超时，并且单线程服务性能终究有限，在 Github 上，[KeyDB](https://github.com/JohnSully/KeyDB) 是官方 Redis 的另一个选择，KeyDB 是 Redis 的分支，完全兼容 Redis 协议，KeyDB 支持多线程，有更好的内存效率和高吞吐量。

## Git 代码托管平台的增强功能
<!--大存储库，大文件，保护分支，只读目录，安全，两步验证/WebAuthn (https://github.com/duo-labs/webauthn)...-->

除了支持用户通过 Git 协议或者通过网页方式读写远程存储库，代码托管平台一般还需要提供一些与开发相关的功能增强用户体验，这些功能在不同平台之间的对比时显得非常重要。

### 缺陷追踪

[SQLite3](https://www.sqlite.org) 使用 2007 年诞生的版本控制系统 [Fossil](https://fossil-scm.org) 托管其源码，与前辈 Git 相比，它集成了 Bug 追踪，Wiki，论坛和技术报告。而对于 Git 来说，这些则需要 Git 代码托管平台自己实现，当然现在无论是 Github/Gitee/Gitlab/BitBucket 还是 Gogs/Gitea 都提供了 `Issues`这样的机制方便开发者第一时间报告软件缺陷或者提出功能建议。`Issues` 这样的功能实现主要在于让用户参与其中，也就是用的人多了，才有人气。而 Github 的 `Issues` 相比其他平台是最活跃的。另外 Github 还提供依赖警报功能（详情可以阅读 [Introducing security alerts on GitHub](https://github.blog/2017-11-16-introducing-security-alerts-on-github/)），另外 Github 还收购了 Semmle 代码分析用于连续漏洞检测 (参考：[Securing software, together](https://github.blog/2019-09-18-securing-software-together/)），这也是其他 Git 代码托管平台可以借鉴的功能。

### 持续集成

在微软收购 Github之后，Github 有了更充足的财力在给用户提供持续集成功能，今年以来 Github 推出了 [GitHub Package Registry](https://github.com/features/package-registry) 和 [Github Actions](https://github.com/features/actions) （相关文章：[GitHub Actions now supports CI/CD, free for public repositories](https://github.blog/2019-08-08-github-actions-now-supports-ci-cd/)，[Introducing GitHub Package Registry](https://github.blog/2019-05-10-introducing-github-package-registry/)），在推出 Github Actions 之前，开发者在 Github 上大多是通过第三方软件实现 CI/CD 功能，比如我的 [M2Team/Privexec](https://github.com/M2Team/Privexec) 就使用 Appveroy。Windows Terminal 则使用 Azure Pipeline。平台的生态繁荣得益于第三方的支持，而对于其他平台，这些 CI/CD 支持就没有这么大的力度了，这也促使其他代码托管平台的 API 趋向 Github 化，WebHook 也逐步趋同，Github 形成了事实上的标准。比如 Gitee 的 APIv5 就保持了对 Github 的兼容。

### 保护分支和只读目录

Gitee 很早就实现了类似 SVN 的保护分支功能，而 Github 目前也同样支持保护分支。实现保护分支的途径很很多条，通常通过服务端 Git 钩子实现，我曾写过 [《服务端 Git 钩子的妙用》](https://forcemz.net/git/2019/07/31/GNKServerSide/) 介绍了如何通过钩子实现保护分支功能。

只读目录功能同样可以通过钩子实现，如果不通过钩子，而是在 git 命令中实现，则要面临修改 git 源码，需要投入大量人力维护的情况。《服务端 Git 钩子的妙用》和 [《实现 Git 目录权限控制》](https://forcemz.net/git/2019/04/01/ImplementGitDirPermissionControl/)对实现目录权限控制有详细介绍。

### 其他版本控制系统接入

对于代码托管平台的附加功能，我的观点是，应该增加持续集成方面的功能，而不是增加像 SVN 这样的兼容功能，以 Gitee 为例，虽然增加了 SVN 接入，但每日请求数不足 1%，如果刨去使用脚本自动更新的请求，那么占比将非常小，而这种附加功能却在架构设计是带来了很多难题，增加 SVN 支持则是得不偿失的。

### 大文件大存储库

公共 Git 代码托管平台很多时候实际上是给用户提供免费服务，为了过多避免大文件大存储库占用平台资源，对其作出限制必不可少，通常是大文件限制 100MB, 存储库限制 1GB. 存储库的检测简单的遍历存储库 objects 目录即可，而大文件的检测则复杂一些。Gitee 最初使用 Grit 检测 commit 是否引入了 blob 原始大小大于限制的文件，但这种机制需要解析 Git 对象，检测容易坍塌（一是检测超时，二是检测逃逸，三是存储库体积膨胀），后开使用原生钩子，改变了检测机制，则避免了这些问题。详细情况可以阅读[《服务端 Git 钩子的妙用》](https://forcemz.net/git/2019/07/31/GNKServerSide/)。

禁止大文件推送这只是堵，那么大文件应该如何存放呢？Github 推出了 LFS 方案，目前 LFS 功能已经被大多数平台支持，Github 将 LFS 存储到 AWS 上，而 Gitee/Gitlab/Gogs/Gitea 大多使用自建的 LFS 服务器，存储在特定服务器上。

### 安全性增强

Github 最近宣布了支持 WebAuthn: [GitHub supports Web Authentication (WebAuthn) for security keys](https://github.blog/2019-08-21-github-supports-webauthn-for-security-keys/)，这种机制可以使用生物识别从而避免输入用户密码，随着信息技术的不断发展，一方面，安全机制不断完善，另一方面，用户面临的风险也会多样化，复杂化。代码托管平台管理了开发者的核心资产，因此在安全上绝不能掉以轻心。当然需要做的不仅仅是及时跟进新的安全机制，还需要对整个系统及时进行安全升级，淘汰旧的协议（比如 SSL3/TLS1.1），旧的加密，哈希算法（DSA，MD5/SHA1），及时采用新的协议（TLS1.3）,新的加密，哈希算法（ED25519，SHA3）等等。


## 文件服务
<!--附件下载，发布文件，Archive 下载-->

一个优秀的 Git 代码托管平台，应该在软件的开发整个周期都给用户提供帮助，比如下载源码，软件发布。源码下载主要指 Archive 功能，软件的发布则需要平台提供 Release/附件下载功能。

### Archive

我们知道 git-archive 命令可以将存储库特定的 commit/branch 打包成一个 zip/tar 文件，而在 Git Over SSH（Git Over TCP） 实现中，只要我们允许 `git-upload-archive` 命令在远程服务器上运行，就打包远程服务器上的存储库的特定分支。但由于 git-upload-archive 与 git-upload-pack/git-receive-pack 存在一些不同，是的 HTTP 协议无法实现 archive 协商。提供 archive 下载则需要另辟蹊径。

我们在远程服务器上运行 git-archive 将其输出作为响应体的内容返回给 HTTP Client 便可实现 archive 下载功能，由于 archive 下载实际上是将 git tree/blob 遍历然后写入到归档文件后压缩（tar.gz/tar.bz2 ...）或者是压缩后写入文件（zip），二者都非常消耗 CPU 资源，因此我们在实现 archive 下载功能的同时应该设计 archive 的缓存功能（当然缓存应该支持过期）。gitlab-workhorse 实现的 archive 下载功能便是先尝试命中缓存，如果没有缓存则调用 git 命令然后生成写入到缓存文件。Gitee 最近实现的 blaze-archive 也采用了类似的机制，但 blaze-archive 是一个独立的命令，这个命令实际上是被 git-srv 调用，brzox 与 git-srv 通信，brzox 将 archive 返回给 HTTP Client，而缓存的删除则是 blaze 负责的。 

### 附件，Release

附件，Release 可以选择云方案，如果要将附件和 LFS 统一管理，实际上国内的阿里云，腾讯云之类的并不合适，这些平台对并不支持类似 AWS `x-amz-content-sha256` 这样的头部，而是 `Content-MD5` 因此这些云平台要支持 LFS 则要花费多一些功夫。选择国外的 AWS, Azure 则需要考虑经济，网络等问题。当然无论如何使用云平台都需要考虑经济问题。

平台自建附件，Release 功能可以使用分布式文件系统，如 FastDFS, 但 FastFDS 并不是一个好的选择，历史比较久，存储机制安全机制现在来说都不是很优秀。有个更好的选择是 [Minio](https://github.com/minio/minio), minio 使用 Golang 开发，支持 AWS API。许可协议是 `Apache 2.0`，商用没有阻碍，因此是用来搭建附件，Release 以及 LFS 存储服务器的不二选择。

## 道路漫漫

软件开发一直是一个飞速变化的领域，而代码托管也要不断面临新的挑战，道路漫漫，吾辈不休。

