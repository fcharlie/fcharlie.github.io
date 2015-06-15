---
layout: post
title:  "Windows AppContainer 降权，隔离与安全"
date:   2015-06-11 16:58:16
categories: container
---

##Windows 权限策略的发展
从 Windows 8开始，我在使用 Windows 系统的同时也就不再关闭 UAC 了，并且不再安装任何国产的安全软件，这些软件仗着运行在管理员权限上肆意的推行
“全家桶策略”，Windows 多年来一直是最流行的操作系统，大多数人的焦点都会放在上面，也包括黑客，各种企业，早期Windows系统在权限的管理上非常粗放。
无论是恶意软件还是其他软件都可以获得较高的权限，这样就能够对系统大肆修改，并且直接或间接破化系统，收集数据，妨碍竞争对手。   
软件的权限理应得到限制，而不是放任自流。
Windows XP 是历史上最受欢迎的版本之一，然而，一直以来XP的权限问题都被人诟病，微软也决心对这一问题进行改进，从Vista开始，Windows引入了 UAC 机制，
它要求用户在执行可能会影响计算机运行的操作或执行更改影响其他用户的设置的操作之前，提供权限或管理员‌密码。这是一个可喜的进步，不过在早期用户都会
要求关闭 UAC,当我开始使用 Windows 的时候，，那个时候用的是 Windows 7,我也是这样做的。Windows 7在 UAC 的改进主要是一些小的细节。    
从 Windows 8 引入 Metro(Morden) App 开始，Windows 出现了一个新的进程隔离机制，即 AppContainer。Windows Store 应用运行在 AppContainer 的容器当中
权限被极大的限制，很多危险的操作无法进行，微软通过 Windows Store 进行应用分发，能够控制来源，这样能够极大的降低恶意软件的困扰。   
而 AppContainer 同样能够支持传统的 Desktop 应用，本文将介绍 通过 AppContainer 启动一个桌面程序，当然，先从降权说起。

##UAC 降权
基于 Win32 的应用程序，如果要提权，非常简单，第一可以在 manifest 文件中写入 'requireAdministrator' Visual Studio 项目属性中可以设置。

{% highlight xml %}
<?xml version="1.0" encoding="utf-8"?>
<asmv1:assembly manifestVersion="1.0" xmlns="urn:schemas-microsoft-com:asm.v1" xmlns:asmv1="urn:schemas-microsoft-com:asm.v1" xmlns:asmv2="urn:schemas-microsoft-com:asm.v2" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <assemblyIdentity version="1.0.3.0" name="Force.Metro.Native.iBurnMgr.app"/>
 <trustInfo xmlns="urn:schemas-microsoft-com:asm.v2">
    <security>
      <requestedPrivileges xmlns="urn:schemas-microsoft-com:asm.v3">
        <requestedExecutionLevel level='requireAdministrator' uiAccess='false' />
      </requestedPrivileges>
    </security>
  </trustInfo>
</asmv1:assembly>
{% endhighlight %}
    
第二，可以使用 ShellExecute，将第二个参数设置为 “runas” 即可，ShellExecute 本质上将参数写入到 SHELLEXECUTEINFO ，然后调用 ShellExecuteEx 实现。
在 ReactOS 和 Windows Vista 以前的版本，通过 IShellExecuteHook 接口实现。Vista 以后被否决了。

当然，还可以有其他不受官方建议的方法。

但是如果需要降权，微软没有直接的方案供开发者选择。常见的选择有 通过拿到 Explorer 的 token 启动进程，或者是通过计划任务启动进程。

##计划任务降权
使用计划任务降权大概需要十几个 COM 接口:

{% highlight cpp %}
  ITaskService *iTaskService = nullptr;
  ITaskFolder *iRootFolder = nullptr;
  ITaskDefinition *iTask = nullptr;
  IRegistrationInfo *iRegInfo = nullptr;
  IPrincipal *iPrin = nullptr;
  ITaskSettings *iSettings = nullptr;
  ITriggerCollection *iTriggerCollection = nullptr;
  ITrigger *iTrigger = nullptr;
  IRegistrationTrigger *iRegistrationTrigger = nullptr;
  IActionCollection *iActionCollection = nullptr;
  IAction *iAction = nullptr;
  IExecAction *iExecAction = nullptr;
  IRegisteredTask *iRegisteredTask = nullptr;

