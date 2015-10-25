---
layout: post
title:  "Subversion 和 GIT"
date:   2015-10-16 21:30:16
categories: developer
---
##前言 
大多数时候，开发者需要学习的版本控制系统为 Subversion 或者是 GIT。
这二者已然是两个版本控制流派的代表。在开发软件的过程中，往往是需要多个人参与，
版本控制系统的协同工作的重要性不言而喻，除此之外，版本控制软件对整个开发流程的
记录对于缺陷追踪也是非常重要的。版本控制系统也是软件开发的基础设施。

这里该指出，版本控制系统是软件，而代码托管是服务.
代码托管本质是提供代码存储，并基于代码存储提供附加服务的在线服务。   
Git:   
<pre>将存储库做一个 Copy 放到远程服务器上，并且，设置为本地存储库的远程。</pre>

Subversion:   
<pre>将中心存储库放置到远程服务器上，本地作为一个工作检出。</pre>

笔者开始接触版本控制系统是大学的时候，最开始安装了 TortoiseSVN ，然而 TortoiseSVN 
仅仅是占据了硬盘空间而没有发挥作用，很多开发者在接触新事物的时候，并不一定会有极大的热情去了解，
有的走了很多弯路后返回到了原地，有的深入以后，觉得其中异常的精彩。当我在 Windows 下编译 LLVM 的时候，
Subversion 开始发挥作用，彼时，几乎所有开源的大型软件都是使用 Subversion 进行托管，当然还有部分CVS。
GIT 远远没有目前流行。后来参加工作后，就是代码托管的工作，对 Subversion 和 Git 有了一定程度的了解，
逐渐有了自己的思考。


##Git 技术内幕
Git 的受欢迎程度已经高于 Subversion,人们对 Git 的探索也就更进一步。有诸如 《Pro Git》这样的大作。
而 Subversion 的文档少很多，国际化支持并不好。相对来说研究 Git 的实现并不难，而 Subversion 协议比较复杂，    
研究的人也少了。   

Git 支持多种协议 http, git , ssh, file ,以内部机制区分为哑协议和智能协议，哑协议非常简单，简单的说，
客户端通过 URL 直接拿取服务端的文件。  
Git 智能协议实现了两类 RPC 调用，一个是 fetch-pack<->upload-pack, 另一个是 send-pack<->receive-pack。

任何 Git 远程操作都需要获得远程仓库的引用列表，与自身的引用列表进行比对
1. Fetch-Upload


2. Send-Receive

基于 HTTP 的智能协议和基于 SSH，Git 协议本质上并无太大的不同，都是通过这两类 RPC 调用，实现本地仓库和远程仓库的数据交换。

##Git 开发演进
开发 Git 的人大多知道 libgit2,这是一个基于 C89 开发的 git 开发库，支持绝大多数 git 特性。

##Subversion 内幕
与  Git 完全不同的是，svn 的仓库存储在远程中央服务器上，开发者检出的代码只是特定版本，特定目录的代码，本地为工作目录。


##Subversion 兼容实现
Github 基于 HTTP 协议的方式实现了对 Subversion 的兼容，而 GIT@OSC 基于 svn 协议方式实现了对 Subversion 的不完全兼容。
