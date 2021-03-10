---
layout: post
title: Charlie's Resume
permalink: /about/
---


# 简介

昵称：Force Charlie，博文<br/>
邮箱：forcemz@outlook.com<br/>
个人博客： [https://forcemz.net](https://forcemz.net)<br/> 
Github 主页：[https://github.com/fcharlie](https://github.com/fcharlie)<br/>
Gitee 主页：[https://gitee.com/ipvb](https://gitee.com/ipvb)<br/>
开源极客，几个冷门开源项目作者。<br/>
前 Gitee 核心开发者，分布式、读写分离架构设计和实施者；Git 专栏[《Git 的反思》](https://gitee.com/Git-uses/rethinkgit) 作者；国内代码托管行业老兵。<br/>
文学爱好者，喜欢滑板，自行车，爬山，刷剧等等。<br/>

# 教育

2010.09 ~ 2014.06 本科 湘南学院 计算机系 通信工程  

# 工作经验

2014.06 ~ 2020.04 深圳市奥思网络科技有限公司（开源中国），Gitee 团队核心工程师，主要负责 Gitee 基础架构设计和开发，
Git 基础软件开发，服务器软件开发。

-   实现 Gitee 代码托管平台的 Subversion（SVN）接入，国内第一家。
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
-   基于 Go 开发的 Blaze， Git GC, 备份，存储库体积分析后台任务服务。
-   Gitee 存储路由架构改造。并基于 Golang 编写了验证服务 Banjo 将基础服务验证 API 独立出来。
-   基于 Go 开发了 blaze-archive Git archive 扩展工具，支持 archive 缓存，支持 `tar.br`,`tar.zst`,`tar.xz`,`tar.sz` 等压缩格式。
-   设计和实现了 Gitee 存储库读写分离方案 RONE/RIME。

Git-Secure 开源地址 [https://gitee.com/oscstudio/git-secure](https://gitee.com/oscstudio/git-secure)  
Git-Analyze 开源地址 [https://gitee.com/oscstudio/git-analyze](https://gitee.com/oscstudio/git-analyze)

2020-05 ~ 至今 腾云扣钉

-   设计规划代码扫描功能能力增强
-   Subversion 目录权限修复
-   代码托管高可用方案设计和实现
-   代码托管功能补全和调优

# 技能

擅长语言:

-   C
-   C++
-   Go
-   C#
-   PowerShell
-   Java
-   D
-   Perl
-   Shell
-   Python（不太喜欢）
-   Makefile
-   Ruby（不太喜欢）


相应操作系统研究方向：

-   Linux ,多线程，多进程，网络编程。
-   Windows Win32 GUI 开发，以及命令行开发。  
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

# 个人作品

博客站点 [https://forcemz.net/](https://forcemz.net/)


|作品|源码地址|描述|
|---|---|---|
|Baulk|[github.com/baulk/baulk](https://github.com/baulk/baulk)|基于 C++20 的极简 Windows 绿色包管理器|
|Bela|[github.com/fcharlie/bela](https://github.com/fcharlie/bela)|基于 C++20 开发的 Windows 系统功能库|
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
