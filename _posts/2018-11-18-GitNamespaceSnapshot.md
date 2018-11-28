---
layout: post
title:  "基于 Git Namespace 的存储库快照方案"
date:   2018-11-18 10:00:00
published: false
categories: git
---

# 前言

[Git](https://en.wikipedia.org/wiki/Git) 是一种分布式的版本控制系统，分布式版本控制系统的一大特性就是远程存储库和本地存储库都包含存储库的完整数据。
而集中式的版本控制系统只有在中心服务器上才会包含存储库完整的数据，本地所谓的存储库只是远程服务器特定版本的 `checkout`。当中心服务器故障后，如果没有备份服务器，那么集中式的版本控制系统存储库的数据绝大部分就会被丢失。这很容易得出分布式版本控制系统的代码要必集中式的版本控制系统更加安全。

但是，安全并不是绝对的，尤其当 `Git` 被越来越多的人使用后，用户也会需要 Git 吸收集中式版本控制系统的特性来改进用户体验，这种情形下，Git 分布式版本控制系统的安全性也就面临挑战。终端用户获取的不是完整的数据，为了保证存储库的安全仍然需要备份或者镜像远程服务器上的存储库。（用户可以使用浅表克隆，单分支克隆或者使用 git vfs(GVFS) 之类的技术加快 git 访问。）

Git 给开发者非常大的自由，git 可以修改 commit 重新提交，也可以强制推送引用到远程服务器，覆盖特定的引用，不合理的使用强制推送是非常危险的，这很容易造成代码丢失，对于企业存储库来说，合理的快照能够代码丢失后减小代码资产的损失。

在 Gitee 提供了企业版后，我们也经常接收到用户对于代码资产安全的反馈，为了利用有限的资源提供更加安全的服务，存储库备份与快照方案的设计也就非常重要了。

## 术语及组件

|Name|Feature|
|---|---|
|Git Native Hook|基于 C++ 编写的原生钩子，大文件检查，分支权限，消息及同步队列，[Git 原生钩子的深度优化](https://forcemz.net/git/2017/11/22/GitNativeHookDepthOptimization/)|
|Blaze|基于 Go 编写的备份，GC，存储库删除队列服务|
|Git Diamond|基于 Asio 编写的内部 git 协议传输服务器|
|Git Snapshot|基于 C++ 编写的快照和恢复工具|

## Gitee 目前备份方案

**Gitee** 的企业存储库有两个安全策略保证其安全，分别是 `DGIT` 触发式同步和 `rsync` 定期快照方案。

`DGIT` 触发式同步的组件有：`Git Native Hook` ，`Redis`，`Blaze`，`Git Diamond`。GNK(Git Native Hook) 作为符号链接存在与服务器上存储库目录下的 `hooks` 之中，当用户推送代码后，GNK 会被触发，在执行完授权和大文件检测后，将同步事件写入到 `Redis` 队列被 `Blaze` 读取，`Blaze` 获取到当前服务器的 `Slave` 机器，将当前存储库使用 `git push --mirror` 的方式同步到 `Slave` 上，传输协议为 `git://`，`Slave` 机器上的接收端为 `Git Diamond`，`Git Diamond` 是专门的 Git 内部传输服务端。


`rsync` 快照方案字面意思非常容易理解，即定期运行 `rsync` 将企业存储库快照到企业备份服务器上的特定目录。而本文的优化主要也就是是取代 `rsync` 的快照方案。

举一个简单的例子： 使用 `rsync` 方式备份以一个平均大小为 `1GB` 的存储库来计算，90天一个周期，每 7 天备份一次，需要耗费的存储空间为 `12.9 GB`。按照尊享版 100GB 空间，使用率 `20%` 计算：

```
12.9 GB*20=258 GB
```
一百个尊享版企业就需要 `25.2TB` 存储空间。

我们可以看到基于 `rsync` 的快照方案是非常占用存储空间的。实际上，此方案不仅占用存储空间在同步的时候还很耗时，按照目前的方案，每一次快照都需要重新完整的获取存储库的所有数据，还是以前面 1GB 存储库为例，12.9个周期内，内网流量消耗为 12.9 GB。

基于 `rsync` 的快照方案也成了 `Gitee` 企业备份的痛点。在前段时间，我突然想到可以使用基于 `Git Namespace` 快照使用企业快照，这样一来可以大幅度的节省存储空间，于是开始开发实现，也就有了本文。

## Git 引用快照方案

Git 存储库的资产主要是对象和引用，对象实际上是按照哈希存储到存储库中的，在之前的备份方案中，在同一个存储库的不同备份中，存在有大量重复的对象，这些对象占用了存储空间。我们只要在不同的快照中复用这些对象即可。git 存储库中复用对象可以使用 `GIT_NAMESPACE` 隔离模拟成不同的存储库，也可以使用对象借用，借用对象可能使 `Bitmap` 的优化失败，并且多次快照可能会形成借用目录的链式依赖带来问题，因此在技术选型上也就选择了基于 GIT_NAMESPACE 来实现快照。

### Git Snapshot 原理

在快照存储库时，如果存储库不存在，我们先需要将存储库以镜像的方式克隆下来，命令行如下（git clone 支持目录递归创建）：

```shell
git clone git@gitee.com/oscstudio/git.git --mirror --bare /home/git/enbk/os/oscstudio/git.git
```

如果存储库存在，则是用 `fetch`，命令行如下：

```shell
git fetch git@gitee.com/oscstudio/git.git '+refs/heads/*:refs/heads/*' '+refs/tags/*:refs/tags/*' '+refs/fetches/*:refs/fetches/*' '+refs/pull/*:refs/pull/*' '+refs/pull/*:refs/pull/*' '+refs/git-as-svn/*:refs/git-as-svn/*' --prune
```


## 总结

在研究此方案时，需要对 Git 的相关技术细节非常清楚，比如，笔者在编写 `packed-refs` 时，最开始未了解到 `packed-refs` 引用是按字母排序的，直接写入导致一些引用异常，写入失败。

## 其他

1. Git 服务器并不知道用户是否是强制推送，除非开启了 `receive.denyNonFastForwards=true`，`receive-pack` 通过 commit 回溯扫描检查才能知道用户是否强制推送（这通常会减缓远程服务器的响应速度，并且与一次性推送的 commit 数目密切相关）。

2. 合理的强推有时候是必要的，比如使用强推删除涉密信息，修改提交内容，合并提交等等， 笔者在给 Git 贡献代码时就多次使用强推。 [PR: http: add support selecting http version](https://github.com/gitgitgadget/git/pull/69) 中也强推了好几次。