{% endhighlight %}   
使用这些接口创建一个计划任务，值得注意的是，计划任务的创建需要管理员权限运行，然而本程序的功能就是从管理员降权到标准用户，所以这个设定每一点影响。  
在这个过程中最有价值的代码是：  
{% highlight cpp %}  
 DO(iPrin->put_RunLevel(TASK_RUNLEVEL_LUA))
{% endhighlight %}    
[MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/aa383572(v=vs.85).aspx) 的描述中，表示以较低权限运行，与之对应的是 “TASK_RUNLEVEL_HIGHEST”。 
通过计划任务降权的完整代码： [UAC 降权测试](https://github.com/fstudio/Phoenix/blob/master/test/Container/uacdown.cpp) 
从 Process Explorer 的进程属性就可以看到：
![TaskSchdLauncher](https://raw.githubusercontent.com/fstudio/Phoenix/master/doc/Container/Images/taskschdlauncher.png)
如果用户名是内置的 Administrator，并且开启了 [对内置管理员使用批准模式](https://technet.microsoft.com/zh-cn/library/dd834795.aspx) 至少在Windows 8.1 (Windows Server 2012 R2)会失败的。
所以这个时候需要采取第二种策略。

##使用 Explorer 的 Token 启动进程
简单点就是拿桌面进程的 Token，然后使用桌面的 Token 启动进程。

{% highlight cpp %}
HRESULT WINAPI ProcessLauncherExplorerLevel(LPCWSTR exePath,LPCWSTR cmdArgs,LPCWSTR workDirectory)
{
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  HANDLE hShellProcess = nullptr, hShellProcessToken = nullptr,
  hPrimaryToken = nullptr;
  HWND hwnd = nullptr;
  DWORD dwPID = 0;
  HRESULT hr = S_OK;
  BOOL ret = TRUE;
  DWORD dwLastErr;
  hwnd = GetShellWindow();
  if (nullptr == hwnd) {
    return HRESULT(3);
  }

  GetWindowThreadProcessId(hwnd, &dwPID);
  if (0 == dwPID) {
    return HRESULT(4);
  }

  // Open the desktop shell process in order to query it (get the token)
  hShellProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID);
  if (!hShellProcess) {
    dwLastErr = GetLastError();
    return HRESULT(5);
  }

  // From this point down, we have handles to close, so make sure to clean up.

  bool retval = false;
  // Get the process token of the desktop shell.
  ret = OpenProcessToken(hShellProcess, TOKEN_DUPLICATE, &hShellProcessToken);
  if (!ret) {
    dwLastErr = GetLastError();
    hr = HRESULT(6);
    goto cleanup;
  }

  // Duplicate the shell's process token to get a primary token.
  // Based on experimentation, this is the minimal set of rights required for
  // CreateProcessWithTokenW (contrary to current documentation).
  const DWORD dwTokenRights = TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY |
                              TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT |
                              TOKEN_ADJUST_SESSIONID;
  ret = DuplicateTokenEx(hShellProcessToken, dwTokenRights, nullptr,
                         SecurityImpersonation, TokenPrimary, &hPrimaryToken);
  if (!ret) {
    dwLastErr = GetLastError();
    hr = 7;
    goto cleanup;
  }
  // Start the target process with the new token.
  wchar_t *cmdArgsT = _wcsdup(cmdArgs);
  ret = CreateProcessWithTokenW(hPrimaryToken, 0, exePath, cmdArgsT, 0, nullptr,
                                workDirectory, &si, &pi);
  free(cmdArgsT);
  if (!ret) {
    dwLastErr = GetLastError();
    hr = 8;
    goto cleanup;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  retval = true;

cleanup:
  // Clean up resources
  CloseHandle(hShellProcessToken);
  CloseHandle(hPrimaryToken);
  CloseHandle(hShellProcess);
  return hr;
}
{% endhighlight %}

##启动低完整性进程


##AppConatiner





##备注：
1. 用户账户控制(UAC): [https://en.wikipedia.org/wiki/User_Account_Control](https://en.wikipedia.org/wiki/User_Account_Control)    
2. 资源管理器在开启内置管理员的批准模式下降权是成功的，据说也是采用的计划任务降权？      
