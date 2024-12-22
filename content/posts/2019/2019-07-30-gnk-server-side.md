+++
title = "服务端 Git 钩子的妙用"
date = "2019-07-30T20:00:00+08:00"
categories = "git"
+++
# 背景
在 [《Pro Git》](https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks) 的 **Server-Side Hooks** 介绍了三种钩子，分别是 `pre-receive` 已经 `update` 还有 `post-receive`。GITEE 最初是基于 Gitlab 发展而来，最开始在服务端使用的是 `update` 钩子。我们使用 `update` 钩子完成一些保护分支/只读分支的功能。随着 Gitee 的不断发展，也有人提出了一些新的服务需求，为了解决大仓库大文件，我们开发了 Git Native Hook (GNK)。新的钩子利用了 git 的一些新的特性，在不同的阶段实现不同的功能，这里就介绍一下这些钩子的妙用。

在阅读本文之前，你可以阅读 [《Pro Git》](https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks) 关于服务端钩子的技术细节，你还可以阅读 [《Git 原生钩子的深度优化》](https://forcemz.net/git/2017/11/22/GitNativeHookDepthOptimization/)以及[《实现 Git 目录权限控制》](https://forcemz.net/git/2019/04/01/ImplementGitDirPermissionControl/)。

## 分支权限管理

人们对于权限管理的要求往往会有一个变迁过程，从最开始的一刀切，到后来的精细化控制，事实告诉我们，如果不提供细粒度的权限控制，那么总会有用户提出需求的。

在实现 GNK 之前，我们使用 gitlab-shell 的某个脚本软链到 hooks 目录作为 update 钩子，每更新一个引用，update 钩子将被调用一次，格式为:

```shell
$GIT_DIR/hooks/update refname oldrev newrev
```
只要以 `refs/heads/` 开头的引用被修改了，并且此引用对应的分支名已经被存储在数据库中作为只读分支或者保护分支，我们可以让钩子返回非零值，让推送失败，这样就可以实现分支权限细粒度管理的功能。然而不利的因素是，当一次性推送多个分支时，update 钩子将被启动多次，update 钩子需要鉴权，鉴权请求也会发送多次，这实际上是不利于系统优化的。

在 GNK 中，我们使用 GNK pre-receive 钩子实现鉴权，pre-receive 在标准输入读取所有被更新的引用，循环的使用 `getline` 即可获得所有修改的分支，这样一来多个分支的验证就可以合并成一个，这可以极大的减少鉴权请求次数。

pre-receive 标准输入格式：

```txt
refname SP oldrev SP newrev LF
...
refnameN SP oldrev SP newrev LF
```

除了保护分支，只读分支，我们还可以在 GNK 中实现禁止用户推送创建新的分支功能，我们知道当 **oldrev** 为 **0000000000000000000000000000000000000000** 时，表示用户创建了新的引用（newrev 与 oldrev 是不能同时为 `zerooid` 的，这是没有意义的）。如果存在 oldrev 为全零且 `refname`  以 `refs/heads/` 开头，我们便可以知道用户创建了新的分支，使用一个 `bool` 类型的 `nbe` 告知 API ，API 授权失败 pre-receive 返回非零值即可禁止用户推送创建分支。

## 存储配额和大文件

在 update 钩子中，检测存储库配额往往不切实际，多次调用 update 钩子可能需要执行多次统计存储库体积，这是不划算的，但不做存储库体积检测，存储库配额限制很容易失效，出现限制逃逸。在 GNK 中，我们使用 pre-receive 进行配额计算，当用户配额超出限制时，允许三次重试机会，如果强制推送后运行强制 gc 后，存储库体积恢复到限制之下，用户即可正常使用存储库。同时如果用户的限制次数还存在，但存储库体积小于配额限制，依然是可以推送的，这通常与 git 自动 gc 有关。

绝大多数版本控制系统都是针对文本优化的，或者是基于文本补丁的，git 是基于文件快照的，这些版本控制系统在管理文本文件，程序源码有着天然的优势。而大文件往往会带来麻烦。

git 使用 [mmap(2)](https://linux.die.net/man/2/mmap) 读写文件，使用 [zlib](https://github.com/madler/zlib) 将原始的文件，tree 结构，commit 文件压缩成 blob, tree, commit。所以一些限制确实存在，第一，32 位程序的内存最大是 4GB，因此，你应该始终使用 64 位系统和 64 位 git，无论是 Windows Linux 还是 macOS。zlib 的压缩算法 `deflate` 历史悠久，压缩解压数度并不快，并且压缩比也不高，大文件带来的后果很明显，推送的过程打包解包的时间急剧增加，用户从远程服务器上拉取数据的时间同样如此，如果文件不是 ANSI/UTF-8/UTF-16 这样的文本文件也就是通常意义上的二进制文件，那么 CPU 的占用将更高，耗时也更长。

在 gitlab-shell 中，由于当时的开发者对钩子认识的局限性，使用 `commit between` 机制在 `update` 钩子中遍历比较文件修改，查看文件原始大小，这样带来的后果是大文件限制容易被突破，不同的引用之间可能出现文件重复检查，API 多次调用，悬空对象不断产生。在 GNK 中，我们使用 pre-receive 钩子分析松散对象和 `packet index` 文件，判断压缩后的文件大小来判断文件是否超出限制。这里更多的细节可以阅读 [《Git 原生钩子的深度优化》](https://forcemz.net/git/2017/11/22/GitNativeHookDepthOptimization/)。这种机制的好处是使用环境隔离避免了产生悬空对象从而避免用户存储库配额的误消耗。而分析 `.idx` 文件比解析 commit 然后解析 blob 要高效的多，以 Linux 内核打包体积 `1345.25 MB` 对象 `6361509`，耗时长 `553ms`。而我们很少直接推送超过 1GB 的存储库，就算存储库 1GB，对象也很少超过 600W。同样 GNK pre-receive 也只需要在授权阶段返回配额信息即可，不需要多次请求。

## 存储库数据搜集

在使用 gitlab-shell update 钩子时，存储库体积的更新并不及时，有时候还需要手动点击更新。

在 GNK 中，我们在 pre-receive 计算了存储库体积，这个数据除了与配额做比较之外，还可以被收集起来，这里我们将一些信息序列化插入到队列中，由 blaze 获取处理，更新用户的存储库大小。

存储库大小在数据库中以 MB 为单位，我们的更新采用了模糊策略，体积转换相等后不更新，存储库体积大于 80MB 时，体积变更在 2MB 及以上则更新，其余是 1MB, 存储库体积近似不变不更新。

```go
const (
	KB    int64 = 1024
	MB    int64 = 1024 * 1024
	GB    int64 = 1024 * 1024 * 1024
)
// FastAbs todo
func FastAbs(n int64) int64 {
	y := n >> 63
	return (n ^ y) - y
}

// RecordSize todo
func RecordSize(size, oldsize int64) int64 {
	newsize := size / MB
	// When repo size%MB >720KB ~ size/MB+1
	if size%MB > 720*KB {
		newsize++
	}
	if newsize == oldsize || (oldsize > 80 && FastAbs(newsize-oldsize) < 2) {
		return oldsize
	}
	return newsize
}

```

在 GNK pre-receive 进行存储库大小分析时，我们还可以对存储库松散对象进行计算，当松散对象数目超过一定限制，则可以产生一个任务给 blaze，让其启动一个普通的 `git gc` 进程，将松散对象打包到 pack 文件从而提高存储库的响应速度。

## 只读目录和禁止推送暴露邮箱

在 [《实现 Git 目录权限控制》](https://forcemz.net/git/2019/04/01/ImplementGitDirPermissionControl/) 一文中，我提到过如何实现 git 目录权限控制，通常来说，增加自定义功能应当避免修复 git 源码，而应该在钩子中自定义实现，所以最后的选择是在 update 钩子中实现只读，具体的实现可以阅读 《实现 Git 目录权限控制》。

与只读目录类似，libgit2 在 pre-receive 阶段并不能很好的处理环境隔离机制，因此，我们在 GNK 中选择在 update 阶段在 每一次 update 钩子执行时进行 `revwalk` 检查用户的邮箱是否在提交的 commit 中。为了避免多次请求 API 服务，通过 `basalt--git-srv` `brzox--git-srv` 的推送进行了优化。

## 推送动态

用户推送代码后，Gitee 应该出现推送动态。在 gitlab-shell 中，此功能有 update 钩子实现，多分支时往往需要请求多次，而在 GNK post-receive 中，我们使用 `redis pipeline` 以及分组合并策略，优化了大量分支推送时，多次访问 redis 。

## 备份

post-receive 往往不会影响存储库是否推送成功，主要的目的就是为了告知用户推送成功，此时我们便可以使用 post-receive 实现存储库同步，在 GNK 中，开启 post-receive 同步后，post-receive 发送任务给 blaze，blaze 或者存储库的备份机器，实现备份，目前 gitee 的企业存储库所在的服务器已经使用此方案稳定运行。


## 最后

实现一个好的功能需要对背景有充足的了解，改善 git 服务的体验也要从 git 的特性出发。
