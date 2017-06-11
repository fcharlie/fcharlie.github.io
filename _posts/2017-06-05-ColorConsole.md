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

大部分编程语言的入门从 `Helloworld` 开始，也就是将文本 `Helloworld` 输出到标准输出。在 C++ 中使用 `std::cout` ，在 C 中使用 `printf` 以及在 C# 中使用 `Console.Write`。进程启动时，操作系统或者父进程会设置好进程的标准输出<sup>1</sup>。默认情况下，标准输出设备是 `控制台 console` 或者是 `终端 tty` 当然在启动进程前，可以将标准输出**重定向**到 `管道 (Pipe/Named Pipe, Pipe/FIFO)` `文件` 而在 Unix like 系统中，还可以将输出重定向到 `socket` 等其他 Unix 文件。在 Windows 上，如果要将 IO 重定向到 socket 需要使用 `WSASocket` 创建 socket，并 使用 flag `WSA_FLAG_OVERLAPPED` 。

输出的设备或者文件存在多样性，对于 CRT 而言，标准输出的实现就要兼顾这些设备，通常来说，操作系统会提供 `WriteFile` `write` 这样的 API 或者系统调用支持输出，这些函数的输出优先考虑的是本机默认编码，比如 Unix 上，一般都是 UTF-8，对于兼容性大户 Windows 来说，虽然内部编码都是 UTF-16 但是输出到文件时，任然优先选择的是本机 `Codepage` 也就是代码页，在简中系统中是 936.

## Printf 的心路历程

