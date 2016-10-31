---
layout: post
title:  "Windows 下载功能的实现 - C++ 篇"
date:   2016-11-03 20:00:00
published: true
categories: windows
---

# 前言


## URLDownloadToFile (Urlmon.dll)

## HttpClient (Windows Runtime -cppwinrt)

## IBackgroundCopyJob (BITS)

[BITS](https://msdn.microsoft.com/en-us/library/windows/desktop/aa362708(v=vs.85).aspx) - Background Intelligent Transfer Service 
(后台智能传输服务) 是 Windows 的一个重要功能.
笔者在开发 [Clangbuilder](https://github.com/fstudio/clangbuilder) 时,需要使用 PowerShell 下载软件使用的 cmdlet 是 Start-BitsTransfer,
Start-BitsTransfer 实际上使用的是 Windows 的 BITS 服务,而 Windows 更新功能也是使用的 BITS, Chrome 同步扩展也是使用的 BITS.

## WinHTTP family


