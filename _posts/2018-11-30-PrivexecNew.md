---
layout: post
title:  "Privexec 杂谈"
date:   2018-11-30 21:00:00
published: true
categories: windows
---

# 前言

本站的开篇就是讲的 [《Windows AppContainer 降权，隔离与安全》](https://forcemz.net/container/2015/06/12/AppContainer/)，一晃三年多过去了，这三年之中，我开发了一个 [Privexec](https://github.com/M2Team/Privexec)，一个以其他权限启动进程的工具，支持启动 `AppContainer` 进程，前段实现有用户发起了功能请求<sup>1</sup>，让 `Privexec` 支持设置 `AppContaner` 的 `Capabilities`，而不是像以前一样在启动 `AppContainer` 进程时使用 `CreateWellKnownSid` 创建所有的与 `AppContainer` 相关的 `Capabilities SIDs`。于是乎，我就花了一点时间将 Privexec 重构了一番，有所感悟，便将其写下来。

## Process

在 Windows 平台上，创建进程有 `WinExec`，`system`，`_spawn/_wspawn`，`CreateProcess`，`ShellExecute` 等多种途径，但上述函数基本上还是由 `CreateProcess Family` 封装的。在 Windows 使用 `C/C++` 创建进程应当优先使用 `CreateProcess`，CreateProcess 有三个变体，主要是为了支持以其他权限启动进程， CreateProcess 及其变体如下：

|Function|Feature|Details|
|---|---|---|
|CreateProcessW/A|创建常规进程，权限继承父进程权限|支持 EXTENDED_STARTUPINFO_PRESENT 设置 AppContainer|
|CreateProcessAsUserW/A|使用主 Token 创建进程，子进程权限与 Token 限定一致|调用者须有 SE_INCREASE_QUOTA_NAME 权限|
|CreateProcessWithTokenW|使用主 Token 创建进程，子进程权限与 Token 限定一致|调用者须有 SE_IMPERSONATE_NAME 权限|
|CreateProcessWithLogonW/A|使用指定用户凭据启动进程||

`CreateProcessWithTokenW` 是 `Windows Vista` 才引入桌面系统的，没有 ANSI 版本，Github 上最常见的用法是打开桌面进程，获取其 Token，然后拷贝这个 Token 创建与桌面权限一致的进程，这是一种降权机制，仅当当前登录用户是标准用户才能降权成功。
`CreateProcessWithTokenW` 内部在调用好几个函数之后才会调用 `CreateProcessAsUserW`，在 `Privexec` 中并未使用它。
在 `ReactOS` 中，没有实现 `CreateProcessWithTokenW`，下面附上 `CreateProcessWithTokenW` 调用图：

![CreateProcessWithTokenW](https://i.imgur.com/V51b0vx.png)
![CreateProcessWithTokenW](https://i.stack.imgur.com/Vn7Qe.png)

实际上在 `ReactOS` 中，`CreateProcessAsUser` 最终依然调用的是 `CreateProcess` 创建进程，然后将进程挂起，设置好 `Token` 后再恢复进程。在 Windows 中，你可以使用 `Process Monitor` 跟踪 `CreateProcessAsUser` 的调用堆栈，在 Windows 10  18.09 (10.0.17763.167) 中，`CreateProcessAsUser` 会调用 `CreateProcessInternalW`，而 CreateProcess 也是调用 `CreateProcessInternalW`。如果用 `IDA Freeware` <sup>2</sup> 反汇编 `KernelBase.dll` 可以发现 `CreateProcessAsUserW` 的权限最后传递到 `CreateProcessInternalW` 由 `CreateProcessInternalW` 处理了，这里与 `ReactOS` 有一定差别。

![CreateProcessAsUserW](https://github.com/M2Team/Privexec/raw/master/docs/images/austack.png)

在 Windows 中，如果要实现 `UAC` 提权，需要调用 `ShellExecute` 以 `runas` 的参数启动新的进程。如果在程序编译的清单文件中添加了如下清单代码：

```
 <trustInfo xmlns="urn:schemas-microsoft-com:asm.v2">
    <security>
      <requestedPrivileges xmlns="urn:schemas-microsoft-com:asm.v3">
        <requestedExecutionLevel level='requireAdministrator' uiAccess='false' />
      </requestedPrivileges>
    </security>
  </trustInfo>
```
Shell 在启动清单附带有 UAC 提权的可执行文件时也会发生提权。以 `Windows 10` 为例，提权最终由 `AicLaunchAdminProcess` 函数实现，此函数在 `Windows.Storage.dll`，然后与 `Appinfo` 服务通信，`Appinfo` 使用 `CreateProcessAsUserW` 启动进程，并将其父进程设置为 `ShellExecute` 调用者。Vista 时期的细节<sup>3</sup>如下：

1.   AppInfo goes and talks to the Local Security Authority to get the elevated token of the logged in user of Session 1.
2.   AppInfo loads up a STARTUPINFOEX structure (new to Vista), and calls the brand new Vista API InitializeProcThreadAttributeList() with room for one attribute.
3.   OpenProcess() is called to get a handle to the process that initiated the RPC call.
4.   UpdateProcThreadAttribute() is called with PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, and uses the handle retrieved in step 3.
5.   CreateProcessAsUser() is called with EXTENDED_STARTUPINFO_PRESENT and the results of steps 1 and 4.
6.   DeleteProcThreadAttributeList() is called.
7.   Results are gathered, and handles are cleaned up.

(Appinfo 服务描述：使用辅助管理权限便于交互式应用程序的运行。如果停止此服务，用户将无法使用辅助管理权限启动应用程序，而执行所需用户任务可能需要这些权限。)

当我们了解到 Windows 创建进程的粗略细节，那么就可以对 Privexec 创建其他权限的进程做出封装了。Privexec 支持的权限类型有：

+   AppContainer
+   Mandatory Integrity Control
+   No Elevated(UAC)
+   Administrator
+   System
+   TrustedInstaller

**AppContainer** 通常可以使用 `CreateProcess` 创建，使用 `EXTENDED_STARTUPINFO_PRESENT` 额外的 `dwCreateFlags` 创建权限为 `AppContainer` 的进程即可。AppContainer 配置需由 `CreateAppContainerProfile` 创建，相应的 `Capability SID` 可由 `RtlDeriveCapabilitySidsFromName` 或者 `CreateWellKnownSid` 创建，而 `Privexec(GUI)` 受限与 UI 限制，目前仅支持 9 个 `WellKnownSid`，而 `wsudo` 支持 `--appx` 从文件中设置。

 **Mandatory Integrity Control** 主要利用 `SetTokenInformation` 将高完整性级别的 `Token` 设置为低完整性级别的 `Token` 然后使用  `CreateProcessAsUser`。

 **No Elevated(UAC)** 当当前用户为管理员时需要使用计划任务启动 UAC 未提升进程，而计划任务主机进程也是使用 `CreateProcessAsUser` 启动非提升的进程。当当前用户不为管理员时，使用 `CreateProcess` 创建进程即可。

**Administrator** 当当前进程不为管理员，则需要 UAC 提权，细节前文由描述。反之则使用 `CreateProcess` 即可。 

**System** 当前进程必须是管理员权限等级及以上进程，需要开启 `SE_DEBUG_NAME` 权限，然后获得系统权限进程的 Token, 将自身权限模拟 `System` Token，然后拷贝自身 `Token` 修改 `Token` 为 `Primary Token`，即 `hPrimary`，因为只有主 Token 才能被用于创建子进程。《Windows Internal 7th Edition》作者之一 [Pavel Yosifovich](https://github.com/zodiacon) 就写了一个 sysrun 的例子：[sysrun](https://github.com/zodiacon/sysrun)。

**TrustedInstaller** 此权限严格意义上来说，是属于 `Windows Modules Installer` 服务的专有权限，服务描述为：
>启用 Windows 更新和可选组件的安装、修改和移除。如果此服务被禁用，则此计算机的 Windows 更新的安装或卸载可能会失败。

因此要获得此权限，需要先模拟到 `System` 权限，然后启动 `TrustedInstaller` 服务，然后获得服务进程的权限句柄，以该句柄拷贝启动新的进程。

Privexec 的进程启动相关代码在：[https://github.com/M2Team/Privexec/tree/master/include/process](https://github.com/M2Team/Privexec/tree/master/include/process)，使用 C++17。

实际上，你完全可以启动一个服务，需要启动进程时，与服务通讯，然后让服务使用 `CreateProcessAsUser` 启动新的进程，当然这个时候必须要考虑到工具的可信，避免恶意程序与服务通讯启动高权限进程。

[毛利](https://github.com/MouriNaruto) 的 [NSudo](https://github.com/M2Team/NSudo) 与 Privexec 类似，但实现基本使用 `CreateProcessAsUser` + `Token` 创建进程。

题外话：在 Windows 平台上，启动进程依然有不小的代价，中间环节多，而在 Unix 平台，启动进程有 `fork/exec`，实际上要实现类似 `CreateProcess` 之类的逻辑需要使用 `fork/exec` 联合使用。现实带来了遗憾，Windows 上实现 `fork` 和 Unix 实现更高效的 `CreateProcess` 成了两个大难题。当然，在实现 `Windows Subsystem for Linux`<sup>4</sup>，微软也在改进其创建进程的流程，不过 `Minimal process` 并没有让 `cygwin` 这样的系统收益，至少目前依然这样。

![picoprocess](https://msdnshared.blob.core.windows.net/media/2016/05/picoProcess-1024x763.png)

## Privexec (GUI)

Privexec 狭义上是指 `Privexec (GUI)` 这个程序，现在应该之 `Privexec` 整个项目，包括 `wsudo`。在 Windows 平台上，使用 C++ 开发桌面应用虽然有很多选择，比如 `API, MFC, WTL, QT, DirectUI` 等等，但实际上你很难选择一个**现代的 UI 框架**，比如自适应大小，高 `DPI` 支持，Emoji 支持。开发者干了大量的脏活累活，然后做出来的界面效果差强人意。Privexec 最初只是我自己开发好玩的一个工具（目前大概依然是），界面上只要极简就行了，而 Windows 10 <sup>5</sup>改进了基于对话框应用高 DPI 的支持，为了支持我的 `Surface Pro 4` ，于是选择了使用对话框做主界面。 界面如下：

![Privexec](https://github.com/M2Team/Privexec/raw/master/docs/images/appcontainer.png)

在此次改造之前 Privexec 使用传统的 `Win32 API+ 全局变量` 这样的代码逻辑，在不断添加功能后发现代码越来越杂乱无章，于是就用 C++ 类重构了，大量使用 C++17 代码。代码地址为：[https://github.com/M2Team/Privexec/tree/master/Privexec](https://github.com/M2Team/Privexec/tree/master/Privexec)。

我们都知道，目前 C 的 API 支持回调函数的一定只能使用全局函数或者静态成员函数。在使用 C++ 面向对象开发 UI 程序时，需要在窗口消息回调函数中先获取对象的 `this` 指针，然后再调用对象的事件处理程序。方案由多种，比如 ATL 使用的 `Thunk`<sup>6</sup>，还有通过 `GWLP_USERDATA` 去传递 `this` 指针，这种用户回调携带 `this` 指针的做法简直太常见了。

ATL Thunk 相关的文章有：[https://www.hackcraft.net/cpp/windowsThunk/](https://www.hackcraft.net/cpp/windowsThunk/)

实际上，Privexec 并不需要学习 ATL/WTL 那样，使用 `Thunk` 技术，完全没有到那个程序，所以我使用了 `GWLP_USERDATA` 的做法：

```c++
INT_PTR WINAPI App::WindowProc(HWND hWnd, UINT message, WPARAM wParam,
                               LPARAM lParam) {
  App *app{nullptr};
  if (message == WM_INITDIALOG) {
    auto app = reinterpret_cast<App *>(lParam);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    app->Initialize(hWnd);
  } else if ((app = GetThisFromHandle(hWnd)) != nullptr) {
    return app->MessageHandler(message, wParam, lParam);
  }
  return FALSE;
}

```

Privexec 使用了 `pugixml` 用于解析 `AppManifest`，使用 `json.hpp` 解析 Alias 配置文件。

## WSUDO

WSUDO 是 Priexec 的命令行版本。

![WSUDO](https://github.com/M2Team/Privexec/raw/master/docs/images/wsudo.png)

## Details

1.   [Feature Request: AppContainer "Capabilities" Selection](https://github.com/M2Team/Privexec/issues/2)
2.   [IDA Support: Freeware Version](https://www.hex-rays.com/products/ida/support/download_freeware.shtml)
3.   [Vista UAC: The Definitive Guide](https://www.codeproject.com/Articles/19165/Vista-UAC-The-Definitive-Guide)
4.   [Pico Process Overview](https://blogs.msdn.microsoft.com/wsl/2016/05/23/pico-process-overview/)
5.   [High-DPI Scaling Improvements for Desktop Applications in the Windows 10 Creators Update (1703)](https://blogs.windows.com/buildingapps/2017/04/04/high-dpi-scaling-improvements-desktop-applications-windows-10-creators-update/#GhtloWCUWO8rEeRG.97)
6.   [Thunk](https://en.wikipedia.org/wiki/Thunk)