---
layout: post
title:  "Git Analyze 工具实现与原理"
date:   2016-08-12 20:00:00
published: true
categories: git
---

# 前言



## Analyze 


## Rollback

## Pal

libgit2 使用的是 UTF-8 编码,在 Windows 中转变为 UTF16 编码,使用 Windows API 完成一系列操作.

如果按照默认的 main 传递命令行参数,那么可能会发生错误,在 Windows 中, 创建进程是通过 CreateProcess 这样的 API 实现的, 
NT 内核将命令行参数写入的进程的 PEB 中, CRT 初始化时,根据启动函数类型执行不同的策略 (WinMain wWinMain main wmain) ,
比如 main , CRT 通过  GetCommandLineA 获得命令行参数,然后将 LPCSTR 转变成 char * Argv[] 的形式. GetCommandLineA 获得
的命令行参数也是由 PEB 的命令行参数转换编码过来的. main 命令行参数的编码即当前代码页的编码,也就是 CP_ACP ,
比如 Windows 下常见的 936 GBK. 

这样一来,libgit2 传入非 西文字符 就会操作失败, 为了支持 Windows 平台,笔者使用 wmain ,然后将命令行参数依次转变为 UTF-8,
这样就可以解决不支持非西文字符的问题.



