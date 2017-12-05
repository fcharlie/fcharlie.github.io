---
layout: post
title:  "Git 巨型存储库的解决方案"
date:   2017-12-15 10:00:00
published: false
categories: git
---

## 前言

通常来说，分布式版本控制系统适合较小的存储库，[分布式版本控制系统](https://en.wikipedia.org/wiki/Distributed_version_control) 意味着存储库和工作目录都存在在开发者自己的机器上，当开发者需要克隆一个巨大的存储库时，为了获得完整的拷贝，版本控制软件不得不从远程服务器上下载大量的数据。这是分布式版本控制系统最大的缺陷之一。

这种缺陷并不会阻碍 git 的流行，自 2008 年以来，git 已经成为事实上的版本控制软件的魁首，诸如 GCC<sup>1</sup>，LLVM<sup>2</sup>  这样的基础软件也已迁移到或者正在迁移到 git。那么如何应对这种大存储库的危机呢？

## 浅表克隆和稀疏检出

很早之前，我的构建 LLVM 的时候，一直使用 svn 去检出 LLVM 源码，当时并不知道 git 能够支持浅表克隆，目前，在 Clangbuilder<sup>3</sup> 的源码中，检出 LLVM 全部使用 git ，使用浅表克隆。在 git 克隆的时候使用 `--depth=N` 参数就能够支持克隆最近的 N 个 commit，这种机制对于像 CI 这样的服务来说，简直是如虎添翼。

```
       --depth <depth>
           Create a shallow clone with a history truncated to the specified number of commits. Implies
           --single-branch unless --no-single-branch is given to fetch the histories near the tips of all branches.
```

与常规克隆不同的是，浅表可能需要多执行一次请求，用来协商 commit 的深度信息。

在服务器上支持浅表克隆一般不需要做什么事。如果使用 *git-upload-pack* 命令实现克隆功能时，对于 HTTP 协议要特殊设置，需要及时关闭 *git-upload-pack* 的输入。否则，git-upload-pack 会阻塞不会退出。对于 Git 和 SSH 协议，完全不要考虑那么多，HTTP协议是 **Request--Respone** 这种类型的，而 Git 和 SSH 协议则没有这个限制。

而稀疏检出指得是在将文件从存储库中检出到目录时，只检出特定的目录。这个需要设置 `.git/info/sparse-checkout`。稀疏检出是一种客户端行为，只会优化用户的检出体验，并不会减少服务器传输。

 ## Git LFS 的初衷

对于 Github 而言，大文件耗费了他们大量的存储和带宽。git 实质上是一种文件快照系统。创建提交时会将文件打包成新的 blob 对象。这种机制意味着 git 在管理大文件时是非常占用存储的。比如一个 1GB 的 PSD 文件，修改 10 次，存储库就可能是 10 GB。当然，这取决于 zip 对 PSD 文件的压缩率。同样的，这种存储库在网络上传输，需要耗费更多的网络带宽。

Github 2015 年推出了 Git LFS，在前面的博客中，我介绍了如何实现一个 Git LFS 服务器<sup>4</sup>，这里也就不再多讲了。

## GVFS 的原理

好了，说道今天的重点了。微软有专门的文件介绍了 **《Git 缩放》**<sup>5</sup> **《GVFS 设计历史》**<sup>6</sup>，相关的内容也就不赘述了。

## 使用 Libgit2

## GVFS 应用分析

## 相关信息


## 链接

1.   [Moving to git](https://gcc.gnu.org/ml/gcc/2015-08/msg00140.html)
2.   [Moving LLVM Projects to GitHub](http://llvm.org/docs/Proposals/GitHubMove.html)
3.   [Checkout LLVM use --depth](https://github.com/fstudio/clangbuilder/blob/63f45b5b99d6b2f8473356dfbe3454238f6dee2e/bin/LLVMRemoteFetch.ps1#L33)
4.   [Git LFS 服务器实现杂谈](http://forcemz.net/git/2017/04/16/Moses/)
5.   [Git at scale](https://www.visualstudio.com/zh-hans/learn/git-at-scale/)
6.   [GVFS Design History](https://www.visualstudio.com/zh-hans/learn/gvfs-design-history/)