在一起我曾经思考过 `printf` 是如何实现的，很多开发者在开始也有同样的疑惑，在知乎上，就有人提问：[printf()等系统库函数是如何实现的？](https://www.zhihu.com/question/28749911) ，在这个问题下，也有很多人回复了，有兴趣的用户可以看一下，在 Unix 的 CRT 中，printf 的调用历程在这篇文章中有详细介绍：[Where the printf() Rubber Meets the Road](http://blog.hostilefork.com/where-printf-rubber-meets-road/)

在 Windows 10 中，新增了 `Universal CRT (UCRT)`  [CRT Library Features](https://msdn.microsoft.com/en-us/library/abx4dbyh.aspx)，[Introducing the Universal CRT](https://blogs.msdn.microsoft.com/vcblog/2015/03/03/introducing-the-universal-crt/)，与之前的 Visual C++ CRT 有了很大的不同，全部代码使用 C++11 重构，不用疑惑，正是使用 C++11 实现 **C** Runtime。笔者对 printf 的分析也是基于 ucrt 源码。

Visual C++ 会将 CRT/C++ STL 源码一同发布（没有构建文件），在 Visual Studio 的安装目录下的 `VC\crt\src` ，而 `UCRT` 源码则在 `%ProgramFiles(x86)%Windows Kits\10\Source\$BuildVersion\ucrt` 

在 UCRT 中 printf 是个内联函数，调用了 `_vfprintf_l`，`_vfprintf_l` 也是内联 的，它调用了 `__stdio_common_vfprintf`。在 ucrt 源码路径 `stdio\output.cpp` 中 `__stdio_common_vfprintf` 调用了模板函数 `common_vfprintf` ，而 `common_vfprintf` 则在内部调用了模板类 `output_processor` ,`output_processor` 使用了输出适配器模板类 `stream_output_adapter` 

```c++
template <typename Character>
class stream_output_adapter
    : public output_adapter_common<Character, stream_output_adapter<Character>>
{
public:
    typedef __acrt_stdio_char_traits<Character> char_traits;

    stream_output_adapter(FILE* const public_stream) throw()
        : _stream{public_stream}
    {
    }

    bool validate() const throw()
    {
        _VALIDATE_RETURN(_stream.valid(), EINVAL, false);

        return char_traits::validate_stream_is_ansi_if_required(_stream.public_stream());
    }

    bool write_character_without_count_update(Character const c) const throw()
    {
        if (_stream.is_string_backed() && _stream->_base == nullptr)
        {
            return true;
        }

        return char_traits::puttc_nolock(c, _stream.public_stream()) != char_traits::eof;
    }

    void write_string(
        Character const*      const string,
        int                   const length,
        int*                  const count_written,
        __crt_deferred_errno_cache& status
        ) const throw()
    {
        if (_stream.is_string_backed() && _stream->_base == nullptr)
        {
            *count_written += length;
            return;
        }

        write_string_impl(string, length, count_written, status);
    }

private:

    __crt_stdio_stream _stream;
};
```

File stream 的写入流程是 `write_strings` -> `write_string_impl` -> `write_character` -> `write_character_without_count_update` ,然后是 `char_traits::puttc_nolock`。

```c++
#define _CORECRT_GENERATE_FORWARDER(prefix, callconv, name, callee_name)                     \
    __pragma(warning(push))                                                                  \
    __pragma(warning(disable: 4100)) /* unreferenced formal parameter */                     \
    template <typename... Params>                                                            \
    prefix auto callconv name(Params&&... args) throw() -> decltype(callee_name(args...))    \
    {                                                                                        \
        _BEGIN_SECURE_CRT_DEPRECATION_DISABLE                                                \
        return callee_name(args...);                                                         \
        _END_SECURE_CRT_DEPRECATION_DISABLE                                                  \
    }                                                                                        \
    __pragma(warning(pop))
```

`char_traits::puttch_nolock` 实际上是通过 `__acrt_stdio_char_traits` `__crt_char_traits` 定义的静态成员函数，，printf 对应的是 `_fputc_nolock`。

`_fputc_nolock` 和 `fputc` 类似，实际上 `fputc` 也会调用它，在 `_fputc_nolock` 中调用了 `__acrt_stdio_flush_and_write_narrow_nolock`，在源码 `stdio/_flsbuf.cpp` 中，
`__acrt_stdio_flush_and_write_narrow_nolock` 又调用了 `common_flush_and_write_nolock<char>` 

往下一步走会调用 `write_buffer_nolock` -> `_write` ->`_write_nolock(lowio/write.cpp)` 

然后根据不同设备和字符类型，`_write_nolock` 会调用：

+  Console ANSI write_double_translated_ansi_nolock
+  Console UTF16 write_double_translated_unicode_nolock
+  File ANSI write_text_ansi_nolock
+  File UTF16 write_text_utf16le_nolock
+  File UTF8 write_text_utf8_nolock
+  File Binary write_binary_nolock

最后终究要调用 `WriteFile`, 所以读写文件在 Windows 上为什么不使用 `WriteFile` ? 

值得一提的是，在 Windows 中，如果使用 fopen 打开文件，尽量使用 `rb` `wb` 之类的标志，显示的制定文件类型是 `binary`, 否则自动添加 `CR` 就不好了。

通过源码，我们还知道 UTF-16 或者 UTF-8 一般还是需要转换成 Ansi 才能输出到控制台。

UCRT 还提供了 `_cputs` `_cprintf` `_cputws` `_cwprintf`  这样的函数，这些函数处理流程类似但要简单的多，`output_processor` 的输出适配器是 `console_output_adapter`，无论是字符类型是 wchar_t 还是 char 最终都会调用 `_putwch_nolock` 及 `__dcrt_write_console_w`，最后使用 `WriteConsoleW` 写入到控制台。

`_cwprintf` 与 `wprintf` 相比，输出 Unicode 字符要容易的多，不过，在使用标准输出的时候，你不能假定程序一定拥有控制台。

## WriteConsole 内部原理

虽然在 Windows/ReactOS 中，CRT 写入到标准输出的使用了 `WriteFile` ，WriteFile 是如何写到控制台的？

在 `ReactOS` 源码中，WriteFile 将检查 `hFile` 其值是否为 `STD_INPUT_HANDLE` ,`STD_OUTPUT_HANDLE` ,`STD_ERROR_HANDLE`  如果是就从 PEB 中获得对应的控制台句柄，否则使用句柄 `hFile` 原本的值，然后就判断是否是控制台句柄，如果是控制台，则调用 `WriteConsoleA`，对于其他类型文件会直接调用 `NtWriteFile`。

将 `STD_*_HANDLE` 转换为 Windows 内核对象：

```c++
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

判断是否是控制台文件：

```c++
#define IsConsoleHandle(h)  \
    (((ULONG_PTR)(h) & 0x10000003) == 0x3)
```

或者

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

有兴趣的可以参阅 ReactOS WriteFile 源码：
[WriteFile to Console](https://github.com/reactos/reactos/blob/40a16a9cf1cdfca399e9154b42d32c30b63480f5/reactos/dll/win32/kernel32/client/file/rw.c#L38)

对于控制台句柄，CloseHandle，ReadFile，CreateFile ，以及 WriteFile 都要单独的调用对象的控制台 API。比如说，如果文件名是 `CON`, `CONOUT$`, `CONIN$`, `\\.\CON` 时就会使用 `OpenConsoleW` 打开控制台。然后返回控制台的句柄。

WriteConsoleA 又是如何写入到图形界面呢？在 Windows Technet 有两幅图分别介绍了 Vista 以前的控制台结构和 Windows 7 的控制台架构 [Windows 7 / Windows Server 2008 R2: Console Host](https://blogs.technet.microsoft.com/askperf/2009/10/05/windows-7-windows-server-2008-r2-console-host/)

在 Windows 7 以前，WriteConsole 通过 LPC 与 CSRSS（Client Server Runtime Process） 通信：

![Windows](https://msdnshared.blob.core.windows.net/media/TNBlogsFS/BlogFileStorage/blogs_technet/askperf/WindowsLiveWriter/Windows7WindowsServer2008R2ConsoleHost_7F3D/image_c064c0f7-4048-4dba-86bd-4a9722b53a11.png)

由于CSRSS 以 `Local System` 权限运行，这样的逻辑容易导致 [Shatter attack](https://en.wikipedia.org/wiki/Shatter_attack)，于是在 Windows 7 中出现了新的 `Console Host` ：

![Windows7OrLater](https://msdnshared.blob.core.windows.net/media/TNBlogsFS/BlogFileStorage/blogs_technet/askperf/WindowsLiveWriter/Windows7WindowsServer2008R2ConsoleHost_7F3D/image_7f7ebef5-47db-4d0c-aa78-5dd0e6bb75c8.png)

在这种架构中，WriteConsole LPC 调用将控制台消息发送到了一个 Conhost 宿主进程，这个进程是在 CreateProcess 中自动创建的。

在 ReactOS 中，依然使用的是 `CsrCaptureMessageBuffer`  将数据发送到 CSRSS。源码在这里： [WriteConsole](https://github.com/reactos/reactos/blob/master/reactos/dll/win32/kernel32/client/console/readwrite.c)

ReactOS 文档：[ReactOS](https://doxygen.reactos.org/index.html)


WriteFile 输出到控制台时，实际调用的是 **WriteConsoleA**，在前面我们还知道使用 wprintf 时，CRT 会将文本内容转换成 Ansi(Codepage) 然后再使用 WriteFile 写入到控制台窗口。

我们知道，绘制字符的时候，ANSI 将会最终转换成 UTF-16LE 编码，然后经 DrawTextExW 或者其他函数绘制出来，如果使用了 wprintf 这样的函数，势必会经过两次转换 **UTF16->Codepage->UTF16**，并且由于 Codepage 的字符集一般是不全面的，这就导致字符编码在转换的时候发生丢失，比如 emoji 之类还是不要妄想通过 wprintf 输出。

在 Windows 中，内码是 Unicode，而控制台也支持使用 `WriteConsoleW` 这样的 API 输出文本，如果我们直接使用 `WriteConsoleW` 就可以避免出现字符无法呈现或者乱码的问题了。如果控制台的图形对各种字体字符支持更好，这个 API 也就能够输出彩色字符或者更多的 Emoji，遗憾的是，目前 Console 的改进任然在计划中，暂时没有完成。

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

[Console Improvements in the Windows 10 Technical Preview](https://blogs.windows.com/buildingapps/2014/10/07/console-improvements-in-the-windows-10-technical-preview/)

[Add emoji support to Windows Console](https://github.com/Microsoft/BashOnWindows/issues/590)

[UTF-8 rendering woes](https://github.com/Microsoft/BashOnWindows/issues/75#issuecomment-304415019)

[Alacritty - A cross-platform, GPU-accelerated terminal emulator](https://github.com/jwilm/alacritty)

## 备注
1. 父进程未显式设置标准输入输出和标准错误时，子进程会继承父进程的值，在 Windows 中，GUI 程序的标准输入输出和 Unix 下重定向到 `/dev/null` 类似，但启动的 CUI 子进程默认下依然有控制台窗口
