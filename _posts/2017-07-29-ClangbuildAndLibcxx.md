---
layout: post
title:  "Clangbuilder 和 libcxx"
date:   2017-07-29 10:00:00
published: true
categories: developer
---

# 前言

作为一个 C++ 开发者，自然少不了与编译器打交道。
笔者在 2013年1月17日 发布了第一个 ClangOnWindows 二进制包 [ClangOnWindows](https://sourceforge.net/projects/clangonwin/)，时至今日，一共被下载 19212 次，实际上构建 LLVM/Clang 是一个非常耗时的事，于是乎笔者于 2014 年国庆节期间编写了 Clangbuilder，这是一个基于 Powershell 的 LLVM/Clang 自动化构建工具，能够自动下载工具链（除 Visual Studio）,自动获取 LLVM 源码，自动执行构建命令。Clangbuilder 的初期并不支持 libcxx 的构建，原因无他，当时 libcxx 不支持 Windows (for MSVC)，现在依然如此，不过现在可以使用 `clang-cl` 构建 libcxx。本文即介绍通过 Clangbuilder 构建 libcxx。


## Clangbuilder

[Clangbuilder](https://github.com/fstudio/clangbuilder) 是一个基于 Powershell 的 LLVM/Clang 自动化构建工具，通常用户在双击 `install.bat` 脚本后，脚本将自动安装以来，所有的依赖配置在 [config/package.json](https://github.com/fstudio/clangbuilder/blob/master/config/packages.json) 中。

|工具|版本|
|---|---|
|MinGit|2.13.3|
|CMake|3.9.0|
|Python(Embed)|3.6.2|
|Subversion|1.9.4|
|NSIS|3.0|
|gnuwin|1.0|
|ninja|1.7.2|
|swigwin|3.0.11|
|vswhere|2.1.3|
|nuget|4.1.0|