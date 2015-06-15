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
使用这些接口创建一个计划任务，值得注意的是，计划任务的创建需要管理员权限运行，本程序的功能就是从管理员降权到标准用户，所以这个限制没有影响。  
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
简单点就是拿桌面进程的 Token，然后使用桌面的 Token 启动进程。这需要桌面正在运行 （Explorer 作为桌面进程正在运行），也就是常说的桌面得存在，并且权限是标准的。

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

当然，通过人肉合成一个 Token 启动进程也是能够实现降低程序权限的，这些比较复杂，本文也就不细说了。

##启动低完整性进程
强制完整性控制（英语：Mandatory Integrity Control）是一个在微软Windows操作系统中从Windows Vista开始引入，并沿用到后续版本系统的核心安全功能。强制完整性控制通过完整性级别标签来为运行于同一登录会话的进程提供隔离。此机制的目的是在一个潜在不可信的上下文（与同一账户下运行的其他较为可信的上下文相比）中选择性地限制特定进程和软件组件的访问权限。
Windows Vista 定义了四个完整性级别:

>低 (SID: S-1-16-4096)   
>中 (SID: S-1-16-8192)   
>高 (SID: S-1-16-12288)   
>系統 (SID: S-1-16-16384)   

利用这一特性，我们可以使用低级别权限启动一个进程:

{% highlight cpp %}
#include <Windows.h>
#include <Sddl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <wchar.h>
#include <iostream>


#pragma comment(lib,"kernel32")
#pragma comment(lib,"Advapi32")
#pragma comment(lib,"user32")

BOOL WINAPI CreateLowLevelProcess(LPCWSTR lpCmdLine) {
  BOOL b;
  HANDLE hToken;
  HANDLE hNewToken;
  // PWSTR szProcessName = L"LowClient";
  PWSTR szIntegritySid = L"S-1-16-4096";
  PSID pIntegritySid = NULL;
  TOKEN_MANDATORY_LABEL TIL = {0};
  PROCESS_INFORMATION ProcInfo = {0};
  STARTUPINFOW StartupInfo = {0};
  StartupInfo.cb=sizeof(STARTUPINFOW);
  ULONG ExitCode = 0;

  b = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken);
  if(!b)
      return FALSE;
  b = DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation,
                       TokenPrimary, &hNewToken);
  b = ConvertStringSidToSidW(szIntegritySid, &pIntegritySid);
  TIL.Label.Attributes = SE_GROUP_INTEGRITY;
  TIL.Label.Sid = pIntegritySid;

  // Set process integrity levels
  b = SetTokenInformation(hNewToken, TokenIntegrityLevel, &TIL,
                          sizeof(TOKEN_MANDATORY_LABEL) +
                              GetLengthSid(pIntegritySid));

  // Set process UI privilege level
  /*b = SetTokenInformation(hNewToken, TokenIntegrityLevel,
  &TIL, sizeof(TOKEN_MANDATORY_LABEL) + GetLengthSid(pIntegritySid)); */
  wchar_t *lpCmdLineT = _wcsdup(lpCmdLine);
  // To create a new low-integrity processes
  b = CreateProcessAsUserW(hNewToken, NULL, lpCmdLineT, NULL, NULL, FALSE, 0,
                          NULL, NULL, &StartupInfo, &ProcInfo);
  CloseHandle(hToken);  
  CloseHandle(hNewToken);
  CloseHandle(ProcInfo.hThread);
  CloseHandle(ProcInfo.hProcess);
  LocalFree(pIntegritySid);
  free(lpCmdLineT);
  return b;
}

int wmain(int argc,wchar_t *argv[])
{
    if(argc>=2)
    {
        std::wcout<<L"Start LowLevel App: "<<argv[1]<<L"\t Return Code[BOOL]: "<<CreateLowLevelProcess(argv[1])<<std::endl;
    }
    return 0;
}

{% endhighlight %}

第一步获得当前进程的  Token ,然后使用这个令牌创建一个新的令牌，由 SID "S-1-16-4096" 得到一个 SID 指针，将 SID 指针添加到 TOKEN_MANDATORY_LABEL 结构中，而后用SetTokenInformation将令牌与 完整性级别结合在一起，最后使用CreateProcessAsUser 创建进程。通过完整性级别启动的进程是没有多少权限的，譬如打开一个记事本，新建一个文件另存为，基本上都无法写入。 使用 Process Explorer 可以查看启动进程的权限属性。 

