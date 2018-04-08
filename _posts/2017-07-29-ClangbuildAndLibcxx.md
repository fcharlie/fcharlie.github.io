---
layout: post
title:  "Clangbuilder 和 libcxx"
date:   2017-07-29 10:00:00
published: true
categories: developer
---

# 前言

作为一个 C++ 开发者，自然少不了与编译器打交道。
笔者在 2013年1月17日 发布了第一个 ClangOnWindows 二进制包 [ClangOnWindows](https://sourceforge.net/projects/clangonwin/)，截至 2017-07-29，一共被下载 19212 次，实际上构建 LLVM/Clang 是一个非常耗时的事，于是乎笔者于 2014 年国庆节期间编写了 Clangbuilder，这是一个基于 Powershell 的 LLVM/Clang 自动化构建工具，能够自动下载工具链（除 Visual Studio）,自动获取 LLVM 源码，自动执行构建命令。Clangbuilder 的初期并不支持 libcxx 的构建，原因无他，当时 libcxx 不支持 Windows (for MSVC)，现在依然如此，不过现在可以使用 `clang-cl` 构建 libcxx。本文即介绍通过 Clangbuilder 构建 libcxx。


## Clangbuilder

[Clangbuilder](https://github.com/fstudio/clangbuilder) 是一个基于 Powershell 的 LLVM/Clang 自动化构建工具，通常用户在双击 `InitializeEnv.bat` 脚本后，安装脚本将自动安装构建 LLVM/Clang 所需的工具依赖。核心以来如下：

```json
{
    "core": [
        "7z",
        "cmake",
        "git",
        "ninja",
        "nsis",
        "nuget",
        "python2",
        "vswhere"
    ]
}
```

Clangbuilder devinstall 还支持如下工具：

```txt
7z                  18.03               7-Zip is a file archiver with a high compression ratio
aria2               1.33.1              The ultra fast download utility
cmake               3.11.0              CMake is an open-source, cross-platform family of tools designed to build, test and package software
curl                7.59.0              Curl is a command-line tool for transferring data specified with URL syntax.
git                 2.17.0              Git is a modern distributed version control system focused on speed
gnuutils            1.0                 GNU utils for Windows
hg                  4.5.2               Mercurial is a free, distributed source control management tool.
ninja               1.8.2               Ninja is a small build system with a focus on speed.
nsis                3.03                NSIS (Nullsoft Scriptable Install System) is a professional open source system to create Windows installers.
nuget               4.6.1               NuGet is the package manager for .NET. The NuGet client tools provide the ability to produce and consume packages.
openssh             v7.6.1.0p1-Beta     Portable OpenSSH
putty               0.70                PuTTY: a free SSH and Telnet client.
python2             2.7.14              Python 2.7
swigwin             3.0.12              Simplified Wrapper and Interface Generator
vswhere             2.4.1               Locate Visual Studio 2017 and newer installations.
wget                1.19.4              A command-line utility for retrieving files using HTTP, HTTPS and FTP protocols.
```

我们支持 msi 和 zip 以及单文件工具，当安装 `7z` 后，可以解压 `tar.gz`，`.iso` 等格式的文件

下载依赖后，Clangbuilder 会自动构建一个图形化工具叫 `ClangbuilderUI`，用户可以使用 ClangbuilderUI 来一键构建或者启动环境。

![CangbuilderUI](https://github.com/fstudio/clangbuilder/raw/master/docs/images/cbui.png)


由于 Windows 10 新增了很多特性，ClangbuilderUI 目前已经不支持 Windows 10 以下的版本，开发者可以自己去修改 ClangbuilderUI。

## Libc++

Libc++ 是一个非常优秀的 C++ 标准库的实现，目前是 macOS iOS 上默认的 C++ 标准库，其开发者非常活跃，因此，此项目在吸收新技术的方面值得称赞，比如 libc++ 目前已经实验性的支持 `coroutine` 了（微软 Visual C++ 团队大牛贡献）。由于是重新实现的，避免了很多 C++98 时期以来的坑。但是在 Windows 上一直处于无法编译的尴尬，使用 MinGW 编译的也就只能使用 MinGW 的一系列工具了，在 libcxx 开发者和 Visual C++ STL 开发者 STL 等人的共同努力下，libc++ 终于能够被 `clang-cl` 编译，由于目前 Visual C++ 不支持 `include_next` 特性，因此也就不能编译 libcxx。

详细的文档可以查看 [Experimental Support for Windows](http://libcxx.llvm.org/docs/BuildingLibcxx.html#experimental-support-for-windows)

因此在 Windows 上基于 MSVC 构建兼容的 Clang 版本时如果需要构建 libcxx 时，需要支持编译器为 clang-cl，而 Clangbuilder 目前支持 `NinjaBootstarp` 和 `NinjaIterate` 两种机制构建 libcxx ，在 ClangbuilderUI 中选择相应的 Engine 即可。第一种顾名思义就是使用 Ninja 自举，先使用 MSVC 构建第一个版本的 Clang, 然后再使用 CMake 生成 ninja 构建文件，这是将编译器替换为 clang-cl，然后设置编译 Libcxx， 如果代码没有错误，也就构建成功了。

而 NinjaIterate 需要预先构建的 clang-cl，这种非常适用于笔者这种经常构建 clang 的人士，修改 [config/prebuilt.json](https://github.com/fstudio/clangbuilder/blob/master/config/prebuilt.json) 设置好预构建的 clang 的路径和架构，Clangbuilder 将自动识别并构建。没有代码错误就能很快构建成功。

```json
{
    "LLVM": {
        "Path": "D:/LLVM",
        "Arch": "x64"
    }
}
```

Clangbuilder 支持将 LLVM 自动打包成安装文件，由于 libcxx 的项目文件并未将 `c++.dll` 安装配置好，因此，在使用 libc++ 的时候需要将 c++.dll 拷贝道 PATH 或者项目目录等。

使用 libc++ 的命令行：

>clang++ -std=c++11 -stdlib=libc++ -nostdinc++ -Iinclude\c++\v1 -Llib hello.cc -lc++

然后就可以运行程序了。

也可以使用

>clang-cl -std:c++14  -Iinclude\c++\v1 hello.cc c++.lib

值得注意的是，静态链接似乎是不起作用的。

## 最后

Clangbuilder 是个好工具，希望对大家有用。