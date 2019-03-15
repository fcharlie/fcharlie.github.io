---
layout: post
title:  "Subversion Ssh 简述"
date:   2018-06-23 10:00:00
published: true
categories: svn
---

# 简述

实现 Subversion 的 SSH 协议非常简单，即在让 ssh 服务器支持 svnserve 命令即可，但对于 git-as-svn 这样的设施怎么做？可以将 git-as-svn 包装成 svnserve 命令，实际上并不是非常高效，如果托管平台使用自己编写的 ssh 服务器，就可以将流量代理到 git-as-svn ,git-as-svn 只需要在 URL 和验证流程上做一些小的努力即可。
