---
layout: post
title:  "坐和放宽 - 您的计算机需要更新"
date:   2019-06-25 10:00:00
published: false
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

2017年3月补丁星期二，微软发布了 SMB

### Microsoft SMBv1 Vulnerability

[SMB Exploited: WannaCry Use of "EternalBlue"](https://www.fireeye.com/blog/threat-research/2017/05/smb-exploited-wannacry-use-of-eternalblue.html)

### Intel CPU vulnerability



## 功能更新

### HTTP/2

### TLS 1.3

### ED25519

### Git Wire Protocol

## Windows 更新的讨论

Windows 更新的被批评最多的是早期的强制更新（强制重启），不过在 Windows 10 1903, 这种情况已经没有发生了，在撰写本文此章节时，Windows Update 提醒我重启（没有弹窗，仅任务栏 Windows Update 图标发生了变化。注：笔者未设置暂停更新。），但并未干扰我工作。实际上目前的 Ubuntu 系统也会提醒用户更新系统，并且更新完有可能提醒用户重启计算机，1903 与其是相似的。