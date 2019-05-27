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

Bela 的字符串函数基本基于 `Abseil`，`Unicode` 转换基于 LLVM 的 `ConvertUTF.cpp`，最初 `ConvertUTF` 的版权属于 **Unicode.org** , `charconv` 基于 `Visual C++ STL`，`EscapeArgv` 借鉴了 Golang 源码，`endian.hpp`，`tokenziecmdline.hpp` 借鉴了 `LLVM Support Library` 等等。

# 二. Bela 字符串功能库

## bela::error_code

`bela::error_code` 位于 `<bela/base.hpp>` 类似 `std::error_code`，其主要目的是简化 Windows API 错误信息的格式化，当人们使用 `make_system_error_code` 就可以将 Windows 错误信息保存到 `bela::error_code` 对象，利用 C++ RAII 机制，完全可以不用担心内存释放，同样你还可以使用 `bela::make_error_code` 构造自己的错误信息，`bela::make_error_code` 依赖 `bela::strings_internal::CatPieces`，因此，你可以像使用 `bela::StringCat` 使用 `bela::make_error_code`


## StringCat

`StringCat` 学习了 `absl::StrCat`，唯一的不同在于使用 `wchar_t` 作为字符基本类型，在 Windows 系统中，`StrCat` 实际上被 `shlwapi` 作为宏定义使用了，为了避免在使用 Windows API 时造成困惑，我将其命名为 `StringCat`。这种函数的好处是连接字符串时只需要一次内存分配，可以将基本类型转变为字符串类型，然后连接到一起，十分有用，并且，`bela::StringCat` 比 `absl::StrCat` 有趣的一点是支持 `char32_t` Unicode 码点，因此，你可以使用 `bela::StringCat` 连接 Unicode 码点来拼接 Emoji 或者其他字符，然后输出到 `Windows Terminal` 或者显示到文本或者使用 Dwrite 绘制到图形界面上。

`StringCat` 定义在 `<bela/strcat.hpp>` 文件中。

## Ascii

bela 学习了 `absl/strings/ascii.h` ，并将其移植到 `wchar_t`。在移植的时候要考虑在 Windows 系统上 `wchar_t` 的范围是 `0~65535`，因此一些函数需要判断大于 `0xFF` 时的策略。在 `<bela/ascii.hpp>` 文件中 `AsciiStrToUpper` ，`AsciiStrToLower`， `StripLeadingAsciiWhitespace`，`StripTrailingAsciiWhitespace`， `StripAsciiWhitespace` 这些函数十分有帮助。

## 其他字符串函数

