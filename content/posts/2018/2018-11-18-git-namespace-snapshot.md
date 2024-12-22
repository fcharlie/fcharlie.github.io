+++
title = "基于 Git Namespace 的存储库快照方案"
date = "2018-11-18T10:00:00+08:00"
categories = "git"
+++

# 前言

[Git](https://en.wikipedia.org/wiki/Git) 是一种分布式的版本控制系统，分布式版本控制系统的一大特性就是远程存储库和本地存储库都包含存储库的完整数据。
而集中式的版本控制系统只有在中心服务器上才会包含存储库完整的数据，本地所谓的存储库只是远程服务器特定版本的 `checkout`。当中心服务器故障后，如果没有备份服务器，那么集中式的版本控制系统存储库的数据绝大部分就会被丢失。这很容易得出分布式版本控制系统的代码要必集中式的版本控制系统更加安全。

但是，安全并不是绝对的，尤其当 `Git` 被越来越多的人使用后，用户也会需要 Git 吸收集中式版本控制系统的特性来改进用户体验，这种情形下，Git 分布式版本控制系统的安全性也就面临挑战。终端用户获取的不是完整的数据，为了保证存储库的安全仍然需要备份或者镜像远程服务器上的存储库。（用户可以使用浅表克隆，单分支克隆或者使用 git vfs(GVFS) 之类的技术加快 git 访问。）

Git 给开发者非常大的自由，git 可以修改 commit 重新提交，也可以强制推送<sup>1</sup>引用到远程服务器，覆盖特定的引用，不合理的使用强制推送是非常危险的，这很容易造成代码丢失，对于企业存储库来说，合理的快照能够代码丢失后减小代码资产的损失。(但这并不是说绝对禁止强制推送<sup>2</sup>)

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

我们可以看到基于 `rsync` 的快照方案是非常占用存储空间的。实际上，此方案不仅占用存储空间在同步的时候还很耗时，按照目前的方案，每一次快照都需要重新完整的获取存储库的所有数据，还是以前面 1GB 存储库为例，12.9个周期内，内网流量消耗为 *~13 GB*。

基于 `rsync` 的快照方案也成了 `Gitee` 企业备份的痛点。在前段时间，我突然想到可以使用基于 `Git Namespace` 快照使用企业快照，这样一来可以大幅度的节省存储空间，于是开始开发实现，也就有了本文。

## Git 引用快照方案

Git 存储库的资产主要是对象和引用，对象实际上是按照哈希存储到存储库中的，在之前的备份方案中，在同一个存储库的不同备份中，存在有大量重复的对象，这些对象占用了存储空间。我们只要在不同的快照中复用这些对象即可。git 存储库中复用对象可以使用 `GIT_NAMESPACE` 隔离模拟成不同的存储库，也可以使用对象借用，借用对象可能使 `Bitmap` 的优化失败，并且多次快照可能会形成借用目录的链式依赖带来问题，因此在技术选型上也就选择了基于 GIT_NAMESPACE 来实现快照。

使用引用快照方案能够节省大量的存储，下面有两个快照间隔的对比：

90 天周期内，7天一次快照，存储库取平均值 1GB：

|方案|存储消耗|存储库数据流量消耗|引用数目（平均值 1W）|存储占比|
|---|---|---|---|---|
|rsync|~13 GB|13 GB|1 * 13 W|100%|
|git snapshot|1 GB|1 GB|13 W|~7.8%|
|git snapshot (double) |2 GB|2 GB|13 W * 2|~15.5%|

90 天周期内，每天快照一次，存储库取平均值 1GB：

|方案|存储消耗|存储库数据流量消耗|引用数目（平均值 1W）|存储占比|
|---|---|---|---|---|
|rsync|90 GB|90 GB|1 * 90 W|100%|
|git snapshot|1 GB|1 GB|90 W|~1.1%|
|git snapshot (double) |2 GB|2 GB|90 W * 2|~2.2%|

### Git Snapshot 原理

在快照存储库时，如果存储库不存在，我们先需要将存储库以镜像的方式克隆下来，命令行如下（git clone 支持目录递归创建）：

```shell
git clone git@gitee.com/oscstudio/git.git --mirror --bare /home/git/enbk/os/oscstudio/git.git
```

如果存储库存在，则是用 `fetch`，命令行如下：

```shell
git fetch git@gitee.com/oscstudio/git.git '+refs/heads/*:refs/heads/*' '+refs/tags/*:refs/tags/*' '+refs/fetches/*:refs/fetches/*' '+refs/pull/*:refs/pull/*' '+refs/pull/*:refs/pull/*' '+refs/git-as-svn/*:refs/git-as-svn/*' --prune
```

其中不同前缀的引用实际类型不一样：

|Prefix|Type|
|---|---|
|refs/heads/|常规分支|
|refs/tags|标签|
|refs/fetches/|PR 功能相关|
|refs/pull/|PR 功能相关|
|refs/git-as-svn/|svn 功能相关|

与 `Subversion` 相比，Git 创建分支非常轻量级，而在 Git 中，分支实际上对应的是以 `refs/heads/` 开头的引用。那么引用的创建同样应该是轻量级的。所以我们只需要将 `fetch` 命令中的引用创建基于名称空间的快照即可：

**快照变换**：

```
refname ---> refs/namespaces/yyyy-MM-dd/refname
```

创建 Git 引用可以使用 git 命令行也可以使用 `libgit2`。命令行创建引用如下：

```shell
$ git update-ref refs/namespaces/2018-11-18/refs/heads/master 1a410efbd13591db07496601ebc7a059dd55cfe9
```

使用 libgit2 API: [git_reference_create](https://libgit2.org/libgit2/#HEAD/group/reference/git_reference_create)  创建引用代码如下：

```c++
git_reference *ref=nullptr;
if(git_reference_create(&ref,repo,"refs/namespaces/2018-11-18/refs/heads/master",id,1,"cretae")!=0){
    auto err=giterr_last();
    fprintf(stderr,"create error %s\n",err->kmessage);
    return false;
}
```

当引用数目比较少时无论是使用命令行还是使用 API 都是不错的选择，但像 Gitee 的源码存储库 Gitlab 这样的项目引用超过 **1W** 个时，无论是 API 还是命令行都将变得无比缓慢。这很容易理解，创建一次快照需要创建同等数目的引用，libgit2 创建引用需要在磁盘上创建同等数目的松散引用，在创建引用前需要创建 refname.lock 这样的文件避免与其他 git 进程或者 API 发生访问冲突。这样下来创建文件的系统调用也就是难以接受的。而 `git` 命令行在这点上也是无能为力的。最开始我直接使用 `libgit2` API，当创建到第十次快照时，一次快照需要花费几分钟，这肯定时不能接受的。

专门的企业快照机器实际上并不需要运行其他业务，也就不用担心创建 Git 引用的安全问题，这时候做个取舍就可以实现优化，比如直接创建引用，引用存储在磁盘上分为 `loose references` 和 `packed references`。如果还是创建松散引用势必需要创建数量巨多的文件，这样一来优化非常优先，因此我们应该直接编写 `packed-refs`，`packed-refs` 格式即 `$COMMITID SP $REFNAME LF`，非常简单。我们按照要求格式化输出即可。

```packed-refs
# pack-refs with: peeled fully-peeled sorted
$COMMITID $REFNAME\n
```


创建 `packed-refs` 之前先使用 `git pack-refs --all --prune` 命令将原有的引用打包到一起，然后扫描旧的 `packed-refs` 写入到新的 `packed-refs` ，成功后替换文件即可。

这里需要注意几点：

1.   packed-refs 引用名需要按照字母排序，否则引用会找不到。
2.   使用内存映射技术减少拷贝非常必要。
3.   可以使用 C++17 std::string_view 这样的容器方便解析。存储在容器中也可以使用 `string_view`。
4.   HEAD 也需要快照，我们的做法时将其放在 `.git/_enbk/$NS` 路径下。
5.   为了保证引用被删除时感知不到，可以先从远程服务器运行 `ls-remote` 获得引用列表，写入快照时删除不存在的即可。为了支持检测，可以使用 `absl::flat_hash_set` 之类的无序支持异构查找容器。

经测试，快照的时间减少幅度非常大，但引用数量达到 `24W` 时，快照时间为 300 多毫秒（机械硬盘)。


```txt
./bin/git-snapshot bk git@localhost:oschina/gitlab.git -n 2018-11-30 -V
username: (none)
use privatekey: /home/example/.ssh/id_rsa
url: git@localhost:oschina/gitlab.git
remote git@localhost:oschina/gitlab.git has 10721 refs
克隆到纯仓库 '/home/git/repositories/snapshot/os/oschina/gitlab.git'...
remote: 枚举对象: 497921, 完成.
remote: 对象计数中: 100% (497921/497921), 完成.
remote: 压缩对象中: 100% (99609/99609), 完成.
remote: 总共 497921 （差异 393964），复用 497921 （差异 393964）
接收对象中: 100% (497921/497921), 211.43 MiB | 51.58 MiB/s, 完成.
处理 delta 中: 100% (393964/393964), 完成.
snapshot write 21442 refs
apply /home/git/repositories/snapshot/os/oschina/gitlab.git HEAD to refs/heads/master
ls-remote time: 88 ms
prepare time:   12684 ms
snapshot time:  17 ms
total time:     12789 ms
```

### 存储库从快照恢复

恢复存储库的时候使用名称空间隔离，然后 clone 到特定的目录即可。命令如下： 

```shell
git -c protocol.ext.allow=always clone 'ext::git --namespace=yyyy-MM-dd %s /path/bk/dir.git' /path/save/dir.git --mirror 
```


## 总结

在研究此方案时，需要对 Git 的相关技术细节非常清楚，比如，笔者在编写 `packed-refs` 时，最开始未了解到 `packed-refs` 引用是按字母排序的，直接写入导致一些引用异常，写入失败。

## 其他

1. Git 服务器并不知道用户是否是强制推送，除非开启了 `receive.denyNonFastForwards=true`，`receive-pack` 通过 commit 回溯扫描检查才能知道用户是否强制推送（这通常会减缓远程服务器的响应速度，并且与一次性推送的 commit 数目密切相关）。

2. 合理的强推有时候是必要的，比如使用强推删除涉密信息，修改提交内容，合并提交等等， 笔者在给 Git 贡献代码时就多次使用强推。 [PR: http: add support selecting http version](https://github.com/gitgitgadget/git/pull/69) 中也强推了好几次。