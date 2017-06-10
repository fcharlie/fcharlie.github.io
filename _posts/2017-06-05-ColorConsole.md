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

![wsudo3](https://raw.githubusercontent.com/M2Team/Privexec/master/images/wsudo3.png)

本文将讲述如何在 Windows 中实现同时支持标准控制台和 MSYS2 Cygwin 终端模拟器以及 VT 模式的 Windows 控制台颜色输出。

## 关于标准输出

大部分编程语言的入门从 `Helloworld` 开始，也就是 `Helloworld` 输出到标准输出。在 C++ 中使用 `std::cout` ，在 C 使用 `printf` 以及 C# 使用 `Console.Write` 等等。进程启动时，操作系统或者父进程会设置好进程的标准输出<sup>1</sup>。默认情况下，标准输出设备是 `控制台 console` 或者是 `终端 tty` 当然在启动进程前，可以将标准输出**重定向**到 `管道 (Pipe/Named Pipe, Pipe/FIFO)` `文件` 而在 Unix like 系统中，还可以将输出重定向到 `socket` 等其他 Unix 文件。在 Windows 上，如果要将 IO 重定向到 socket 需要使用 `WSASocket` 创建 socket，并 使用 flag `WSA_FLAG_OVERLAPPED` 。

输出的设备或者文件存在多样性，对于 CRT 而言，标准输出的实现就要兼顾这些设备，通常来说，操作系统会提供 `WriteFile` `write` 这样的 API 或者系统调用支持输出，这些函数的输出优先考虑的是本机默认编码，比如 Unix 上，一般都是 UTF-8，对于兼容性大户 Windows 来说，虽然内部编码都是 UTF-16 但是输出到文件时，任然优先选择的是本机 `Codepage` 也就是代码页，在简中系统中是 936.

## Printf 的心路历程

在知乎上，很早就有人提问：[printf()等系统库函数是如何实现的？](https://www.zhihu.com/question/28749911)

[Where the printf() Rubber Meets the Road](http://blog.hostilefork.com/where-printf-rubber-meets-road/)

`_cputws` `__dcrt_write_console_w` `WriteConsoleW`


`fputc (stdio/fputc.cpp)`
`__acrt_stdio_flush_and_write_narrow_nolock (_flsbuf.cpp)` `_write (lowio/write.cpp)`  `_write_nolock` `WriteFile`

`output_processor` `puttc_nolock` `_APPLY` `_fputc_nolock`


## WriteConsole 内部原理


ReactOS `CsrCaptureMessageBuffer` 

[WriteConsole](https://github.com/reactos/reactos/blob/master/reactos/dll/win32/kernel32/client/console/readwrite.c)

```c
HANDLE
TranslateStdHandle(IN HANDLE hHandle)
{
    PRTL_USER_PROCESS_PARAMETERS Ppb = NtCurrentPeb()->ProcessParameters;

    switch ((ULONG)hHandle)
    {
        case STD_INPUT_HANDLE:  return Ppb->StandardInput;
        case STD_OUTPUT_HANDLE: return Ppb->StandardOutput;
        case STD_ERROR_HANDLE:  return Ppb->StandardError;
    }

    return hHandle;
}
```
ReactOS 在使用 WriteFile 写入文件时，当文件是控制台时：

[WriteFile to Console](https://github.com/reactos/reactos/blob/40a16a9cf1cdfca399e9154b42d32c30b63480f5/reactos/dll/win32/kernel32/client/file/rw.c#L38)

```c++
#define IsConsoleHandle(h)  \
    (((ULONG_PTR)(h) & 0x10000003) == 0x3)
```

在 ReactOS CMD 中是这样做的：
```c++
BOOL IsConsoleHandle(HANDLE hHandle)
{
    DWORD dwMode;

    /* Check whether the handle may be that of a console... */
    if ((GetFileType(hHandle) & ~FILE_TYPE_REMOTE) != FILE_TYPE_CHAR)
        return FALSE;

    /*
     * It may be. Perform another test... The idea comes from the
     * MSDN description of the WriteConsole API:
     *
     * "WriteConsole fails if it is used with a standard handle
     *  that is redirected to a file. If an application processes
     *  multilingual output that can be redirected, determine whether
     *  the output handle is a console handle (one method is to call
     *  the GetConsoleMode function and check whether it succeeds).
     *  If the handle is a console handle, call WriteConsole. If the
     *  handle is not a console handle, the output is redirected and
     *  you should call WriteFile to perform the I/O."
     */
    return GetConsoleMode(hHandle, &dwMode);
}
```

https://doxygen.reactos.org/index.html

Windows 7 or Later Conhost

https://blogs.technet.microsoft.com/askperf/2009/10/05/windows-7-windows-server-2008-r2-console-host/

![Windows](https://msdnshared.blob.core.windows.net/media/TNBlogsFS/BlogFileStorage/blogs_technet/askperf/WindowsLiveWriter/Windows7WindowsServer2008R2ConsoleHost_7F3D/image_c064c0f7-4048-4dba-86bd-4a9722b53a11.png)

![Windows7OrLater](https://msdnshared.blob.core.windows.net/media/TNBlogsFS/BlogFileStorage/blogs_technet/askperf/WindowsLiveWriter/Windows7WindowsServer2008R2ConsoleHost_7F3D/image_7f7ebef5-47db-4d0c-aa78-5dd0e6bb75c8.png)

https://blogs.windows.com/buildingapps/2014/10/07/console-improvements-in-the-windows-10-technical-preview/

## 控制台彩色输出


Windows 控制台支持 16 色输出。

```csharp
    [Serializable]
    public enum ConsoleColor
    {
        Black = 0,
        DarkBlue = 1,
        DarkGreen = 2,
        DarkCyan = 3,
        DarkRed = 4,
        DarkMagenta = 5,
        DarkYellow = 6,
        Gray = 7,
        DarkGray = 8,
        Blue = 9,
        Green = 10,
        Cyan = 11,
        Red = 12,
        Magenta = 13,
        Yellow = 14,
        White = 15
    }
```


## 终端模拟器颜色输出

在 Windows 上，还有 Cygwin 和 MSYS2 MSYS 这样的模拟 Unix 的环境。wsudo 对齐支持也有必要。

## VT 模式颜色输出


[24-bit Color in the Windows Console!](https://blogs.msdn.microsoft.com/commandline/2016/09/22/24-bit-color-in-the-windows-console/)

[Console Virtual Terminal Sequences](https://msdn.microsoft.com/en-us/library/windows/desktop/mt638032.aspx)

[support 256 color](https://github.com/Microsoft/BashOnWindows/issues/345)

## 其他

[Add emoji support to Windows Console](https://github.com/Microsoft/BashOnWindows/issues/590)

[UTF-8 rendering woes](https://github.com/Microsoft/BashOnWindows/issues/75#issuecomment-304415019)

## 备注
1. 父进程未显式设置标准输入输出和标准错误时，子进程会继承父进程的值，在 Windows 中，GUI 程序的标准输入输出和 Unix 下重定向到 `/dev/null` 类似，但启动的 CUI 子进程默认下依然有控制台窗口
