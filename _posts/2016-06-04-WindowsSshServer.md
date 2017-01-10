---
layout: post
title:  "Windows 10 原生 SSH 功能探索"
date:   2016-06-04 20:00:00
published: true
categories: windows
---

# Windows 10 原生 SSH 功能探索

在 Windows 10 Insider 14352 中，增加了两个服务 SshBrokerGroup SshProxyGroup，
通过命令

`netstat -aon |findstr "22"`

即可找到 端口 22 绑定的进程也就是 SshProxyGroup 服务进程，而 22 端口是 SSH 服务器监听的端口，
在 Windows 目录 *C:\Windows\System32* 可以发现 几个与 SSH 相关的 dll 和 exe：

+ SshBroker.dll
+ SshProxy.dll
+ SshSession.exe
+ SshSftp.exe

实际上 Windows SSH 服务器名称应该是: **Microsoft SSH Server for Windows**


## Insider

在 Windows 注册表中，对 SSH 服务配置

```txt
HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\SshBroker
HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\SshProxy
HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\SharedAccess\Parameters\FirewallPolicy\FirewallRules
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Ssh\Broker
```

其中在 `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Ssh\Broker\DevToolsUser` auth-method 中支持 key 和密码两种验证方式

UA 标识为 **SSH-2.0-MS_1.100**

### SshSession Insider

SshSession 基于 C++ 开发，大量使用了 C++11 的特性，尤其是 lambda，加密 API 使用的是 Windows 自身的加密 API。

通过调试至少可以发现 SshSession 包含以下源码：

```txt
onecore\net\ssh\sshsession\session.cpp
onecore\net\ssh\channel\channeldirecttcpip.cpp
onecore\net\ssh\transport\transportimpl.cpp
onecore\net\ssh\auths\authsmethodpassword.cpp
onecore\net\ssh\authpublickey\authpublickeybase.cpp
onecore\net\ssh\kex\kexdiffiehellman.cpp
onecore\net\ssh\kex\kexgssapi.cpp
onecore\net\ssh\kex\hexecdh.cpp
onecore\net\ssh\kex\kexbase.cpp
onecore\net\ssh\kex\kexdhbase.cpp
onecore\net\ssh\kexhostkey\kexhostkeybase.cpp
onecore\net\ssh\mac\macbcrypt.cpp
onecore\net\ssh\cipher\cipherbcrypt.cpp
onecore\net\ssh\compression\compressionzlib.cpp
onecore\net\ssh\random\randomdefault.cpp
onecore\net\ssh\core\buffer.cpp
onecore\net\ssh\core\dispatcherport.cpp
onecore\net\ssh\core\timerimpl.cpp
```

## 如何体验

开启 SshProxy 和 SshBroker 服务，打开 MSYS2 或者 git for Windows 或者是 Bash on Windows (Bash on Windows 需指定账户名)

>ssh localhost 

输入当前账户密码即可登录。

## 关于服务

在 Windows Insider 14361 中，SSH Proxy ，SSH Broker 服务修改为非默认启动。

## Windows 上 SSH  解决方案

在 Windows 上，SSH 客户端（服务器）解决方案有很多，包括

+ putty
+ secureCRT
+ MobaXterm
+ KiTTY
+ mRemoteNG
+ Xshell 4
+ Bitvise SSH Client
+ Token2Shell
+ Cygwin runtime based OpenSSH

Windows 10 UWP 应用的 SSH 客户端其中有免费方案 SSH Remote，
收费方案：[Token2Shell/MD](http://www.microsoft.com/zh-cn/store/apps/token2shell-md/9nblggh2ncx9) （Token2Shell 是桌面版）
Token2Shell/MD 对于 DPI 缩放支持较好。

![Token2Shell/MD](https://store-images.s-microsoft.com/image/apps.23934.13510798885708666.95e390d3-4778-4e6b-8dfa-1bd63d467671.e8096d29-4323-464a-a2fe-65a70d4d847c?w=580&h=326&q=60&mode=letterbox&background=black)

 无论是 MSYS，MSYS2，Cygwin 中的 SSH 都是使用的 OpenSSH 方案，当然三者都是基于 cygwin 的兼容层实现的。

## SSH 库

SSH 相应的库有多种实现，这里选择几个比较流行的实现：

| Name | License | Language | Features  | Who uses | WebSite |
|--------|-------------|-----------|-------------|--------------------------|
| libssh | LGPL  | C C++  | Client Serer, Linux, Windows, Unix,BSD | Github | https://www.libssh.org/ |
| libssh2 | MIT | C | Client , Linux, Windows, Unix,BSD | libgit2 | https://www.libssh2.org/ |
| ssh.net | Unknown | C# | Client , Windows | Posh-SSH |http://sshnet.codeplex.com |

## 关于 Win32-OpenSSH

Windows PowerShell 团队在 Github 上，fork 创建了 Windows 平台原生的 OpenSSH [Win32-OpenSSH](github.com/PowerShell/Win32-OpenSSH)，
在 Win32-OpenSSH 中，PowerShell 团队使用 IOCP， Win32 加密 API 来替代 OpenSSH 加密和网络方面的实现。这样一来，OpenSSH 在 Windows
上可以获得更优异的性能。

## Update

14383 not found SSH Server Broker and SSH Server Proxy

14385 Found 

14388 Not Found

14390 Not Found

14393 Red Stone Found