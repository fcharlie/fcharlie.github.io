---
layout: post
title:  "Subversion 和 GIT 开发者演进之 2015"
date:   2015-10-16 21:30:16
published: true
categories: developer
---
## 前言
在开发软件的过程中，往往是需要多个人参与，版本控制系统的协同工作的重要性不言而喻，除此之外，
版本控制软件对整个开发流程的记录对于缺陷追踪也是非常重要的。版本控制系统也是软件开发的基础设施。

笔者开始接触版本控制系统是大学的时候，最开始安装了 TortoiseSVN ，然而 TortoiseSVN 
仅仅是占据了硬盘空间而没有发挥作用，很多开发者在接触新事物的时候，并不一定会有极大的热情去了解，
有的走了很多弯路后返回到了原地，有的深入以后，觉得其中异常的精彩。当我在 Windows 下编译 LLVM 的时候，
Subversion 开始发挥作用，彼时，几乎所有开源的大型软件都是使用 Subversion 进行托管，当然还有部分 CVS。
GIT 远远没有目前流行。后来参加工作后，就是代码托管的工作，对 Subversion 和 Git 有了一定程度的了解，
逐渐有了自己的思考。

大多数人对版本控制系统的解读都是站在使用者的角度，而本文是基于一个代码托管的开发者立场。

