---
layout: post
title:  "Git 基于名称空间的存储库快照备份方案"
date:   2018-11-18 10:00:00
published: false
categories: git
---
# 前言

[Git](https://en.wikipedia.org/wiki/Git) 是一种分布式的版本控制系统，分布式版本控制系统的一大特性就是远程存储库和本地存储库都包含存储库的完整数据。这一特性意味着通常来说用户的代码资产要比 svn 之类的集中式版本控制系统更加安全。而集中式的版本控制系统仅仅只有远程服务器包含存储库完整的数据，本地只是特定版本的 `checkout`。

当 `Git` 被越来越多的人使用后，Git 所面临的困难也与日俱增。为了加快存储库的获取，Git 势必增加一些措施减少存储库获取的时间，比如浅表克隆，单分支克隆等等，更进一步的有 `git-vfs` `git partial clone` 等（`git partial clone` 未发布）。当用户使用这些功能时，本地的存储库也就不再是一个完整的存储库了。

Git 给开发者非常大的自由，git 可以修改 commit 重新提交，也可以强制推送引用到远程服务器，覆盖特定的引用，你可能不知道的是，git 服务器并不知道用户是否是强制推送，除非开启了 `receive.denyNonFastForwards=true`，`receive-pack` 通过 commit 回溯扫描检查才能知道用户是否强制推送（这通常会减缓远程服务器的响应速度，并且与一次性推送的 commit 数目密切相关）。

笔者在 [PR: http: add support selecting http version](https://github.com/gitgitgadget/git/pull/69) 中也强推了好几次。


## Gitee 目前备份方案


## 引用快照备份方案


## 总结