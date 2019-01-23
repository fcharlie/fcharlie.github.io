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

在 Unix 操作系统上，对于文件格式的检测通常是 `file` ([`file — determine file type`](https://linux.die.net/man/1/file)) 实现的，file 的源码在 Github 上有镜像：[https://github.com/file/file](https://github.com/file/file)。file 这样的工具通过分析文件魔数，文件头部特征分析文件格式，这样的工具严重依赖 **Magdir**，Magic 文件越多支持的格式越丰富。

