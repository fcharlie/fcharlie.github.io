---
layout: post
title:  "在 Windows 中实现 sudo"
date:   2019-08-07 12:00:00
published: false
categories: windows
---
# 前言

*这篇文章的想法来源于我在 [Windows Terminal Issue#146](https://github.com/microsoft/terminal/issues/146) 的[评论](https://github.com/microsoft/terminal/issues/146#issuecomment-515812461)。*

[sudo](https://linux.die.net/man/8/sudo) 以另一个用户执行命令，通常是 `root`。当普通用户需要以其他权限执行某项工作时，通常需要获得指定用户的权限，以目标权限 `root` 为例，我们期望以 root 权限运行，可以使用 `su` 登录到 `root` 用户，在这种情况下，一直到退出 `root`。都使用的是 `root` 权限，这实际上并不是安全的，处于高级别权限的时间应当尽量的短。而使用 `sudo` 获得 root 权限要安全的多，这种情况下，只有特定的命令才会获得 root 权限，而不是整个用户和 shell. 话又说回来，sudo 是如何获得 root 权限的？在 Windows 中的 sudo 又是怎么一回事，如何在 Windows 中实现类似的 sudo.

## Linux 的 sudo 内幕

Linux 的权限机制大致概括为 `UGO` 和 `RWX`，文件权限为 `R` 读权限，`W` 写权限，`X` 可执行权限。基于 UGO 模型设置。`U` 代表用户， `G` 代表组，`O` 代表其他用户。

了解 Linux sudo 的原理之前，我们需要先了解文件的属性。我们可以通过 `stat` 命令或者 `stat` 系统调用查看文件的属性，这里我们以 [musl: arch/x86_64/bits/stat.h](https://github.com/bminor/musl/blob/master/arch/x86_64/bits/stat.h) 为参考：

```c
#ifndef S_IRUSR
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXU 0700
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IRWXG 0070
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#define S_IRWXO 0007
#endif

struct stat {
	dev_t st_dev;
	ino_t st_ino;
	nlink_t st_nlink;

	mode_t st_mode;
	uid_t st_uid;
	gid_t st_gid;
	unsigned int    __pad0;
	dev_t st_rdev;
	off_t st_size;
	blksize_t st_blksize;
	blkcnt_t st_blocks;

	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	long __unused[3];
};
```
我们使用 `stat` 命令查看 `/usr/bin/sudo` 命令属性：

```txt
  File: /usr/bin/sudo
  Size: 149080          Blocks: 296        IO Block: 4096   regular file
Device: 2h/2d   Inode: 3659174697569688  Links: 1
Access: (4755/-rwsr-xr-x)  Uid: (    0/    root)   Gid: (    0/    root)
Access: 2018-05-12 07:38:43.000000000 +0800
Modify: 2018-01-18 08:08:16.000000000 +0800
Change: 2018-05-12 07:49:34.051952000 +0800
 Birth: -

```

![](https://user-images.githubusercontent.com/6904176/62181312-84b53080-b385-11e9-822d-89833ef8ac3a.png)


这里我们可以看到 sudo 命令的程序文件，所有者 ID 和 组 ID 都是 **0**，即 `root`，与普通文件不同之处 `sudo` 设置了 `S_ISUID` 我们查看 GNU [14.9.5 The Mode Bits for Access Permission](https://www.gnu.org/software/libc/manual/html_node/Permission-Bits.html) 以及 [30.4 How an Application Can Change Persona](https://www.gnu.org/software/libc/manual/html_node/How-Change-Persona.html#How-Change-Persona) 其中有一段 

>If a process has a file ID (user or group), then it can at any time change its effective ID to its real ID and back to its file ID. Programs use this feature to relinquish their special privileges except when they actually need them. This makes it less likely that they can be tricked into doing something inappropriate with their privileges.

sudo 命令启动后可以运行 `setuid(0)` 将自身权限设置为 `root` 然后验证用户凭据，有效时则可以以 root 用户权限运行相应的命令。在 Linux/POSIX 系统中，虽然 `sudo` 的原理并不复杂，但是实现一个 sudo 程序，处理好不同用户的环境变量，权限的有效期，这些都需要一些代码实现，索性有开源的 `sudo` 可供人使用。

## 在 Windows 实现 Sudo

理解 Windows 权限机制需要了解 [ACL](https://en.wikipedia.org/wiki/Access-control_list) 以及 [User Account Control (UAC)](https://en.wikipedia.org/wiki/User_Account_Control)。在 Windows 系统上并不存在原生的 sudo 命令，标准用户的提权在开启 UAC 时，会出现一个提权对话框和安全桌面，那么在 Windows 中，从标准用户到特权用户 (Administrator) 内部是怎样的实现呢？如果我们需要实现一个不需要 UI 交互的 `Windows sudo` 又应该如何实现呢？

### UAC 提权与 sudo 实现探讨

在 [《Privexec 杂谈》](https://forcemz.net/windows/2018/12/01/PrivexecNew/) 一文中，就讲过 UAC 提权，这里再重复一遍。

在 Windows 中，如果要实现 `UAC` 提权，需要调用 `ShellExecute` 以 `runas` 的参数启动新的进程。或者设置引用程序清单（此类由 shell 打开时会提权，也是 ShellExecute 一类），清单示例如下：

```xml
 <trustInfo xmlns="urn:schemas-microsoft-com:asm.v2">
    <security>
      <requestedPrivileges xmlns="urn:schemas-microsoft-com:asm.v3">
        <requestedExecutionLevel level='requireAdministrator' uiAccess='false' />
      </requestedPrivileges>
    </security>
  </trustInfo>
```

以 `Windows 10` 为例，`ShellExecute` 提权最终由 `AicLaunchAdminProcess` 函数实现，此函数目前实现在 `Windows.Storage.dll` 中，UAC 提权需要与 `Appinfo` 服务通信，`Appinfo` 验证提权行为后使用 `CreateProcessAsUserW` 启动进程，并将其父进程设置为 `ShellExecute` 调用者。调用细节(Windows Vista)<sup>1</sup>如下：

1.   AppInfo goes and talks to the Local Security Authority to get the elevated token of the logged in user of Session 1.
2.   AppInfo loads up a STARTUPINFOEX structure (new to Vista), and calls the brand new Vista API InitializeProcThreadAttributeList() with room for one attribute.
3.   OpenProcess() is called to get a handle to the process that initiated the RPC call.
4.   UpdateProcThreadAttribute() is called with `PROC_THREAD_ATTRIBUTE_PARENT_PROCESS`, and uses the handle retrieved in step 3.
5.   CreateProcessAsUser() is called with `EXTENDED_STARTUPINFO_PRESENT` and the results of steps 1 and 4.
6.   DeleteProcThreadAttributeList() is called.
7.   Results are gathered, and handles are cleaned up.

Appinfo 服务描述：
>使用辅助管理权限便于交互式应用程序的运行。如果停止此服务，用户将无法使用辅助管理权限启动应用程序，而执行所需用户任务可能需要这些权限。

sudo 是 CUI 程序，如果我们需要实现类似 sudo 这样的程序，像标准输入输出的继承，工作目录的设置都必不可少，遗憾的是，在使用 ShellExecuteEx 启动管理员进程时，无法设置子进程的工作目录，也无法让子进程继承当前的控制台，终端，因此提权后，如果子进程子系统是 `Windows CUI` 时，会弹出一个新的控制台窗口。

在 UAC 提权的过程，我们知道创建进程实际上是由 Appinfo 服务创建的，因此，如果在 `CreateProcessAsUserW` 之际直接 `lpCurrentDirectory` 即可设置子进程的工作目录。

sudo 不需要 UI 交互验证用户的权限，这个时候，可以在终端或者控制台提示用户输入密码，在 Appinfo 中调用 [`LogonUserW`](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-logonuserw) 验证用户凭据的合法性。当然，密码在跨进程传输的过程中要保证安全。

唯一值得商榷的是，AppInfo 服务如何获得调用进程的标准输入输出标准错误，将控制台或者管道给子进程继承。Windows 拥有控制台 API [`AttachConsole`](https://docs.microsoft.com/en-us/windows/console/attachconsole)，可以让进程连接到另一个进程的控制台，这在后面有介绍，但是否可以不需要 `AttachConsole` 直接获取特定进程的控制台句柄，并将子进程的 `STARTUPINFOW` 的 `hStdInput`,`hStdOutput` 以及 `hStdError` 设置到控制台，如果调用进程的终端是 cygwin，msys2 这样的以管道模拟的，则需要获得这些管道的名称，然后使用 `CreateFileA` 创建句柄，绑定到子进程的输入输出错误。

在 Appinfo 服务中实现 sudo 的逻辑的困难在于 Windows 团队需要谨慎处理各种情况，避免安全问题引入 Windows。并且，实现 sudo 逻辑到上线可能需要几个 Windows 发行版。

### sudo bridge 设想

在 WSL 发布之后，[winpty](https://github.com/rprichard/winpty) Ryan Prichard 开源了 [wslbridge](https://github.com/rprichard/wslbridge) 项目，wslbridge 分为前端和后端，前端是一个基于 cygwin 的 Windows 程序，后端是一个 Linux 程序，当使用 PTY 模式时 wslbridge-backend 会使用 `forkpty` 创建一个 `pseudoterminal` 然后将终端的数据通过 socket 发送到 wslbridge-frontend （通常会被 mintty conemu 这样的终端读取呈现给用户）. wslbridge-frontend 接受到用户输入的数据发送到 wslbridge-backend. 如果不使用 `PTY` 模式，wslbridge 还可以使用 Pipe 去模拟，这和 Cygwin/MSYS 在 Windows 的机制是一样的。 

### 需要 UI 交互的 wsudo

[AttachConsole](https://docs.microsoft.com/en-us/windows/console/attachconsole)

[Windows Terminal: oDispatchers::ConsoleHandleConnectionRequest](https://github.com/microsoft/terminal/blob/0d8f2998d6fdfa6013854ea66ccf26ed34ba8de2/src/server/IoDispatchers.cpp#L141)

[Windows Terminal: IoDispatchers::ConsoleClientDisconnectRoutine](https://github.com/microsoft/terminal/blob/0d8f2998d6fdfa6013854ea66ccf26ed34ba8de2/src/server/IoDispatchers.cpp#L267)

[ReactOS: AttachConsole](https://github.com/reactos/reactos/blob/3f1ab92d3aca8b7b0965a1004e4a5f25b4d64025/dll/win32/kernel32/client/console/console.c#L2675)


## Final

最小特权原则 [Principle of least privilege](https://en.wikipedia.org/wiki/Principle_of_least_privilege)

## 备注

1.   [Vista UAC: The Definitive Guide](https://www.codeproject.com/Articles/19165/Vista-UAC-The-Definitive-Guide)