![MIC](https://raw.githubusercontent.com/fstudio/Phoenix/master/doc/Container/Images/LowLevelSava.png)

强制完整性运用最多的应该是 IE 浏览器，从 IE8 开始，IE 浏览器的保护模式就是 MIC，而 MIC 是 Windows 权限细粒度的一次重大的发展，在前几年，在学校开发 ACM 在线测评系统之时，评测系统就是基于 MIC+Job Object 实现的。

##AppConatiner
从 Windows 8 开始，微软引入了新的安全机制，AppConatiner 所有的 Store App 就是运行在应用容器之中，并且 IETab 也是运行在应用容器之中，应用容器在权限的管理上非常细致，也就是说非常“细粒度”。
微软也为传统的Desktop应用程序提供了一系列的API来创建一个AppContainer，并且使进程在AppContainer中启动。比如使用CreateAppContainerProfile创建一个容器SID，使用DeleteAppContainerProfile查找一个已知容器名的SID，删除一个容器DeleteAppContainerProfile配置文件。GetAppContainerFolderPath 获得容器目录。

通过 AppContainer 启动进程的一般流程是，通过 CreateAppContainerProfile 创建一个容器配置，得到 SID 指针，为了避免创建失败，先用 DeleteAppContainerProfile 删除此容器配置。细粒度的配置需要 [WELL_KNOWN_SID_TYPE](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379650(v=vs.85).aspx)    
得到容器配置后，启动进程时需要使用 STARTUPINFOEX 结构，使用 InitializeProcThreadAttributeList UpdateProcThreadAttribute 将 PSID 和 SECURITY_CAPABILITIES::Capabilities （也就是 WELL_KNOWN_SID_TYPE 得到的权限设置）添加到 STARTUPINFOEX::lpAttributeList 
使用 CreateProcess 中第七个参数 添加 EXTENDED_STARTUPINFO_PRESENT，然后再用 reinterpret_cast 转换 STARTUPFINFOEX 指针变量输入到 CreateProcess 倒数第二个（C语言用强制转换）。

下面是一个完整的例子。

{% highlight cpp %}
#include <vector>
#include <memory>
#include <type_traits>
#include <Windows.h>
#include <sddl.h>
#include <Userenv.h>
#include <iostream>

#pragma comment(lib,"Userenv")
#pragma comment(lib,"Shlwapi")
#pragma comment(lib,"kernel32")
#pragma comment(lib,"user32")
#pragma comment(lib,"Advapi32")
#pragma comment(lib,"Ole32")
#pragma comment(lib,"Shell32")

typedef std::shared_ptr<std::remove_pointer<PSID>::type> SHARED_SID;

bool SetCapability(const WELL_KNOWN_SID_TYPE type, std::vector<SID_AND_ATTRIBUTES> &list, std::vector<SHARED_SID> &sidList) {
  SHARED_SID capabilitySid(new unsigned char[SECURITY_MAX_SID_SIZE]);
  DWORD sidListSize = SECURITY_MAX_SID_SIZE;
  if (::CreateWellKnownSid(type, NULL, capabilitySid.get(), &sidListSize) == FALSE) {
    return false;
  }
  if (::IsWellKnownSid(capabilitySid.get(), type) == FALSE) {
    return false;
  }
  SID_AND_ATTRIBUTES attr;
  attr.Sid = capabilitySid.get();
  attr.Attributes = SE_GROUP_ENABLED;
  list.push_back(attr);
  sidList.push_back(capabilitySid);
  return true;
}

static bool MakeWellKnownSIDAttributes(std::vector<SID_AND_ATTRIBUTES> &capabilities,std::vector<SHARED_SID> &capabilitiesSidList)
{

    const WELL_KNOWN_SID_TYPE capabilitiyTypeList[] = {
        WinCapabilityInternetClientSid, WinCapabilityInternetClientServerSid, WinCapabilityPrivateNetworkClientServerSid,
        WinCapabilityPicturesLibrarySid, WinCapabilityVideosLibrarySid, WinCapabilityMusicLibrarySid,
        WinCapabilityDocumentsLibrarySid, WinCapabilitySharedUserCertificatesSid, WinCapabilityEnterpriseAuthenticationSid,
        WinCapabilityRemovableStorageSid,
    };
    for(auto type:capabilitiyTypeList) {
        if (!SetCapability(type, capabilities, capabilitiesSidList)) {
            return false;
        }
    }
    return true;
}


HRESULT AppContainerLauncherProcess(LPCWSTR app,LPCWSTR cmdArgs,LPCWSTR workDir)
{
    wchar_t appContainerName[]=L"Phoenix.Container.AppContainer.Profile.v1.test";
    wchar_t appContainerDisplayName[]=L"Phoenix.Container.AppContainer.Profile.v1.test\0";
    wchar_t appContainerDesc[]=L"Phoenix Container Default AppContainer Profile  Test,Revision 1\0";
    DeleteAppContainerProfile(appContainerName);///Remove this AppContainerProfile
    std::vector<SID_AND_ATTRIBUTES> capabilities;
    std::vector<SHARED_SID> capabilitiesSidList;
    if(!MakeWellKnownSIDAttributes(capabilities,capabilitiesSidList))
        return S_FALSE;
    PSID sidImpl;
    HRESULT hr=::CreateAppContainerProfile(appContainerName,
        appContainerDisplayName,
        appContainerDesc,
        (capabilities.empty() ? NULL : &capabilities.front()), capabilities.size(), &sidImpl);
    if(hr!=S_OK){
        std::cout<<"CreateAppContainerProfile Failed"<<std::endl;
        return hr;
    }
    wchar_t *psArgs=nullptr;
    psArgs=_wcsdup(cmdArgs);
    PROCESS_INFORMATION pi;
    STARTUPINFOEX siex = { sizeof(STARTUPINFOEX) };
    siex.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    SIZE_T cbAttributeListSize = 0;
    BOOL bReturn = InitializeProcThreadAttributeList(
        NULL, 3, 0, &cbAttributeListSize);
    siex.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, cbAttributeListSize);
    bReturn = InitializeProcThreadAttributeList(siex.lpAttributeList, 3, 0, &cbAttributeListSize);
    SECURITY_CAPABILITIES sc;
    sc.AppContainerSid = sidImpl;
    sc.Capabilities = (capabilities.empty() ? NULL : &capabilities.front());
    sc.CapabilityCount = capabilities.size();
    sc.Reserved = 0;
    if(UpdateProcThreadAttribute(siex.lpAttributeList, 0,
        PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES,
        &sc,
        sizeof(sc) ,
        NULL, NULL)==FALSE)
    {
        goto Cleanup;
    }
    BOOL bRet=CreateProcessW(app, psArgs, nullptr, nullptr,
        FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, workDir, reinterpret_cast<LPSTARTUPINFOW>(&siex), &pi);
    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
Cleanup:
    DeleteProcThreadAttributeList(siex.lpAttributeList);
    DeleteAppContainerProfile(appContainerName);
    free(psArgs);
    FreeSid(sidImpl);
    return hr;
}

