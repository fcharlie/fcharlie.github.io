---
layout: post
title:  "评论文件解析"
date:   2021-03-11 10:00:00
published: true
categories: toolset
---

## 前言

2019 年，我曾经写过一篇名为[《文件的解析》](https://forcemz.net/toolset/2019/01/25/FileParsing/)的文章，粗略的讲述了如何识别文件的特征，分析文件的类型。随着认识的不断加深，知识的不断积累，代码的不断重构，徘徊之间，我将文中提到的 Planck 分析库在 [Bela](https://github.com/fcharlie/bela) 项目中用 C++20 重写了，还将 Golang 的 PE/ELF/MachO/ZIP 库用 C++ 重写了一遍，然后在 [BelaUtils](https://github.com/fcharlie/BelaUtils) 项目中基于 Bela 文件分析库实现了文件类型检测工具 - **Bona**，工具简单的推广后总算是有几个人收藏，之后有一点想法，觉得要和大家分享，便有了此文。

## 评论文件解析

“大家明白，不论做什么事，不懂得那件事的情形，它的性质，它和它以外的事情的关联，就不知道那件事的规律，就不知道如何去做，就不能做好那件事。” 文件的解析亦是如此，要解析文件，首先得对文件的格式有个大致的了解，然后分析它区别于其它文件的特征，这样就能快速检测，好在计算机行业是一个经验积累的行业，人们在设计新的文件格式也通常会借鉴旧有的文件格式，吸取精华破除糟粕。为了方便一些程序快速的检测文件，有些文件会聪明的使用魔数的机制去标识文件头，这样能大大提高文件格式的识别速度。

## 字节序和字节序的处理

在计算机中，大端和小端代表了两种不同的理解，网络字节序使用大端，x86/AMD64 使用小端，ARM 支持大端也支持小端，但小端居多，最后我们发现，市面上绝大多数的计算机及其操作系统的本机字节序是小端。

大端和小端的转换通常使用 `bswap*/_byteswap_*` 这样的函数，当然需要先确定自己的主机字节序，主机字节序是固定的，使用 C++ 编程时可以使用 `constexpr` 或者宏定义。

对于 ARM 等一些处理器则需要关注内存对齐的问题，在转换字节序时，如果引用的是指针则需要考虑内存对齐。x86/AMD64 会自己处理好。

## ZIP 文件的压缩和解压

解压 ZIP 文件的思路有两种，第一种是从文件末尾开始找到中央目录结尾，中央目录结尾的魔数为 `'P','K',0x05,0x06`，中央目录结尾的用处是记录 zip 文件中文件/目录的数量和偏移，如果 zip 是一个大文件或者目录条目数大于 0xFFFF 还需要检测 64 位中央目录结尾，再读取一个个中央目录条目，读取这些信息后再按文件偏移读取解压文件，这属于标准的做法。

另一种是从文件头部开始一个个目录的读取，但这并不是合适的方法，对于一些自解压的文件，这里就显得无能为力。

实际上在编写 zip 解压缩库的时候，我确实感到 zip 的设计充满了历史感，很多功能只能通过扩展来实现，导致规范篇幅冗长，很多客户端也就只实现了规范中的一部分而不是全部。

ZIP 编码默认是 ANSI 的，因此在分享 zip 文件的时候可能会遇到乱码的情况，正确的做法是在压缩时强制使用 UTF-8，解压时使用编码检测机制自动识别编码。 

## PE 文件的分析

PE 文件的解析比较简单，毕竟这东西都解析烂了，很多人都写过，要注意的主要是 `RVA`，其他的也就没有什么可以说的了。

上面讲到 zip 的解析从末尾开始，而 PE 的解析从开头开始，于是我们将一个 ZIP 文件追加到 PE 文件的尾部，就可以实现自解压文件了，在 PE 中，我们将在 Section 之外的数据称之为 `Overlay` 数据，当然里面有一些细节需要处理。Overlay 数据还被用于存储 PE 的校验和，或者病毒将一些数据放在这段空间等等。

## ELF 文件的分析

ELF 文件的解析和 PE 类似，也没什么好说的，当然也可以存在 `Overlay` 数据，也就是说，你也可以按照这样的逻辑实现自解压文件，这种机制和 STGZ 的原理有一点类似。

## MachO 文件的分析

MachO 的解析稍微复杂，与 PE/ELF 不太相同，另外 MachO 还有 FatMachO 机制，也就使得解析要稍稍复杂，MachO 的概念也和 PE/ELF Section 之类的概念有一些差别，解析复杂一点。

### UniversalChardet 的故纸堆

在初入职场时，我知道的文件编码检测有 `UniversalChardet`，但最近发现 FireFox 已经把 `UniversalChardet` 删除了，而是使用 Rust 编写的 `chardetng`，`UniversalChardet` 的衍生版本 uchardet 仍然被 [Freedesktop](https://www.freedesktop.org/wiki/Software/uchardet/) 维护，但由于许可证的问题，实际上包括我在内的一些人是不会选择的。我在 Bona 中检测 ZIP 文件编码使用的是 [Compact Encoding Detection](https://github.com/google/compact_enc_det)。

+   https://github.com/mozilla/gecko-dev/commit/1447a771ccd75a183526169605a83f5c06db07b6
+   https://phabricator.services.mozilla.com/D27794
+   https://hsivonen.fi/chardetng/
+   https://github.com/hsivonen/chardetng