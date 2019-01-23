---
layout: post
title:  "文件的魔法"
date:   2019-01-25 21:00:00
published: false
categories: toolset
---

# 前言

本文探讨的是计算机文件，**计算机文件** 用于记录数据到计算机设备上，维基百科上有简短的介绍：

>A computer file is a computer resource for recording data discretely in a computer storage device. Just as words can be written to paper, so can information be written to a computer file.

当人们需要使用这些文件的时候，需要从光盘，磁盘，闪存等设备上将文件读取到内存，按照文件的格式进行解析，然后供用户使用。在这个过程中，对文件格式的识别尤为重要，只有在识别出文件格式之后，才能够选择合适的处理程序对文件进行解析。在 Windows 上通常是 Shell 外壳（`Shell32.dll`）根据文件后缀名在注册表中找到对应的关联程序然后使用特定的程序处理相应的文件，比如 `.docx` 的关联程序往往是 `Microsoft Word`。`.txt` 的关联程序是 `Notepad`。但如果文件没有后缀名时，Windows Shell 就需要用用户自己选择对应的关联程序了。

在 Unix 操作系统上，命令行下检测文件格式的检测通常使用 `file` ([`file — determine file type`](https://linux.die.net/man/1/file)) ，file 的源码在 Github 上有镜像：[https://github.com/file/file](https://github.com/file/file)。file 这样的工具通过分析文件魔数，文件头部特征分析文件格式，这样的工具严重依赖 **Magdir**，Magic 文件越多支持的格式越丰富。file 这样的命令与 Windows 资源管理器相比，已经有很大的进步。

在图形系统中，文件的检测由文件管理器实现，像 `Gnome Nemo` 这样的文件管理器会优先处理文件后缀名，在识别不到文件格式时才会去根据文件特征检测文件格式。Nemo 依赖 glib(gio [`_xdg_mime_magic_lookup_data`](https://github.com/GNOME/glib/blob/cbfa776fc149fcc3e351fbdf68c1a299519f4905/gio/xdgmime/xdgmimemagic.c#L657))，和 file 的原理类似但没有 `file` 强大。

我最开始去了解文件类型的检测是在实现 LFS 服务器的时候，基于 C++ 编写的 LFS 服务器使用的是 `libmagic`, libmagic 即 `file` 的一部分，而基于 `Golang` 编写的 LFS 服务器使用的则是 [https://github.com/h2non/filetype](https://github.com/h2non/filetype)

在重构完 `Privexec` 之后，突然想写一个文件类型检测工具，最开始叫做 `FileView` 后来改名为 `Planck`，当 `Planck` 大概能用的时候，想把一些见解分享给大家，于是有了此篇文章。

## 文本文件还是二进制

### 快速区分文本二进制

在计算机中，文本文件实际上支持二进制文件的一种，这种文件几乎只由可打印字符，控制字符组成，但实际上，文本文件还是偶尔会携带不可见字符，要判断一个文件是否是二进制文件/文本文件，有一个简单的方法，即检测文件中是否存在 `NUL`，虽然这种方法可能误差较大，但还是被 `git` 使用了。

```c
//https://github.com/git/git/blob/d166e6afe5f257217836ef24a73764eba390c58d/xdiff-interface.c#L188
int buffer_is_binary(const char *ptr, unsigned long size)
{
	if (FIRST_FEW_BYTES < size)
		size = FIRST_FEW_BYTES;
	return !!memchr(ptr, 0, size);
}
```

### 编码的文本

## 可执行文件

### PE

### ELF

### Mach-O


## 文档格式


## 压缩文件

### ZIP 文件格式

### 其他压缩格式