int wmain(int argc,wchar_t *argv[])
{
    if(argc>=2)
    {
        std::wcout<<L"Start AppContainer App: "<<argv[1]<<L"\t Return Code[HRESULT]: "<<AppContainerLauncherProcess(nullptr,argv[1],nullptr)<<std::endl;
    }
    return 0;
}
{% endhighlight %}
使用 Process Explorer 查看进程属性可得到下图：
![AppContainer](https://raw.githubusercontent.com/fstudio/Phoenix/master/doc/Container/Images/appcontainer.png)   

当我们操作时，可以看到如下结果：
![Open](https://raw.githubusercontent.com/fstudio/Phoenix/master/doc/Container/Images/appcontainer-open.png)   

除了 IE ,Google 的开源浏览器 Chromium 也在沙箱代码中添加了 AppContainer 的支持：  
[http://src.chromium.org/chrome/trunk/src/sandbox/win/src/app_container.cc](http://src.chromium.org/chrome/trunk/src/sandbox/win/src/app_container.cc)  

Windows 的系统安全正得益于以上种种技术的出现，程序不是说想干什么就能干什么了。危险的系统操作被限制，特别是 AppContainer ，应用之间的隔离加深，跨应用的数据难以窃取。

##其他
很多开发者在 Windows 上使用沙箱来实现安全隔离，而沙箱本质上也是利用权限隔离以及 Hook 之类的技术来实现。而容器则可以在权限隔离的基础上添加资源限制来实现，类似于作业对象限制资源，当然，如果要更加安全，隔离更加深入，必须从内核上做出努力。
开发者并不一定要专注在容器，安全上，然而一定不要滥用权限。

##备注：
1. 用户账户控制(UAC): [https://en.wikipedia.org/wiki/User_Account_Control](https://en.wikipedia.org/wiki/User_Account_Control)    
2. 资源管理器在开启内置管理员的批准模式下降权是成功的，据说也是采用的计划任务降权？      

