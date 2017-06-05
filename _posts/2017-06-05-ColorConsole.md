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


## 终端模拟器颜色输出


## VT 模式颜色输出
