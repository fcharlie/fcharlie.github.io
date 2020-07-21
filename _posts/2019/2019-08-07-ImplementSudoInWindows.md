---
layout: post
title:  "在 Windows 中实现 sudo"
date:   2019-08-07 12:00:00
published: true
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

>Windows 属于多用户操作系统，这里的权限讨论并未讲述来宾账户，仅限于标准账户，即同时属于 User 和 Administrators 组且开启了 UAC 提示的账户。

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

如果在标准用户中使用 `CreateProcessW` 启动需要提升的进程，会返回 `elevation required`（GetLastError 740）错误。

我们可以看到 Windows 中，提权本质上是通过去特权服务进行通信，校验后，由特权用户创建进程，这里并没有使用 `S_ISUID` 这样的机制，当然 UAC 白名单的自动提升和 `setuid` 也是不一样的，前者是系统创建进程便是创建了管理员权限，后者是 fork-exec 后，通过 `setuid` 切换到 `root`（Linux/Apple sudo 均是如此）。

sudo 是 CUI 程序，如果我们需要实现类似 sudo 这样的程序，像标准输入输出的继承，工作目录的设置都必不可少，遗憾的是，在使用 ShellExecuteEx 启动管理员进程时，无法设置子进程的工作目录，也无法让子进程继承当前的控制台，终端，因此提权后，如果子进程子系统是 `Windows CUI` 时，会弹出一个新的控制台窗口。

在 UAC 提权的过程，我们知道创建进程实际上是由 Appinfo 服务创建的，因此，如果在 `CreateProcessAsUserW` 之际直接 `lpCurrentDirectory` 即可设置子进程的工作目录。

sudo 不需要 UI 交互验证用户的权限，这个时候，可以在终端或者控制台提示用户输入密码，在 Appinfo 中调用 [`LogonUserW`](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-logonuserw) 验证用户凭据的合法性。当然，密码在跨进程传输的过程中要保证安全。

