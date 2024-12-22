+++
title = "MSYS2 Launcher 开发杂记"
date = "2016-01-21T21:30:16+08:00"
draft = true
categories = "toolset"
+++
# 前言

在我前面的博客中，曾经写过一篇 [《基于清单的启动器的实现》](http://forcemz.net/toolset/2015/11/27/NewLauncher.html) ,说了几类新的 Launcher ，
而在本文中，依旧说的是启动器，这次说的是 MSYS2 启动器，顺带说一说 使用 Win32 API 在 MSVC，Mingw64, Cygwin 中相互兼容的策略。

# Minimal GNU（POSIX）system on Windows   

最开始接触 MSYS 已经有一些年头了，那个时候，我不知疲倦的捣鼓着各种开发工具，写下一行行 "Hello world"，从网络上找到各种简单的例子，
然后在 Mingw，Cygwin , Microsoft Visual C++ ,OpenWATCOM 等开发工具下编译。MSYS 通常是和 Mingw 一起的，Mingw (Minimalist GNU for Windows
) 提供一套 GNU 工具集和 Win32 API ，也就是 w32api，而 MSYS 则提供一个有限的 POSIX 模拟环境，使用 Mingw 编译的项目可以不依赖第三方库。     
MSYS 提供了诸如 bash tar wget ls ln 等 Unix 上不可或缺的工具，但同时工具集的版本存在严重的滞后，MSYS 最近几年非常不活跃。

## Mintty   

在使用 Mingw 的时候，比较舒服的是 MSYS 提供的 Mintty 工具，这是一个终端模拟器，在 Unix like 系统上，终端模拟器是非常常见的工具，Ubuntu 有 Terminal,
Kubuntu 有 Kconsole。而在 Windows 平台上，终端模拟器还是比较少见的，这是因为 Windows 本质上是一个图形化操作系统，人们直接过运行运行命令行程序，
然后操作系统自动分配一个控制台窗口，并将程序的输入输出定向到此控制台窗口，人们也可以通过运行 CMD 或者 PowerShell shell 工具来执行命令行程序，实际上
二者本身也是命令行程序，也就是子系统为 Console（GUI 程序也可以使用 AllocConsole 分配一个控制台窗口或者 AttachConsole 绑定到父进程的控制台窗口）。

Mintty 子系统是 WINDOWS ,是一个地地道道的 GUI 程序，这意味着，它有完整的消息循环，当然可以有更多的自定义设置。

Windows 有多种子系统，可以通过 MSDN,TechNet 文档查看，在 winnt.h (wdk ntimage.h) 中也有定义。    
也可以通过 Visual C++ 链接器查看 ( Link version 14)：      

>BOOT_APPLICATION | CONSOLE | EFI_APPLICATION | EFI_BOOT_SERVICE_DRIVER | EFI_ROM |
>EFI_RUNTIME_DRIVER | NATIVE | POSIX | WINDOWS | WINDOWSCE


# MSYS 2


# MSYS2 launcher

## Win32 API 的使用

ISO C 提供了一系列的字符串函数，如 strcpy strcat 等，在 C11 标准中，添加了 _s 的安全字符串函数，Visual C/C++ 很早
就实现了安全字符串函数，而 Mingw 同样支持安全字符串函数。实际上，Mingw CRT 和 msvcrt.dll 也是休戚相关的。而 
Cygwin 在支持安全字符函数和宽字符版本函数并不是很完善。

```cpp
#ifndef __CYGWIN__
#define  SECURITY_WIN32
#include <Security.h>
#include <StrSafe.h>
#else
#define STRSAFE_MAX_CCH 2147483647
#define STRSAFE_E_INVALID_PARAMETER ((HRESULT)0x80070057L)

inline HRESULT StringCchLengthW(
  LPCWSTR psz,
  size_t  cchMax,
  size_t  *pcchLength)
{
  HRESULT hr = S_OK;
  size_t cchMaxPrev = cchMax;
  while(cchMax && (*psz!=L'\0')) {
    psz++;
    cchMax--;
  }
  if(cchMax==0)
    hr = STRSAFE_E_INVALID_PARAMETER;
  if(pcchLength) {
    if(SUCCEEDED(hr))
      *pcchLength = cchMaxPrev - cchMax;
    else
      *pcchLength = 0;
  }
  return hr;
}

inline HRESULT StringCchCopyW(
  LPWSTR  pszDest,
  size_t  cchDest,
  LPCWSTR pszSrc)
{
  HRESULT hr=S_OK;
  if(cchDest==0||pszDest==nullptr)
    return STRSAFE_E_INVALID_PARAMETER;
  while(cchDest &&(*pszSrc!=L'\0')){
    *pszDest++=*pszSrc++;
    cchDest--;
  }
  if(cchDest==0){
    pszDest--;
    hr=STRSAFE_E_INVALID_PARAMETER;
  }
  *pszDest=L'\0';
  return hr;
}

inline HRESULT StringCchCatW(
  LPWSTR  pszDest,
  size_t  cchDest,
  LPCWSTR pszSrc)
{
  HRESULT hr = S_OK;
  if(cchDest==0||pszDest==nullptr)
    return STRSAFE_E_INVALID_PARAMETER;
  size_t lengthDest;
  if(StringCchLengthW(pszDest,cchDest,&lengthDest)!=S_OK){
    hr=STRSAFE_E_INVALID_PARAMETER;
  }else{
    hr=StringCchCopyW(pszDest+lengthDest,cchDest-lengthDest,pszSrc);
  }
  return hr;
}

#endif
```

C++ 提供宽字符版本的字符串类 std::wstring 

```cpp
typedef basic_string<char, char_traits<char>, allocator<char>> string;
typedef basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>> wstring;
typedef basic_string<char16_t, char_traits<char16_t>, allocator<char16_t>> u16string;
typedef basic_string<char32_t, char_traits<char32_t>, allocator<char32_t>> u32string;
```

由于 CreateProcessW 第二个参数 lpCommandLine 使用的并不是 const WCHAR * ，为了避免缓冲区被修改，如果使用 
std::wstring 需要将命令行字符串拷贝到新的缓冲区，这样一来，std::wstring 的优势反而没有了。
