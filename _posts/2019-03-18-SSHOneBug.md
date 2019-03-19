---
layout: post
title:  "记录 SSH 的一个 Bug"
date:   2019-03-18 18:00:00
published: true
categories: ssh
---

## 服务器上错误的命令行

最近在改进 BSSHD，将不被允许的命令打印到日志，但是我遇到了一个不符合预期的输出，比如，在客户端运行 SSH，命令如下：

```shell
ssh git@localhost echo "There are spaces in the statement" done
```

我将命令行使用如下处理后输出：

```go
	fmt.Fprintf(os.Stderr, "%s\n", strings.Join(s.Command(), ";"))
```

我们在服务器上命令行的输出如下：

```shell
echo;There;are;spaces;in;the;statement;done
```

上述输出与预期完全不一致，通常情况下，命令行的输出应该如下：

```shell
echo;There are spaces in the statement;done
```

我们可以编译一个命令行程序验证一下：

```go
// echo.go
// go build echo.go
package main

import (
  "fmt"
  "os"
  "strings"
)

func main(){
 fmt.Fprintf(os.Stderr, "%s\n", strings.Join(os.Args, ";"))
}

```

然后运行输出结果如下：

```shell
./echo "There are spaces in the statement" done
./echo;There are spaces in the statement;done
```

