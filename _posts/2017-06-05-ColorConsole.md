---
layout: post
title:  "Privexec 的内幕（一）标准输出原理与彩色输出实现"
date:   2017-06-05 20:00:00
published: true
categories: windows
---

# 前言

[Privexec](https://github.com/M2Team/Privexec) 是笔者借鉴远景好友 MouriNaruto 的 [NSudo](https://github.com/M2Team/NSudo) 而开发的一个**提权或者降权**执行进程的工具。其中 wsudo 是 Privexec 的命令行版本。

在 wsudo 中，笔者使用了 Privexec.Console 提供彩色输出，截图如下：

![wsudo](https://raw.githubusercontent.com/M2Team/Privexec/master/docs/images/wsudo.png)


![wsudo3](https://raw.githubusercontent.com/M2Team/Privexec/master/images/doscs/wsudo3.png)

本文将讲述标准输出是如何输出到控制台的，以及怎样在 Windows 中实现同时支持标准控制台和 MSYS2 Cygwin 终端模拟器以及 VT 模式的控制台彩色输出。

## 关于标准输出

大部分编程语言的入门从 `Helloworld` 开始，也就是将文本 `Helloworld` 输出到标准输出。在 C++ 中使用 `std::cout` ，在 C 中使用 `printf` 以及在 C# 中使用 `Console.Write`。进程启动时，操作系统或者父进程会设置好进程的标准输出<sup>1</sup>。默认情况下，标准输出设备是 `控制台 console` 或者是 `终端 tty` 当然在启动进程前，可以将标准输出**重定向**到 `管道 (Pipe/Named Pipe, Pipe/FIFO)`，`文件` 而在 Unix like 系统中，还可以将输出重定向到 `socket` 等其他 Unix 文件。在 Windows 上，如果要将 IO 重定向到 socket 需要使用 `WSASocket` 创建 socket，且不要使用 `WSA_FLAG_OVERLAPPED` 标志。

输出的设备或者文件存在多样性，对于 CRT 而言，标准输出的实现就要兼顾这些设备，通常来说，操作系统会提供 `WriteFile` `write` 这样的 API 或者系统调用支持输出，一般来说，printf 这样的函数也是使用这样的 API 或者系统调用实现。这些函数的输出优先考虑的是本机默认编码，比如 Unix 上，一般都是 UTF-8，对于兼容性大户 Windows 来说，虽然内部编码都是 UTF-16 但是输出到文件时，任然优先选择的是本机代码页，比如在简体中文系统中是，代码页也就是 936。

## Printf 的心路历程

在以前我曾经思考过 `printf` 是如何实现的，很多开发者在开始也有同样的疑惑，在知乎上，就有人提问：[printf()等系统库函数是如何实现的？](https://www.zhihu.com/question/28749911) ，在这个问题下，有很多人回复了，有兴趣的用户可以看一下；

在 Unix 的 CRT 中，printf 的调用历程在这篇文章中有详细介绍：[Where the printf() Rubber Meets the Road](http://blog.hostilefork.com/where-printf-rubber-meets-road/)

在 Windows 10 中，新增了 `Universal CRT (UCRT)`  [CRT Library Features](https://msdn.microsoft.com/en-us/library/abx4dbyh.aspx)，[Introducing the Universal CRT](https://blogs.msdn.microsoft.com/vcblog/2015/03/03/introducing-the-universal-crt/)，与之前的 Visual C++ CRT 有了很大的不同，全部代码使用 C++11 重构，不用疑惑，正是使用 C++11 实现 **C** Runtime。笔者对 printf 的分析也是基于 ucrt 。

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

最后终究要调用 `WriteFile`，对于不需要缓冲区的文件读写为什么不直接使用 `WriteFile` ？

值得一提的是，在 Windows 中，如果使用 FILE 读写文件，尽量使用 `rb` `wb` 之类的标志，显示的指定文件类型是 `binary`, 否则自动添加 `CR` 就不好了。

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

我们知道，绘制字符的时候，ANSI 文本最终将会转换成 UTF-16LE 文本，然后经 DrawTextExW 或者其他函数绘制出来，如果使用了 wprintf 这样的函数，势必会经过两次转换 **UTF16->CodePage->UTF16**，在 Unicode 中存在的字符不一定存在于代码页中，这就导致文本在转换编码的时候发生丢失，输出到控制台时，可能是乱码或者干脆截断了。所以在 Windows 控制台中， Unicode 编码，特别是 emoji 还是不要妄想通过 wprintf 输出。

在 Windows 中，内码是 Unicode，而控制台也支持使用 `WriteConsoleW` 这样的 API 输出文本，如果我们直接使用 `WriteConsoleW` 就可以避免出现字符无法呈现或者乱码的问题了。如果控制台的图形对各种字体字符支持更好，这个 API 也就能够输出彩色字符或者更多的 Emoji。遗憾的是，目前 Console 字体渲染的改进任然在计划中，暂时不支持 Emoji 和各种特殊字体。

## 控制台彩色输出

讲了这么长一段，该讲如何实现彩色输出了，首先，我们要知道 Windows 控制台 API 是支持颜色输出的，不过，这些 API 仅支持 16 色输出。
在 .Net Core [corefx](https://github.com/dotnet/corefx/blob/master/src/System.Console/src/System/ConsoleColor.cs#L10) 有如下一个枚举定义了控制台基本的颜色：

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
在 C++ 中，如果在 `WriteConsoleW` 调用之前使用了 `SetConsoleTextAttribute` 设置输出格式，那么就能输出带有上述颜色的文本内容了。在 Privexec.Console 中，使用控制台 API 输出颜色代码如下：

```c++
int WriteConhost(int color, const wchar_t *data, size_t len) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleScreenBufferInfo(hConsole, &csbi);
  WORD oldColor = csbi.wAttributes;
  WORD color_ = static_cast<WORD>(color);
  WORD newColor;
  if (color > console::fc::White) {
    newColor = (oldColor & 0x0F) | color_;
  } else {
    newColor = (oldColor & 0xF0) | color_;
  }
  SetConsoleTextAttribute(hConsole, newColor);
  DWORD dwWrite;
  WriteConsoleW(hConsole, data, (DWORD)len, &dwWrite, nullptr);
  SetConsoleTextAttribute(hConsole, oldColor);
  return static_cast<int>(dwWrite);
}
```

在这里，我们选择的是 `STD_OUTPUT_HANDLE`，使用 `&0x0F` 或者 `&0xF0` 的目的是不修改原有的背景色或者前景色，第二次调用 `SetConsoleTextAttribute` 的目的是恢复控制台原有的颜色。

## 终端模拟器彩色输出

在 Windows 上，还有 Cygwin 和 MSYS2 MSYS 这样的模拟 Unix 的环境。wsudo 对其支持也非常有必要。
这些环境启动进程往往是通过管道通信，这个时候，我们可以判断是否是终端还是控制台。
```c++
bool IsWindowsConhost(HANDLE hConsole, bool &isvt) {
  if (GetFileType(hConsole) != FILE_TYPE_CHAR) {
    return false;
  }
  DWORD mode;
  if (!GetConsoleMode(hConsole, &mode)) {
    return false;
  }
  if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
    isvt = true;
  }
  return true;
}
```
如果使用 GetFileType(hConsole) 得到的文件类型不是 `FILE_TYPE_CHAR` 我们就可以确定不是控制台，并且如果不支持 `GetConsoleMode` 函数，也要视其不是控制台。

核心代码如下：

```c++
namespace console {
std::string wchar2utf8(const wchar_t *buf, size_t len) {
  std::string str;
  auto N = WideCharToMultiByte(CP_UTF8, 0, buf, (int)len, nullptr, 0, nullptr,
                               nullptr);
  str.resize(N);
  WideCharToMultiByte(CP_UTF8, 0, buf, (int)len, &str[0], N, nullptr, nullptr);
  return str;
}

struct TerminalsColorTable {
  int index;
  bool blod;
};

namespace vt {
namespace fg {
enum Color {
  Black = 30,
  Red = 31,
  Green = 32,
  Yellow = 33,
  Blue = 34,
  Magenta = 35,
  Cyan = 36,
  Gray = 37,
  Reset = 39
};
}
namespace bg {
enum Color {
  Black = 40,
  Red = 41,
  Green = 42,
  Yellow = 43,
  Blue = 44,
  Magenta = 45,
  Cyan = 46,
  Gray = 47,
  Reset = 49
};
}
}
bool TerminalsConvertColor(int color, TerminalsColorTable &co) {

  std::unordered_map<int, TerminalsColorTable> tables = {
      {console::fc::Black, {vt::fg::Black, false}},
      {console::fc::DarkBlue, {vt::fg::Blue, false}},
      {console::fc::DarkGreen, {vt::fg::Green, false}},
      {console::fc::DarkCyan, {vt::fg::Cyan, false}},
      {console::fc::DarkRed, {vt::fg::Red, false}},
      {console::fc::DarkMagenta, {vt::fg::Magenta, false}},
      {console::fc::DarkYellow, {vt::fg::Yellow, false}},
      {console::fc::DarkGray, {vt::fg::Gray, false}},
      {console::fc::Blue, {vt::fg::Blue, true}},
      {console::fc::Green, {vt::fg::Green, true}},
      {console::fc::Cyan, {vt::fg::Cyan, true}},
      {console::fc::Red, {vt::fg::Red, true}},
      {console::fc::Magenta, {vt::fg::Magenta, true}},
      {console::fc::Yellow, {vt::fg::Yellow, true}},
      {console::fc::White, {vt::fg::Gray, true}},
      {console::bc::Black, {vt::bg::Black, false}},
      {console::bc::Blue, {vt::bg::Blue, false}},
      {console::bc::Green, {vt::bg::Green, false}},
      {console::bc::Cyan, {vt::bg::Cyan, false}},
      {console::bc::Red, {vt::bg::Red, false}},
      {console::bc::Magenta, {vt::bg::Magenta, false}},
      {console::bc::Yellow, {vt::bg::Yellow, false}},
      {console::bc::DarkGray, {vt::bg::Gray, false}},
      {console::bc::LightBlue, {vt::bg::Blue, true}},
      {console::bc::LightGreen, {vt::bg::Green, true}},
      {console::bc::LightCyan, {vt::bg::Cyan, true}},
      {console::bc::LightRed, {vt::fg::Red, true}},
      {console::bc::LightMagenta, {vt::bg::Magenta, true}},
      {console::bc::LightYellow, {vt::bg::Yellow, true}},
      {console::bc::LightWhite, {vt::bg::Gray, true}},
  };
  auto iter = tables.find(color);
  if (iter == tables.end()) {
    return false;
  }
  co = iter->second;
  return true;
}
int WriteTerminals(int color, const wchar_t *data, size_t len) {
  TerminalsColorTable co;
  auto str = wchar2utf8(data, len);
  if (!TerminalsConvertColor(color, co)) {
    return static_cast<int>(fwrite(str.data(), 1, str.size(), stdout));
  }
  if (co.blod) {
    fprintf(stdout, "\33[1;%dm", co.index);
  } else {
    fprintf(stdout, "\33[%dm", co.index);
  }
  auto l = fwrite(str.data(), 1, str.size(), stdout);
  fwrite("\33[0m", 1, sizeof("\33[0m") - 1, stdout);
  return static_cast<int>(l);
}
```
这些终端环境基本上将文本视为 UTF8 编码，为了让文字正常显示，我们需要将其转换为 UTF8。经过测试，中文都能正常显示。

## VT 模式颜色输出

在 Windows 10 中，新增了 **Windows Subsystem for Linux** ，可以通过 Bash 命令启动终端运行 Linux 程序，Windows 控制台还增加了 VT 模式 [Console Virtual Terminal Sequences](https://msdn.microsoft.com/en-us/library/windows/desktop/mt638032.aspx)，并且支持24-Bit 颜色：[24-bit Color in the Windows Console!](https://blogs.msdn.microsoft.com/commandline/2016/09/22/24-bit-color-in-the-windows-console/)，这意味着，可以像 Linux 一样在 printf 中添加转义字符控制颜色输出。
在 Github 中也有 Issues 讨论: [support 256 color](https://github.com/Microsoft/BashOnWindows/issues/345)
笔者在开发时发现 WriteConsoleW 也支持 VT 模式，对于也添加了代码支持 VT 模式：

```c++
int WriteConsoleInternal(const wchar_t *buffer, size_t len) {
  DWORD dwWrite = 0;
  auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  if (WriteConsoleW(hConsole, buffer, (DWORD)len, &dwWrite, nullptr)) {
    return static_cast<int>(dwWrite);
  }
  return 0;
}
int WriteVTConsole(int color, const wchar_t *data, size_t len) {
  TerminalsColorTable co;
  if (!TerminalsConvertColor(color, co)) {
    return WriteConsoleInternal(data, len);
  }
  std::wstring buf(L"\x1b[");
  if (co.blod) {
    buf.append(L"1;").append(std::to_wstring(co.index)).push_back(L'm');
  } else {
    buf.append(std::to_wstring(co.index)).push_back(L'm');
  }
  WriteConsoleInternal(buf.data(), (DWORD)buf.size());
  auto N = WriteConsoleInternal(data, (DWORD)len);
  WriteConsoleInternal(L"\x1b[0m", (sizeof("\x1b[0m") - 1));
  return static_cast<int>(N);
}
template <typename... Args>
int PrintConsole(const wchar_t *format, Args... args) {
  std::wstring buffer;
  size_t size = StringPrint(nullptr, 0, format, args...);
  buffer.resize(size);
  size = StringPrint(&buffer[0], buffer.size() + 1, format, args...);
  return WriteConsoleInternal(buffer.data(), size);
}
```
由于 VT 模式支持 256 色，这里还增加了 `PrintConsole` 模板函数，支持用户自定义输出多一些色彩。

## 输出函数自动选择

在不考虑 freopen 这样的重新设置标准输出输出的情况下，Privexec.Console 使用如下代码支持自动选择不同的输出函数

```c++
class ConsoleInternal {
public:
  ConsoleInternal() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
      impl = WriteFiles;
      return;
    }
    if (GetFileType(hConsole) == FILE_TYPE_DISK) {
      impl = WriteFiles;
      return;
    }
    bool isvt = false;
    if (IsWindowsConhost(hConsole, isvt)) {
      if (isvt) {
        impl = WriteVTConsole;
        return;
      }
      impl = WriteConhost;
      return;
    }
    impl = WriteTerminals;
  }
  int WriteRealize(int color, const wchar_t *data, size_t len) {
    return this->impl(color, data, len);
  }

private:
  int (*impl)(int color, const wchar_t *data, size_t len);
};
int WriteInternal(int color, const wchar_t *buf, size_t len) {
  static ConsoleInternal provider;
  return provider.WriteRealize(color, buf, len);
}
``` 
使用 `console::Print` 不用担心乱码和颜色问题，在这几个主流的环境中都能正常显示（包括 ConEmu）。Print 使用的完全是 wchar_t。

## 其他

很欣慰的是 Windows 控制台团队在 Windows 10 开发之处就在不断改进控制台： 比如 [Console Improvements in the Windows 10 Technical Preview](https://blogs.windows.com/buildingapps/2014/10/07/console-improvements-in-the-windows-10-technical-preview/)

还有计划中的 Emoji 支持： [Add emoji support to Windows Console](https://github.com/Microsoft/BashOnWindows/issues/590)

以及基于 DirectWrite 改进控制台字体渲染的计划： [UTF-8 rendering woes](https://github.com/Microsoft/BashOnWindows/issues/75#issuecomment-304415019)

当然 ConEmu 也有计划使用 DirectWrite 改进其渲染。

不过遗憾的是，Mintty 的开发者并不认为有使用 DirectWrite 改进渲染的必要。 

基于 Rust 的跨平台 GPU 终端 [Alacritty - A cross-platform, GPU-accelerated terminal emulator](https://github.com/jwilm/alacritty) 也计划在 1.0 时对 Windows 提供支持，字体渲染也有 DirectWrite 的身影。

Privexec.Console 官方并不会支持 Windows 10 以前的版本，毕竟作者精力有限。

## 备注
1. 父进程未显式设置标准输入输出和标准错误时，子进程会继承父进程的值，在 Windows 中，GUI 程序的标准输入输出和 Unix 下重定向到 `/dev/null` 类似，但启动的 CUI 子进程默认下依然有控制台窗口
