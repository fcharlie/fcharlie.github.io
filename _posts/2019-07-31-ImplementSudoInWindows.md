---
layout: post
title:  "在 Windows 中实现 sudo"
date:   2019-07-31 20:00:00
published: false
categories: windows
---
# 前言

*这篇文章的想法来源于我在 [Windows Terminal Issue#146](https://github.com/microsoft/terminal/issues/146) 的[评论](https://github.com/microsoft/terminal/issues/146#issuecomment-515812461)。*

[sudo](https://linux.die.net/man/8/sudo) 以另一个用户执行命令，通常是 `root`。当普通用户需要以其他权限执行某项工作时，通常需要获得指定用户的权限，以目标权限 `root` 为例，我们期望以 root 权限运行，可以使用 `su` 登录到 `root` 用户，在这种情况下，一直到退出 `root`。都使用的是 `root` 权限，这实际上并不是安全的，处于高级别权限的时间应当尽量的短。而使用 `sudo` 获得 root 权限要安全的多，这种情况下，只有特定的命令才会获得 root 权限，而不是整个用户和 shell. 话又说回来，sudo 是如何获得 root 权限的？在 Windows 中的 sudo 又是怎么一回事，如何在 Windows 中实现类似的 sudo.

## Linux 的 sudo 内幕

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

这里我们可以看到 sudo 命令的程序文件，所有者 ID 和 组 ID 都是 **0**，即 `root`，与普通文件不同之处 `sudo` 设置了 `S_ISUID` 我们查看 GNU [14.9.5 The Mode Bits for Access Permission](https://www.gnu.org/software/libc/manual/html_node/Permission-Bits.html) 以及 [30.4 How an Application Can Change Persona](https://www.gnu.org/software/libc/manual/html_node/How-Change-Persona.html#How-Change-Persona) 其中有一段 

>If a process has a file ID (user or group), then it can at any time change its effective ID to its real ID and back to its file ID. Programs use this feature to relinquish their special privileges except when they actually need them. This makes it less likely that they can be tricked into doing something inappropriate with their privileges.

sudo 命令启动后可以运行 `setuid(0)` 将自身权限设置为 `root` 然后验证用户凭据，有效时则可以以 root 用户权限运行相应的命令。在 Linux/POSIX 系统中，`sudo` 的原理并不复杂，当然要处理好不同用户的环境变量，涉及到的代码还是比较繁琐的。

## 在 Windows 实现 Sudo

### ShellExecute 内幕与 sudo

### C/S sudo 机制

### 需要 UI 交互的 wsudo