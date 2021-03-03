---
layout: post
title:  "评论文件解析"
date:   2021-08-25 10:00:00
published: false
categories: toolset
---

## 前言

2019 年，我曾经写过一篇名为[《文件的解析》](https://forcemz.net/toolset/2019/01/25/FileParsing/)的文章，粗略的讲述了如何识别文件的特征，分析文件的类型。随着认识的不断加深，知识的不断积累，代码的不断重构，徘徊之间，我将文中提到的 Planck 分析库在 [Bela](https://github.com/fcharlie/bela) 项目中用 C++20 重写了，还将 Golang 的 PE/ELF/MachO/ZIP 库用 C++ 重写了一遍，然后在 [BelaUtils](https://github.com/fcharlie/BelaUtils) 项目中基于新库实现了文件类型检测工具 - **Bona**，工具简单的推广后总算是有几个人收藏，之后有一点想法，觉得要和大家分享，便有了此文。

## 评论文件解析




## 字节序和字节序的处理

## ZIP 文件的压缩和解压

## PE 文件的分析

## ELF 文件的分析

## MachO 文件的分析

### UniversalChardet 的故纸堆

https://github.com/mozilla/gecko-dev/commit/1447a771ccd75a183526169605a83f5c06db07b6
https://phabricator.services.mozilla.com/D27794
https://hsivonen.fi/chardetng/
https://github.com/hsivonen/chardetng