---
layout: post
title: Charlie's Resume
permalink: /about/
---


# 简介

昵称：江二十三  
邮箱：forcemz@outlook.com  
个人博客： [https://forcemz.net](https://forcemz.net)  
Github 主页：[https://github.com/fcharlie](https://github.com/fcharlie)  
Gitee 主页：[https://gitee.com/ipvb](https://gitee.com/ipvb)  
开源极客，极简 Windows 包管理器 Baulk 以及几个冷门开源项目的作者。  
前 Gitee 核心开发者，分布式、读写分离架构设计和实施者；Git 专栏[《Git 的反思》](https://gitee.com/Git-uses/rethinkgit) 作者。

# 教育

2010.09 ~ 2014.06 本科 湘南学院-计算机系-通信工程  
-   主导开发基于 ASP.NET 的 ACM 在线测评系统。

# 工作经验

**2014.06 ~ 2020.04 深圳奥思网络科技有限公司 - 核心工程师** 

>注：奥思网络科技有限公司即业内熟知的开源中国，该公司拥有截至目前（2024-02）国内最大的代码托管平台 **Gitee**

负责 Gitee 的架构改造，演进工作：

-   在 Git 存储库中实现兼容 Subversion（SVN） 客户端的接入，国内第一家。
    - 分布式改造
    - 实现 `svn+ssh` 协议支持
-   使用 C++ 分析 git 对象格式开发了服务端钩子，截止到笔者离开 Gitee，钩子提供以下功能。
    - 实现了毫秒级的存储库大文件，大仓库分析功能，降低了平台硬件成本，提高了稳定性。 
    - 支持分支权限管理，如保护分支，只读分支。
    - 禁止推送暴露私有邮箱地址。
    - 目录级别只读文件功能。
    - 存储库响应，一些多读等。
-   对 Gitee 进行分布式架构改造
    - 开发 Nginx 模块，实现动态路由，解决了 Gitlab 服务的分布式问题
    - 针对 Git HTTP/SSH/Git 协议实施了服务端，前端组件，解决了分布式问题。
-   基于 PowerShell™ Core 开发了 Git 存储库加密工具， git-secure，使用 AES 256 加密。
-   设计并实施 Git LFS 功能。
-   设计并实施企业级基于名称空间快照的 Git 备份方案，与原有的机制相比能节省 90% 的存储成本。
-   设计和实现了 Gitee 存储库读写分离方案 RONE/RIME。

一些其他工作未列举到此处。

Git-Secure 开源地址 [https://gitee.com/oscstudio/git-secure](https://gitee.com/oscstudio/git-secure)  
Git-Analyze 开源地址 [https://gitee.com/oscstudio/git-analyze](https://gitee.com/oscstudio/git-analyze)

**2020-05 ~ 2021-07 腾云扣钉 - 高级工程师**

负责 CODING 代码托管平台底层架构的演进。

- 代码扫描功能
- Subversion 目录权限修复
- 代码托管高可用方案设计和实现
- Git 目录级别权限控制（业内首个读级别权限控制）
- 代码托管功能补全和调优

**2021-07 ~ 至今 蚂蚁集团 - 技术专家**

负责蚂蚁内部代码托管平台（`AntCode`）的架构演进和功能维护。  
负责基于云的下一代版本控制系统 **Zeta** 的规划和演进。

- 负责蚂蚁内部代码托管平台架构演进。
    - 2022 年完成存储库分片，一些多读，重构队列系统等架构改造工作。
        - 稳定性得到巨大提升，改造前（2021） 故障频繁，改造后（2022- 至今）基本（底层）没出过故障。
        - 运营成本下降，服务器资源利用率提高 400%。
    - 2023 年，AntCode 率先全面支持 SHA256，这个比 Gitlab 早三四个月。
    - 基础功能优化，功能改进
        - 重构合并，冲突检测功能，大存储库调用冲突检测 API 耗时从最差数十秒缩短到几十毫秒（系 libgit2 缺陷）。
        - 存储库自动优化，自动决策，自动 GC。
        - 自建存储库语言分析。
        - 服务平滑重启改造。
        - 针对 AI 场景技术支持改造。
- 开发了内部存储库治理工具 blat
    - 支持自动扫描并清理大文件，比 git filter-branch 等其它仓库治理工具更简单，更智能，更高效。（业内对该功能有较强需求，但公司暂无计划）
    - 支持历史线性化重写。
    - 是第一款支持将 Git SHA1 存储库无损转变为 SHA256 的工具。
    - 支持扫描并清理存储库过期（或已合并）分支。
    - 简化 git 部分克隆操作，使其体验接近 Zeta 检出代码。
- 作为架构师，设计并主导开发了**基于云的下一代版本控制系统 - Zeta**，包括服务端，客户端，迁移工具，其特点有：
    - 支持海量文件，存储库体积无限制。
    - 支持海量分支和标签，不会因分支/标签数量降低操作体验。
    - 无需依赖 Git LFS 之类的扩展即可支持大文件，大文件体积无限制。
    - 适合单一大库，AI 模型，游戏研发。
    - 设计上参考了 Git 优秀的设计理念，同时也规避了 Git 的缺陷。
    - 依赖现代化的云基础设施，特别是分布式数据库，对象存储（也可以是分布式文件系统，亦可解耦）。
    - Zeta 在对象格式，存储格式，传输协议上进行了仔细的设计，将业内的技术积累运用到版本控制系统的设计之中，比如文件格式识别，ZSTD 压缩，BLAKE3 哈希算法等等。
    - Zeta 针对 OSS 的限制，以及 AI 研发的兴起，引入了针对大文件的特殊设计，解决了单个文件的体积问题。
    - Zeta 目前解决了在忽略文件系统大小写系统中存在的路径冲突问题，该问题 git 中存在未得到解决。
    - Zeta 在客户端和服务端都实现了完整的迁移工具，可以将未使用 submodule 的 git 存储库完整的迁移到 Zeta（包含 submodule 的会忽略），客户端还支持迁移使用了 Git LFS 的 Git 存储库。

Zeta 的在设计上摆脱了很多 Git 的桎梏，因此在 AI，游戏研发等场景拥有很强的吸引力。目前在蚂蚁内部，很多 AI 模型已经使用 Zeta 存储，在 AI 等场景，Zeta 有望取代 Git 成为特定平台的基础设施。

# 技能

包括但不限于以下语言和框架：  
语言：C，C++，C#，PowerShell，Go，Java，D，Perl，Shell，Python，Makefile，Ruby

框架: Boost Boost.Asio，Qt，WTL，Win32 ，ASP.NET，WPF，webpy，nginx Perl，Vibe.d，Rails ，Puma  

操作系统技术：

-   Linux ：多线程，多进程，网络编程。
-   Windows: Win32 程序开发，主要是 GUI 程序。  
-   其他：FreeBSD，ReactOS 内核研究

技术领域：Windows 系统编程，Linux 系统编程，文件的压缩解压，Windows 权限管理，文件特征的分析


通常来说，本人精通 git 和 svn。  

# 个人作品

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
+   [go-git](https://github.com/go-git/go-git/commits?author=fcharlie)
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