## 版本控制系统见闻
版本控制系统的历史可以追溯到20世纪70年代，这是一个军方开发的 CCC （变更和配置控制）系统，名字叫做 
[CA Software Change Manager](https://en.wikipedia.org/wiki/CA_Software_Change_Manager)  随后，版本控制系统开始发展起来。     

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

Git 由 Linux 之父， Linus Torvalds 为了替代 BitKeeper 而开发的，关于 Git 的诞生，可以看对 Linus 本人的采访：
[ 10 Years of Git: An Interview with Git Creator Linus Torvalds ](http://www.linux.com/news/featured-blogs/185-jennifer-cloer/821541-10-years-of-git-an-interview-with-git-creator-linus-torvalds/) 
Git 非常流行， Linux, FreeBSD, .NET Core CLR, .NET Core Fx, Minix, Android 等项目都使用 Git 来管理，
Git 的社区非常成熟，有很多代码托管网站提供托管服务，如 Github, Bitbucket, 国内有 OSC@GIT，coding，gitcafe, CSDN code, jd code 等等。

技术上同样优秀的版本控制系统 Mercurial 的使用者少很多，也有著名的浏览器 Mozilla Firefox,服务器 Nginx,以及编程语言 Python。
Mercurial 使用 Python 实现，或许这一点也限制了 Mercurial 的发展。 

在维基百科中有一个 VCS 列表：
 [Template:Version control software](https://en.wikipedia.org/wiki/Template:Version_control_software) 
记录了多种版本控制系统，诞生时间，分类。


大多数时候，开发者需要学习的版本控制系统为 Subversion 或者是 GIT。这二者已然是两个版本控制流派的代表。  


## Git 技术内幕
本节主要介绍 Git 的存储和传输    

### Git 存储     
git 仓库在磁盘上可以表现为两种形式，带有工作目录的普通仓库和不带工作目录的裸仓库。   
我们可以创建一个标准仓库：    

>mkdir gitrepo &&cd gitrepo &&git --init &&tree -a

如下
{% highlight sh %}
.
├── .git
│   ├── branches
│   ├── COMMIT_EDITMSG
│   ├── config
│   ├── description
│   ├── HEAD
│   ├── hooks
│   │   ├── applypatch-msg.sample
│   │   ├── commit-msg.sample
│   │   ├── post-update.sample
│   │   ├── pre-applypatch.sample
│   │   ├── pre-commit.sample
│   │   ├── prepare-commit-msg.sample
│   │   ├── pre-push.sample
│   │   ├── pre-rebase.sample
│   │   └── update.sample
│   ├── index
│   ├── info
│   │   └── exclude
│   ├── logs
│   │   ├── HEAD
│   │   └── refs
│   │       └── heads
│   │           └── master
│   ├── objects
│   │   ├── 89
│   │   │   └── 50b8b1af3c4cc712edb5a995c83a53eb03e6be
│   │   ├── d0
│   │   │   └── 2d9281b58703d020c3afe3e2ace204d6d462ae
│   │   ├── e6
│   │   │   └── 9de29bb2d1d6434b8b29ae775ad8c2e48c5391
│   │   ├── info
│   │   └── pack
│   └── refs
│       ├── heads
│       │   └── master
│       └── tags
└── helloworld

{% endhighlight %}

实际上我们创建一个裸仓库会发现和普通仓库的 .git 目录结构是一致的。

>mkdir gitbare.git &&cd gitbare.git &&git init --bare &&tree -a

{% highlight  sh %}
.
├── branches
├── config
├── description
├── HEAD
├── hooks
│   ├── applypatch-msg.sample
│   ├── commit-msg.sample
│   ├── post-update.sample
│   ├── pre-applypatch.sample
│   ├── pre-commit.sample
│   ├── prepare-commit-msg.sample
│   ├── pre-push.sample
│   ├── pre-rebase.sample
│   └── update.sample
├── info
│   └── exclude
├── objects
│   ├── info
│   └── pack
└── refs
    ├── heads
    └── tags

9 directories, 13 files

{% endhighlight %}

当我们创建一个仓库时，默认情况下会创建工作目录，在工作目录下有个 .git 的子目录，这才是存储库的目录。
而我们通常修改代码的目录称之为工作目录。

众所周知，git 是分布式版本控制系统，这就意味着，只要获得了 .git 目录的完整数据，就可以在任意位置恢复成一个带有工作目录的仓库。
而 GIT 克隆一个存储库也仅仅是将 .git/objects 目录下的 object 和 .git/refs (.git/packed-refs|.git/info/refs) 所存储的引用列表传输到本地，并应用。

对于 Subversion 一样的集中式版本控制系统，就相当于 .git 目录被托管在中央服务器上，而本地的 .svn 只是工作目录的元数据。  
二者不同的机制带来的直接差别就是一旦中央服务器宕机，git 可以迅速的迁移到其他服务器，并且数据的丢失的可能性很小，
而 Subversion 服务器就没有这么好的运气了。

每一次提交，git 都会把修改的文件快照，还有更新的目录结构，以及提交信息，打包成一个个 object，这些 object 被loose object,
 所以 git 的 object 可能是 blob tree commit 等。打包的过程会使用 zip 压缩，这种被广泛运用的压缩格式实上压缩率较低，压缩速度也慢，
 但好处有广泛的支持，专利上比较友好。

如果调用 git gc 命令后，git-gc 会将这些 object 打包成 pack 文件，gc 过后保存文件差异，但这样是有代价的，我司的代码托管平台某台机器一日触发了 GC,
然后 CPU 占用突然上升，抢占其他进程资源，直接导致用户体验下降，如果使用的是分布式文件系统，这种现象将更加突出。关于 git gc 更多的内容可以查看 
ProGit ，或者去查阅 git 技术文档。

### Git 传输协议
Git 支持多种协议 http, git , ssh, file ,以内部机制区分为哑协议和智能协议，哑协议非常简单，简单的说，
客户端通过 URL 直接拿取服务端的文件。  
Git 智能协议实现了两类 RPC 调用，一个是 fetch-pack<->upload-pack, 另一个是 send-pack<->receive-pack。

任何 Git 远程操作都需要获得远程仓库的引用列表，与自身的引用列表进行比对      

这里以 HTTP 为例
1. Fetch-Upload      
Step 1:   
Request       
{% highlight sh %}
C: GET $GIT_URL/info/refs?service=git-upload-pack HTTP/1.0
{% endhighlight %}

Response 
{% highlight  sh %}
S: 200 OK
S: Content-Type: application/x-git-upload-pack-advertisement
S: Cache-Control: no-cache
S:
S: 001e# service=git-upload-pack\n
S: 004895dcfa3633004da0049d3d0fa03f80589cbcaf31 refs/heads/maint\0multi_ack\n
S: 0042d049f6c27a2244e12041955e262a404c7faba355 refs/heads/master\n
S: 003c2cb58b79488a98d2721cea644875a8dd0026b115 refs/tags/v1.0\n
S: 003fa3c2e2402b99163d1d59756e5f207ae21cccba4c refs/tags/v1.0^{}\n
{% endhighlight %}


Step 2:    
Request    
{% highlight sh %}
C: POST $GIT_URL/git-upload-pack HTTP/1.0
C: Content-Type: application/x-git-upload-pack-request
C:
C: 0032want 0a53e9ddeaddad63ad106860237bbf53411d11a7\n
C: 0032have 441b40d833fdfa93eb2908e52742248faf0ee993\n
C: 0000
{% endhighlight %}

Response     
{% highlight sh %}
S: 200 OK
S: Content-Type: application/x-git-upload-pack-result
S: Cache-Control: no-cache
S:
S: ....ACK %s, continue
S: ....NAK
{% endhighlight %}

2. Send-Receive
实际上 push 的过程也是 GET 和 POST， 只不过，git-upload-pack 要变成 git-receive-pack ，POST 时，后者请求体中包含有 差异 package。 

对于 git HTTP 来说，权限验证通常是 HTTP 的一套，也就是 WWW-Authenticate， 绝大多数的 HTTP 服务器也就支持 Basic。    
即：    

>user:password ->Base64 encode -->dXNlcjpwYXNzd29yZA==

所以从安全上来说，如果使用 HTTP 而不是 HTTPS ， 对 GIT 远程仓库进行写操作简直就是在裸奔。    

git HTTP 支持的 HTTP 返回码并不多， 200 30x 304 403 404 410  

关于 HTTP 的更多文档细节可以去这个地址查看：
[HTTP Protocol](https://www.kernel.org/pub/software/scm/git/docs/technical/http-protocol.html) 

基于 HTTP 的智能协议和基于 SSH，Git 协议本质上并无太大的不同，都是通过这两类 RPC 调用，实现本地仓库和远程仓库的数据交换。

HTTP 协议是通过 http smart server 运行 git-xxx-pack，对其输入数据，然后读取 git-xxx-pack 输出。
SSH 则是通过 ssh 服务器在远程机器上运行 git-xxx-pack ，数据传输的过程使用 SSH 加密。
而 GIT 协议 (git://) 协议则是 通过远程服务器 git-daemon 运行 git-xxx-pack 实现数据的交互。通常来说 git:// 无法实现差异化的权限管理，
也就是要么全部只读，全部可写。

>git help daemon


一些更多的技术内幕可以参考 社区大作 《Pro Git》

## Git 代码托管平台的开发演进     
虽然 GIT 是分布式版本控制，但是对于代码托管平台来说又是一回事了。传统的代码托管平台服务主要运行在一台计算机上，
额外的存储机器通过例如 NFS 之类的分布式文件系统被挂载到服务所在机器，成为一个存储目录。

这样的坏处显而易见:       
1. 网络流量过于集中，这点很好解释，网络入口都是服务所在的机器，对于 git 这样频繁 IO 读写的服务，NFS 有缓存，也显得力不从心。      
2. 运算过于集中，git-upload-pack git-receive-pack 都运行在服务机器上，无论是将 loose object 打包成 package 还是 解包 package 成 loose object，
都是运算密集型。CPU 飙升也就很常见了。

如果要偷懒实现一个折衷的分布式方案，动态代理算是不错的选择，对于 HTTP 协议来说，重定向和代理都是很常见的，
使用 NGINX 之类的服务器就可以方便的实现代理，然后编写模块实现动态获取路由即可,更进一步使用 upstream 的方式
来复用连接，提高效率。而 GIT 的 HTTP 访问与 托管服务的 web 页面都可以用这种策略实现分布式。

对于 SSH 来说，除了端口转发就是动态反向代理了。当然也可以使用 RPC, 通过编写 RPC 客户端伪装成 git-upload-pack 或
git-receive-pack 实现 SSH 的分布式。

### 基于 RPC 的 GIT 分布式设计    
客户端访问仓库时，路由智能到达 DNS 所记录的机器或者是无差别代理的机器(前端机器)，往往不能到达特定的存储机器，
开发者使用分布式文件系统或者 分布式 RPC 或者代理等多种方案实现 前端到存储的关键一步。这里主要说分布式 RPC 与
GIT smart 的应用。     
分布式 RPC 框架很多，其中著名的有 Apache Thrift ,此项目是 Facebook 开源并贡献给 Apache 基金会的，支持多种语言。   
对于 GIT 操作，只需要实现 4个函数。一下是 Thrift 接口文件的一部分：    
{% highlight cpp%}
service GitSmartService{
	i32 Checksum(1:i32 client);
	string FetchRemoteReferences(1:string repositoryPath);
	binary FetchRemoteDiffPackage(1:string repositoryPath, 2:string clientReferences)
	string PushRemoteRefereces(1:string repositoryPath);
	string PushRemoteDiffPackage(1:string repositoryPath, 2:binary clientPackage);
}
{% endhighlight %}

然后存储服务器通过 pipe 读取存储机器上的 git-upload-pack /git-receive-pack 的输入输出。
在 Linux 上通过管道读取 git upload-pack 的输出：     
{% highlight cpp%}
int FetchRemoteReferencesCli(std::string &result,const std::string &path){
	result.clear();
	int pid,fd[2];
	if(pipe(fd)<0){
		printf("oops\n");
	}
	if((pid=fork())<0){
		printf("fork failed \n");
		return -1;
	}else if(pid==0){
		if(fd[1]!=STDOUT_FILENO){
			if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO){  
				return -1; 
			} 
            close(fd[1]);  
		}
		if(execlp("git","git","upload-pack","--stateless-rpc","--advertise-refs",path.c_str(),NULL)==-1){
			printf("execlp failed \n");
			exit(0);
		}
	}else{
		char buffer[4096]={0};
		close(fd[1]);
		int n=0;
		while((n=read(fd[0],buffer,4096))){
			result.append(buffer,n);
		}
		close(fd[0]);
	}
	return 0;
}
{% endhighlight %}


前端服务器上，编写 模拟 git-upload-pack 或者是 git-receive-pack 的程序。用户通过 ssh 访问远程仓库时执行的 git 工具变成了模拟后的 
git-upload-pack /git-receive-pack, 当使用 HTTP 访问时，可以整合成 RPC 客户端整合直接整合进 HTTP 服务器，比如 NGINX 模块，
或者也可 使用 传统的 Git Smart HTTP 库的方式，总的来说 Thrift 有多种语言支持，Git Smart HTTP 整合 Thrift RPC 并不成问题。    
这个唯一的问题是实现异步比较麻烦，两者都需要实现异步模式，git 仓库可能非常大，一次性克隆传输数据几百 MB 或者上 GB, 
这个时候 4nK 发送非常必要。

### 基于 libgit2 的 smart 协议实现     
GIT 除了 Linus 本人实现，kernel.org 托管的官方版本外，还有 jgit,libgit2 等，git 是一系列命令组成，几乎没有剥离出共享库的能力，
这样的后果导致其他语言使用 git 时，不得不使用管道等进程间通讯的模式与 git 工具交互。而 jgit 使用 Java 实现，基本上没有其他流行语言的绑定能力。  
libgit2 是一个 GIT 的兼容实现，基于 C89 开发，支持绝大多数 git 特性。开发非常活跃，有多种语言绑定，如 C# Ruby 等，
其中 C# 绑定 Libgit2Sharp 被 VisualStudio, Github for Windows 等使用，而 Ruby 绑定 Rugged ，被 Github, GIT@OSC 等代码托管平台使用。

libgit2 并没有合适的 GIT smart 服务器后端实现，多数情况下，libgit2 主要面向的是客户端，由于 git 是分布式的，对于仓库的读写也就客户端
和服务器的行为也是类似的。



## Subversion 内幕
此部分中 **SVN 协议** 指 Apache Subversion 程序 svn（以及兼容的客户端） 与远程服务器上的 Apache Subversion svnserve （以及兼容的服务器） 进程通讯的协议，
即 Subversion protocol，协议默认端口是 3690，基于 TCP, 传输数据使用 ABNF 范式。

在这里指出，与  Git 完全不同的是，svn 的仓库存储在远程中央服务器上，开发者检出的代码只是特定版本，特定目录的代码，本地为工作拷贝。

### Subversion HTTP 协议实现
Subversion HTTP 协议是一种 基于  WebDAV/DeltaV  的协议，WebDAV 在 HTTP 1.1 的基础上扩展了多个 Method, 绝大多数的服务器并不支持 WebDAV,
这样的后果就是，除了 Apache 可以使用 mod_dav_svn 插件，基本上再也没有其他的服务器能快速的支持 Subversion 的 HTTP 协议了。代理还是可以的。

WebDAV 协议在 HTTP 1.1 的基础上 使用 XML 的方式呈现数据，对于 Subversion 这种集中式版本控制系统来说，绝大多数操作都是在线的，
WebDAV 包裹这些操作就变得很繁琐。

比如一个 update-report 请求：   
{% highlight xml %}
  <S:update-report send-all="true" xmlns:S="svn:">
    <S:src-path>http://localhost:8080/repos/test/httpd/support</S:src-path>
    <S:target-revision>2</S:target-revision>
    <S:entry rev="2"  start-empty="true"></S:entry>
  </S:update-report>
{% endhighlight %}

然后服务器返回：     
{% highlight xml %}
<S:update-report xmlns:S="svn:" xmlns:V="..." xmlns:D="DAV:" send-all="true">
  <S:target-revision rev="2"/>
  <S:open-directory rev="2">
    <D:checked-in>
      <D:href>/repos/test/!svn/ver/2/httpd/support</D:href>
    </D:checked-in>
    <S:set-prop name="svn:entry:committed-rev">2</S:set-prop>
    ... more set props ...
    <S:add-file name="ab.c">
      <D:checked-in>
        <D:href>/repos/test/!svn/ver/2/httpd/support/ab.c</D:href>
      </D:checked-in>
      <S:set-prop name="svn:entry:committed-rev">2</S:set-prop>
      ... more set props for the file ...
      <S:txdelta>...base64-encoded file content...</S:txdelta>
    </S:add-file>
    <S:add-directory name="os" bc-url="/repos/test/!svn/bc/2/httpd/os">
      <D:checked-in>
        <D:href>/repos/test/!svn/ver/2/httpd/os</D:href>
      </D:checked-in>
      ...directory contents...
    </S:add-directory>
  </S:open-directory>
</S:update-report>
{% endhighlight %}

不同的请求，xml 的内容也完全不同，Subversion HTTP 协议的复杂也让很多开发者望而却步。 

在 Subversion 的路线图中，基于 WebDAV/DeltaV 的 HTTP 接入将被 基于 HTTP v2 的实现取代。   
[A Streamlined HTTP Protocol for Subversion](http://svn.apache.org/repos/asf/subversion/trunk/notes/http-and-webdav/http-protocol-v2.txt)      

### Subversion SVN 协议实现
与 HTTP 不同的是，一个完整的基于 SVN 协议的连接中，仓库的操作是上下文相关的。   
当客户端的连接过来时，服务器，通常说的 svnservice 将发送一段信息给客户端，告知服务器的能力,这个过程叫握手 Handshake。

{% highlight sh %}
S: ( minver:number maxver:number mechs:list ( cap:word ... ) )
{% endhighlight %}

Example:   
{% highlight sh %}
( success ( 2 2 ( ) ( edit-pipeline svndiff1 absent-entries depth inherited-props log-revprops ) ) ) 
{% endhighlight %}


这个时候客户端获知了这些数据，如果无法兼容，服务器，那么将断开与服务器的连接，否则，将发送请求数据给服务器，格式如下：      
{% highlight sh %}
C: response: ( version:number ( cap:word ... ) url:string
              ? ra-client:string ( ? client:string ) )
{% endhighlight %}

Example:    
{% highlight sh %}
( 2 ( edit-pipeline svndiff1 absent-entries depth mergeinfo log-revprops ) 36:svn://subversion.io/subversion/trunk 53:SVN/1.8.13-SlikSvn-1.8.13-X64 (x64-microsoft-windows) ( ) )
{% endhighlight %}

与 GIT 数据包类似的地方有一点，git 每一行数据前 4 个16进制字符代表本行的长度，而 这里的 10 进制字符代表 字符的长度，比如 URL 长度36，UA 53。

服务器此时的行为就得通过解析 URL 获得中央仓库的位置，判断协议是否兼容，而 UA 有可能为空，格式并不是非常标准，所以这是值得注意的地方。  

服务器将决定使用那种授权方式，MD5 一般是 Subversion 客户端默认的，无法第三方库支持，而 PLAIN 和 ANONYMOUS 需要 SASL 模块的支持，
在 Ubuntu 上编译 svn,先安装 libsasl2-dev。


{% highlight sh %}
S: ( ( mech:word ... ) realm:string )
{% endhighlight %}


客户端不支持此授权方式时，会输出错误信息，“无法协商验证方式”

这里的 Realm 是 subversion 客户端存储用户账户用户名和密码信息的一个 key,只要 realm 一致，就会取相同的 用户名和密码。 
realm [RFC2617](http://www.ietf.org/rfc/rfc2617.txt)

Example:   
{% highlight sh %}
( success ( ( PLAIN ) 36:e967876f-5ea0-4ff2-9c55-ea2d1703221e ) ) 
{% endhighlight %}

如果是 MD5 ，验证协商如下：    
{% highlight sh %}
S: ( mech:word [ token:string ] )
{% endhighlight %}
这个 Token 是随机生成的 UUID, C++ 可以使用 boost 生成，也可以使用平台的 API 生成。   

如果是 PLAIN 授权机制，这里就是用户名和密码经 Base64 编码了, 用 NUL(0) 分隔   

> usernameNULpassword --> Base64 Encoded

Example:   
{% highlight sh %}
( PLAIN ( 44:YWRtaW5Ac3VidmVyc2lvbi5pbyU1QzBwYXNzd29yZA== ) ) 
{% endhighlight %}

对于纯 svn 协议来说，使用 PLAIN 并不安全，且当 Subversion 只作为 GIT 代码托管平台的一个服务来说，
使用 CRAM-MD5 并不利于服务整合，这也是一个缺陷了。


这是服务器的下一步骤：
{% highlight sh %}
S:  challenge: ( step ( token:string ) )
S:           | ( failure ( message:string ) )
S:           | ( success [ token:string ] )
{% endhighlight %}

Incorrect credentials:         
{% highlight sh %}
( failure ( 21:incorrect credentials ) ) 
{% endhighlight %}

Success    
{% highlight sh %}
( success ( ) )
{% endhighlight %}

随后服务器再发送存储库 UUID, capabilities 给客户端
{% highlight sh %}
S: ( uuid:string repos-url:string ( cap:word ... ) )
{% endhighlight %}

Example:   
{% highlight sh %}
( ( 36:0f475597-c342-45b4-88c5-7dc0857b8ba4 36:svn://subversion.io/subversion/trunk ( edit-pipeline svndiff1 absent-entries depth inherited-props log-revprops ))
{% endhighlight %}

如果是 svn up/commit 或者其他的操作，这个时候会检查 uuid 是否匹配，当然也会检查 URL 是否匹配。

如果客户端觉得一切都 OK 啦，那么就会开始下一阶段的操作，command 模式，这些规则可以从 Subversion 官方存储库查看 
[Subversion Protocol](http://svn.apache.org/repos/asf/subversion/trunk/subversion/libsvn_ra_svn/protocol)    

与 GIT 或者 SVN HTTP 不同的是，一个完整的 基于 svn 协议的 SVN 操作，只需要建立一次 socket，Subversion 客户端此时是阻塞的，并且屏蔽了 Ctrl+C 等
信号， 仓库体积巨大时，这种对连接资源的占用非常突出，因为有数据读取， socket 并不会超时。单纯按并发来算，svn 服务器的并发就收到了限制。

### Subversion 兼容实现
Github 基于 HTTP 协议的方式实现了对 Subversion 的兼容，而 GIT@OSC 基于 svn 协议方式实现了对 Subversion 的不完全兼容。

基于 HTTP 协议实现的 Subversion 兼容服务和 基于 SVN 协议的 Subversion 兼容服务二者并不能说谁就一定好，HTTP 协议很容易导致网关超时，
多大数情况下，一次完整的操作时成千上万的 HTTP 请求构成，HTTP 协议支持需要 HTTP 服务器能够支持 WebDAV, XML 解析过程比较麻烦，
Subversion 官方也计划使用 HTTP v2 取代 WebDAV，但 HTTP 协议的好处还是有的，比如很多企业并不一定开放 SVN 端口 3690，
可以和 gitlab 之类的服务整合。

而 SVN 协议也有不好的地方，比如连接时间过长，服务器并发上不去，容易阻塞，与 HTTP 服务整合不便，但同时 SVN 协议能够支持较大存储库。

实际上兼容实现 SVN 接入往往没有原生的 SVN 服务好，这点事毋庸置疑的。

### Subversion 协议代理服务器的实现
前面 SVN 协议，虽然不全，但是那些协议内容足够实现一个 SVN 协议动态代理服务器了。普通代理服务器的实现比较简单，具有路由能力的代理服务器
则有点难度，主要难度体现在需要解析特定协议的请求内容，然后得到用户请求资源所在的服务器。

笔者在 2015 年底实现了一个 SVN 协议动态代理服务器，以下是实现思路  

在客户端 C 和代理服务器 S 建立连接后， S 向 C 发送一个数据包：

+ 服务器头
{% highlight sh %}
#S to C
( success ( 2 2 ( ) ( edit-pipeline svndiff1 absent-entries depth inherited-props log-revprops ) ) )
{% endhighlight %}


C 接收到 S 的数据后，必须做出选择，并发送第一个请求给 S。     
{% highlight sh %}
#C to S
( 2 ( edit-pipeline svndiff1 absent-entries depth mergeinfo log-revprops ) 43:svn://subversion.io/apache/subversion/trunk 53:SVN/1.8.13-SlikSvn-1.8.13-X64 (x64-microsoft-windows) ( ) )
{% endhighlight %}

S 接收到 C 的请求后，解析 数据包，提取到 URL 为 svn://subversion.io/apache/subversion/trunk , 而 Gitlab 的规则是 host/user/repo, 
如果不同用户的存储库放在不同机器上，这个时候提取到用户为 apache, 交由路由选择模块去处理得到后端的地址，也就是真实 svnserve 的 IP 和端口。

建立与后端服务器 B 的连接。这个时候 S 读取 B 的数据包，也就是前面的服务器头,接收完毕直接丢弃即可，然后将客户端 C 的头请求转发给后端服务器。   
{% highlight sh %}
#S to B
( 2 ( edit-pipeline svndiff1 absent-entries depth mergeinfo log-revprops ) 43:svn://subversion.io/apache/subversion/trunk 53:SVN/1.8.13-SlikSvn-1.8.13-X64 (x64-microsoft-windows) ( ) )
{% endhighlight %}

这里值得注意的是 svnkit，Subversion Javahl 并没有添加 UA 字符串，所以解析时略过即可。

至此，代理服务器的后面就不必关系细节了，通常使用 Boost.ASIO 等异步框架，在使用 Boost.ASIO 时，先以阻塞的方式与客户端通信，待到与后端存储服务器
建立连接后才使用异步的方式转发数据包。

{% highlight sh %}
Client <---> Proxy Server <---> Backend Subversion Server
{% endhighlight %}

一个基本的 SVN 协议动态代理服务器就实现了。

## 结尾
如果你不是专业的 Git 或者 Subversion 开发者，你可能会觉得上面的内容没什么用处，实际上也没什么技术难度。
