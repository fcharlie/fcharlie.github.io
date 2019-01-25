---
layout: post
title:  "文件的魔法 - 文件格式的检测"
date:   2019-01-25 20:00:00
published: false
categories: toolset
---

# 前言

本文探讨的是计算机文件，**计算机文件** 用于记录数据到计算机设备上，维基百科上有简短的介绍：

>A computer file is a computer resource for recording data discretely in a computer storage device. Just as words can be written to paper, so can information be written to a computer file.

当人们需要使用这些文件的时候，需要从光盘，磁盘，闪存等设备上将文件读取到内存，按照文件的格式进行解析，然后供用户使用。在这个过程中，正确的或得文件格式信息是非常重要的，只有在识别出文件格式之后，才能够选择正确的的处理程序对文件进行解析。在 Windows 上通常是 Shell 外壳（`Shell32.dll`）根据文件后缀名在注册表中找到对应的关联程序然后使用特定的程序处理相应的文件，比如 `.docx` 的关联程序往往是 `Microsoft Word`。`.txt` 的关联程序是 `Notepad`。但如果文件没有后缀名时，Windows Shell 就需要用用户自己选择对应的关联程序了。

在 Unix 操作系统上，命令行下检测文件格式的检测通常使用 `file` ([`file — determine file type`](https://linux.die.net/man/1/file)) ，file 的源码在 Github 上有镜像：[https://github.com/file/file](https://github.com/file/file)。file 这样的工具通过分析文件魔数，文件头部特征分析文件格式，这样的工具严重依赖 **Magdir**，Magic 文件越多支持的格式越丰富。file 这样的命令与 Windows 资源管理器相比，已经有很大的进步。

在图形系统中，文件的检测由文件管理器实现，像 `Gnome Nemo` 这样的文件管理器会优先处理文件后缀名，在识别不到文件格式时才会去根据文件特征检测文件格式。Nemo 依赖 glib(gio [`_xdg_mime_magic_lookup_data`](https://github.com/GNOME/glib/blob/cbfa776fc149fcc3e351fbdf68c1a299519f4905/gio/xdgmime/xdgmimemagic.c#L657))，和 file 的原理类似但没有 `file` 强大。

`file` 程序目前已经被移植到 Windows 使用，比如 `Cygwin`，`MSYS2` 的 Bash 环境中，均携带有 `file` 命令。

我最开始去了解文件类型的检测是在实现 LFS 服务器的时候，基于 C++ 编写的 LFS 服务器使用的是 `libmagic`, libmagic 即 `file` 的一部分，而基于 `Golang` 编写的 LFS 服务器使用的则是 [https://github.com/h2non/filetype](https://github.com/h2non/filetype)

在重构完 `Privexec` 之后，突然想写一个文件类型检测工具，最开始叫做 `FileView` 后来改名为 `Planck`，当 `Planck` 大概能用的时候，想把一些见解分享给大家，于是有了此篇文章。

## 背景知识

### 字节序

字节序：[Endianness](https://en.wikipedia.org/wiki/Endianness)，字节顺序，又称端序或尾序（英语：Endianness），在计算机科学领域中，指存储器中或在数字通信链路中，组成多字节的字的字节的排列顺序。

+  x86、MOS Technology 6502、Z80、VAX、PDP-11等处理器为小端序；
+  Motorola 6800、Motorola 68000、PowerPC 970、System/370、SPARC（除V9外）等处理器为大端序；
+  ARM、PowerPC（除PowerPC 970外）、DEC Alpha、SPARC V9、MIPS、PA-RISC及IA64的字节序是可配置的。

网络字节序为 `Big Endian`，目前 Windows x86, AMD64, ARM, ARM64 均为 `Little Endian`。

Planck 中字节序转换代码在：[https://github.com/fcharlie/Planck/blob/master/include/endian.hpp](https://github.com/fcharlie/Planck/blob/master/include/endian.hpp)。

### 文件十六进制查看工具

查看文件信息可以使用支持 16 进制的工具查看，GUI 的工具有 `Sublime Text`，`010Editor` 等，CLI 的工具有 `hexdump`，`xxd`，还有最近带颜色高亮的

+   Hastyhex: [https://github.com/skeeto/hastyhex](https://github.com/skeeto/hastyhex)
+   Hexyl: [https://github.com/sharkdp/hexyl](https://github.com/sharkdp/hexyl)

Hastyhex 基于 C 编写，但不支持指定长度，对 Windows 10 控制台支持不太好，于是我 fork Hastyhex，对其改进，使其支持特定长度和从指定位置开始读取。在 Windows 上改进了控制台颜色输出。

Unix 版本：[HastyHex : a faster hex dumper](https://github.com/fcharlie/hastyhex)   
针对 Windows 10 控制台改进的版本：[https://github.com/fcharlie/Planck/tree/master/utils/hastyhex](https://github.com/fcharlie/Planck/tree/master/utils/hastyhex)

## 文件，硬链接，软连接，快捷方式

在存储设备上，一个文件通常是常规文件，但文件也有可能指向其它文件。

### 硬链接与软链接

[Hard Link](https://en.wikipedia.org/wiki/Hard_link) 通常意味着一个原始文件可能存在有多个文件名，比如 Linux 一个 inode 对应多个路径。

>In computing, a hard link is a directory entry that associates a name with a file on a file system. All directory-based file systems must have at least one hard link giving the original name for each file. The term “hard link” is usually only used in file systems that allow more than one hard link for the same file.

Windows NTFS，Unix EXT4，ZFS，Btrfs 等文件系统均支持硬链接，ReFS 暂时不支持硬链接。

在 Windows 中，硬链接被广泛使用，尤其是 [Side-by-side assembly](https://en.wikipedia.org/wiki/Side-by-side_assembly) 机制大量使用了硬链接 ，查看 `C:\Windows\System32` 的文件，基本都会有相应的硬链接存在于 `C:\Windows\WinSxS`。

Git 在克隆本地存储库时，`objects` 目录的对象文件（主要是 pack）创建的是硬链接。这样避免了复制，git 的**对象**文件名与其内容的 SHA1 一致，当文件内容改变时，文件名也会改变，因此，使用硬链接不用当心互相修改破坏。

在 Windows 中，可以使用 `GetFileInformationByHandle`, `FindFirstFileNameW`, `FindNextFileNameW` 组合查询文件所有的硬链接。
在 POSIX 系统中，查询硬链接需要解析对应的 inode，如果 inode 值相同，则互为硬链接。`struct stat` 结构中有 `st_nlink` 表示此文件有多少个硬链接。

[Symbolic link](https://en.wikipedia.org/wiki/Symbolic_link) 符号链接（软链接）是一类特殊的文件， 其包含有一条以绝对路径或者相对路径的形式指向其它文件或者目录的引用。

软链接在 Unix 系统中被广泛使用，在终端中输入命令：`ls -l /usr/bin` 可以看到大量的软链接。

早期符号链接的实现，采用直接分配磁盘空间来存储符号链接的信息，这种机制与普通文件一致。这种符号链接文件里包含有一个指向目标文件的文本形式的引用，以及一个指示自己为符号链接的标志。

这样的存储方式被证明有些缓慢，并且早一些小型系统上会浪费磁盘空间。一种名为快速符号链接的新型存储方式能够将文本形式的链接存储在用于存放文件信息的磁盘上的标准数据结构之中（inode）。为了表示区别，原先的符号链接存储方式也被称作慢速符号链接。NTFS 文件系统的符号链接是基于 NTFS ReparsePoint 功能实现。

在 POSIX 系统中，`readlink` 可以解析符号链接获得真实的目标路径，在 Windows 中，则可以使用 `GetFinalPathNameByHandleW` 获得文件真实的路径。

NTFS 系统还支持一些其他的重解析点，包括 `MountPoint`, 与 UWP 快捷命令目标相关的 `AppExecLink`, 与 Windows 10 Unix domain socket 相关的 `AF Unix`, 与 OneDrive 相关的 `OneDrive`, 与 Git VFS（GVFS） 相关的 `ProjFS`, 以及与 WIM 挂载相关的 `WimImage` 等等。Planck 中实现了函数 [`ResolveTarget`](https://github.com/fcharlie/Planck/blob/a400828e62804b9c38c4e164e9f3efe559245e50/lib/inquisitive/resolve.cc#L82) 用于分析重解析点。

### 快捷方式和桌面文件

在 Windows 系统中，桌面快捷方式文件的后缀名为 `.lnk`，用户只需要点击桌面上的快捷方式就可以很方便的打开应用程序，网站或者文件。快捷方式的格式名称叫做 `Shell Link`，是一种二进制格式文件，相应的规范在 [[MS-SHLLINK]: Shell Link (.LNK) Binary File Format](https://msdn.microsoft.com/en-us/library/dd871305.aspx)。在 Planck 中，ShellLink 的定义和实现分别是 [lib/inquisitive/shl.hpp](https://github.com/fcharlie/Planck/blob/master/lib/inquisitive/shl.hpp) 和 [lib/inquisitive/shl.cc](https://github.com/fcharlie/Planck/blob/master/lib/inquisitive/shl.cc)，目前只支持解析 `HasLinkInfo` 以及 `HasRelativePath` 标志的快捷方式。

在 X-Window 系统上，也存在一种类似桌面快捷方式的文件，后缀名为 `.desktop` ，当文件属性为可执行时，文件管理器会解析 `Icon`，`Name` 然后读取设置的图标，名称显示。下面是我 Ubuntu 系统上的 `wireshark.desktop` 文件内容。

```shell
#!/usr/bin/env xdg-open
[Desktop Entry]
Name=Wireshark
Comment=Wireshark build
GenericName=Demo Application
Exec=/opt/wireshark/bin/wireshark
Icon=wireshark
Type=Application
StartupNotify=true
Categories=GNOME;GTK;Development;Documentation;
MimeType=text/plain;

```

可以看出，这是一个标准的 [`Shebang`](https://en.wikipedia.org/wiki/Shebang_(Unix)) 可执行文件，在 Ubuntu 中，`xdg-open` 自身为 `Shell` 脚本（在MacOS X下的Darwin中，解释器指定的文件必须是可执行的二进制文件，并且本身不能是脚本。），虽然这是一个 `Shebang` 可执行文件，但遗憾的是，在 Ubuntu 中，你无法直接从命令行中使用 `xdg-open` 启动相应的程序，这是一个存在了超过 9 年的 BUG: [xdg-open *.desktop opens text editor](https://bugs.launchpad.net/ubuntu/+source/glib2.0/+bug/378783)

## 文本文件还是二进制

在计算机中，文本文件实际上支持二进制文件的一种，这种文件几乎只由可打印字符，控制字符组成，而二进制文件则包含大量的不可见字符。处理程序将按照定义的二进制格式对二进制文件进行解析。

### 快速区分文本二进制

实际上，文本文件还是偶尔会携带不可见字符，这样情况下我们很难 100% 区分一个文件是否是文本文件（二进制文件）。如果能够容忍一些误差，
，我们可以检测文件中是否存在 `NUL` 来区分文件是文本文件还是二进制文件。虽然这种方法可能误差较大，但是检测过程非常简单，速度也非常可观，这种方法也被 `git` 使用，用于在 diff 过程中判断文件是否是二进制：

```c
//https://github.com/git/git/blob/d166e6afe5f257217836ef24a73764eba390c58d/xdiff-interface.c#L188
int buffer_is_binary(const char *ptr, unsigned long size)
{
	if (FIRST_FEW_BYTES < size)
		size = FIRST_FEW_BYTES;
	return !!memchr(ptr, 0, size);
}
```

我们知道，在 C 语言的标准库函数 `strlen` 中，字符串的长度计算是通过判断字符是否是 [Null-terminated string](https://en.wikipedia.org/wiki/Null-terminated_string)，这就意味着大多数时候，ASCII 文本文件不应该有 `NUL`，在 UTF-8 与 ASCII 兼容，这种情况下是一致的。当然这种设计也保守批评：[The Most Expensive One-byte Mistake](http://queue.acm.org/detail.cfm?id=2010365)

```c
size_t strlen(const char *s)
{
	const char *a = s;
	for (; *s; s++);
	return s-a;
}
```

### 编码的文本

ASCII 编码的范围是 0 ~127，这就意味着只能用于 `A-Z;a-z;0-9,+-` 数字，英文字母一些基本符号控制字符等少量的字符，如果存储非英语国家的文字基本上是不现实的，就算把 128 ~ 255 全部用上，像中文这种有几千上万文字的语言是无法表示的。为了支持更多的文字，后来人们制定了国际标准化的 US4(UTF-32) US2(UTF-16)，UTF-8，国内制定 GBK。当编码的种类多起来的时候，问题又来了，如何确定文件编码？

例如 `UTF-16`，`UTF-32` 这样的编码，由于是多字节的，因此可能存在多字节序，通过检测多字节序就可以简单的获得文件编码：

|编码|起始字符|
|---|---|
|UTF-32 BE|0x0,0x0,0xFE,0xFF|
|UTF-32 LE|0xFF,0xFE,0x00,0x00|
|UTF-16 BE|0xFE,0xFF|
|UTF-16 LE|0xFF,0xFE|
|UTF-8 with BOM|0xEF,0xBB,0xBF|

UTF-8 是一种字节序无关的可变字节编码（1 ~ 4 字节），因此，不带字节序没有任何问题，并且 ASCII 编码 0 ~ 127 完全是 UTF-8 的子集，如果不携带字节序，能够很好的兼容以前的 ASCII 文本。这也是 UTF-8 在 Unix 系统上被广泛使用的原因之一。而 Windows 记事本采用 UTF-8 with BOM 也由于这一点广受批评。

Windows 系统是一个国际化做的非常棒的操作系统，对于各国的本地字符集支持也非常好，比如，在中国大陆，文本编辑器的默认编码是 ANSI，是 ASCII 扩展编码，0 ~ 127 编码与 ASCII 相同，0x80 ~ 0xFFFF 则表示对应代码页的所有编码。我们可以看到，ANSI 编码的范围小于 UTF-8，并且绝大多数 ANSI 字符的码点相同数字的 UTF-8 码点都是有效的 UTF-8 字符，因此如果要区分 `UTF-8 without BOM` 还是 `ANSI`，实际上相当麻烦。

有效的 UTF-8 字符区间：
```c
/*
 * legal utf-8 byte sequence
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 *  Code Points        1st       2s       3s       4s
 * U+0000..U+007F     00..7F
 * U+0080..U+07FF     C2..DF   80..BF
 * U+0800..U+0FFF     E0       A0..BF   80..BF
 * U+1000..U+CFFF     E1..EC   80..BF   80..BF
 * U+D000..U+D7FF     ED       80..9F   80..BF
 * U+E000..U+FFFF     EE..EF   80..BF   80..BF
 * U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
 * U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
 * U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
 *
 */
```

另外，对于 ANSI 而言，不同字符集的都重复使用着 0x80 ~ 0xFFFF 编码区间，这进一步加大了文本字符检测的难度。

文本编码的检测有两个比较流行的实现，一个是 IE 的 [IMultiLanguage](https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/platform-apis/aa741022(v=vs.85))，另一个是 Firefox 的 [UniversalCharsetDetection](https://github.com/mozilla/gecko/tree/central/extensions/universalchardet/src/base)，后者的准确性更高，使用更加广泛，比如 `Notepad++` 就是使用了 `universalchardet`。

用户通常不应直接使用 Mozilla 目录中的 `Universalchardet`，`Universalchardet` 与 Firefox 整合较为紧密，剥离稍微有点麻烦，最近的版本只有很少的几个 `LangModels` 实现。如果要使用 `Universalchardet`，可以使用 Freedesktop 维护的：[uchardet](https://www.freedesktop.org/wiki/Software/uchardet/)，这个库基于 `Universalchardet` 发展起来的，能编译成动态库或者静态库供开发者整合到自己的程序之中。

但 `uchardet` 的许可证为 `MPL 1.1` ,`GPL  2.0` `LGPL 2.1`，程序在依赖 `uchardet` 时要考虑许可证的问题。如果仅仅只需要判断文本是否是 UTF-8，可以按照上图的 UTF-8 编码区间对文件进行分析，代码如下：

```c++
// Thanks
// https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/blob/master/2018/05/08/checkutf8.c
static const uint8_t utf8d[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,        // 00..1f
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,        // 20..3f
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,        // 40..5f
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   //
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,        // 60..7f
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //
    1,   1,   1,   1,   1,   9,   9,   9,   9,   9,   9,   //
    9,   9,   9,   9,   9,   9,   9,   9,   9,   9,        // 80..9f
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   //
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   //
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,        // a0..bf
    8,   8,   2,   2,   2,   2,   2,   2,   2,   2,   2,   //
    2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   //
    2,   2,   2,   2,   2,   2,   2,   2,   2,   2,        // c0..df
    0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, //
    0x3, 0x3, 0x4, 0x3, 0x3,                               // e0..ef
    0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, //
    0x8, 0x8, 0x8, 0x8, 0x8                                // f0..ff
};

static const uint8_t utf8d_transition[] = {
    0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, //
    0x6, 0x1, 0x1, 0x1, 0x1,                               // s0..s0
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //
    1,   1,   1,   1,   1,   1,   0,   1,   1,   1,   1,   //
    1,   0,   1,   0,   1,   1,   1,   1,   1,   1,        // s1..s2
    1,   2,   1,   1,   1,   1,   1,   2,   1,   2,   1,   //
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //
    1,   2,   1,   1,   1,   1,   1,   1,   1,   1,        // s3..s4
    1,   2,   1,   1,   1,   1,   1,   1,   1,   2,   1,   //
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   //
    1,   3,   1,   3,   1,   1,   1,   1,   1,   1,        // s5..s6
    1,   3,   1,   1,   1,   1,   1,   3,   1,   3,   1,   //
    1,   1,   1,   1,   1,   1,   3,   1,   1,   1,   1,   //
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,        // s7..s8
};

static inline uint32_t updatestate(uint32_t *state, uint32_t byte) {
  uint32_t type = utf8d[byte];
  *state = utf8d_transition[16 * *state + type];
  return *state;
}

bool validate_utf8(const char *c, size_t len) {
  const unsigned char *cu = (const unsigned char *)c;
  uint32_t state = 0;
  for (size_t i = 0; i < len; i++) {
    uint32_t byteval = (uint32_t)cu[i];
    if (updatestate(&state, byteval) == UTF8_REJECT) {
      return false;
    }
  }
  return true;
}
```

## 可执行文件

在计算机中，可执行文件是非常特殊的存在，现代计算机的运行离不开应用程序，而应用程序在磁盘上的形式就是可执行文件，维基百科上有简短的介绍：

>可执行文件在计算机科学上，指一种内容可被计算机解释为程序的计算机文件。通常可执行文件内，含有以二进制编码的微处理器指令，也因此可执行文件有时称为二进制档。这些二进制微处理器指令的编码，于各种微处理器有所不同，故此可执行文件多数要分开不同的微处理版本。一个计算机文件是否为可执行文件，主要由操作系统的传统决定。例如根据特定的命名方法（如扩展名为exe）或文件的元数据信息（例如UNIX系统设置“可执行”权限）。

可执行文件的格式非常多，但目前应用比较广泛的只有 PE(PE32+)，ELF，Mach-O。主要的操作系统分别是 Windows，Linux，macOS。

### 可执行文件的比较

不同的可执行文件的特性有一些不同，维基百科上有个比较：[Comparison of executable file formats](https://en.wikipedia.org/wiki/Comparison_of_executable_file_formats)。

我这里将 PE(PE32+)，ELF，Mach-O 的格式比较贴出来：

|格式名|操作系统|文件扩展名|显式处理器声明|任意节（Sections）|元数据|签名|字符串表|符号表|64位|胖二进制|可以包含图标|
|---|---|---|---|---|---|---|---|---|---|---|---|
|PE|Windows, ReactOS<br>HX DOS Extender<br>BeOS (>=R3)|.EXE|✔|✔|✔|✔|✔|✔|❌|❌|✔|
|PE32+|Windows 64-bit|.EXE|✔|✔|✔|✔|✔|✔|✔|✔|✔|
|ELF|Unix-like, OpenVMS|none|✔|✔|✔|✔|✔|✔|✔|Extension|Extension|
|Mach-O|NeXTSTEP<br>macOS, iOS, watchOS<br>tvOS|none|✔|<=256|✔|✔|✔|✔|✔|✔|❌|

### PE

PE 是 Windows NT 系统的可执行文件格式，同样还被 ReactOS 使用，PE32+ 是 PE 格式的一种改进，用于支持 64位处理器。要查看 PE 文件格式可以查看：[PE Format](https://docs.microsoft.com/en-us/windows/desktop/Debug/pe-format)。在 Windows SDK 中，`winnt.h` 已经定义了大量的 PE 结构，但并不完整，如果要获得更加完整的结构，需要使用 Windows WDK 的 `ntimage.h`，但一些新的硬件定义需要去 [PE Format](https://docs.microsoft.com/en-us/windows/desktop/Debug/pe-format) 查找。

```c++
/// #define PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64   13
#ifndef IMAGE_FILE_MACHINE_ARM64
//// IMAGE_FILE_MACHINE_ARM64 is Windows
#define IMAGE_FILE_MACHINE_ARM64 0xAA64 // ARM64 Little-Endian
#endif

#ifndef IMAGE_FILE_MACHINE_RISCV32
#define IMAGE_FILE_MACHINE_RISCV32 0x5032
#endif
#ifndef IMAGE_FILE_MACHINE_RISCV64
#define IMAGE_FILE_MACHINE_RISCV64 0x5064
#endif
#ifndef IMAGE_FILE_MACHINE_RISCV128
#define IMAGE_FILE_MACHINE_RISCV128 0x5128
#endif

#ifndef IMAGE_FILE_MACHINE_CHPE_X86
#define IMAGE_FILE_MACHINE_CHPE_X86 0x3A64 /// defined in ntimage.h
#endif

#ifndef IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG
#define IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG 17 // XBOX Code Catalog
#endif
```

EXE，DLL 文件的魔数是 `{'M','Z',0x90,0x0}` 这实际上是 `IMAGE_DOS_HEADER` 的 `e_magic`，实际上还有不同系统的签名并不一致：

```c++
#ifndef _MAC

#include "pshpack4.h"                   // 4 byte packing is the default

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

#include "pshpack2.h"                   // 16 bit headers are 2 byte packed

#else

#include "pshpack1.h"

#define IMAGE_DOS_SIGNATURE                 0x4D5A      // MZ
#define IMAGE_OS2_SIGNATURE                 0x4E45      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x4C45      // LE
#define IMAGE_NT_SIGNATURE                  0x50450000  // PE00
#endif
```
PE 格式的 `IMAGE_NT_HEADERS` 才是真正的 NT 头，DOS 头或者 OS2 头，主要用于兼容，毕竟 Windows 操作系统是从 16 位过来的。

`IMAGE_FILE_HEADER` 结构存储了机器架构，可执行文件特征和可选头大小等，解析到 `IMAGE_OPTIONAL_HEADER` 才算正式解析 PE。IMAGE_OPTIONAL_HEADER32 与 IMAGE_OPTIONAL_HEADER64 中的成员顺序有一些差别，这样的好处是在以 32位 IMAGE_OPTIONAL_HEADER 读取 64 位 PE 时依然能够解析到基本字段（反之也是一样）。解析 PE 很重要的一个函数是 [`ImageRvaToVa`](https://docs.microsoft.com/en-us/windows/desktop/api/dbghelp/nf-dbghelp-imagervatova) 在映射为文件的文件的映像头中查找相对虚拟地址（RVA），并返回文件中相应字节的虚拟地址。

解析 PE 文件导入导出，资源等需要解析可选头的 `DataDirectory` 数组，数组的序号对应的时不同的资源：

```c++
#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor
```

`IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR` 对应的 14 实际上在 .Net 中被使用，用于指向 `IMAGE_COR20_HEADER` 信息。

解析 PE 文件的库非常多，有被 [`Avast Threat Labs`](https://github.com/avast-tl/pelib) 使用的 `pelib`（没错，就是那个杀毒软件 Avast），还有 [https://github.com/hasherezade/bearparser](https://github.com/hasherezade/bearparser)，[https://github.com/lief-project/LIEF](https://github.com/lief-project/LIEF) 等非常优秀的开源库。在 .NET 平台还有 [PeNet](https://github.com/secana/PeNet)。其中 `LIFF` 还支持 ELF，Mach-O，ART，OAT 等格式。在 LLVM 的源码中 PE 文件解析代码在 [llvm/lib/Object/COFFObjectFile.cpp](https://github.com/llvm/llvm-project/blob/master/llvm/lib/Object/COFFObjectFile.cpp) 文件中。

分析 PE 的工具非常多，Windows Internal 7th 作者之一的 Pavel Yosifovich 也开发了一个 [Portable Executable Explorer](https://github.com/zodiacon/PEExplorer)。

Planck 分析了 [PE](https://github.com/fcharlie/Planck/blob/master/lib/inquisitive/pe.cc) 文件的机器类型，子系统，依赖，特征等。后来利用 Planck 的成果将 [PEAnalyzer](https://github.com/fcharlie/PEAnalyzer) 重构了一番，截图如下：

![PEAnalyze](https://github.com/fcharlie/PEAnalyzer/raw/master/docs/images/view.png)

我有时候需要从 `MSYS2 Mingw64` 中提取 `wget.exe`，经常需要手动查看文件依赖，非常麻烦，实现 Planck PE 解析模块后，于是编写了 [Nodeps](https://github.com/fcharlie/nodeps) 用于将 PE 文件将同目录下的所有依赖拷贝到目标目录。


### ELF

[Executable and Linkable Format (ELF, formerly named Extensible Linking Format)](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) 是一种运用非常广泛的可执行文件格式，目前 Unix-like 操作系统的可执行文件格式绝大多数都是 ELF 。ELF 的魔数是 `{0x7f,'E','L','F'}`。ELF 解析库有前面的 [LIFF](https://github.com/lief-project/LIEF) 还有被 `Avast Threat Labs` 使用的 [elfio](https://github.com/avast-tl/elfio) 官方版本地址是：[https://github.com/serge1/ELFIO](https://github.com/serge1/ELFIO)

与 PE 显著不同的是，ELF 文件可以有 `SONAME` `RPATH` `RUPATH` 这样的节。除了可执行文件主动加载依赖动态库，有操作系统或者可执行文件加载器被动加载依赖时，PE 文件依赖 dll 可以从 PATH 以及 PE 文件所在目录加载，而 ELF 只能加载 LD_LIBRARY_PATH 以及 RPATH RUPATH 指定目录下的动态链接库。PE 的机制容易带来注入问题，而 Windows 操作系统目前也增加了 KnownDlls 机制减少此类问题的发生。而 ELF 的机制在分发二进制时容易带来一些麻烦，但目前很多操作系统已经支持 `RUPATH=$ORIGIN/../lib` 这样的方式设置 `RUPATH`。另外 ELF 计算真实地址时不像 P需要使用 `ImageRvaToVa` 换算，在 ELF 文件的处理过程中，只需要将偏移地址与文件映射的起始地址相加即可得到数据地址。

ELF 程序在安装的时候可以主动修改 RPATH/RUPATH，cmake 也支持 `CMAKE_INSTALL_RPATH` 用于设置 `RPATH/RUPATH`。RPATH 和 RUPATH 的区别有篇博客有介绍：[RPATH and RUNPATH](https://blog.qt.io/blog/2011/10/28/rpath-and-runpath/)，不同操作系统链接器的处理也稍微有一些差别，大多数时候只要设置一个即可。

我将 cmake 中替换 RPATH 的功能抽出来，创建了项目： [cmchrpath](https://github.com/fcharlie/cmchrpath)，在 cmchrpath 中还有 `elfinfo` 用于查看 ELF 的一些基本信息。

### Mach-O

我没有任何 mac 设备，因此没有进一步分析 Mach-O 格式，实际上很多前辈们写了非常不错的文章，比如：[PARSING MACH-O FILES](https://lowlevelbits.org/parsing-mach-o-files/)。

Mach-O 一个鲜明的特性就是它是一个支持 FatBinary的格式（PE32+ 实际上也支持，但使用较少），这意味着不同的处理器架构指令能够存储在同一文件当中，在 Mac 将处理器从 PowerPC 架构迁移到 Intel 的过程中运用非常广泛。

在 Planck 中，Mach-O 格式的定义目录为：[lib/inquisitive/macho.hpp](https://github.com/fcharlie/Planck/blob/master/lib/inquisitive/macho.hpp)

### 自解压文件




### 可执行文件的移植

[Windows Subsytem for Linux](https://docs.microsoft.com/en-us/windows/wsl/about)

[Wine: Wine Is Not an Emulator](https://www.winehq.org/)

[Bash on Ubuntu on macOS](https://github.com/linux-noah/noah)

[Mach-o loader for linux](https://github.com/shinh/maloader)

[Porting Windows Dynamic Link Libraries to Linux](https://github.com/taviso/loadlibrary)

## 文档格式

大家比较熟知的文档格式有 `PDF`，`RTF`，`Microsoft WORD (.doc)`

## 压缩文件

### ZIP 文件格式

### 其他压缩格式