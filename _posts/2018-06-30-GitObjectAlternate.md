---
layout: post
title:  "Git Fork 的对象共享"
date:   2018-06-30 10:00:00
published: false
categories: git
---

# 前言


将fork 后的存储库的备用对象设置到源存储库的 objects 目录 可以通过设置环境变量 `GIT_ALTERNATE_OBJECT_DIRECTORIES` 实现，fork 时将 heads tags 等拷贝一份到 `refs/namespaces` 目录，主要是为了防止 GC 清理掉这些对象。源存储库克隆的时候需要隐藏 `namespaces`

```sh
git -c transfer.hideRefs="refs/namespaces"
```

而 fork 后的存储库只需要建立存储库然后设置好 `objects/info/alternates` 即可。

`objects/info/alternates` 内容如下：

```txt
/path/to/repo.git/objects
```