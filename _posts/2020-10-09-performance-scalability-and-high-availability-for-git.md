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

对于分布式系统而言，**可扩展性**（也可称之为 **伸缩性**），**高可用**，**性能** 是老生常谈的关键问题。

## Git 的关键性能

## 解决平台的扩展性

## 实现平台的高可用

### 基于状态流转的读写分离

### 依赖多写的高可用

```patch
diff --git a/builtin/receive-pack.c b/builtin/receive-pack.c
index 439f29d6c7..a5fe76ed33 100644
--- a/builtin/receive-pack.c
+++ b/builtin/receive-pack.c
@@ -53,6 +53,7 @@ static struct strbuf fsck_msg_types = STRBUF_INIT;
 static int receive_unpack_limit = -1;
 static int transfer_unpack_limit = -1;
 static int advertise_atomic_push = 1;
+static int balanced_atomic;
 static int advertise_push_options;
 static int unpack_limit = 100;
 static off_t max_input_size;
@@ -214,6 +215,11 @@ static int receive_pack_config(const char *var, const char *value, void *cb)
 		return 0;
 	}
 
+	if (strcmp(var,"receive.balancedatomic") == 0) {
+		balanced_atomic = git_config_bool(var, value);
+		return 0;
+	}
+
 	if (strcmp(var, "receive.advertisepushoptions") == 0) {
 		advertise_push_options = git_config_bool(var, value);
 		return 0;
@@ -1463,7 +1469,11 @@ static void execute_commands_atomic(struct command *commands,
 		if (cmd->error_string)
 			goto failure;
 	}
-
+	if (balanced_atomic) {
+		/// TODO Add a new hook
+		// balanced_update update hash
+		// rollback
+	}
 	if (ref_transaction_commit(transaction, &err)) {
 		rp_error("%s", err.buf);
 		reported_error = "atomic transaction failed";
@@ -1546,7 +1556,7 @@ static void execute_commands(struct command *commands,
 	free(head_name_to_free);
 	head_name = head_name_to_free = resolve_refdup("HEAD", 0, NULL, NULL);
 
-	if (use_atomic)
+	if (use_atomic || balanced_atomic )
 		execute_commands_atomic(commands, si);
 	else
 		execute_commands_non_atomic(commands, si);

```

## 用户代码的高可靠

## 思考
