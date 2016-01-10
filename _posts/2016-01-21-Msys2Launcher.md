---
layout: post
title:  "MSYS2 Launcher 开发杂记"
date:   2016-01-21 21:30:16
published: false
categories: toolset
---
#前言
在我前面的博客中，曾经写过一片 [《基于清单的启动器的实现》](http://forcemz.net/toolset/2015/11/27/NewLauncher.html) ,说了几类新的 Launcher ，
而在本文中，依旧说的是启动器，这次说的是 MSYS2 启动器，顺带说一说 使用 Win32 API 在 MSVC，Mingw64, Cygwin 中相互兼容的策略。

#Minimal GNU（POSIX）system on Windows     
最开始接触 MSYS 已经有一些年头了，那个时候，我不知疲倦的捣鼓着各种开发工具，写下一行行 "Hello world"，从网络上找到各种简单的例子，
然后在 Mingw，Cygwin , Microsoft Visual C++ ,OpenWATCOM 等开发工具下编译。MSYS 通常是和 Mingw 一起的，Mingw (Minimalist GNU for Windows
) 提供一套 GNU 工具集和 Win32 API ，也就是 w32api，而 MSYS 则提供一个有限的 POSIX 模拟环境，使用 Mingw 编译的项目可以不依赖第三方库。     
MSYS 提供了诸如 bash tar wget ls ln 等 Unix 上不可或缺的工具，但同时工具集的版本存在严重的滞后，MSYS 最近几年非常不活跃。

##Mintty    
在使用 Mingw 的时候，比较舒服的是 MSYS 提供的 Mintty 工具，这是一个终端模拟器，在 Unix like 系统上，终端模拟器是非常常见的工具，Ubuntu 有 Terminal,
Kubuntu 有 Kconsole。而在 Windows 平台上，终端模拟器还是比较少见的，这是因为 Windows 本质上是一个图形化操作系统，人们直接过运行运行命令行程序，
然后操作系统自动分配一个控制台窗口，并将程序的输入输出定向到此控制台窗口，人们也可以通过运行 CMD 或者 PowerShell shell 工具来执行命令行程序，实际上
二者本身也是命令行程序，也就是子系统为 Console（GUI 程序也可以使用 AllocConsole 分配一个控制台窗口或者 AttachConsole 绑定到父进程的控制台窗口）。

Mintty 子系统是 WINDOWS ,是一个地地道道的 GUI 程序，这意味着，它有完整的消息循环，当然可以有更多的自定义设置。

Windows 子系统 ( Link version 14)：    

>BOOT_APPLICATION | CONSOLE | EFI_APPLICATION | EFI_BOOT_SERVICE_DRIVER | EFI_ROM |
>EFI_RUNTIME_DRIVER | NATIVE | POSIX | WINDOWS | WINDOWSCE


#MSYS 2


#MSYS2 launcher