---
layout: post
title:  "Privexec 杂谈"
date:   2018-11-30 21:00:00
published: false
categories: windows
---

# 前言

本站的开篇就是讲的 [《Windows AppContainer 降权，隔离与安全》](https://forcemz.net/container/2015/06/12/AppContainer/)，一晃三年多过去了，这三年之中，我开发了一个 [Privexec](https://github.com/M2Team/Privexec)，一个以其他权限启动进程的工具，支持启动 `AppContainer`，前段实现拥有胡报告请求<sup>1</sup>支持设置 `AppContaner` 的 `Capabilities`，而不是像以前一样直接设置了所有的与 `AppContainer` 相关的 `WellKnownSID`。于是乎，我就花了一点时间将 Privexec 重构了一番，有所感悟，便按键写下来。

## Process

## Privexec


## WSUDO


## Details

1.   [Feature Request: AppContainer "Capabilities" Selection](https://github.com/M2Team/Privexec/issues/2)
2.   TODO