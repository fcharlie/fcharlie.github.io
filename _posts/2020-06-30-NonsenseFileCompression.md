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

如果我们使用 VIM 自带的十六进制查看工具 xxd（或者 [hexyl](https://github.com/sharkdp/hexyl)，你也就可以使用 baulk 安装 belautils，然后使用 hastyhex），我们可以发现，[ZIP](https://en.wikipedia.org/wiki/Zip_(file_format)) 压缩文件以 `PK` 开头。`PK`
是已故著名程序员 *Phil Katz* 的英文缩写。值得一说的是，虽然 Phil Katz 创造了 ZIP 格式，但他却贫困潦倒，死在了汽车旅馆（[Phil Katz 的故事](http://www.bbsdocumentary.com/library/CONTROVERSY/LAWSUITS/SEA/katzbio.txt)）。目前 ZIP 的规范由 PKWARE 发布，PKWARE 是 Phil Katz 创立，Phil Katz 死后，其家人将 PKWARE 卖出，目前 ZIP 的格式规范版本为 [APPNOTE-6.3.9.TXT](https://pkware.cachefly.net/webdocs/APPNOTE/APPNOTE-6.3.9.TXT)，规范可以阅读但不可分发。

ZIP 的格式可以大致描述为：*文件头+文件压缩内容+文件头+文件压缩内容...*，这种布局的好处是，不需要解压缩就可以直到压缩文件的原始大小，可以获得压缩包中所有的文件信息，所以我们可以在使用 7z 快速打开压缩包后，只解压特定的文件。

![](https://s1.ax1x.com/2020/07/18/U28wXn.png)

ZIP 的使用非常广泛，比如 Microsoft Office 2007 后的 `.pptx`，`.docx`，`.xslx` 文件，如果使用 7z GUI 打开，我们可以发现，这就是一个 ZIP 压缩文件，同样的，Java 的 Jar 包，Android 的 APK，Windows UWP 的 appx 文件无一不是基于 ZIP 格式。

在 2006 年之前，ZIP 格式不支持 Unicode，所以我们经常会遇到，下载其他 Windows 上压缩的 ZIP 文件可能出现乱码，这是因为二者的代码页不相同，使用的本地编码不同，2006 年的 APPNOTE-6.3.0.TXT 规定了在 ZIP 文件中按照 UTF-8 存储文件名的机制，具体可以参考 Minizip/`archive/zip` 中的相关字段。在此之前，如果要存储 UTF-8 的文件名，InfoZip 使用了额外的字段将 UTF-8 文件名保存到中央目录头，也就是上图中的尾部。14 年过去了，一些软件比如 7z/Explorer 在创建 ZIP 时，仍然没有使用 UTF-8 存储文件名，也没有使用 InfoZip 的兼容机制，在分享包含非 ASCII 字符文件名的压缩包时，不同的代码页的系统就会出现乱码。计算机软件应该与时俱进，不要保持老掉牙的惯例。minizip 内部编码全为 UTF-8，创建压缩文件时就使用了 UTF-8，这就很好。在我维护的 7z 版本 [baulk7z](https://github.com/baulk/baulk7z) 中，默认设置 ZIP 压缩时选择 UTF-8。另外，git 在创建 ZIP 格式的 archive 也会标记文件名编码为 UTF-8。

ZIP 压缩文件格式在文件头中会使用魔数标明压缩文件条目的压缩算法，常见的魔数（常用）如下:

|压缩算法|魔数|备注|
|---|---|---|
|Store|0|存储目录|
|8|deflate|默认格式|
|9|deflate64|与 deflate 类似，但字典要大一些，开源 ZIP 基本不支持|
|12|bzip2||
|14|lzma||
|20|deprecated (use method 93 for zstd)||
|93|Zstandard (zstd) Compression||
|94|MP3 Compression||
|95|XZ Compression||
|96|JPEG variant||
|97|WavPack||
|98|PPMd verson I, Rev 1||
|99|AE-x encryption marker||

Deflate 压缩算法使用非常广泛，比如我们使用的 git 版本控制软件在将文件纳入版本控制时，就会使用 deflate 算法将其压缩，说到 deflate，就不得不提 `zlib`，`zlib` 提供了 deflate 压缩算法实现，操作系统，浏览器，数据库，IDE，办公软件，很多软件都可能在使用 zlib。deflate64 是 deflate 的大字典版本，但受限于专利，开源软件支持 deflate64 的较少，目前已知的是 7z、Windows 资源管理器，.Net 能够压缩解压 deflate64 压缩的 ZIP 文件，LZMA 压缩算法压缩的 ZIP 文件通常是 7z 创建的，但 7z 的机制有点反常规，因此其他 ZIP 库对其支持较差。

Zstandard 是 Facebook 开发的一个新的压缩算法，无论在压缩率还是速度都有一个较好的均衡，同样的压缩比，要比 deflate 速度快很多，许可协议友好，很多开源软件使用，就连 Linux 内核都在使用，而早期 WinZip 将数字 93 添加为 Zstandard 压缩算法的魔数，提供了 ZIPX 压缩格式，这个事情不知是不是 PKWARE 开发者忽略了还是怎么的，PKWARE 今年在规范中将数字 20 作为 ZSTD 的魔数，后来有开发者在 [7-Zip-zstd](https://github.com/mcmilk/7-Zip-zstd) 上寻求在 ZIP 中添加 ZSTD 压缩算法的支持，遇到这个情况，我赶紧给 minizip 提交了实现代码，还给 archiver 贡献了对 ZSTD 的支持。但在 7-Zip-zstd 作者发送邮件给 PKWARE 之后，PKWARE 赶紧又更新了规范，把 20 给废弃了。于是我贡献的项目又得一一的发送 PR 去更新对 ZSTD 的支持。

在 [bali](https://github.com/balibuild/bali) [baulk](https://github.com/baulk/baulk) 这些我自己开发的软件中，我都在其压缩（解压）ZIP 时增加了 ZSTD 压缩算法的支持。回顾给 [minizip](https://github.com/nmoinvaz/minizip/pulls?q=is%3Apr+author%3Afcharlie+is%3Aclosed)，[libzip](https://github.com/nih-at/libzip/pull/181)，[7-zip-zstd](https://github.com/mcmilk/7-Zip-zstd/pull/140)，[archiver](https://github.com/mholt/archiver/pulls?q=is%3Apr+is%3Aclosed+author%3Afcharlie)，这些 ZIP 压缩库（压缩软件）的维护都十分有限，PR/Issue 解决较慢，或者根本不理，有一种劝退感，开发软件的基础设施维护和演进确实不那么美好。

### Tar 和它的伙伴们


`tar.*` 格式文件通常是 tar 文件使用特定的压缩算法压缩而成，相应的表格如下：

|后缀名|压缩算法|
|---|---|
|tgz,tar.gz|GZIP (deflate)|
|tar.bz2|bzip2|
|tar.xz|lzma|
|tar.sz|snappy|
|tar.br|brotli|
|tar.zst|zstd|

### ZIP vs Tar vs 7z 人生的分歧

## 最后

这篇文章比较短，主要是最近事情比较多。