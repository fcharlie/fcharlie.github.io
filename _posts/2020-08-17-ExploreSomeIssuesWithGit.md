---
layout: post
title:  "探讨 Git 版本控制系统的若干问题 - 2020 版"
date:   2020-08-17 20:00:00 +0800
published: false
categories: git
---

## 前言

自 2014 年大学毕业以来，我一直从事代码托管相关工作，工作的内容和 git 相关，期间积累了很多心得体会，这大概是一般的 git 使用者很少会感知到的，一直以来，我也想将这些见解分享给大家，之前我写过《探讨 Git 代码托管平台的若干问题 - 2019 版》，这篇文章主要偏向代码托管平台的开发，与普通开发者存在一定的距离，快一年过去了，我又有了新的体会，写一篇关于 Git 版本控制系统的若干问题也就有了动机。

## GitFlow 与 Git 的表与里

在使用 Git 进行团队协作的时候，网络上铺天盖地的推荐方案是使用 Gitflow 工作流，什么是 Gitflow？Atlassian 写了一篇文章：[Gitflow Workflow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow) 对其有个简单的介绍，简而言之，就是在 `develop` 分支快速迭代，将 `master` 作为稳定分支，使用 `feature` 分支添加新的功能。说实话，我并不是很喜欢 Gitflow，Git 的分支和 SVN 这样的版本控制系统相比，足够轻量，比如 SVN 新建分支需要拷贝分支，而 git 就是创建对某个 commit 的引用即可，这样看来，git 中的分支都是均权的，Gitflow 的概念就像话术了，比如我们完全可以把稳定分支命名未 `stable` 或者 `release`，把快速迭代分支命名为 `trunk`。

在 git 的世界中，我们理解了分支和最终纳入版本控制的文件是如何组织的，就能更好的思考应该使用什么样的工作流。Git 的存储结构太多文章和文档有过说明，这里详述繁琐且没必要，简单的说一下，在 `refs` 目录或者 `packed-refs` 存储了 git 存储库的引用信息，引用名（`refs/heads/master`）与特定的 commit ID 映射，引用只有唯一的 commit ID，commit ID 却可以跟多个引用关联，比如我们使用 `git checkout -b` 或者 `git switch -c` 就可以基于当前的分支的 commit 创建新的分支。分支是包含特定前缀 `refs/heads/` 的引用。

CommitID 是 git 对象的顶层数据，在 git 对象中，包含 `commit`，`tree`，`blob` 这样的对象，一个 commit 有一个 tree，零个或者多个 parent commit，我们可以使用特定的命令查看 commit 的内容：

```shell
# https://github.com/BLAKE3-team/BLAKE3.git
git cat-file -p 107f5c089f356334ee4abaeeca8c31704661f37d
```

其内容可能如下

```text
tree 107f5c089f356334ee4abaeeca8c31704661f37d
parent f2005678f84a8222be69c54c3d5457c6c40e87d2
author Jack O'Connor <jack.oconnor@zoom.us> 1593462857 -0400
committer Jack O'Connor <jack.oconnor@zoom.us> 1593463133 -0400

stop being a jerk and add the context string to test_vectors.json
```

Git tree 对象存储了存储库的目录结构，tree 的格式为特殊的二进制格式，我们同样可以用 `git cat-file` 查看 tree 对象。

```shell
git cat-file -p 107f5c089f356334ee4abaeeca8c31704661f37d
```

tree 对象的 `pretty` 输出如下：

```text
040000 tree c44c5f00bbfd67a6e5597292b811055fa5f90034    .github
100644 blob fa8d85ac52f19959d6fc9942c265708b4b3c2b04    .gitignore
100644 blob 3a605f255e8cfd344c202bef8fe1645fd49a2095    CONTRIBUTING.md
100644 blob 720e9a4ab4976211fe78ea44681150460cf9ed81    Cargo.toml
100644 blob f5892efc3b9bac4beeb60e554e85f32e8692599e    LICENSE
100644 blob c9f8bd517d5e8da7b178cf15804ea46ddb9e35c9    README.md
040000 tree 6391f865df6b772571669f0193cfa84eb4a0ad40    b3sum
040000 tree 278f91232f481d861dfda22ca6825a39bc3dd41c    benches
100644 blob 38fc722db514b11c3febff8c9aad887b6b62c86d    build.rs
040000 tree aa2ca2f0ab338bdf2effa1ad2333f09f70025a94    c
040000 tree d0d0767a68d9cc36a2f27c390a07b974216e8ddc    media
040000 tree e6826f60f7ad5377a8d05313db87c743174757af    reference_impl
040000 tree 9118932371d77760db085bfc7341b41128486eb1    src
040000 tree 13786a8e749207634046e73490725bcfc2714ca9    test_vectors
040000 tree ade669b07438519f8133e02a575ffac3cf588250    tools
```

在 git 中 blob 存储的就是文件本身，其格式为 `type SPACE OCT_LENGTH NUL $FILE_CONTENT`，存储在磁盘的内容是使用 Deflate 压缩的，文件名在压缩前由 SHA1 计算文件头和文件内容得来，也就是说使用 Deflate 解压缩后，使用 SHA1 就可以获得与之匹配的 Blob ID，我们可以通过 `git cat-file` 查看文件内容

```shell
git cat-file -p fa8d85ac52f19959d6fc9942c265708b4b3c2b04
```

```txt
Cargo.lock
target
```

git 的内里还是十分简单的，无论分支名怎样变，内部的组织结构并没有显著差别，是一种简单的轻量的分支形式，因此，我们在选择 Git Workflow 时，或许应该解放思想，实事求是，遵循自己的业务模型。

像操作系统，比如 Windows，Linux 内核，FreeBSD，以及 LLVM，GCC 这样的大型软件，需要定期发布新版本，然后给新版本执行一个生命周期，开发流程类似于 `Trunk-Release-Tag` 在 Git 语义下，我们认为是 `Master-Release-Tag`，即在主干分支上不断演进，定期从主干创建特定的 Release 分支，然后 Release 不添加新的功能，只修复 BUG，按照发布路线图和实际测试情况发布新的 Release (也就是 Tag)。一般的大型软件均会采取这种演进模型。

对于 Web 服务而言，类似 Gitflow 的模型确实是不错的选择，快速演进能够及时修复问题和增加新的功能，但如果服务不仅仅提供给公有云用户，那么还是需要考虑到版本化。

## 常见的 Git 的错误用法


## Git 的重新思考

### 对象存储的重新思考

### 哈希算法的选择

自从 SHA1 被破解以来，Git 选择新的哈希算法便提上了日程，经过多次讨论，Git 开发者最终选择了不是最佳的 SHA256。

### 高可用

### 流水线
