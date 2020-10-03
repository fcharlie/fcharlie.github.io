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

## 解决平台的扩展性

## 实现平台的高可用

### 基于状态流转的读写分离

### 依赖多写的高可用

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

## 思考
