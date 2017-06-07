---
layout: post
title:  "Privexec 的内幕（一） Windows 彩色控制台的实现"
date:   2017-06-05 20:00:00
published: true
categories: windows
---

# 前言

[Privexec](https://github.com/M2Team/Privexec) 是笔者借鉴远景好友 MouriNaruto 开发的 [NSudo](https://github.com/M2Team/NSudo) 开发的一个改变权限执行进程的工具，支持提权和降权。其中 wsudo 是 Privexec 的命令行版本。

在 wsudo 中，笔者使用了 Privexec.Console 提供彩色输出，截图如下：

![wsudo](https://raw.githubusercontent.com/M2Team/Privexec/master/images/wsudo.png)


![wsudo2](https://raw.githubusercontent.com/M2Team/Privexec/master/images/wsudo2.png)

本文将讲述如何在 Windows 中实现同时支持标准控制台和 MSYS2 Cygwin 以及 VT 模式的 Windows 控制台颜色输出。

## 标准控制台彩色输出

`_cputws` `__dcrt_write_console_w` `WriteConsoleW`

ReactOS `CsrCaptureMessageBuffer` https://github.com/reactos/reactos/blob/master/reactos/dll/win32/kernel32/client/console/readwrite.c

Windows 7 or Later Conhost

https://blogs.technet.microsoft.com/askperf/2009/10/05/windows-7-windows-server-2008-r2-console-host/

![Windows](https://msdnshared.blob.core.windows.net/media/TNBlogsFS/BlogFileStorage/blogs_technet/askperf/WindowsLiveWriter/Windows7WindowsServer2008R2ConsoleHost_7F3D/image_c064c0f7-4048-4dba-86bd-4a9722b53a11.png)

![Windows7OrLater](https://msdnshared.blob.core.windows.net/media/TNBlogsFS/BlogFileStorage/blogs_technet/askperf/WindowsLiveWriter/Windows7WindowsServer2008R2ConsoleHost_7F3D/image_7f7ebef5-47db-4d0c-aa78-5dd0e6bb75c8.png)

https://blogs.windows.com/buildingapps/2014/10/07/console-improvements-in-the-windows-10-technical-preview/

## 终端模拟器颜色输出


## VT 模式颜色输出
