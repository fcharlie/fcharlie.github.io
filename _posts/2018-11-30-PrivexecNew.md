---
layout: post
title:  "Privexec 杂谈"
date:   2018-11-30 21:00:00
published: false
categories: windows
---

# 前言

本站的开篇就是讲的 [《Windows AppContainer 降权，隔离与安全》](https://forcemz.net/container/2015/06/12/AppContainer/)，一晃三年多过去了，这三年之中，我开发了一个 [Privexec](https://github.com/M2Team/Privexec)，一个以其他权限启动进程的工具，支持启动 `AppContainer` 进程，前段实现有用户发起了功能请求<sup>1</sup>，让 `Privexec` 支持设置 `AppContaner` 的 `Capabilities`，而不是像以前一样在启动 `AppContainer` 进程时使用 `CreateWellKnownSid` 创建所有的与 `AppContainer` 相关的 `Capabilities SIDs`。于是乎，我就花了一点时间将 Privexec 重构了一番，有所感悟，便按键写下来。

## Process

在 Windows 平台上，创建进程有 `WinExec`，`system`，`_spawn/_wspawn`，`CreateProcess`，`ShellExecute` 等多种途径，但绝大多数使用的是基于`CreateProcess Family` 封装的。CreateProcess 家族如下：

|Function|Feature|Details|
|---|---|---|
|CreateProcessW/A|创建常规进程，权限继承父进程权限|支持 EXTENDED_STARTUPINFO_PRESENT 设置 AppContainer|
|CreateProcessAsUserW/A|使用主 Token 创建进程，子进程权限与 Token 限定一致|调用者须有 SE_INCREASE_QUOTA_NAME 权限|
|CreateProcessWithTokenW|使用主 Token 创建进程，子进程权限与 Token 限定一致|调用者须有 SE_IMPERSONATE_NAME 权限|
|CreateProcessWithLogonW/A|使用指定用户凭据启动进程||

`CreateProcessWithTokenW` 是 `Windows Vista` 才引入桌面系统的，没有 ANSI 版本，Github 上最常见的用法是打开桌面进程，获取其 Token，然后拷贝这个 Token 创建与桌面权限一致的进程，这是一种降权机制，仅当当前登录用户是标准用户才能降权成功。
`CreateProcessWithTokenW` 内部在调用好几个函数之后才会调用 `CreateProcessAsUserW`，在 `Privexec` 中并未使用它。
在 `ReactOS` 中，没有实现 `CreateProcessWithTokenW`，下面附上 `CreateProcessWithTokenW` 调用图：

![CreateProcessWithTokenW](https://i.imgur.com/V51b0vx.png)
![CreateProcessWithTokenW](https://i.stack.imgur.com/Vn7Qe.png)

实际上在 `ReactOS` 中，`CreateProcessAsUser` 最终依然调用的是 `CreateProcess` 创建进程，然后将进程挂起，设置好 `Token` 后再恢复进程。

在 `Windows` 平台，`CreateProcess`

Privexec 需要实现管理员提升到 `System` `TrustInstaller` 权限，还需要实现 UAC 降权，启动低完整性进程，`AppContainer` 进程。

<!-- 通过计划任务降权或者 UAC 提权都是无法继承调用者的进程句柄的，这是由于实际上调用者分别是 `taskschd` 服务和 `` -->

<!-- AicLaunchAdminProcess-->
## Privexec


## WSUDO


## Details

1.   [Feature Request: AppContainer "Capabilities" Selection](https://github.com/M2Team/Privexec/issues/2)
2.   TODO