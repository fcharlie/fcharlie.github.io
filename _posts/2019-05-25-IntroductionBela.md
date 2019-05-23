---
layout: post
title:  "介绍 Bela"
date:   2019-05-25 10:00:00
published: true
categories: cxx
---

# 前言

在前面，我曾经写过一篇文章 [《字符串格式化漫谈》](https://forcemz.net/cxx/2019/04/29/StringFormattingTalk/) 文章最后提到了 [`Bela`](https://github.com/fcharlie/bela) 里面实现了一个类型安全的 `bela::StrFormat`，实际上 `bela` 还有很多有趣的功能，本文也就是说一说 Bela 有哪些有趣功能和故事。

# 一. Bela 的缘由

之前，我开发了几个开源软件，如 Windows 系统以指定权限启动进程的 [Privexec](https://github.com/M2Team/Privexec)，Clang Windows 操作系统上自动化构建依赖管理工具 [Clangbuilder](https://github.com/fstudio/clangbuilder)，还有 PE 分析工具 [PEAnalyzer](https://github.com/fcharlie/PEAnalyzer)，文件分析工具 [Planck](https://github.com/fcharlie/Planck) 等等。在编写这些工具时要重复编写一些代码，毕竟大家都知道 C++ STL 有时候并不能称心如意。在 [Google Abseil](https://github.com/abseil/abseil-cpp) 开源后，我借鉴了这个项目的一些代码重新造了一些 `wchar_t` 版本的轮子，后来把这些代码单独抽离出来，进一步改进，也就成了现在的 `Bela`。不直接用 `Absl` 的原因很简单，它不支持 `wchar_t`。格式化字符串不使用 `fmtlib` 的原因也很简单，不喜欢异常，它的代码库也比较大。叫 `bela ['bələ]` 的原因依然很简单，简短易读易拼写。

Bela 的字符串函数基本基于 `Abseil`，`Unicode` 转换基于 LLVM 的 `ConvertUTF.cc` , `charconv` 基于 `Visual C++ STL`。

# 二. Bela 字符串功能库

## StrFormat

Bela 目前提供了一个类型安全简单的 `StrFormat`, `StrFormat` 基于 C++ 变参模板，使用 `union` 记录参数类型，在解析时按照输入的占位符将其他类型转换为字符串连接在一起，从而实现格式化功能。

支持的类型和响应的占位符如下表所示：

|类型|占位符|备注|
|---|---|---|
|char|`%c`|ASCII 字符，会被提升为 wchar_t|
|unsigned char|`%c`|ASCII 字符，会被提升为 wchar_t|
|wchar_t|`%c`|UTF-16 字符|
|char16_t|`%c`|UTF-16 字符|
|char32_t|`%c`|UTF-32 Unicode 字符，会被转为 UTF-16 字符，这意味着可以使用 Unicode 码点以 %c 的方式输出 emoji。|
|short|`%d`|16位整型|
|unsigned short|`%d`|16位无符号整型|
|int|`%d`|32位整型|
|unsigned int|`%d`|32位无符号整型|
|long|`%d`|32位整型|
|unsigned long|`%d`|32位无符号整型|
|long long|`%d`|64位整型|
|unsigned long long|`%d`|64位无符号整型|
|float|`%f`|会被提升为 `double`|
|double|`%f`|64位浮点|
|const char *|`%s`|UTF-8 字符串，会被转换成 UTF-16 字符串|
|char *|`%s`|UTF-8 字符串，会被转换成 UTF-16 字符串|
|std::string|`%s`|UTF-8 字符串，会被转换成 UTF-16 字符串|
|std::string_view|`%s`|UTF-8 字符串，会被转换成 UTF-16 字符串|
|const wchar_t *|`%s`|UTF-16 字符串|
|wchar_t *|`%s`|UTF-16 字符串|
|std::wstring|`%s`|UTF-16 字符串|
|std::wstring_view|`%s`|UTF-16 字符串|
|const char16_t *|`%s`|UTF-16 字符串|
|char16_t *|`%s`|UTF-16 字符串|
|std::u16string|`%s`|UTF-16 字符串|
|std::u16string_view|`%s`|UTF-16 字符串|
|void *|`%p`|指针类型，会格式化成 `0xffff00000` 这样的字符串|

如果不格式化 UTF-8 字符串，且拥有固定大小内存缓冲区，可以使用 `StrFormat` 的如下重载，此重载可以轻松的移植到 POSIX 系统并支持异步信号安全:

```c++
template <typename... Args>
ssize_t StrFormat(wchar_t *buf, size_t N, const wchar_t *fmt, Args... args)
```
我们基于 `StrFormat` 实现了类型安全的 `bela::FPrintF`，这个函数能够根据输出设备的类型自动转换编码，如果是 `Conhost` 则会输出 `UTF-16`，否则则输出 `UTF-8`。如果 `Conhost` 不支持 `VT` 模式，bela 则会将输出字符串中的 ASCII 颜色转义去除，但 bela 并没有做 Windows 旧版本的适配，我们应该始终使用 Windows 最新发行版。

下面是一个示例：

```cpp
///
#include <bela/strcat.hpp>
#include <bela/stdwriter.hpp>

int wmain(int argc, wchar_t **argv) {
  auto ux = "\xf0\x9f\x98\x81 UTF-8 text \xE3\x8D\xA4"; // force encode UTF-8
  wchar_t wx[] = L"Engine \xD83D\xDEE0 中国";
  bela::FPrintF(
      stderr,
      L"Argc: %d Arg0: \x1b[32m%s\x1b[0m W: %s UTF-8: %s __cplusplus: %d\n", argc, argv[0], wx, ux, __cplusplus);
  char32_t em = 0x1F603;//😃
  auto s = bela::StringCat(L"Look emoji -->", em, L" U: ",
                           static_cast<uint32_t>(em));
  bela::FPrintF(stderr, L"emoji test %c %s\n", em, s);
  bela::FPrintF(stderr, L"hStderr Mode: %s hStdin Mode: %s\n",
                bela::FileTypeName(stderr), bela::FileTypeName(stdin));
  return 0;
}

```

请注意，如果上述 emoji 要正常显示，应当使用 `Windows Terminal` 或者是 `Mintty`。

# 三. Bela Windows 系统功能库

## PathCat 路径规范化连接函数

## ExecutableExistsInPath 查找可执行文件

在 Windows cmd 中，有一个命令叫做 `where`，用于查找命令或者可执行文件的路径，而 `ExecutableExistsInPath` 则提供了相同的功能，我们可以按照输入的命令或者路径查找对应的可执行文件，这里有一个 `where` 实现：

```cpp
////
#include <bela/strcat.hpp>
#include <bela/stdwriter.hpp>
#include <bela/path.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s command\n", argv[0]);
    return 1;
  }
  std::wstring exe;
  if (!bela::ExecutableExistsInPath(argv[1], exe)) {
    bela::FPrintF(stderr, L"command not found: %s\n", argv[1]);
    return 1;
  }
  bela::FPrintF(stdout, L"%s\n", exe);
  return 0;
}
```