这就说明，SSH 的解析出了问题。我们使用的是 [github.com/gliderlabs/ssh](https://github.com/gliderlabs/ssh)，这个问题是否是与 `gliderlabs/ssh` 解析有关，我们不能过早的下结论，我们可以修改 `gliderlabs/ssh`  的源码验证一番。

`gliderlabs/ssh` 的命令行解析代码在：[https://github.com/gliderlabs/ssh/blob/ef6d89046be36104109e42ac1fee6601f9be95d7/session.go#L217](https://github.com/gliderlabs/ssh/blob/ef6d89046be36104109e42ac1fee6601f9be95d7/session.go#L217)：

```go
			var payload = struct{ Value string }{}
			gossh.Unmarshal(req.Payload, &payload)
			sess.cmd, _ = shlex.Split(payload.Value, true)

			// If there's a session policy callback, we need to confirm before
			// accepting the session.
			if sess.sessReqCb != nil && !sess.sessReqCb(sess, req.Type) {
				sess.cmd = nil
				req.Reply(false, nil)
				continue
			}
```

我们可以在 `shlex.Split` 前面插入一段代码 

```go
		fmt.Fprintf(os.Stderr, "command: [%s]\n", payload.Value)
```

然后或得 BSSHD 命令行输出：

```txt
command: [echo There are spaces in the statement done]
echo;There;are;spaces;in;the;statement;done
```
而 command 的内容直接从 SSH Exe 请求的加密数据解析出来的，这就意味着 SSH 传入了错误的命令。那么我们翻阅 OpenSSH 源码。

## 普遍的糟糕

在 OpenSSH 的源码镜像中：[https://github.com/openssh/openssh-portable/blob/9edbd7821e6837e98e7e95546cede804dac96754/ssh.c#L1061](https://github.com/openssh/openssh-portable/blob/9edbd7821e6837e98e7e95546cede804dac96754/ssh.c#L1061) 有一段代码：

```c
		/* A command has been specified.  Store it into the buffer. */
		for (i = 0; i < ac; i++) {
			if ((r = sshbuf_putf(command, "%s%s",
			    i ? " " : "", av[i])) != 0)
				fatal("%s: buffer error: %s",
				    __func__, ssh_err(r));
		}
```

这段代码将字符串数组变成了字符串，但忘记了字符串数组中的每一个都有可能含有空格字符，那么在命令行解析之时就会出现与预期不一致的结果。这个问题在 Dropbear SSH 中也出现了：

```c
//https://github.com/mkj/dropbear/blob/cb945f9f670e95305c7c5cc5ff344d1f2707b602/cli-runopts.c#L390
	if (i < (unsigned int)argc) {
		/* Build the command to send */
		cmdlen = 0;
		for (j = i; j < (unsigned int)argc; j++)
			cmdlen += strlen(argv[j]) + 1; /* +1 for spaces */

		/* Allocate the space */
		cli_opts.cmd = (char*)m_malloc(cmdlen);
		cli_opts.cmd[0] = '\0';

		/* Append all the bits */
		for (j = i; j < (unsigned int)argc; j++) {
			strlcat(cli_opts.cmd, argv[j], cmdlen);
			strlcat(cli_opts.cmd, " ", cmdlen);
		}
		/* It'll be null-terminated here */
		TRACE(("cmd is: %s", cli_opts.cmd))
	}

```

而像 libssh libssh2 以及 go `crypto/ssh` 的惯用方法也未考虑到命令行存在空格的问题，因此，很多基于这些库实现的客户端也未考虑这个问题。

另外，在 [SSH RFC 4252](https://tools.ietf.org/html/rfc4254#section-6.5) 中，对 `exec` 命令行的解析，仅仅只有如下一段话：

```
      byte      SSH_MSG_CHANNEL_REQUEST
      uint32    recipient channel
      string    "exec"
      boolean   want reply
      string    command

   This message will request that the server start the execution of the
   given command.  The 'command' string may contain a path.  Normal
   precautions MUST be taken to prevent the execution of unauthorized
   commands.
```

这种没有明确规定命令行解析的规范实际上很依赖服务器的实现，只能依赖事实标准，比如跟随 OpenSSH 的实现。

我觉得 Windows 这一点做的比较好，比如 Windows 上命令行的解析实际上是有规范的：[Parsing C Command-Line Arguments](https://docs.microsoft.com/en-us/previous-versions/ms880421(v=msdn.10))。

## OpenSSH 服务端的命令行解析

在 OpenSSH SSHD 的源码中，命令行的处理流程如下：

当请求为 `exec` 时：

```c
//https://github.com/openssh/openssh-portable/blob/9edbd7821e6837e98e7e95546cede804dac96754/session.c#L2221
		if (strcmp(rtype, "shell") == 0) {
			success = session_shell_req(ssh, s);
		} else if (strcmp(rtype, "exec") == 0) {
			success = session_exec_req(ssh, s);
		} else if (strcmp(rtype, "pty-req") == 0) {
			success = session_pty_req(ssh, s);
		} else if (strcmp(rtype, "x11-req") == 0) {
			success = session_x11_req(ssh, s);
		} else if (strcmp(rtype, "auth-agent-req@openssh.com") == 0) {
			success = session_auth_agent_req(ssh, s);
		} else if (strcmp(rtype, "subsystem") == 0) {
			success = session_subsystem_req(ssh, s);
		} else if (strcmp(rtype, "env") == 0) {
			success = session_env_req(ssh, s);
		}
```

获得 `Command` 并执行命令：

```c
//https://github.com/openssh/openssh-portable/blob/9edbd7821e6837e98e7e95546cede804dac96754/session.c#L2047
static int
session_exec_req(struct ssh *ssh, Session *s)
{
	u_int success;
	int r;
	char *command = NULL;

	if ((r = sshpkt_get_cstring(ssh, &command, NULL)) != 0 ||
	    (r = sshpkt_get_end(ssh)) != 0)
		sshpkt_fatal(ssh, r, "%s: parse packet", __func__);

	success = do_exec(ssh, s, command) == 0;
	free(command);
	return success;
}
```

无论是何种途径，在 OpenSSH 中，sshd 启动进程都是通过 `sh -c` 这样的方式实现的，这就意味着，命令行应当符合 sh 的标准。

```c
// https://github.com/openssh/openssh-portable/blob/9edbd7821e6837e98e7e95546cede804dac96754/session.c#L1681
	if (!command) {
		char argv0[256];

		/* Start the shell.  Set initial character to '-'. */
		argv0[0] = '-';

		if (strlcpy(argv0 + 1, shell0, sizeof(argv0) - 1)
		    >= sizeof(argv0) - 1) {
			errno = EINVAL;
			perror(shell);
			exit(1);
		}

		/* Execute the shell. */
		argv[0] = argv0;
		argv[1] = NULL;
		execve(shell, argv, env);

		/* Executing the shell failed. */
		perror(shell);
		exit(1);
	}
	/*
	 * Execute the command using the user's shell.  This uses the -c
	 * option to execute the command.
	 */
	argv[0] = (char *) shell0;
	argv[1] = "-c";
	argv[2] = (char *) command;
	argv[3] = NULL;
	execve(shell, argv, env);
	perror(shell);
	exit(1);
```



## 最后

这个 Bug 的修复技术上并不难，只需要将命令行数组转变为兼容 `sh -c` 的方式即可。但让 OpenSSH 修复或许有点麻烦。