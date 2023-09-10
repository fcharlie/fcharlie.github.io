---
layout: post
title: Charlie's Resume
permalink: /about/
---


# 简介

昵称：Force Charlie，博文
邮箱：forcemz@outlook.com  
个人博客： [https://forcemz.net](https://forcemz.net)  
Github 主页：[https://github.com/fcharlie](https://github.com/fcharlie)  
Gitee 主页：[https://gitee.com/ipvb](https://gitee.com/ipvb)  
开源极客，几个冷门开源项目作者。  
前 Gitee 核心开发者，分布式、读写分离架构设计和实施者；Git 专栏[《Git 的反思》](https://gitee.com/Git-uses/rethinkgit) 作者。

# 教育

2010.09 ~ 2014.06 本科 湘南学院 计算机系 通信工程  
2012.05 ~ 2014.06 年主导开发基于 ASP.NET 的 ACM 在线测评系统。

# 工作经验

2014.06 ~ 2020.04 深圳市奥思网络科技有限公司（开源中国），码云团队核心工程师，主要负责码云（Gitee）基础架构设计和开发，
Git 基础软件开发，服务器软件开发。

-   实现码云（GITEE）代码托管平台的 Subversion（SVN）接入，国内第一家。
-   从事 GIT 基础服务开发，包括不限于分布式架构，实用工具等。
-   开发了 svnsbz, 即 svn:// 协议动态代理服务器,支持实时监测并加载黑名单（基于 Linux iNotify）。
-   开发了 Git 分布式前端，NGINX 模块，核心路由模块，核心路由库实现了 O1 高效路由缓存，支持域名解析，内部 DNS 缓存以及实时 DNS。
-   开发了 Git 分布式核心服务 git-srv,并实现了 分布式版本的 git-upload-pack git-receive-pack git-upload-pack。
-   开发了 Git 协议服务器 git-daemon 基于 Boost.Asio，分布式版本后端与 git-srv 通信。
-   开发了 Aton API 服务器，支持 git 同步，创建存储库，分布式基础服务状态检查。
-   开发了 Git 同步服务 git-diamond，即 git-daemon 内部版本。
-   开发了 Aquila Git HTTP Server 基于 .Net Core，HTTP 服务使用 Asp.Net Core 的 Kestrel。
-   开发了 Git HTTP 服务器 - Brzo，基于 Boost.Asio 实现,速度超过一般的 Git HTTP 服务器实现，支持平滑重启。
-   开发了 Git Analyze 工具，这些工具能够检测大文件回滚历史，嫁接分支等。
-   开发了 Moseo Git LFS 服务器，支持 SSH 验证，基于 cpprestsdk。
-   基于 PowerShell™ Core 开发了 Git 存储库加密工具， git-secure，使用 AES 256 加密。
-   基于 Go 开发了 Git SSH 服务器分布式版，Git HTTP/SSH/Git 服务器私有化部署版。其中 HTTP 服务器整合了 Git VFS 功能。
-   开发了 基于 libgit2 的 Git VFS (VFSforGit) 服务器支持程序。
-   基于 Nginx round robin 模块重新实现了 Nginx 路由模块。
-   码云 Git 的 Git/HTTP/SSH 协议对 Git Wire Protocol 的支持。
-   基于 libssh 实现 sserver 对 svn+ssh 的支持，亦基于 Go 实现了 ssh 服务器支持 sserver。
-   基于 Go 开发了新版 LFS 服务器，LFS 对象存储在对象存储上，支持阿里云，Azure， AWS S3, 腾讯云。
-   基于 Go 开发了本地存储的 LFS 服务器，目前 Gitee 使用的是这种。
-   负责码云分布式存储架构变更设计和编码实现。
-   基于 libgit2 开发了基于 git namespace 快照的企业存储库快照备份方案。
-   Git 原生钩子支持大文件检测，存储库分析，分支权限管理，git 只读文件功能，git 禁止推送暴露用户私有邮箱，以及存储库响应式同步机制。
-   基于 Go 开发的 Blaze， git GC, 备份，存储库体积分析后台任务服务。
-   Gitee 存储路由架构改造。并基于 Golang 编写了验证服务 Banjo 将基础服务验证 API 独立出来。
-   基于 Go 开发了 blaze-archive Git archive 扩展工具，支持 archive 缓存，支持 `tar.br`,`tar.zst`,`tar.xz`,`tar.sz` 等压缩格式。
-   设计和实现了 Gitee 存储库读写分离方案 RONE/RIME。

Git-Secure 开源地址 [https://gitee.com/oscstudio/git-secure](https://gitee.com/oscstudio/git-secure)  
Git-Analyze 开源地址 [https://gitee.com/oscstudio/git-analyze](https://gitee.com/oscstudio/git-analyze)

2020-05 ~ 2021-07 腾云扣钉

- 代码扫描功能
- Subversion 目录权限修复
- 代码托管高可用方案设计和实现
- 代码托管功能补全和调优

2021-07 ~ 至今 蚂蚁集团

负责蚂蚁内部代码托管平台的架构演进和功能维护。

- 负责蚂蚁内部代码托管平台架构演进。
    - 2022 年完成存储库分片，一些多读，重构队列系统等架构改造，稳定性得到巨大提升，资源利用率提高 400%，相较于 2021 年频繁故障，2022 年因底层导致的故障没有再发生过。
    - 2023 年，代码平台率先全面支持 SHA256，这个比 Gitlab 早三四个月。
    - 基于最新版 git 实现一些功能，包括 git merge，冲突检测等，完全移除了 libgit2（早期 借鉴了github gitlay），gitlay 截止 2023-09 还未完全移除 libgit2。
    - 存储库自动优化，自建存储库语言分析，服务平滑重启改造，针对 AI 等场景的支持改造等。
    - 其他比较重要但没啥技术突破的事情。
- 开发了内部存储库治理工具 blat
    - 支持自动扫描并清理大文件，比 git filter-branch 等其它仓库治理工具更简单，更智能，更高效。（云效 Codeup 的同学曾希望我们开源，业内对该功能有较强需求，目前暂无计划）
    - 支持历史线性化重写。
    - 是第一款支持将 Git SHA1 存储库无损转变为 SHA256 的工具（目前 git 的 submodule 是不能混用的，故不兼容）。
    - 支持扫描并清理存储库过期（或已合并）分支。
    - 简化 git 部分克隆操作，使其体验接近 Zeta 检出代码。
- 设计并开发了 Zeta，其特点有：
    - 吸取了 Git 优秀的设计理念，同时也吸取了 Git 设计缺陷的教训。
    - Zeta 是一种云版本控制系统（不是纯粹的集中式版本控制系统，也不是分布式版本控制系统）。
    - 基于数据分离的原则，依托于 OSS 和分布式数据库，设计了其存储架构。
    - Zeta 在对象格式，存储格式，传输协议上进行了仔细的设计，将业内的技术积累运用到版本控制系统的设计之中，比如文件格式识别，ZSTD 压缩，BLAKE3 哈希算法等等。
    - Zeta 存储库不再担心存储库的体积，容量问题，也不会因此带来负载问题。
    - Zeta 针对 OSS 的限制，以及 AI 研发的兴起，引入了针对大文件的特殊设计，解决了单个文件的体积问题。
    - 虽然 Zeta 还不够成熟，但其理念是优秀的，也似乎是下一代版本控制系统的共识。业内也有类似的实验性方案，但目前似乎是没有 Zeta 成熟的。

# 技能

包括但不限于以下语言和框架：  
语言:

-   C
-   C++
-   C#
-   PowerShell
-   Go
-   Java
-   D
-   Perl
-   Shell
-   Python（不太喜欢）
-   Makefile
-   Ruby（不太喜欢）



框架:

-   Boost Boost.Asio
-   Qt  
-   WTL
-   Win32  
-   ASP.NET
-   WPF
-   webpy
-   nginx Perl
-   Vibe.d
-   Rails  
-   Puma  

操作系统：

-   Linux ,多线程，多进程，网络编程。
-   Windows Win32 程序开发，主要是 GUI 程序。  
-   FreeBSD
-   ReactOS 内核研究
-   Minix  
-   Hurd  
-   Haiku  

技术领域：

+   Windows 系统编程
+   Linux 系统编程
+   文件的压缩解压
+   Windows 权限管理
+   文件特征的分析


通常来说，本人精通 git 和 svn。  


# 个人作品

博客站点 [https://forcemz.net/](https://forcemz.net/)


|作品|源码地址|描述|
|---|---|---|
|Baulk|[github.com/baulk/baulk](https://github.com/baulk/baulk)|基于 C++17 的极简 Windows 绿色包管理器|
|Bela|[github.com/fcharlie/bela](https://github.com/fcharlie/bela)|基于 C++17/20 开发的 Windows 系统功能库|
|BelaUtils|[github.com/fcharlie/BelaUtils](https://github.com/fcharlie/BelaUtils)|基于 Bela 重写的 PEAnalyzer/Krycekium 等|
|Bali|[github.com/balibuild/bali](https://github.com/balibuild/bali)|基于 Golang 的 Golang 构建打包工具，能够创建 STGZ 安装包和嵌入 Windows 应用程序清单，图标版本等|
|Buna|[github.com/fcharlie/buna](https://github.com/fcharlie/buna)|从 Golang debug 模块 fork 过来的增强的可执行文件格式分析库|
|Clangbuilder|[github.com/fstudio/clangbuilder](https://github.com/fstudio/clangbuilder)|基于 PowerShell 的 LLVM 自动构建工具集|
|TunnelSSH|[github.com/balibuild/tunnelssh](https://github.com/balibuild/tunnelssh)|一个机智的 SSH 客户端|
|svnsrv|[gitee.com/oschina/svnsrv](https://gitee.com/oschina/svnsrv)|开源跨平台的 svn 协议动态代理服务器。|
|iBurnMgr|[github.com/fcharlie/iBurnMgr](https://github.com/fcharlie/iBurnMgr)|基于 Direct2D 开发的 USB 启动盘制作软件。|
|Krycekium|[github.com/fcharlie/Krycekium](https://github.com/fcharlie/Krycekium)|基于 Msi API 编写的 `.msi` 安装包解压工具，用于提取制作绿色软件|
|Kismet|[WIN32: github.com/fcharlie/Kismet](https://github.com/fcharlie/Kismet)<br>[UWP: github.com/fcharlie/KismetUWP](https://github.com/fcharlie/KismetUWP)|基于 RHash 编写 Hash 计算工具，支持 MD5,SHA1 SHA1DC,SHA2,SHA3|
|Ginkgo|[github.com/fstudio/Ginkgo](https://github.com/fstudio/Ginkgo)|基于 WPF 开发的 Metro 风格 Batch 编辑器|
|whois|[github.com/fcharlie/whois](https://github.com/fcharlie/whois)|Windows 平台 whois 实现 (基于 C++17 WSAPoll)|
|Privexec|[github.com/M2Team/Privexec](https://github.com/M2Team/Privexec)|Windows 运行特定权限工具<br>支持 **System**，**TrustedInstaller** 提权<br>支持 **UAC 降权**，启动**低完整性**权限进程，启动 **AppContainer** 进程<br>包含 Privexec，AppExec，wsudo<br>wsudo 能够一定程度上模拟 sudo|
|PE Analyzer|[github.com/fcharlie/PEAnalyzer](https://github.com/fcharlie/PEAnalyzer)|基于 C++ & Direct2D 开发的 PE 分析软件<br>能够解析 PE 文件依赖|
|MsysLauncher|[github.com/fcharlie/msys2-launcher](https://github.com/fcharlie/msys2-launcher)|MSYS2 运行环境启动器|
|WiFiAssistant|[github.com/fcharlie/WiFiAssistant](https://github.com/fcharlie/WiFiAssistant)|Windows 平台 WiFi 无线承载网络开启助手|
|Planck|[github.com/fcharlie/Planck](https://github.com/fcharlie/Planck)|基于魔数和文件头的文件分析工具（库）|
|Angelo|[github.com/fcharlie/Angelo](https://github.com/fcharlie/Angelo)|基于 AspNet Core 编写的跨平台 git http 服务|


## 开源参与

本人积极参与开源项目，目前参与贡献的其他开源项目有：

+   [Windows Terminal](https://github.com/microsoft/terminal/commits?author=fcharlie)
+   [git](https://github.com/git/git/commits?author=fcharlie)
+   [vcpkg](https://github.com/Microsoft/vcpkg/commits?author=fcharlie)
+   [libgit2](https://github.com/libgit2/libgit2/commits?author=fcharlie)
+   [parallel-hashmap](https://github.com/greg7mdp/parallel-hashmap/commits?author=fcharlie)
+   [cpptoml](https://github.com/skystrife/cpptoml/commits?author=fcharlie)
+   [git-http-backend(golang)](https://github.com/asim/git-http-backend/commits?author=fcharlie)
+   [git-for-windows](https://github.com/git-for-windows/MINGW-packages/commits?author=fcharlie)
+   [archiver](https://github.com/mholt/archiver/commits?author=fcharlie)
+   [minizip](https://github.com/nmoinvaz/minizip/commits?author=fcharlie)
+   [libzip](https://github.com/nih-at/libzip/commits?author=fcharlie)
+   [7-Zip-zstd](https://github.com/mcmilk/7-Zip-zstd/commits?author=fcharlie)

## 活动

1.  2016 年作为讲师参与 Ubuntu Kylin 16.04 发布活动。
2.  2018 年作为讲师参与 Ubuntu Kylin 18.04 发布活动。