|函数|功能|文件|
|---|---|---|
|StrSplit|字符串切割，Delimiter 支持按字符，按字符串或者按长度，支持跳过空或者空白，比 Golang 的 strings.Split 要好用一些|`<bela/str_split.hpp>`|
|StrReplaceAll|替换字符串|`<bela/str_replace.hpp>`|
|ConsumePrefix，ConsumeSuffix，StripPrefix，StripSuffix|删除特定前缀或者后缀|`<bela/strip.hpp>`|
|StartsWith，EndsWith，EqualsIgnoreCase，StartsWithIgnoreCase，EndsWithIgnoreCase|特定的比较函数，前两者 C++20 被引入（std::string::starts_with，std::string::ends_with），Visual 2019 16.1 C++ /std:c++latest 开启|`<bela/match.hpp>`|
|Substitute，SubstituteAndAppend|在前文  [《字符串格式化漫谈》](https://forcemz.net/cxx/2019/04/29/StringFormattingTalk/) 有提及，字符串填充。|`<bela/subsitute.hpp>`|
|SimpleAtob，SimpleAtoi|字符串转整型或者 Boolean，要转换浮点类型，请使用 `<bela/charconv.hpp>`|`<bela/numbers.hpp>`|

## 编码转换

在 Bela 中，我基于 LLVM 的 ConvertUTF 实现了 UTF-16/UTF-8 UTF-32 的一些函数，声明文件均在 `<bela/codecvt.hpp>`

|函数|功能|
|---|---|
|char32tochar16|Unicode 码点转 UTF-16，缓冲区长度应当至少为 2|
|char32tochar8|Unicode 码点转 UTF-8，缓冲区长度至少为 4|
|c16tomb|UTF16 编码转 UTF-8，低级 API|
|mbrtowc|UTF-8 编码转 UTF-16 (wchar_t)，低级 API|
|mbrtoc16|UTF-8 编码转 UTF-16 (char16_t)，低级 API|
|ToNarrow|UTF-16 转 UTF-8|
|ToWide|UTF-8 转  UTF-16|

涉及到编码转换时，应当使用高级 API `bela::ToNarrow` 和 `bela::ToWide`

## StrFormat

Bela 目前提供了一个类型安全简单的 `StrFormat`, `StrFormat` 基于 C++ 变参模板，使用 `union` 记录参数类型，在解析时按照输入的占位符将其他类型转换为字符串连接在一起，从而实现格式化功能。`bela::StrFormat` 借鉴了 `Chromium SafeNPrintf` 函数，但支持的类型要比 `SafeNPrintf` 多很多。

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
/// C++17
#include <bela/strcat.hpp>
#include <bela/stdwriter.hpp>

constexpr auto cv=__cplusplus;

int wmain(int argc, wchar_t **argv) {
  auto ux = "\xf0\x9f\x98\x81 UTF-8 text \xE3\x8D\xA4"; // force encode UTF-8
  wchar_t wx[] = L"Engine \xD83D\xDEE0 中国";
  bela::FPrintF(
      stderr,
      L"Argc: %d Arg0: \x1b[32m%s\x1b[0m W: %s UTF-8: %s C++ version: %d\n",  argc, argv[0], wx, ux, cv);
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

## bela::finaly

在使用 Golang 时，`defer` 可以在函数退出时执行一些代码，在 C++ 中 [Microsoft/GSL](https://github.com/microsoft/gsl) 里面有一个 `finaly` 实现，异曲同工。在这里 我们可以使用 `finaly` 避免资源泄露。

```cpp
#include <bela/finaly.hpp>
#include <cstdio>

int wmain(){
  auto file=fopen("somelog","w+");
  auto closer=bela::finaly([&]{
    if(file!=nullptr){
      fclose(file);
    }
  });
  /// do some codes
  return 0;
}

```

## PathCat 路径规范化连接函数

`PathCat` 函数借鉴了 `StringCat` 函数，将路径组件连接起来。例子如下：

```c++
  auto p = bela::PathCat(L"\\\\?\\C:\\Windows/System32", L"drivers/etc", L"hosts");
  bela::FPrintF(stderr, L"PathCat: %s\n", p);
  auto p2 = bela::PathCat(L"C:\\Windows/System32", L"drivers/../..");
  bela::FPrintF(stderr, L"PathCat: %s\n", p2);
  auto p3 = bela::PathCat(L"Windows/System32", L"drivers/./././.\\.\\etc");
  bela::FPrintF(stderr, L"PathCat: %s\n", p3);
  auto p4 = bela::PathCat(L".", L"test/pathcat/./pathcat_test.exe");
  bela::FPrintF(stderr, L"PathCat: %s\n", p4);
```
`PathCat` 的思路是先将 `UNC` 前缀和盘符记录并去除，然后将所有的参数使用 `PathSpilt` 函数以 Windows 路径分隔符和 Linux 路径分隔符拆分成 `std::wstring_view` 数组，当当前路径元素为 `..` 时，弹出字符串数组一个元素，如果为 `.` 则保持不变，否则将路径元素压入数组。拆分完毕后，遍历数组计算所需缓冲区大小，调整 `std::wstring` 容量，然后进行路径重组。

当第一个参数值为 `.` 时，`PathCat` 将解析第一个路径为当前目录，然后参与解析。如果 `PathCat` 第一个参数是相对路径，`PathCat` 并不会主动将路径转变为绝对路径，因此，你应当主动的将第一个参数设置为 `.` 以期解析为绝对路径。

`PathCat` 并不会判断路径是否存在，因此需要注意。

路径解析错误是很多软件的漏洞根源，合理的规范化路径非常有必要，而 `PathCat` 在规范化路径时，使用 C++17/C++20(Span) 的特性，减少内存分配，简化了规范化流程。

`PathCat` 使用了 `bela::Span` （`<bela/span.hpp>`），`Span` 被 C++20 采纳，`bela::Span` 基于 `absl::Span`。

## PathExists 函数

`PathExists` 函数判断路径是否存在，当使用默认参数时，只会判断路径是否存在，如果需要判断路径的其他属性，可以使用如下方式：

```c++
if(!bela::PathExists(L"C:\\Windows",FileAttribute::Dir)){
    bela::FPrintF(stderr,L"C:\\Windows not dir or not exists\n");
}
```

## LookupRealPath 函数

`LookupRealPath` 用于解析 Windows 符号链接和卷挂载点。

## LookupAppExecLinkTarget 函数

`LookupAppExecLinkTarget` 用于解析 Windows AppExecLink 目标，在 Windows 10 系统中，`AppExecLink` 是一种 Store App 的命令行入口，通常位于 `C:\Users\$Username\AppData\Local\Microsoft\WindowsApps`，这种文件本质上是一种重解析点，因此解析时需要按照重解析点的方法去解析。

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

## 命令行合成，拆分和解析

在 bela 中，我们提供了命令行合成，拆分和解析类，具体如下：

|类名|功能|文件|
|---|---|---|
|ParseArgv|解析命令行参数，类似 GNU `getopt_long`，支持 `wchar_t`，不使用全局变量，错误信息详细|`<bela/parseargv.hpp>`|
|Tokenizer|将命令行字符串 Windows commandline 形式转变为 `wchar_t **Argv` 形式|`<bela/tokenizecmdline.hpp>`|
|EscapeArgv|将 `Argv` 形式命令行参数转为 `commdline` 形式，主要用于 `CreateProcess`|`<bela/escapeargv.hpp>`|

## MapView

在 bela 中，我还提供 `MapView`，这是一个只读的文件内存映射，通常用于文件解析。文件 `<bela/mapview.hpp>` 还有与 `std::string_view` 类似的 `MemView ` 类。

## PESimpleDetailsAze 获得 PE 的简单信息

在 Bela 中，我添加了一个 `PESimpleDetailsAze` 用于获得 PE 可执行文件的一些信息，其结构体如下：
```cpp
struct PESimpleDetails {
  std::wstring clrmsg;
  std::vector<std::wstring> depends; // depends dll
  std::vector<std::wstring> delays;  // delay load library
  PEVersionPair osver;
  PEVersionPair linkver;
  PEVersionPair imagever;
  Machine machine;
  Subsytem subsystem;
  uint16_t characteristics{0};
  uint16_t dllcharacteristics{0};
  bool IsConsole() const { return subsystem == Subsytem::CUI; }
  bool IsDLL() const {
    constexpr uint16_t imagefiledll = 0x2000;
    return (characteristics & imagefiledll) != 0;
  }
};
```

函数的声明如下：
```cpp
std::optional<PESimpleDetails> PESimpleDetailsAze(std::wstring_view file,
                                                      bela::error_code &ec);
```

通过此函数，你可以获得 PE 可执行文件的目标机器类型，子系统，连接器版本，系统版本，Image 版本，PE 的特征，PE 依赖的 dll 和延时加载的 dll。如果是 CLR PE 文件，则clrmsg 不为空描述的是 CLR 的信息。`PESimpleDetailsAze` 并不依赖 `DbgHelp.dll (ImageRvaToVa)`。

# 最后

Bela 应该是不断发展的，如果我有新的 Idea 了，就会及时的移植进去的。
