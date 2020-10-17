---
layout: post
title:  "性能，可扩展性和高可用 - 大型 Git 代码托管平台的关键问题"
date:   2020-10-31 20:00:00 +0800
published: false
categories: git
---

## 前情提要

在 2019 年我曾写过一篇 Git 代码托管平台相关的文章 - [《探讨 Git 代码托管平台的若干问题 - 2019 版》](https://forcemz.net/git/2019/10/01/ExploreSomeIssuesWithGitHost/)，从开始撰写技术博客以来，写过很多的 Git 相关的技术文章，但这一篇是对不同的 Git 代码托管平台做了个简单对比，然后介绍了 Git 托管平台相应的功能，涉及到 Git 代码托管平台的关键问题没有详细分析，随着认识的不断深入，撰写本文的时机已经成熟，在本文中，我将分析大型 Git 代码托管平台的关键问题，并尝试设计这些问题的解决方案。

## 关键问题的定义

对于分布式系统而言，**可扩展性**（也可称之为 **伸缩性**），**高可用**，**性能** 是制约平台发展的关键性问题，也就是说只有在解决这些问题后才能够支撑更大的用户规模，服务更多的用户，创造更多的社会价值，对于代码托管平台而言，这也是一个普适的道理。

系统具备**可扩展性**则意味着系统可以横向扩展，能够支撑更大的用户规模，存储更多的数据。  
系统支持**高可用**则意味着用户几乎能够随时访问系统，系统出现软硬件故障时依然能够继续给用户提供完整的或者有限的服务。  
提升系统的**性能**则有助于加快数据处理，提高用户的访问速度，改善用户操作体验。

## Git 的关键性能

Git 代码托管平台与常规的 Web 服务有着很大的不同，这也是我经常给一些客户，开发人员传递的信息。Git 代码托管平台与一般的 Web 服务不同的是，Git 需要非常多的计算资源和 IO 读写，一般而言其他的 Web 服务或许只有其中一项或者两项有较高的需求。另一方面，程序员善于利用各种软件自动化工具，Git 代码托管平台除了需要为自然用户提供服务之外还需要为这些*机器人*提供服务。如何在这么多的请求背景下改善用户体验，解决性能问题也成了重要问题。

要分析 Git 的性能问题，我们应当从 Git 的原理着手，首先我们知道 Git 通过快照的方式将用户的源代码，文本，图片等等文件纳入到版本控制当中，使用 deflate 压缩存储，用户需要读取文件时从磁盘中读取后通过 inflate（deflate）解压缩然后返回给用户读取（这里我们应该知道 git 使用标准的 [zlib](https://github.com/madler/zlib) 提供的 deflate 算法）。在这个前提下，我们可以确定一台配置确定的机器，解压文件的速度是有上限的， [zstd](https://github.com/facebook/zstd/) 项目做出过一个对比，不考虑到磁盘 IO，Core i9-9900K CPU @ 5.0GHz 提供的数据是：压缩 90 MB/s 解压	400 MB/s。当然如果磁盘的 IO 达不到这个速度，那么这一数据会更小。这一点会不会会不会影响代码托管平台？当然是会的，比如我们在页面上查看存储库的目录，文件，下载 raw，这就需要从存储库中读取压缩后的对象，然后解压，解压后通过网络返回给用户。另外，用户如果在线提交代码，压缩过程通常也会在服务器上发生，这必然会占用服务器较高的资源。这一问题如何优化？对于 raw 下载功能，我们可以引入缓存文件服务器，我们知道文件存在一个唯一的 SHA1 值，我们为每个项目创建一个缓存目录，在缓存目录中，通过文件的 SHA1 值即可获得已经解压的文件，很多一部分请求省去了解压，服务器的压力自然就小了。另外，我们还可以利用 SIMD 指令优化文件的压缩解压过程，而标准的 zlib 并未做到这一点，在 [dotnet](https://github.com/dotnet/runtime/tree/master/src/libraries/Native/Windows/clrcompression) 中就有专门针对 [Intel CPU SIMD 指令集优化版本](https://github.com/dotnet/runtime/tree/master/src/libraries/Native/Windows/clrcompression/zlib-intel)。另外还有 zlib 衍生项目 [zlib-ng](https://github.com/zlib-ng/zlib-ng) 也针对不同的 CPU 利用 SIMD 指令集优化，实际上这只需要我们重新构建 git 二进制即可。

Git 代码托管平台需要提供的核心能力至少包括推送和拉取，拉取代码包括 `git fetch` 和 `git clone`，我们只讨论简单的 Smart 协议，不讨论 Wire（v2） 协议，以 git fetch（HTTP） 为例：

1.   客户端请求 `GET /path/repo.git/info/refs?service=git-upload-pack`
2.   服务端返回引用列表
3.   客户端按引用发送已存在的、需要的 commit 信息
4.   服务端按照所需、已存在的 commit 清点对象，打包返回给客户端

我们逐一分析，在这个过程中会存在哪些性能问题，当客户端需要服务端的引用列表时，服务端上的 git 要将存储库中的引用及其 commit 一一返回给客户端，我们知道，git 的引用存储在存储库的 `refs`目录下按文件存储，每一个引用对应一个文件，也可以打包存储到 `packed-refs` 中，如果存储库引用较少，分支较少，那么这通常不是问题，但是，如果存在一个像 Windows/Chrome 这样的项目，几万个分支，如果这些分支都未打包到 packed-refs，在服务发现的过程我们就会发现，[open()](https://linux.die.net/man/3/open) 系统调用就有几万个，几十上百个请求过来都是几百万的系统调用了，这还能不影响性能吗？解决这个问题的方法 git 早已经提供了，**packed-refs**，将引用打包存储到一个文件中，作为代码托管平台而言该怎么做呢？定期 GC，使引用被打包。 

Git 实际上还面临着其他的性能问题，Git 是一个分布式的版本控制系统，这意味着我们需要获取存储库时要将整个存储库下载到本地，

<!-- 提高 Git 代码托管平台的性能途径不是唯一的，大多可以通过降低 IO 读写，缩短 CPU 时间，优化内存使用等方面实现。

为了缩短 CPU 执行时间，我们可以优化 Git 的压缩解压过程。目前 Git 使用的是标准的 [zlib](https://github.com/madler/zlib)，如果熟悉 zlib 的开发者可以知道 zlib 是一个较为中庸的实现，并未针对 CPU 架构进行深度优化，在 [dotnet](https://github.com/dotnet/runtime/tree/master/src/libraries/Native/Windows/clrcompression) 中就有专门针对 [Intel CPU SIMD 指令集优化版本](https://github.com/dotnet/runtime/tree/master/src/libraries/Native/Windows/clrcompression/zlib-intel)。另外还有 zlib 衍生项目 [zlib-ng](https://github.com/zlib-ng/zlib-ng) 也针对不同的 CPU 利用 SIMD 指令集优化。如果我们在构建 git 的时候能够使用到这些衍生版本，那么 Git 在执行压缩解压计算时，很大的程度上能够缩短 CPU 计算时长，从而达到提高性能的目的。 -->



## 解决平台的扩展性

## 实现平台的高可用

大型的 Git 代码托管平台的一大挑战是实现平台的高可用，对于公有云而言，服务的高可用是平台的重要质量指标，服务稳定运行，故障较少，这也是用户优先选择的因素之一，而私有云客户在选择产品的时候同样会优先选择服务更稳定，拥有较好的高可用解决方案的厂商。

作为一个 Git 代码托管的资深业内人士，如果要选择一个职业上比较有成就感的事情，现在我可能会说是为前东家的 Gitee 设计和实现的 **Git 代码托管平台读写分离架构**。这一方案是目前比较经济划算的商用方案，据了解 Gitee 为了支持华为鸿蒙操作系统开源，专门将相应的存储库存储到读写分离机器组中，另外还有国内某大型国有银行内部使用了该方案实现其代码托管的高可用。

随着认识的不断深入，我对实现 Git 代码托管平台高可用的认知也不断加深，架构设计也有了新的想法，反思以往总总，放眼未来，旧的设计如下，新的模型如下：

### 基于事件驱动型读写分离

设计 Git 代码托管平台读写分离重要的几点是保证写入的唯一性，实现存储库一致性检测机制。核心原则是事件驱动，产生什么事件，执行匹配的操作。

### 混合型分散读聚合写系统

基于事件驱动型读写分离的核心是要求写入的唯一性，但这对存储库原子写入的要求较高，虽然 Git 具备一定的容错能力。

```patch
diff --git a/builtin/receive-pack.c b/builtin/receive-pack.c
index bb9909c52e..bca3843f00 100644
--- a/builtin/receive-pack.c
+++ b/builtin/receive-pack.c
@@ -60,6 +60,7 @@ static int report_status;
 static int report_status_v2;
 static int use_sideband;
 static int use_atomic;
+static int use_balanced_atomic;
 static int use_push_options;
 static int quiet;
 static int prefer_ofs_delta = 1;
@@ -226,6 +227,11 @@ static int receive_pack_config(const char *var, const char *value, void *cb)
 		return 0;
 	}
 
+	if (strcmp(var, "receive.balancedatomic") == 0) {
+		use_balanced_atomic = git_config_bool(var, value);
+		return 0;
+	}
+
 	if (strcmp(var, "receive.advertisepushoptions") == 0) {
 		advertise_push_options = git_config_bool(var, value);
 		return 0;
@@ -1844,6 +1850,8 @@ static void execute_commands_atomic(struct command *commands,
 			goto failure;
 	}
 
+	// TODO: check balance-update hook
+
 	if (ref_transaction_commit(transaction, &err)) {
 		rp_error("%s", err.buf);
 		reported_error = "atomic transaction failed";
@@ -1951,7 +1959,7 @@ static void execute_commands(struct command *commands,
 			    (cmd->run_proc_receive || use_atomic))
 				cmd->error_string = "fail to run proc-receive hook";
 
-	if (use_atomic)
+	if (use_atomic || use_balanced_atomic)
 		execute_commands_atomic(commands, si);
 	else
 		execute_commands_non_atomic(commands, si);


```

## 用户代码的高可靠


## 研发困境

解决 Git 代码托管平台的关键问题通常有很多途径，但实际上，我们往往做不到那么好，比如公司在相关架构上投入的研发力量较小，并且过于追求进度而不考虑架构的长期演进，又或者不愿与参与到核心组件的贡献，比如我们在设计 Git 代码托管平台高可用时，缺乏研发力量和团队支持往往难以通过改进 Git 本身来实现，这实际上很难优雅的达到预期目标，相对而言 Github 通过参与 Git 贡献，自己维护分支，其 DGit(Spoke) 要做的更好，国内阿里巴巴实际上做的不错，参与了多个 Git 特性的实现（不包括本文设计的关键问题）。

## 思考
