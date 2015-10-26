---
layout: post
title:  "Subversion 和 GIT"
date:   2015-10-16 21:30:16
categories: developer
---
##前言
在开发软件的过程中，往往是需要多个人参与，版本控制系统的协同工作的重要性不言而喻，除此之外，
版本控制软件对整个开发流程的记录对于缺陷追踪也是非常重要的。版本控制系统也是软件开发的基础设施。

笔者开始接触版本控制系统是大学的时候，最开始安装了 TortoiseSVN ，然而 TortoiseSVN 
仅仅是占据了硬盘空间而没有发挥作用，很多开发者在接触新事物的时候，并不一定会有极大的热情去了解，
有的走了很多弯路后返回到了原地，有的深入以后，觉得其中异常的精彩。当我在 Windows 下编译 LLVM 的时候，
Subversion 开始发挥作用，彼时，几乎所有开源的大型软件都是使用 Subversion 进行托管，当然还有部分 CVS。
GIT 远远没有目前流行。后来参加工作后，就是代码托管的工作，对 Subversion 和 Git 有了一定程度的了解，
逐渐有了自己的思考。

大多数人对版本控制系统的解读都是站在使用者的角度，而本文是基于一个代码托管的开发者立场。

##版本控制系统历史

版本控制系统的历史可以追溯到20世纪70年代，这是一个军方开发的 CCC （变更和配置控制）系统，名字叫做 
[CA Software Change Manager](https://en.wikipedia.org/wiki/CA_Software_Change_Manager)  
随后，版本控制系统开始发展起来。     
CVS 一度曾经是开源软件的第一选择，比如 GNOME、KDE、THE GIMP 和 Wine，
都曾使用过 CVS 来管理。这是一个集中式的版本控制系统，同样是集中式的还有 Subversion, Visual SouceSafe 
Perforce,Team Foundation Server。    
由于难以忍受 CVS,CollabNet 的开发者开发了著名的 Subversion(SVN) 来取代 CVS, Subversion 诞生于 2000 年，
时至今日，SVN 依然是最流行的集中式版本控制系统，GCC ,LLVM 等开源软件都使用 SVN 管理，代码托管网站方面，
SourceForge 提供 SVN 的代码托管。

Visual SouceSafe（VSS）是微软开发的版本控制系统，到了 2008年，被 Team Foundation Server（TFS） 取代,
TFS 并不是传统意义的版本控制系统，而是云开发协作平台，支持 Team Foundation Version Control 和 Git,
像微软这样的企业，无论是 Windows 还是 Office 还是 其他软件，代码量都非常巨大，只有像 TFS 这样量身定做的系统才合适。

Perforce 是一个商业的版本控制系统，在其官网 [www.perfoce.com](https://www.perforce.com/) 介绍，
有着超过10000个用户使用他们的服务，有 NVIDIA ,Sumsuing,vmware,adidas 等著名企业，而我对他的印象在是 
OpenWATCOM C/C++ 编译器以及 p4merge 工具。p4merge 是 Perforce 提供的一个基于 Qt 开发的跨平台比较工具。

与集中式版本控制系统对应的是分布式版本控制系统 (Distribution Version Control System) 比较流行的有 git 和 Mercurial,
二者均诞生于 2005 年。

Git 由 Linux 之父， Linus Torvalds 为了替代 BitKeeper 而开发的，关于 Git 的诞生，还有一段趣闻 
[ 10 Years of Git: An Interview with Git Creator Linus Torvalds ](http://www.linux.com/news/featured-blogs/185-jennifer-cloer/821541-10-years-of-git-an-interview-with-git-creator-linus-torvalds/) 
Git 非常流行， Linux, FreeBSD, .NET Core CLR, .NET Core Fx, Minix, Android 等项目都使用 Git 来管理，
Git 的社区非常成熟，有很多代码托管网站提供托管服务，如 Github, Bitbucket, 国内有 OSC@GIT，coding，gitcafe, CSDN code, jd code 等等。

技术上同样优秀的版本控制系统 Mercurial 的使用者少很多，也有著名的浏览器 Mozilla Firefox, 著名服务器 Nginx,以及编程语言 Python。
Mercurial 使用 Python 实现，或许这一点也限制了 Mercurial 的发展。 

在维基百科中有一个 VCS 列表：
 [Template:Version control software](https://en.wikipedia.org/wiki/Template:Version_control_software) 
记录了多种版本控制系统，诞生时间，分类。


大多数时候，开发者需要学习的版本控制系统为 Subversion 或者是 GIT。这二者已然是两个版本控制流派的代表。  


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