唯一值得商榷的是，AppInfo 服务如何获得调用进程的标准输入输出标准错误，将控制台或者管道给子进程继承。Windows 拥有控制台 API [`AttachConsole`](https://docs.microsoft.com/en-us/windows/console/attachconsole)，可以让进程连接到另一个进程的控制台，这在后面有介绍，但是否可以不需要 `AttachConsole` 直接获取特定进程的控制台句柄，并将子进程的 `STARTUPINFOW` 的 `hStdInput`,`hStdOutput` 以及 `hStdError` 设置到控制台。如果调用进程的终端是 cygwin，msys2 这样的以管道模拟的，则需要获得这些管道的名称，然后使用 `CreateFileA` 创建句柄，绑定到子进程的输入输出错误。要处理不同的情况还是比较麻烦。

在 Appinfo 服务中实现 sudo 的逻辑的困难在于 Windows 团队需要谨慎处理各种情况，避免安全问题引入 Windows。并且，实现 sudo 逻辑到上线可能需要几个 Windows 发行版。

### sudo bridge 设想

在 WSL 发布之后，[winpty](https://github.com/rprichard/winpty) Ryan Prichard 开源了 [wslbridge](https://github.com/rprichard/wslbridge) 项目，wslbridge 分为前端和后端，前端是一个基于 cygwin 的 Windows 程序，后端是一个 Linux 程序，当使用 PTY 模式时 wslbridge-backend 会使用 `forkpty` 创建一个 `pseudoterminal` 然后将终端的数据通过 socket 发送到 wslbridge-frontend （通常会被 mintty conemu 这样的终端读取呈现给用户）. wslbridge-frontend 接受到用户输入的数据发送到 wslbridge-backend. 如果不使用 `PTY` 模式，wslbridge 还可以使用 Pipe 去模拟终端行为，这和 Cygwin/MSYS 在 Windows 的机制是一样的。 

Windows Terminal 项目实际上包括了 Windows conhost 的源码，基本上 OpenConsole 与目前 Windows 10 conhost.exe 的代码绝大多数是相同的。要了解控制台的一些源码可以好好的看看代码。

WindowsTerminal.exe 是一个 UWP 程序，在启动终端时，通过 conhost.exe(OpenConsole) 创建一个 PTY 模式的终端: [CreateConPty](https://github.com/microsoft/terminal/blob/8fa42e09dfc6cd57d29e517a002d8c7a99e2aebd/src/cascadia/TerminalConnection/ConhostConnection.cpp#L94)。创建终端的 `CreateConPty` 代码如下：


```c++
// Function Description:
// - Creates a headless conhost in "pty mode" and launches the given commandline
//      attached to the conhost. Gives back handles to three different pipes:
//   * hInput: The caller can write input to the conhost, encoded in utf-8, on
//      this pipe. For keys that don't have character representations, the
//      caller should use the `TERM=xterm` VT sequences for encoding the input.
//   * hOutput: The caller should read from this pipe. The headless conhost will
//      "render" it's state to a stream of utf-8 encoded text with VT sequences.
//   * hSignal: The caller can use this to resize the size of the underlying PTY
//      using the SignalResizeWindow function.
// Arguments:
// - cmdline: The commandline to launch as a console process attached to the pty
//      that's created.
// - startingDirectory: The directory to start the process in
// - w: The initial width of the pty, in characters
// - h: The initial height of the pty, in characters
// - hInput: A handle to the pipe for writing input to the pty.
// - hOutput: A handle to the pipe for reading the output of the pty.
// - hSignal: A handle to the pipe for writing signal messages to the pty.
// - piPty: The PROCESS_INFORMATION of the pty process. NOTE: This is *not* the
//      PROCESS_INFORMATION of the process that's created as a result the cmdline.
// - extraEnvVars : A map of pairs of (Name, Value) representing additional
//      environment variable strings and values to be set in the client process
//      environment.  May override any already present in parent process.
// Return Value:
// - S_OK if we succeeded, or an appropriate HRESULT for failing format the
//      commandline or failing to launch the conhost
[[nodiscard]] __declspec(noinline) inline HRESULT CreateConPty(const std::wstring& cmdline,
                                                               std::optional<std::wstring> startingDirectory,
                                                               const unsigned short w,
                                                               const unsigned short h,
                                                               HANDLE* const hInput,
                                                               HANDLE* const hOutput,
                                                               HANDLE* const hSignal,
                                                               PROCESS_INFORMATION* const piPty,
                                                               DWORD dwCreationFlags = 0,
                                                               const EnvironmentVariableMapW& extraEnvVars = {}) noexcept
{
    // Create some anon pipes so we can pass handles down and into the console.
    // IMPORTANT NOTE:
    // We're creating the pipe here with un-inheritable handles, then marking
    //      the conhost sides of the pipes as inheritable. We do this because if
    //      the entire pipe is marked as inheritable, when we pass the handles
    //      to CreateProcess, at some point the entire pipe object is copied to
    //      the conhost process, which includes the terminal side of the pipes
    //      (_inPipe and _outPipe). This means that if we die, there's still
    //      outstanding handles to our side of the pipes, and those handles are
    //      in conhost, despite conhost being unable to reference those handles
    //      and close them.
    // CRITICAL: Close our side of the handles. Otherwise you'll get the same
    //      problem if you close conhost, but not us (the terminal).
    HANDLE outPipeConhostSide;
    HANDLE inPipeConhostSide;
    HANDLE signalPipeConhostSide;

    SECURITY_ATTRIBUTES sa;
    sa = { 0 };
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = nullptr;

    CreatePipe(&inPipeConhostSide, hInput, &sa, 0);
    CreatePipe(hOutput, &outPipeConhostSide, &sa, 0);
    CreatePipe(&signalPipeConhostSide, hSignal, &sa, 0);

    SetHandleInformation(inPipeConhostSide, HANDLE_FLAG_INHERIT, 1);
    SetHandleInformation(outPipeConhostSide, HANDLE_FLAG_INHERIT, 1);
    SetHandleInformation(signalPipeConhostSide, HANDLE_FLAG_INHERIT, 1);

    std::wstring conhostCmdline = L"conhost.exe";
    conhostCmdline += L" --headless";
    std::wstringstream ss;
    if (w != 0 && h != 0)
    {
        ss << L" --width " << (unsigned long)w;
        ss << L" --height " << (unsigned long)h;
    }

    ss << L" --signal 0x" << std::hex << HandleToUlong(signalPipeConhostSide);
    conhostCmdline += ss.str();
    conhostCmdline += L" -- ";
    conhostCmdline += cmdline;

    STARTUPINFO si = { 0 };
    si.cb = sizeof(STARTUPINFOW);
    si.hStdInput = inPipeConhostSide;
    si.hStdOutput = outPipeConhostSide;
    si.hStdError = outPipeConhostSide;
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::unique_ptr<wchar_t[]> mutableCommandline = std::make_unique<wchar_t[]>(conhostCmdline.length() + 1);
    if (mutableCommandline == nullptr)
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = StringCchCopy(mutableCommandline.get(), conhostCmdline.length() + 1, conhostCmdline.c_str());
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    LPCWSTR lpCurrentDirectory = startingDirectory.has_value() ? startingDirectory.value().c_str() : nullptr;

    std::vector<wchar_t> newEnvVars;
    auto zeroNewEnv = wil::scope_exit([&] {
        ::SecureZeroMemory(newEnvVars.data(),
                           newEnvVars.size() * sizeof(decltype(newEnvVars.begin())::value_type));
    });

    if (!extraEnvVars.empty())
    {
        EnvironmentVariableMapW tempEnvMap{ extraEnvVars };
        auto zeroEnvMap = wil::scope_exit([&] {
            // Can't zero the keys, but at least we can zero the values.
            for (auto& [name, value] : tempEnvMap)
            {
                ::SecureZeroMemory(value.data(), value.size() * sizeof(decltype(value.begin())::value_type));
            }

            tempEnvMap.clear();
        });

        RETURN_IF_FAILED(UpdateEnvironmentMapW(tempEnvMap));
        RETURN_IF_FAILED(EnvironmentMapToEnvironmentStringsW(tempEnvMap, newEnvVars));

        // Required when using a unicode environment block.
        dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
    }

    LPWCH lpEnvironment = newEnvVars.empty() ? nullptr : newEnvVars.data();
    bool fSuccess = !!CreateProcessW(
        nullptr,
        mutableCommandline.get(),
        nullptr, // lpProcessAttributes
        nullptr, // lpThreadAttributes
        true, // bInheritHandles
        dwCreationFlags, // dwCreationFlags
        lpEnvironment, // lpEnvironment
        lpCurrentDirectory, // lpCurrentDirectory
        &si, // lpStartupInfo
        piPty // lpProcessInformation
    );

    CloseHandle(inPipeConhostSide);
    CloseHandle(outPipeConhostSide);
    CloseHandle(signalPipeConhostSide);

    return fSuccess ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

```

实际上是通过三根管道与 conhost 相连，conhost.exe 将数据通过管道发送到 WindowsTerminal.exe，从管道接收到信号和数据。

在实现 wsudo 的过程中，我们也可以使用类似的机制，简单点就是创建一个 sudo-service 作为特权服务运行，当用户运行 sudo 时，与服务通信，授权成功后，sudo-service 创建一个 `ConPty` （或者直接使用 OpenConsole）与作为特权进程的父进程。sudo-service 与 sudo 交互数据即可。架构如下图所示：

![](https://user-images.githubusercontent.com/6904176/62018098-da55d580-b1eb-11e9-8cab-8a68ea273d46.png)

这种机制的缺陷是，需要多次转发数据，以输入为例，数据输入从用户到 sudo, sudo 发送给 sudo-service, sudo-service 写入到 OpenConsole. OpenConsole 写入到特权进程。这样一来，大量数据时，可能需要大量 IO 和 CPU。不过这种情况下无需考虑 Console 和 Cygwin/MSYS PTY 的差异。

### NtSetInformationProcess 的 sudo 机制

在 Github 上，Parker Snell 开发了 [wsudo: Proof of concept sudo for Windows](https://github.com/parkovski/wsudo)（和 Privexec wsudo 同名），在这个 wsudo 里面，使用 C/S 架构和 `NtSetInformationProcess` 实现了 sudo 的机制，这种机制实际上与 Linux sudo 类似，即都是从标准用户中启动，这样便可以完整的继承当前的终端设备，环境变量，不同之处在于，这里是 wsudo_client 是通过请求 wsudo_server，授权请求成功返回后，使用 `CREATE_SUSPENDED` 标志创建暂停的子进程，将进程的句柄发送给 `wsudo_server`，`wsudo_server` 使用 `NtSetInformationProcess` 修改子进程的 `Token`，将其提升为特权进程，wsudo_client 再运行 `ResumeThread` 将其唤醒。在 ReactOS 中 `CreateProcessAsUser` 实际上同样使用了 `NtSetInformationProcess`，即使用 `CreateProcessW` 创建挂起的进程后，使用 `NtSetInformationProcess` 设置进程的 `Token` 然后使进程的主线程恢复运行。在 Windows `CreateProcessAsUser` 的机制大致如此，但具体的实现细节存在差异。此方案与 `CreateProcessAsUser` 不同的是并非由子进程的父进程去修改 `Token`，而是交由 `wsudo_server` 这样的特权服务修改其 `Token`。因此 `CreateProcessAsUser` 实际更倾向于降权。而在 `wsudo_server` 这一端，实际上也是一种降权（Local System 权限高于 Administrator），不过整体上看就不一样了。

不过在此例中，wsudo_server 是直接拷贝的服务的 `Token`，这种机制有很大的风险，建议的策略是使用 `LogonUserW` 获得受限的管理员 Token 后，再使用 `GetTokenInformation` 获得 `TokenLinkedToken`，由 `LinkedToken` 创建管理员进程，这与 appinfo 服务的机制类似。当然也可以使用 `WTSQueryUserToken` 获得管理员进程的 Token 再使用 `GetTokenInformation` 获得 `TokenLinkedToken` 创建管理员进程，但 `LogonUserW` 生成的令牌可能有一些限制。


```c++
  HObject currentToken;
  // if (!OpenProcessToken(clientProcess, TOKEN_DUPLICATE | TOKEN_READ,
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_READ,
                        &currentToken))
  {
    log::error("Client {}: Couldn't open client process token: {}", _clientId,
               lastErrorString());
    return false;
  }

  PSID ownerSid;
  PSID groupSid;
  PACL dacl;
  PACL sacl;
  PSECURITY_DESCRIPTOR secDesc;
  if (!SUCCEEDED(GetSecurityInfo(currentToken, SE_KERNEL_OBJECT,
                                 DACL_SECURITY_INFORMATION |
                                   SACL_SECURITY_INFORMATION |
                                   GROUP_SECURITY_INFORMATION |
                                   OWNER_SECURITY_INFORMATION,
                                 &ownerSid, &groupSid, &dacl, &sacl,
                                 &secDesc)))
  {
    log::error("Client {}: Couldn't get security info: {}", _clientId,
               lastErrorString());
    return false;
  }
  WSUDO_SCOPEEXIT { LocalFree(secDesc); };

  SECURITY_ATTRIBUTES secAttr;
  secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  secAttr.bInheritHandle = true;
  secAttr.lpSecurityDescriptor = secDesc;

  //

  HObject newToken;
  if (!DuplicateTokenEx(currentToken, MAXIMUM_ALLOWED, &secAttr,
                        SecurityImpersonation, TokenPrimary, &newToken))
  {
    log::error("Client {}: Couldn't duplicate token: {}", _clientId,
               lastErrorString());
    return false;
  }

  _userToken = std::move(newToken);
  log::info("Client {}: Authorized; stored new token.", _clientId);
```

这种在 Windows 中实现 `sudo` 的机制较简单，复杂性较低。当然需要安装服务，进程间安全通信，避免提权漏洞，这些问题都需要解决，所以并不是那么容易的。而且对于使用 `NtSetInformationProcess` 修改进程权限，Windows 内核团队好像并不意见用户这样做（[NTSetInformationProcess (ProcessAccessToken) fails with STATUS_NOT_SUPPORTED](https://social.msdn.microsoft.com/forums/windowsdesktop/en-US/86602194-c8f7-4c42-b349-fd78e1bdb5f2/ntsetinformationprocess-processaccesstoken-fails-with-statusnotsupported) 2007-01-03）：

```
Hello.

I am a developer on the Windows Kernel Team. Before continuing, I want to stress as a disclaimer that NtSetInformationProcess, class ProcessAccessToken, is an undocumented and unsupported infterface. It is reserved for system component use and is subject to change between operating system releases. That being said, I would like to address your particular concern.

The NT kernel was never intended to allow token switching once a process started running. This is because handles, etc. may have been opened in an old security context, inflight operations may use inconsistent security contexts, etc. As such, it typically does not make sense to switch a process' token once it has begun execution. However, this was not enforced until Vista.

Unfortunately, it is difficult to properly implement setuid() semantics on NT as you have noted, though it too could be susceptible to the issues outlined above. After exploring alternative implementations for Interix we settled on leaving the lazy swap behavior intact for EXEs launched from POSIX binaries (image type = POSIX in the PE image). This was a reasonable compromise since the change was not security-based in nature, and allowed the legacy behavior to persist in conjunction with binaries that had (or should have) better control of their environment.

Arun Kishan

Windows Kernel Team
```

不过上述回复是 2007 年，时至今日，不知道 Windows 内核团队有没有新的看法。

遗憾的是 Parker Snell 采取的是 GPLv3 协议，如果要使用此方案，则可能需要在 `cleanroot` 中实现，在 Windows Terminal 的评论中，我也没有添加此方案的介绍。

### 需要 UI 交互的 wsudo

前面三种情况都是需要重新实现一个服务或者在现有服务基础上改进，在服务中创建管理员进程的 Token，这对于微软来说存在诸多顾虑，而第三方开发者实现也需要慎重考虑避免安全问题，如果我们在保留 UAC 的基础上，可以使用下面的方案。

前面说到，我们可以使用 [AttachConsole](https://docs.microsoft.com/en-us/windows/console/attachconsole) 将进程附加到旧的控制台上，如果可以接受需要 UI 交互，我们可以使用 `AttachConsole` 实现不完整的 `sudo`。

在 [Privexec](https://github.com/M2Team/Privexec) 中，我通过 [`wsudo-tie`](https://github.com/M2Team/Privexec/blob/master/wsudo/wsudo-tie.cc) 命令作为中间件实现不完整的 `sudo`。当用户在控制台中使用 `wsudo -A` 启动管理员进程时，如果目标可执行程序的子系统为 `Windows CUI`（或者后缀为 `.bat`,`.com`,`.cmd`），并且启动参数没有 `--hide` `--new-console`则使用 ShellExecuteEx 启动 `wsudo-tie`，在 `wsudo-tie` 中，设置好工作目录，环境变量，并调用 `FreeConsole`，`AttachConsole` 后，启动新的进程，这样无论是工作目录还是环境变量等，都与预期的相符。下图在控制台中使用 wsudo 启动一个管理员权限的 wsudo，后者再启动了 `TrustedInstaller` 的 `pwsh`。

![](https://user-images.githubusercontent.com/6904176/62051234-8972cb00-b245-11e9-958d-e3af4ca614f7.png)

这种方案借鉴了 [lukesampson/psutils](https://github.com/lukesampson/psutils/blob/master/sudo.ps1) 的 sudo 机制，但有所改善。

wsudo-tie 的方案并不适合 Cygwin/MSYS 的终端，如 Mintty，因为这些终端使用管道模拟而不是像 ConEmu 内部有个控制台，此时使用 AttachConsole 会失败。

另外需要注意的是，在 `wsudo-tie` 中，`CreateProcessW` 启动 `.bat`/`.cmd` 可能会出现找不到文件的情况，我这里使用 `bela::ExecutableExistsInPath` 避免这种情况的发生。

在这种方案中，弹出安全桌面时，显示的是 wsudo-tie 的信息，而不是目标进程的信息，这就意味着没有对目标程序进行数字签名校验，要实现数字签名校验，还有很多事情要做。

AttachConsole 的相关流程可以参考：
+   [Windows Terminal: oDispatchers::ConsoleHandleConnectionRequest](https://github.com/microsoft/terminal/blob/0d8f2998d6fdfa6013854ea66ccf26ed34ba8de2/src/server/IoDispatchers.cpp#L141)
+   [Windows Terminal: IoDispatchers::ConsoleClientDisconnectRoutine](https://github.com/microsoft/terminal/blob/0d8f2998d6fdfa6013854ea66ccf26ed34ba8de2/src/server/IoDispatchers.cpp#L267)
+   [ReactOS: AttachConsole](https://github.com/reactos/reactos/blob/3f1ab92d3aca8b7b0965a1004e4a5f25b4d64025/dll/win32/kernel32/client/console/console.c#L2675)


## 最后

在 Windows 上实现非 Windows 哲学的 sudo 还是比较复杂的，sudo 虽然好玩，但我们还是应该遵循 **最小特权原则 [Principle of least privilege](https://en.wikipedia.org/wiki/Principle_of_least_privilege)** ，减少提权请求。

## 备注

1.   [Vista UAC: The Definitive Guide](https://www.codeproject.com/Articles/19165/Vista-UAC-The-Definitive-Guide)
2.   [How to launch a process as a Full Administrator when UAC is enabled?](https://blogs.msdn.microsoft.com/winsdk/2013/03/22/how-to-launch-a-process-as-a-full-administrator-when-uac-is-enabled/)
3.   [Dealing with Administrator and standard user’s context](https://blogs.msdn.microsoft.com/winsdk/2010/05/31/dealing-with-administrator-and-standard-users-context/)