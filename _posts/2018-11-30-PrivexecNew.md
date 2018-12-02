---
layout: post
title:  "Privexec 杂谈"
date:   2018-11-30 21:00:00
published: true
categories: windows
---

# 前言

本站的开篇就是讲的 [《Windows AppContainer 降权，隔离与安全》](https://forcemz.net/container/2015/06/12/AppContainer/)，一晃三年多过去了，这三年之中，我开发了一个 [Privexec](https://github.com/M2Team/Privexec)，一个以其他权限启动进程的工具，支持启动 `AppContainer` 进程，前段实现有用户发起了功能请求<sup>1</sup>，让 `Privexec` 支持设置 `AppContainer` 的 `Capabilities`，而不是像以前一样在启动 `AppContainer` 进程时使用 `CreateWellKnownSid` 创建所有的与 `AppContainer` 相关的 `Capabilities SIDs`。于是乎，我就花了一点时间将 Privexec 重构了一番，有所感悟，便将其写下来。

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

而内核中创建进程的细节讲起来篇幅过长，大家可以参考 《深入解析 Windows 操作系统 第六版（上册）》P364 *CreateProcess的流程* 。

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
Shell 在启动清单附带有 UAC 提权的可执行文件时也会发生提权。以 `Windows 10` 为例，提权最终由 `AicLaunchAdminProcess` 函数实现，此函数目前实现在 `Windows.Storage.dll` 中，UAC 提权需要与 `Appinfo` 服务通信，`Appinfo` 验证提权行为后使用 `CreateProcessAsUserW` 启动进程，并将其父进程设置为 `ShellExecute` 调用者。调用细节(Windows Vista)<sup>3</sup>如下：

1.   AppInfo goes and talks to the Local Security Authority to get the elevated token of the logged in user of Session 1.
2.   AppInfo loads up a STARTUPINFOEX structure (new to Vista), and calls the brand new Vista API InitializeProcThreadAttributeList() with room for one attribute.
3.   OpenProcess() is called to get a handle to the process that initiated the RPC call.
4.   UpdateProcThreadAttribute() is called with PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, and uses the handle retrieved in step 3.
5.   CreateProcessAsUser() is called with EXTENDED_STARTUPINFO_PRESENT and the results of steps 1 and 4.
6.   DeleteProcThreadAttributeList() is called.
7.   Results are gathered, and handles are cleaned up.

Appinfo 服务描述：
>使用辅助管理权限便于交互式应用程序的运行。如果停止此服务，用户将无法使用辅助管理权限启动应用程序，而执行所需用户任务可能需要这些权限。

当我们了解到 Windows 创建进程的粗略细节，那么就可以对 Privexec 创建其他权限的进程做出封装了。Privexec 支持的权限类型有：

+   AppContainer
+   Mandatory Integrity Control
+   No Elevated(UAC)
+   Administrator
+   System
+   TrustedInstaller

**AppContainer** 通常可以使用 `CreateProcess` 创建，使用 `EXTENDED_STARTUPINFO_PRESENT` 额外的 `dwCreateFlags` 创建权限为 `AppContainer` 的进程即可。AppContainer 配置需由 `CreateAppContainerProfile` 创建，相应的 `Capability SID` 可由 `DeriveCapabilitySidsFromName` 或者 `CreateWellKnownSid` 创建，而 `Privexec(GUI)` 受限与 UI 限制，目前仅支持 9 个 `WellKnownSid`，而 `wsudo` 支持 `--appx` 从文件中设置，`wsudo` 中使用了 `DeriveCapabilitySidsFromName` 创建  `Capabilities SID`。

参数 `EXTENDED_STARTUPINFO_PRESENT` 表示使用 `STARTUPINFOEX` 结构，使用 `InitializeProcThreadAttributeList` 初始化 `STARTUPINFOEX::lpAttributeList`, 使用 `UpdateProcThreadAttribute` 设置 `lpAttributeList` 为 `PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES` 并将 `SECURITY_CAPABILITIES ` 绑定到 `STARTUPINFOEX` 上。

 **Mandatory Integrity Control** 主要使用 `SetTokenInformation` 将高完整性级别的 `Token` 设置为低完整性级别的 `Token` 然后使用  `CreateProcessAsUser` 创建进程。

 **No Elevated(UAC)** 当当前用户为管理员时需要使用计划任务启动 UAC 未提升进程，而计划任务主机进程也是使用 `CreateProcessAsUser` 启动非提升的进程。当当前用户不为管理员时，使用 `CreateProcess` 创建进程即可。

**Administrator** 当当前进程不为管理员，则需要 UAC 提权，细节前文由描述。反之则使用 `CreateProcess` 即可。 

**System** 当前进程必须是管理员权限等级及以上进程，需要开启 `SE_DEBUG_NAME` 权限，然后获得系统权限进程的 Token, 将自身权限模拟 `System` Token，然后拷贝自身 `Token` 修改 `Token` 为 `Primary Token`，即 `hPrimary`，因为只有主 Token 才能被用于创建子进程。《Windows Internal 7th Edition》作者之一 [Pavel Yosifovich](https://github.com/zodiacon) 就写了一个 sysrun 的例子：[sysrun](https://github.com/zodiacon/sysrun)。

**TrustedInstaller** 此权限严格意义上来说，是属于 **Windows Modules Installer** 服务的专有权限，**Windows Modules Installer** 的服务描述为：
>启用 Windows 更新和可选组件的安装、修改和移除。如果此服务被禁用，则此计算机的 Windows 更新的安装或卸载可能会失败。

因此要获得此权限，需要先模拟到 `System` 权限，然后启动 `TrustedInstaller` 服务，然后获得服务进程的权限句柄，以该句柄拷贝启动新的进程。

Privexec 的进程启动相关代码在：[https://github.com/M2Team/Privexec/tree/master/include/process](https://github.com/M2Team/Privexec/tree/master/include/process)，使用 C++17，利用 *Lambda*，*RIIA* 这样的功能可以轻易的写出句柄安全的代码。

```c++
template <class F> class final_act {
public:
  explicit final_act(F f) noexcept : f_(std::move(f)), invoke_(true) {}

  final_act(final_act &&other) noexcept
      : f_(std::move(other.f_)), invoke_(other.invoke_) {
    other.invoke_ = false;
  }

  final_act(const final_act &) = delete;
  final_act &operator=(const final_act &) = delete;

  ~final_act() noexcept {
    if (invoke_)
      f_();
  }

private:
  F f_;
  bool invoke_;
};

// finally() - convenience function to generate a final_act
template <class F> inline final_act<F> finally(const F &f) noexcept {
  return final_act<F>(f);
}

template <class F> inline final_act<F> finally(F &&f) noexcept {
  return final_act<F>(std::forward<F>(f));
}

bool filetodo(std::wstring_view file){
  auto hFile= CreateFileW(file.data(), 
  GENERIC_READ, FILE_SHARE_READ,
  nullptr, OPEN_EXISTING,
  FILE_ATTRIBUTE_NORMAL,
  nullptr);
  if (hFile==INVALID_HANDLE_VALUE) {
    return false;
  }
  auto closer = finally([&] {
    if (hFile!=INVALID_HANDLE_VALUE) {
      CloseHandle(hFile);
    }
  });
  /// some codes...
  return true;
}

```

回过头来说，你完全开发一个服务来实现以其他用户权限启动进程，然后封装一个命令，命令需要启动进程时与服务进行通信，在服务中使用 `CreateProcessAsUser` 启动新的进程。使用服务实现此功能时，需要避免不可信的提权发生。

[毛利](https://github.com/MouriNaruto) 的 [NSudo](https://github.com/M2Team/NSudo) 与 Privexec 类似，但实现基本上是使用 `CreateProcessAsUser` + `Token` 创建进程。

题外话：在 Windows 平台上，启动进程依然有不小的代价，中间环节多，而在 Unix 平台，启动进程有 `fork/exec`，实际上要实现类似 `CreateProcess` 之类的逻辑需要使用 `fork/exec` 联合使用。现实带来了遗憾，Windows 上实现 `fork` 和 Unix 实现更高效的 `CreateProcess` 成了两个大难题。当然，在实现 `Windows Subsystem for Linux`<sup>4</sup>，微软也在改进其创建进程的流程，但目前为止 `Minimal process` 并没有让 `cygwin` 这样的系统受益。

![picoprocess](https://msdnshared.blob.core.windows.net/media/2016/05/picoProcess-1024x763.png)

## Privexec (GUI)

Privexec 狭义上是指 `Privexec (GUI)` 这个程序，现在应该指 `Privexec` 整个项目，包括 `wsudo`。在 Windows 平台上，使用 C++ 开发桌面应用虽然有很多选择，比如 `API, MFC, WTL, QT, DirectUI` 等等，但实际上基本没有一个 **现代的 UI 框架** 可以供你选择，上述 UI 框架或缺少自适应大小支持，或缺少高 `DPI` 支持，也可能缺少 Emoji 支持等等，另一方面不断解决依赖问题也让人厌倦。Privexec 使用 Win32 API 即可，保持极简风格也不错。Privexec 最初只是我自己开发好玩的一个工具（目前大概依然是），UI 开发选择权在我自己，而 Windows 10 <sup>5</sup>改进了基于对话框应用高 DPI 的支持，为了支持我的 `Surface Pro 4` ，于是选择了使用对话框做主界面。 界面如下：

![Privexec](https://github.com/M2Team/Privexec/raw/master/docs/images/appcontainer.png)

在此次改造之前 Privexec 使用传统的 `Win32 API+ 全局变量` 这样的代码逻辑，在不断添加功能后发现代码越来越杂乱无章，于是就用 C++ 类重构了，大量使用 C++17 代码。代码地址为：[https://github.com/M2Team/Privexec/tree/master/Privexec](https://github.com/M2Team/Privexec/tree/master/Privexec)。

我们都知道，目前 C 的 API 支持回调函数的一定只能使用全局函数或者静态成员函数。在使用 C++ 面向对象开发 UI 程序时，需要在窗口消息回调函数中先获取对象的 `this` 指针，然后再调用对象的事件处理程序。方案由多种，比如 ATL 使用的 `Thunk`<sup>6</sup>，还有通过 `GWLP_USERDATA` 去传递 `this` 指针，这种用户回调携带 `this` 指针的做法简直太常见了。

ATL Thunk 相关的文章有：[https://www.hackcraft.net/cpp/windowsThunk/](https://www.hackcraft.net/cpp/windowsThunk/)

实际上，Privexec 并不需要学习 ATL/WTL 那样，使用 `Thunk` 技术，完全没有到那个程度，所以我使用了 `GWLP_USERDATA` 的做法：

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

在 Windows 中，很多 API 实际上是由 `COM` 组件类提供，直接在 C++ 中使用 COM 对象需要小心翼翼避免资源泄漏，到了 8102 年，你应该使用智能指针或者 `ComPtr` 去包裹 COM 对象，或者使用类似下面的代码，使用 RAII 来避免资源泄漏。

```c++
template <class T> class comptr {
public:
  comptr() { ptr = NULL; }
  comptr(T *p) {
    ptr = p;
    if (ptr != NULL)
      ptr->AddRef();
  }
  comptr(const comptr<T> &sptr) {
    ptr = sptr.ptr;
    if (ptr != NULL)
      ptr->AddRef();
  }
  T **operator&() { return &ptr; }
  T *operator->() { return ptr; }
  T *operator=(T *p) {
    if (*this != p) {
      ptr = p;
      if (ptr != NULL)
        ptr->AddRef();
    }
    return *this;
  }
  operator T *() const { return ptr; }
  template <class I> HRESULT QueryInterface(REFCLSID rclsid, I **pp) {
    if (pp != NULL) {
      return ptr->QueryInterface(rclsid, (void **)pp);
    } else {
      return E_FAIL;
    }
  }
  HRESULT CoCreateInstance(REFCLSID clsid, IUnknown *pUnknown,
                           REFIID interfaceId,
                           DWORD dwClsContext = CLSCTX_ALL) {
    HRESULT hr = ::CoCreateInstance(clsid, pUnknown, dwClsContext, interfaceId,
                                    (void **)&ptr);
    return hr;
  }
  ~comptr() {
    if (ptr != NULL)
      ptr->Release();
  }

private:
  T *ptr;
};
```

Privexec 使用了 `pugixml` 用于解析 `AppManifest`，使用 `json.hpp` 解析 Alias 配置文件。

在 Privexec 中的错误提示 `MessageBox` 实际上是使用 `TaskDialog` 编写，而关于对话框同样也是。查找文件/文件夹对话框使用的是 `IFileDialog`。

## WSUDO

WSUDO 是 Priexec 的命令行版本。很早就支持颜色高亮，在前文中曾写过一篇文章 [《Privexec 的内幕（一）标准输出原理与彩色输出实现》](https://forcemz.net/windows/2017/06/06/ColorConsole/) 对此有详细的介绍。此次重构 WSUDO，改进了命令行解析模式，可以通过命令行设置启动进程的**启动目录**，**环境变量**，**AppConatiner** 清单文件，具体的命令行内容如下：

![WSUDO](https://github.com/M2Team/Privexec/raw/master/docs/images/wsudo.png)

在创建进程时，如果 `CreateProcess` 的参数 [`lpEnvironment`](https://docs.microsoft.com/en-us/windows/desktop/api/processthreadsapi/nf-processthreadsapi-createprocessw) 为 `NULL` 时，子进程将继承父进程的环境变量，这与 `Unix` (`exec` 函数家族) 系统是一致的。如果我们要设置子进程的环境变量，我们可以修改 wsudo 的环境变量传递给子进程即可。这次，我给 WSUDO 添加了 `-e(--env)` flag. 可以使用 `-eK=V`，`-e K=V` `--env=K=V`，`--env K=V` 这样的方式设置环境变量。也可以在要启动的命令（Alias）之前以 `K=V` 的方式设置环境变量，这给设计思路来自于 `Unix Shell`。类似于 `make KEY=VALUE all` 

WSUDO 目前还支持 `--new-console` `--wait` 这样的 flag，在 Windows 中，创建 CUI 进程时，默认参数下，如果程序的子系统是 `Windows CUI`，如果父进程也是 `CUI` 程序就会继承父进程的控制台窗口，否则会创建一个新的控制台窗口。这就意味这 WSUDO 在启动 CUI 子进程时，CUI 子进程实际上的窗口也会继承 WSUDO 的当前窗口，而之前的 WSUDO 在子进程启动后就结束了，CMD/PowerShell 的等待也就结束了，这时候如果 CUI 子进程还活跃，可能对控制台窗口读写从而会导致控制台窗口输入输出紊乱。这个问题的解决方法有：

+   WSUDO 等待子进程结束
+   或者启动新控制台

而这次重构专门增加了 `--new-console` 和 `--wait`，并修改 WSUDO 的启动进程的默认行为，具体情形如下：

|子系统|--new-console|--wait|
|---|---|---|
|Windows CUI|默认关闭|默认开启|
|Windows GUI|N/A|默认关闭|

如果子进程子系统是 `Windows GUI` 且命令行参数包含 `--wait`，WSUDO 依然会等待子进程结束。

在启动子进程的之前，我们可以通过解析 `PE` 文件格式去感知可执行程序子系统是否是 `Windows CUI`，具体代码在：[PESubsystemIsConsole](https://github.com/M2Team/Privexec/blob/f9a2cbbfa57a3bda65e6c70b74e80b8cc67af333/include/pe.hpp#L130)

如果要支持 `AppExecLink(IO_REPARSE_TAG_APPEXECLINK)` 这样特殊的文件（这种文件类似于 Windows Symbolic File，是 UWP App 的程序的特殊链接文件。）需要解析重解析点，大致代码如下：

```c++
inline bool readlink(const std::wstring &symfile, std::wstring &realfile) {
  auto hFile = CreateFileW(
      symfile.c_str(), 0,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    return false;
  }
  ReparseBuffer rbuf;
  DWORD dwBytes = 0;
  if (DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, nullptr, 0, rbuf.data,
                      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwBytes,
                      nullptr) != TRUE) {
    CloseHandle(hFile);
    return false;
  }
  CloseHandle(hFile);
  switch (rbuf.data->ReparseTag) {
  case IO_REPARSE_TAG_SYMLINK: {
    auto wstr = rbuf.data->SymbolicLinkReparseBuffer.PathBuffer +
                (rbuf.data->SymbolicLinkReparseBuffer.SubstituteNameOffset /
                 sizeof(WCHAR));
    auto wlen = rbuf.data->SymbolicLinkReparseBuffer.SubstituteNameLength /
                sizeof(WCHAR);
    if (wlen >= 4 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' &&
        wstr[3] == L'\\') {
      /* Starts with \??\ */
      if (wlen >= 6 &&
          ((wstr[4] >= L'A' && wstr[4] <= L'Z') ||
           (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
          wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\')) {
        /* \??\<drive>:\ */
        wstr += 4;
        wlen -= 4;

      } else if (wlen >= 8 && (wstr[4] == L'U' || wstr[4] == L'u') &&
                 (wstr[5] == L'N' || wstr[5] == L'n') &&
                 (wstr[6] == L'C' || wstr[6] == L'c') && wstr[7] == L'\\') {
        /* \??\UNC\<server>\<share>\ - make sure the final path looks like */
        /* \\<server>\<share>\ */
        wstr += 6;
        wstr[0] = L'\\';
        wlen -= 6;
      }
    }
    realfile.assign(wstr, wlen);
  } break;
  case IO_REPARSE_TAG_MOUNT_POINT: {
    auto wstr = rbuf.data->MountPointReparseBuffer.PathBuffer +
                (rbuf.data->MountPointReparseBuffer.SubstituteNameOffset /
                 sizeof(WCHAR));
    auto wlen =
        rbuf.data->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
    /* Only treat junctions that look like \??\<drive>:\ as symlink. */
    /* Junctions can also be used as mount points, like \??\Volume{<guid>}, */
    /* but that's confusing for programs since they wouldn't be able to */
    /* actually understand such a path when returned by uv_readlink(). */
    /* UNC paths are never valid for junctions so we don't care about them. */
    if (!(wlen >= 6 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' &&
          wstr[3] == L'\\' &&
          ((wstr[4] >= L'A' && wstr[4] <= L'Z') ||
           (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
          wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\'))) {
      SetLastError(ERROR_SYMLINK_NOT_SUPPORTED);
      return false;
    }

    /* Remove leading \??\ */
    wstr += 4;
    wlen -= 4;
    realfile.assign(wstr, wlen);
  } break;
  case IO_REPARSE_TAG_APPEXECLINK: {
    if (rbuf.data->AppExecLinkReparseBuffer.StringCount != 0) {
      LPWSTR szString = (LPWSTR)rbuf.data->AppExecLinkReparseBuffer.StringList;
      for (ULONG i = 0; i < rbuf.data->AppExecLinkReparseBuffer.StringCount;
           i++) {
        if (i == 2) {
          realfile = szString;
        }
        szString += wcslen(szString) + 1;
      }
    }
  } break;
  default:
    return false;
  }
  return true;
}
```

WSUDO 还支持内置命令 `alias`，可以增加和删除别名。增加别名时如果别名存在则会被覆盖：

```batch
wsudo alias add ehs "notepad %SYSTEMROOT%/System32/drivers/etc/hosts" "Edit Hosts"
wsudo alias delete ehs
```

## Details

1.   [Feature Request: AppContainer "Capabilities" Selection](https://github.com/M2Team/Privexec/issues/2)
2.   [IDA Support: Freeware Version](https://www.hex-rays.com/products/ida/support/download_freeware.shtml)
3.   [Vista UAC: The Definitive Guide](https://www.codeproject.com/Articles/19165/Vista-UAC-The-Definitive-Guide)
4.   [Pico Process Overview](https://blogs.msdn.microsoft.com/wsl/2016/05/23/pico-process-overview/)
5.   [High-DPI Scaling Improvements for Desktop Applications in the Windows 10 Creators Update (1703)](https://blogs.windows.com/buildingapps/2017/04/04/high-dpi-scaling-improvements-desktop-applications-windows-10-creators-update/#GhtloWCUWO8rEeRG.97)
6.   [A thunk is a computer programming subroutine that is created, often automatically, to assist a call to another subroutine .](https://en.wikipedia.org/wiki/Thunk)