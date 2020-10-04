---
layout: post
title:  "性能，可扩展性和高可用 - 大型 Git 代码托管平台的关键问题"
date:   2020-10-09 20:00:00 +0800
published: false
categories: git
---

## 前情提要

在 2019 年我曾写过一篇 Git 代码托管平台相关的文章 - [《探讨 Git 代码托管平台的若干问题 - 2019 版》](https://forcemz.net/git/2019/10/01/ExploreSomeIssuesWithGitHost/)，从开始撰写技术博客以来，写过很多的 Git 相关的技术文章，但这一篇是对不同的 Git 代码托管平台做了个简单对比，然后介绍了 Git 托管平台相应的功能，涉及到 Git 代码托管平台的关键问题没有详细分析，随着认识的不断深入，撰写本文的时机已经成熟，在本文中，我将分析大型 Git 代码托管平台的关键问题，并尝试设计这些问题的解决方案。

## 关键问题的定义

对于分布式系统而言，**可扩展性**（也可称之为 **伸缩性**），**高可用**，**性能** 是制约平台发展的关键性问题，也就是说解决这些问题才能够支撑更大的用户规模，对于代码托管平台而言，这也是一个普适的道理。**可扩展性**意味着系统可以横向扩展，能够为更多的用户提供服务，能够存储更多的数据；**高可用**则意味着用户的数据较为安全，系统在软硬件故障时依然能够给用户提供完整的或者有限的服务能力；而**性能**则有助于加快数据处理，改善用户体验。

## Git 的关键性能

Git 代码托管平台与常规的 Web 服务有着很大的不同，Git 对内存，CPU，磁盘 IO 的需求都非常大，一般而言其他的 Web 服务或许只有其中一项或者两项有较高的需求。另一方面，程序员善于利用各种软件自动化工具，Git 代码托管平台除了需要为自然用户提供服务之外还需要为这些*机器人*提供服务。如何在这么多的请求背景下改善用户体验，解决性能问题也就成了吾辈的责任。



## 解决平台的扩展性

## 实现平台的高可用

说一件个人职业上比较有成就感的事情，我可能会选择说《 Git 代码托管平台的读写分离》架构设计和实现，回过头来反思，虽然读写分离的设计存在一些细节不够优化，但在国内行业内算得上比较经济划算性能高的方案，老东家也为华为鸿蒙操作系统存储库部署了相关的环境。国内某大型商业银行也使用了该系统。随着认识的不断加深，我在设计 Git 代码托管平台的高可用时有了新的想法，因此，在本节中，我将按照不同的设计思路试图描述合适的高可用方案。

### 基于事件驱动型读写分离

### 混合型分散读聚合写系统


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
