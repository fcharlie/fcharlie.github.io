---
layout: post
title:  "坐和放宽 - 您的计算机需要更新"
date:   2019-06-25 10:00:00
published: true
categories: talk
---

# 前言

今天在微博上看到了新浪科技的一篇微博，其主要内容如下：

>#创事记#【用不胜其烦的update，将Windows用户捆绑上未来战车】网友苦Windows久矣。众所周知，Windows Update是很烦人的更新，其中以win10的用户最为水深火热。目前看来，用户还需要在一次次漫长而琐碎的更新升级中等待，而时代的洪流终会蜿蜒地书写出微软的对与错。@脑极体Unity Windows不胜其烦的update

链接为：[https://m.weibo.cn/status/4385983862343989](https://m.weibo.cn/status/4385983862343989)。

在这片微博中，新浪科技竟然能说出 “网友苦Windows久矣” 这种话，我只听说过“天下苦秦久矣”和“消费者苦联想久矣”，这个真的是过分的指责。国内的科技媒体在技术素养上怎能有如此的认识！科技媒体尚且如此，那么大众的对于软件更新可能就基本没啥积极的看法了。2017-2018 年**永恒之蓝**勒索病毒肆虐的经历还历历在目，而被感染的机器基本上都是未安装补丁（包括已经停止技术支持的 XP 系统）的操作系统。对软件更新没有一个清醒的认识，这信息化急速发展的时代时非常危险的，我们也看到近几年网络犯罪的层出不穷，特别是针对计算机漏洞发起的网络犯罪。所以，及时的查漏补缺非常有必要，而本文也就要来讨论软件的更新问题。

# 软件的更新

软件的更新主要分为两个方面，一个是**安全更新**，另一个是**功能更新**（增加或废除功能）。通常来说，安全更新是必不可少的，安全更新能够修复计算机漏洞，提高软件的稳定性。而功能更新在一定程度上没有那么必不可缺，但功能更新在提高软件服务质量，简化用户操作等方面举足轻重。

## 安全更新

无论是何种计算机，操作系统，系统核心组件，基础软件的安全更新是最为重要的。在实际情况中，硬件，软件的安全状况并不容乐观。下面是一些漏洞的回顾。

2014年4月，国外黑客曝光了 OpenSSL Heartbleed 漏洞（在国内称为心血漏洞）：[OpenSSL 'Heartbleed' vulnerability (CVE-2014-0160)](https://www.us-cert.gov/ncas/alerts/TA14-098A)，此漏洞危害之大，影响之大，堪称网络安全里程碑时间。此漏洞是利用 OpenSSL TLS 心跳包没有做边界检查进行攻击，黑客可以拿到用户加密后的数据，此漏洞使得受影响的用户使用 HTTPS 访问网银，在线支付等行为都变得不安全。 全球有大量网站使用 OpenSSL 实现 HTTPS 接入，这些网站都受到了影响。这件事还深远影响了 OpenSSL 的开发流程，改变了 SSL/TLS 库的格局，导致了 OpenSSL 的分裂，比如 OpenBSD fork OpenSSL 创建了 LibreSSL。Google fork OpenSSL 创建 boringssl 项目，甚至一些厂商开始使用自己的 SSL/TLS 实现，如 Amazon 的 [s2n](https://github.com/awslabs/s2n) 以及 Facebook 的 [Fizz(TLS 1.3)](https://github.com/facebookincubator/fizz)。而一些其他加密/TLS 库也受到了更多关注，如 ARM 的 [mbedtls](https://github.com/ARMmbed/mbedtls)。

2017年3月补丁星期二，微软发布了 SMB 服务漏洞，而在短短的一个月之后，黑客团队就将此漏洞利用，并发布了相应的网络攻击工具，随后，勒索病毒爆发，中国，英国，俄罗斯，整个欧洲都受到影响，特别是高校校内网，大型企业内网政府机构专网。而在整个时间中，这些被勒索病毒攻击的电脑几乎都是未及时安装更新或者生命周期已经终结的计算机。而高校内网，大型企业内网，政府机构专网的计算机通常在软件更新上比较消极，因此损失特别严重。相关漏洞分析：[SMB Exploited: WannaCry Use of "EternalBlue"](https://www.fireeye.com/blog/threat-research/2017/05/smb-exploited-wannacry-use-of-eternalblue.html)

2018年，Intel 不断的爆出 CPU 漏洞，“熔断”（Meltdown）和“幽灵”（Spectre）及其变种，这些漏洞让云服务厂商惊出一身冷汗，Azure 亚马逊都停机维护。这种漏洞的危害非常严重，对于使用虚拟机或者容器隔离的云服务厂商，CPU 漏洞可以轻易突破宿主机隔离，更可获得关键数据。还有黑客演示使用 JavaScript 利用漏洞攻击，普通用户也难以免受其害。Clang GCC 均增加了防御 CPU 漏洞的代码。MSVC 发布了抵御 Spectre 漏洞的编译器版本。

2019-06-24 CURL 作者 又撰文披露了 [OPENSSL ENGINE CODE INJECTION IN CURL](https://daniel.haxx.se/blog/2019/06/24/openssl-engine-code-injection-in-curl/)  [CVE-2019-5443: Windows OpenSSL engine code injection](https://curl.haxx.se/docs/CVE-2019-5443.html)，此缺陷可以导致非特权用户将恶意配置放置在 OpenSSL 搜索目录，当特权用于运行 cURL 时，实现攻击。漏洞真是防不胜防。

[Zero Day Initiative: ZDI](https://www.zerodayinitiative.com/) 是趋势科技建立的一个零日漏洞计划，在 2019-06 月的零日漏洞分享目前就有 3个，相关博客地址：[https://www.zerodayinitiative.com/blog](https://www.zerodayinitiative.com/blog)，6 月份的零日漏洞如下：

+   通过 Ruby On Rails 进行远程代码执行主动存储不安全的反序列化。影响版本 6.0.0.X 和 5.2.X。
+   通过 CVE-2019-1069 利用 Windows 任务计划程序，此操作可以提升到 SYTSEM 权限。
+   MINDSHARE：使用 BELKIN SURF N300 路由器进行硬件逆转。

上述零日漏洞以 CVE-2019-1069 影响最为广泛，如果每个月披露的漏洞数比较接近，那么当计算即一年未更新时，所面临的漏洞风险将非常高，用户的数据安全将无法得到保证。及时的安全更新应该也必须是计算机用户的常规操作。


## 功能更新

与安全更新不同的是，功能更新更多的是改进用户体验，而没有安全更新那么急迫。但一些功能更新也非常重要。

2015 年 IETF 批准了  HTTP/2 协议，主要 [RFC7450: Hypertext Transfer Protocol Version 2 (HTTP/2)](https://httpwg.org/specs/rfc7540.html) 和 [RFC7451: HPACK: Header Compression for HTTP/2](https://httpwg.org/specs/rfc7541.html)。HTTP/2 有以下新特性：

+   多路复用，利用二进制分帧，将多个 HTTP 请求通过同一条信道传输，这节省了网络连接数目，提高了信道的使用率。
+   头部压缩，在 HTTP/1.1 中，基于文本的头部非常冗长，占据了很多空间，在 HTTP/2 中使用头部压缩将节省一部分流量。
+   服务端推送 比如你请求一个 HTML 页面，有可能需要请求相应的 CSS/JS 文件，服务端推送则可让客户端减少请求。
+   流量控制
+   ...

HTTP/2 看起来很不错，但你如果不升级浏览器，则无法享受到这些新特新，对于 Windows 而言，WinHTTP/WinINet 均只有 Windows 10 才支持 HTTP/2，如果没有升级到 Windows 10，则依赖这两个系统库的软件无法支持 HTTP/2。而目前的浏览器均已支持 HTTP/2，主流网站很多都已经支持 HTTP/2。使用最新发布的 curl 运行 `curl -V` 如果有 `HTTP2` 字段，则可以用 `curl` 去检测服务器是否支持 HTTP/2: `curl -I --verbose https://bing.com`。

2018年 TLS 1.3 [RFC8446: The Transport Layer Security (TLS) Protocol Version 1.3](https://tools.ietf.org/html/rfc8446) 发布，TLS 1.3 新增了 0-RTT 提高了 Handshake 性能，删除了一些不安全的加密机制，增强了安全性。要使用 TLS 1.3 你应该使用较新的浏览器，开发软件时应该选择 OpenSSL 1.1.1c 或者更新版本，TLS 1.3 实现列表可以参考：[Implementations](https://github.com/tlswg/tls13-spec/wiki/Implementations)。

几乎每年 Unicode 组织都会发布新的 Unicode 版本，在新的 Unicode 版本中会增加一些新的字符包括 emoji，在今年日本徳仁皇太子即位，年号`令和`，Unicode 发布了 `unicode-12.1-reiwa` 在 BMP 平面增加了 `令和` 的合体字 `U+32FF`。Windows 发布了一些列更新，如果没有更新，日本都可能发生混乱。而新的 emoji 则给用户带来了更多的乐趣。在 Windows 10 中，新增了彩色字体，以前的 emoji 都是黑色的，新的更新让 emoji 回归了本色。

2018 年 Git Wire Protocol 发布，此协议的优势的改进了 git clone 的流程，特别是引用发现，浅表克隆，增加了扩展机制，为以后的新增功能增加了更多的可能。笔者第一时间实现了 Gitee 对 Wire Protocol 的支持，如果开发者没有及时升级到 git 新版本，也就无法享受这些特性了。

## Windows 更新的讨论

Windows 更新的被批评最多的是早期的强制更新（强制重启），不过在 Windows 10 1903, 这种情况已经没有发生了，在撰写本文此章节时，Windows Update 提醒我重启（没有弹窗，仅任务栏 Windows Update 图标发生了变化。注：笔者未设置暂停更新。），但并未干扰我工作。实际上目前的 Ubuntu 系统也会提醒用户更新系统，并且更新完有可能提醒用户重启计算机，1903 与其是相似的。虽然 Windows 更新已经得到了很大的改进，但话说回来，微软依然应该不断的提高软件质量，提高稳定性。特别要接纳用户的反馈意见。

## 总结

**历史是不断发展的，软硬件是不断更新的**，磁盘在使用几年之后便容易出现坏道，磁盘读写速度下降，这时硬件就需要更新了，而软件则需要经常更新，无论安全更新还是功能更新，基本上都是为了让用户更好的更安全的使用计算机。
另外，随着硬件的更新，软件更需要及时更新，大多数时候，硬件更新后，软件及时更新能够更好的发挥出新硬件的优势，比如当硬件增加 SSE AVX 这种指令时，软件更新适配了就可能带来大量的新能提升。
人有生老病死，软件有更新迭代。都是历史的齿轮滚滚向前。
