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

ZIP 的格式可以大致描述为：*文件头+文件压缩内容+文件头+文件压缩内容...*，这种布局的好处是，不需要解压缩就可以直到压缩文件的原始大小，可以获得压缩包中所有的文件信息，所以我们可以在使用 7z 快速打开压缩包后，只解压特定的文件。(下图来源于 [wikipedia](https://en.wikipedia.org/wiki/Zip_(file_format)#/media/File:ZIP-64_Internal_Layout.svg))

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

说到 ZIP 不得不说一下 WinZip，最初，WinZip 使用 PKZIP 提供 ZIP 压缩解压能力，1993 年，WinZip 使用了 Info-ZIP 压缩代码，不再使用 PKZIP，Info-ZIP 是被移植最多的软件之一，至今在 POSIX 平台使用的 zip/unzip 就是 Info-ZIP。WinZip 在 ZIP 格式中的修改通常时增加压缩算法，WinZip 称之为 `zipx`，WinZip 在 ZIP 格式中具有较大的影响力，以至于 PKWARE 为了兼容 WinZip 多次修改 ZIP 规范。另外 PKZIP 自 2014 年也未见后更新（我使用的是评估版本，是否有更新不得而知）。

### Tar 需要压缩算法

[`tar`](https://en.wikipedia.org/wiki/Tar_(computing)) 通常称为 `tarball`，是 POSIX 世界使用最广泛的归档文件格式，但我们需要注意，TAR 本身并不提供压缩功能，而需要 GZIP/BZIP2/LZMA/ZSTD 这样的压缩算法组合制作压缩软件，GNU Tar 通常需要其他 xz, gzip 这样的命令提供创建相关压缩软件的能力，而基于 libarchive 的 bsdtar 则要好一些，可以将压缩能力集成到内置的 filter 中，这样不用调用外部命令就可以支持多种格式。

tar 与常见压缩算法组合如下：

|后缀名|压缩算法|
|---|---|
|tgz, tar.gz|GZIP (deflate)|
|tbz, tar.bz2|bzip2|
|txz, tar.xz|lzma|
|tar.sz|snappy|
|tar.br|brotli|
|tar.zst|zstd|

不同的压缩算法提供了不同的压缩比，压缩解压的速度也不一样，下图是 [zstd](https://github.com/facebook/zstd) Benchmark 获得的一个结果：

| Compressor name         | Ratio | Compression| Decompress.|
| ---------------         | ------| -----------| ---------- |
| **zstd 1.4.5 -1**       | 2.884 |   500 MB/s |  1660 MB/s |
| zlib 1.2.11 -1          | 2.743 |    90 MB/s |   400 MB/s |
| brotli 1.0.7 -0         | 2.703 |   400 MB/s |   450 MB/s |
| **zstd 1.4.5 --fast=1** | 2.434 |   570 MB/s |  2200 MB/s |
| **zstd 1.4.5 --fast=3** | 2.312 |   640 MB/s |  2300 MB/s |
| quicklz 1.5.0 -1        | 2.238 |   560 MB/s |   710 MB/s |
| **zstd 1.4.5 --fast=5** | 2.178 |   700 MB/s |  2420 MB/s |
| lzo1x 2.10 -1           | 2.106 |   690 MB/s |   820 MB/s |
| lz4 1.9.2               | 2.101 |   740 MB/s |  4530 MB/s |
| **zstd 1.4.5 --fast=7** | 2.096 |   750 MB/s |  2480 MB/s |
| lzf 3.6 -1              | 2.077 |   410 MB/s |   860 MB/s |
| snappy 1.1.8            | 2.073 |   560 MB/s |  1790 MB/s |

ZSTD vs Deflate 压缩：

![](https://s1.ax1x.com/2020/07/19/URKpg1.png)

ZSTD vs Deflate 解压缩：

![](https://s1.ax1x.com/2020/07/19/URKiDK.png)

在使用 tar 时，我们创建 tar 归档文件后，再选择合适的压缩算法，就可以制作一个合适的压缩文件，比如 XZ 的压缩比很好高，所以现在很多开源软件使用 `tar.xz` 分发压缩包，但 `xz` 的速度非常慢，这就需要取舍。

于 ZIP 不同的是，`tar.*` 是先归档再压缩，因此如果需要读取 tar 家族压缩包，则需要先解压 tar 包才能读取相关信息，这就带来一个问题，无法随机读写 `tar.*` 压缩包。而于 ZIP 不同的是，在 `tar.*` 压缩包中，所有的文件头部信息先被归档了，和文件内容一起被压缩，这样的好处是有序的数据转变为无序的数据，信息量高，压缩比高。 

### 7z 极致的黑客

我的编辑器 Visual Studio Code 和公司电脑上的 Windows Terminal 使用了国内字体界大牛，微软开发者 [be5invis](https://github.com/be5invis) 的[更纱黑体](https://github.com/be5invis/Sarasa-Gothic)，在 Github 下载页面，字体文件使用 7z 压缩，ttf 字体压缩包大小通常为 200 多 MB。解压后至少 1GB（sarasa-gothic-ttf-0.12.10 解压后 10.2 GB），二进制文件 7z 压缩率非常可观。

[`7z`](https://en.wikipedia.org/wiki/7z) 是 [7-Zip](https://en.wikipedia.org/wiki/7-Zip) 自带的压缩文件格式，由俄罗斯开发者 Igor Pavlov 创建。7z 格式是一种高压缩比的压缩文件格式，在 7z 格式中需要建立流的概念，不如 ZIP/TAR 格式清晰，实现起来有点复杂，除了 7-zip 的官方实现外，[libarchive](https://github.com/libarchive) 也支持 7z 的压缩和解压。

```txt
Archive structure
~~~~~~~~~~~~~~~~~  
SignatureHeader
[PackedStreams]
[PackedStreamsForHeaders]
[
  Header 
  or 
  {
    Packed Header
    HeaderInfo
  }
]
```

7z 支持 Unicode 编码，支持加密标头，这就是说可以连文件名也加密，ZIP 则只做到文件内容加密。7z 不支持存储文件系统权限信息，这基本上限制了它在 POSIX 系统中的使用，7z 不支持提取损坏的文件，不支持随机读取。

7z 的特性做到了极致也限制了它在其他领域的使用。

### RAR 互联网分享而兴起

虽然 WinRAR 是专业软件，但盗版的门槛非常低，早期互联网上，很多人使用 `RAR` 格式分享文件，RAR 格式一度非常流行，RAR 支持分卷压缩，支持按照 Unicode(UTF-16) 存储文件名，2014 年，RAR5 格式推出，查看技术规范 [RAR5 technode.html](https://www.rarlab.com/technote.htm) ，通过分析 RAR5 格式，我们可以发现与 ZIP 格式有一定的相似之处，RAR5 是不是借鉴了 ZIP 格式不得而知。RAR5 支持存储 UTF-8 文件名，强度更高的 AES 加密，支持 NTFS 符号链接和硬链接等等。

RAR/RAR5 格式属于专有格式，虽然 RAR5 的技术规格开放了，但是并不意味着人们可以按照规范实现兼容的 RAR 压缩软件，根据协议，只能提供 RAR 解压功能，rarlab 可以直接下载 [unrar](https://www.rarlab.com/rar_add.htm) 源码或者命令行，用于解压 RAR 文件。


### ZIP vs Tar vs 7z 技术的殊途

ZIP 的机制有利于其他格式采用，比如 Office（2007+） 文档格式，Android APK，Windows APPX，Java Jar，iOS ipa 等等，都是使用了 ZIP 格式。在 Linux 等 POSIX 系统，除了 `tar.*`，其他格式似乎都不是最常规的选择，当然也可以选择 ZIP，但系统默认都安装了 tar，不一定安装了 Info-ZIP。在 Windows 系统中，而为了提供更高的压缩率，Chrome 安装包则会将文件打包成 chrome.7z，并内置一个 7Z 解压它，实际上这个思路在一些安装包制作软件中也有使用。三种格式代表着三种不同的方向，实际上也是计算机技术的分歧，不同的人有不同的看法。

## 最后

这篇文章比较短，主要是最近事情比较多。