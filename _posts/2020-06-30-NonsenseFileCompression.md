---
layout: post
title:  "胡说八道文件压缩"
date:   2020-06-30 20:00:00 +0800
published: true
categories: file
---

# 前言

在我们日常使用计算机的时候，经常会遇到或者使用一些文件，它们通常以 `.zip`，`.rar`，`.7z` 结尾，这些文件就是本文所说的压缩文件，我们可以通过资源管理器，WinRAR，WinZip，或者 7-zip 将其解压，然后查看，编辑，或者进行其他操作。压缩软件被广泛运用于文件分享，软件分发，或作为一个容器承载其他文件集合，比如我们如果使用 Office 2007 以及更高的版本保存 Word/PPT/Excel 文档时，这些文档实际上就是特殊的 ZIP 压缩文件，Word/PPT/Excel 的格式实际上就是将描述元素（文本框，标题...） 信息的 XML 文件，以及一些资源文件使用 Deflate(Fastest) 压缩算法压缩到一个 ZIP 格式文件中。还有 Java 程序员需要经常见到的 Jar 包，Android 开发者打包的 APK 文件，以及 Windows 开发者创建的 Appx(以及 MSIX) 安装包，这些文件都是压缩文件，准确的说，这几种都是 ZIP 压缩文件。

压缩软件的格式非常多，有 ZIP，RAR，7Z，TAR，CAB 等等，其中 ZIP 是运用最广泛的压缩文件格式，而在 Unix 世界 TAR 则更受欢迎。探讨文件的压缩实际上是个有趣的过程，而本文也就是对压缩文件进行一个胡乱的分析。

## 常见的文件压缩格式比较

### ZIP 轶闻和现状

如果我们使用 VIM 自带的十六进制查看工具 xxd（或者 [hexyl](https://github.com/sharkdp/hexyl)），我们可以发现，ZIP 压缩文件以 'PK' 开头。


## 压缩文件

压缩文件历史悠久，zip/7z/rar 各种格式的轶闻颇多，压缩文件通常有两种策略，一种是像 ZIP 这种，压缩粒度是按文件划分，即文件头+压缩后的文件一个个的组合成一个文件，另一种是 `tar.*` 这类，先归档再压缩，即先将文件头+文件内容一个个组合，组合成一个文件后再进行压缩，两种策略各有优劣，比如前者能够方便的查看压缩文件的目录结构，后者能够提供更高的压缩比。但二者也有共同之处，压缩文件格式与算法并不是相同的概念，二者都能使用特定的压缩算法。

`tar.*` 格式文件通常是 tar 文件使用特定的压缩算法压缩而成，相应的表格如下：

|后缀名|压缩算法|
|---|---|
|tgz,tar.gz|GZIP (deflate)|
|tar.bz2|bzip2|
|tar.xz|lzma|
|tar.sz|snappy|
|tar.br|brotli|
|tar.zst|zstd|

ZIP 压缩文件格式在文件头中会使用魔数标明压缩文件条目的压缩算法，相关的魔数（常用）如下（[APPNOTE.TXT](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT)）:

|压缩算法|魔数|备注|
|---|---|---|
|Store|0|存储目录|
|8|deflate|默认格式|
|9|deflate64|与 deflate 类似，但字典要大一些，开源 ZIP 基本不支持|
|12|bzip2||
|14|lzma||
|20|zstd||
|96|JPEG variant||
|97|WavPack||
|98|PPMd verson I, Rev 1||
|99|AE-x encryption marker||

我们可以看到虽然压缩文件的策略有很大的区别，但是还是可以使用相同的压缩算法，`tar.*` 结合压缩算法非常容易，但 ZIP 则涉及到规范文件反而没那么容易，所以最近 ZIP 格式规范才增加了 zstd 算法魔数，开源世界 zstd 也早已被广泛使用。

ZIP 的规范控制在 PKWARE 手中，但 WinZip 的影响力也非常大，[WinZip 使用了 93 作为 ZIP-ZSTD 的魔数](https://github.com/mcmilk/7-Zip-zstd/issues/132)，这实际上会造成困扰。

**2020-06-20 更新** PKWARE 已经更新了 Zip 规则，魔数 20 已经被废弃，93 作为 ZIP-ZSTD 魔数。

|压缩算法|魔数|备注|
|---|---|---|
|Store|0|存储目录|
|8|deflate|默认格式|
|9|deflate64|与 deflate 类似，但字典要大一些，开源 ZIP 基本不支持|
|12|bzip2||
|14|lzma||
|93|zstd||
|96|JPEG variant||
|97|WavPack||
|98|PPMd verson I, Rev 1||
|99|AE-x encryption marker||

## ZIP 的 ZSTD 算法支持

在 [baulk](https://github.com/baulk/baulk) 中，baulk 内置了 ZIP 包的解压功能，使用 minizip 实现，但 minizip 并不支持 ZSTD，于是我今天就初步实现了未经足够测试的 ZIP-ZSTD 解压功能，在 [Bali](https://github.com/balibuild/bali) 中，也增加了生成使用 ZSTD 压缩的 ZIP 包的能力。并且我还给 [archiver](https://github.com/mholt/archiver/pull/223) 提交了一个 PR，但 archiver 的 CI 应该出问题了。

## 最后

这篇文章比较短，主要是最近事情比较多。