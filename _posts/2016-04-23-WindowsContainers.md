---
layout: post
title:  "Windows Containers 与 Project Centennial"
date:   2016-04-23 18:30:00
published: true
categories: container
---

# Windows Containers 与 Project Centennial

随着 Windows 10 Redstone 和 Windows Server 2016 发布日益临近,一些重要的功能也逐渐披露到公众面前。
最让笔者觉得有意思的是 Containers 功能。笔者曾经写过一篇关于应用程序降权的文章：
[Windows AppContainer 降权，隔离与安全](http://forcemz.net/container/2015/06/12/AppContainer/) ) 提到了
如何降低应用程序进程权限，与 Containers 相比，AppContainer 并未将进程与操作系统完全隔离开来，计算资源并不能自定义限制，
并且很多一部分 Win32 API 都无法在基于 AppContainer 创建的进程中运行。

Microsoft 为了丰富 Windows Store 应用的数量, 推出了几类 App Convert, 将非 UWP App 移植到 UWP 平台上来，其中有
Win32/COM 的 Project Centennial 即 DesktopAppConveter, DesktopAppConverter 的实现依赖 Windows Containers。


## Win32 的窘迫

从 Windows 8 开始，Windows 推出了 Store App, Store App 运行在一个 AppContainer 中， 操作系统通过 AppContainer 技术
实现了进程权限的隔离，由于 UAC 的限制，App 无法访问大多数的文件系统目录，也不能进行一些重要的操作，许多系统对进程 UAC 
权限存在依赖的 API 也无法被直接调用。一些系统的 Store App 只得依赖各种 Runtime Broker 通过 LPC 来实现重要的操作。

对于 Store App 来说，使用全新的 API 和不同以往的安全策略，在 AppContainer 中运行恰到好处，而 Win32 App 同样可以运行在
AppContainer 中 (在以前的文章中，我曾经写过一个如何启动基于 Windows AppConatiner 的传统 Win32 应用程序：
，不过，依赖传统配置和文件系统的应用程序如若使用 AppConatiner ，大量的功能将无法使用，这就使得 Windows 面临
一个困境，经历二十多年的积累，Win32 应用的存量已经非常可观，然而 Win32 应用却无法在 Store 中运行，Store 中 App 稀少。

对于传统桌面应用来说，使用虚拟机隔离又显得笨重，缓慢，使用 AppContainer 很多功能又无法直接使用，如果使用 Containers 技术，
那么是再好不过的选择了。

另一方面，Windows 之前并没有一个官方的 Containers 技术，特别是随着 Azure 云服务的推出，微软迫切需要一个 Win32 的 Containers 技术。

在 Windows 10 Redstone Insider 14316 中，只存在于 Windows Server 2016 TP 的 Containers 作为一个可选的功能被添加到了 企业版镜像中，
在 Windows 10 Redstone Insider 14328 中，Containers 被添加到专业版功能当中。笔者才有幸去了解其中的奥秘。

在 Windows Containers 被推出之前，有个基于 Windows Job Object 的 Containers 方案是WinDocks。

## Containers 之前

Windows 有一个重要的功能，即 Windows Job Object, 即 Windows 作业,作业可以将一组进程当作单个实体来处理，作业能够限制进程的 CPU 时间，
内存大小，以及 CPU 亲和性，在 Windows 平台上，有些沙箱应用就使用了作业，而 Visual Studio 在构建项目时使用了作业，当用户取消构建时，
作业中的进程都被终止，也就是多进程编译可以被终止。

这其实也是与 Windows NT 的进程模型有关，NT 系统并未维护进程之间的父子关系，这与 POSIX 系统存在比较明显的区别。

容器隔离的重点是文件系统的隔离，NTFS 是一种分层文件系统，支持符号链接和硬链接，这些是文件系统虚拟化的基石，在 Windows 64 发布之初，
Windows 便支持了文件系统的重定向，即 SysWOW64 ，对于运行在 Windows 64 位系统的 32 位应用程序，如果访问 System32 目录中的文件会被
定向到 SysWOW64 目录下，所以我们可以发现，32位应用在 64位系统打开的 cmd 也是32位的。

Windows 还可以通过卷挂载，软链接等策略是先文件系统的虚拟化和隔离，为了兼容 XP 以前的应用程序，Windows 创建了 C:\User 的软链接 
C:\Documents and Settings 。

从 Office 2013 起，Office 的安装采用 VFS 机制安装，目录 C:\Program Files\Microsoft Office\root\VFS 目录便是 VFS 的 C:\，
而 DesktopAppConverter 多少是吸收了 Office 的经验的。

在 Windows 中，注册表的重要性不言而喻，与 Linux containers 一样，配置文件的隔离一样是非常重要的，Windows 注册表内部存储表现为储巢（Hive）,
注册表存储时并不是一个巨大的，单一的文件，而是分布在不同目录的配置文件，操作系统只需要单独挂载特定的注册表数据就可以实现注册表的隔离。

注册表 Hive List:

>HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\hivelist

重定向 hive

>HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\hiveredirectionlist

关于注册表，运行在64位系统中的32位应用程序访问注册表时，默认情况下会被定位到 WOW6432Node 键下。


## Windows Containers 窥视

Windows 大多数功能通过动态链接库提供，也就是通常说的 dll, 我们可以使用 [Dependency Walker](http://www.dependencywalker.com/) 去了解
这些 dll 的导入导入函数，从而观察到 dll 主要的功能。也可以使用 IDA Pro 之类的专业工具。

Windows Containers 相关的 dll 有 container.dll container_xml.dll CCG.dll 其中 container.dll 实现了容器的创建，运行，修改，和删除等功能。
从附注中我们可以发现，container 主要是 C++ 实现，然后使用 C 导出 API 函数。

我们可以推测 WcCreateContainers 的原型
{% highlight c++ %}
BOOL WINAPI WcCreateContainers(LPHANDLE lppContainer,LPCWSTR destName,BOOL flags);
{% endhighlight %}

在 C:\Windows\System32\containers 目录下，有关 Windows Object (通常是 HANDLE) 对象的重定向

而 CCG.exe CCGLaunchPad.dll 则是 Windows Container Credential Guard Server

在 WOW64 系统中，同样存在 Container.dll ，说明 Container 技术在 Win32 中是存在的。

## 附注

{% highlight text %}
/// container.dll Import function
Address          Ordinal Name                                                                                                    Library                                  
-------          ------- ----                                                                                                    -------                                  
000000001002A000         FilterConnectCommunicationPort                                                                          FLTLIB                                   
000000001002A004         FilterLoad                                                                                              FLTLIB                                   
000000001002A008         FilterSendMessage                                                                                       FLTLIB                                   
000000001002A00C         FilterDetach                                                                                            FLTLIB                                   
000000001002A010         FilterAttach                                                                                            FLTLIB                                   
000000001002A018         CreateCompartment                                                                                       IPHLPAPI                                 
000000001002A01C         DeleteCompartment                                                                                       IPHLPAPI                                 
000000001002A020         InitializeCompartmentEntry                                                                              IPHLPAPI                                 
000000001002A024         SetJobCompartmentId                                                                                     IPHLPAPI                                 
000000001002A028         GetJobCompartmentId                                                                                     IPHLPAPI                                 
000000001002A030         UuidToStringW                                                                                           RPCRT4                                   
000000001002A034         RpcStringFreeW                                                                                          RPCRT4                                   
000000001002A03C         IsDebuggerPresent                                                                                       api-ms-win-core-debug-l1-1-1             
000000001002A040         OutputDebugStringW                                                                                      api-ms-win-core-debug-l1-1-1             
000000001002A048         UnhandledExceptionFilter                                                                                api-ms-win-core-errorhandling-l1-1-1     
000000001002A04C         SetUnhandledExceptionFilter                                                                             api-ms-win-core-errorhandling-l1-1-1     
000000001002A050         SetLastError                                                                                            api-ms-win-core-errorhandling-l1-1-1     
000000001002A054         GetLastError                                                                                            api-ms-win-core-errorhandling-l1-1-1     
000000001002A05C         GetVolumePathNameW                                                                                      api-ms-win-core-file-l1-2-1              
000000001002A060         CreateFileW                                                                                             api-ms-win-core-file-l1-2-1              
000000001002A064         GetVolumeNameForVolumeMountPointW                                                                       api-ms-win-core-file-l1-2-1              
000000001002A06C         CloseHandle                                                                                             api-ms-win-core-handle-l1-1-0            
000000001002A074         HeapAlloc                                                                                               api-ms-win-core-heap-l1-2-0              
000000001002A078         GetProcessHeap                                                                                          api-ms-win-core-heap-l1-2-0              
000000001002A07C         HeapFree                                                                                                api-ms-win-core-heap-l1-2-0              
000000001002A084         InitializeSListHead                                                                                     api-ms-win-core-interlocked-l1-2-0       
000000001002A08C         DeviceIoControl                                                                                         api-ms-win-core-io-l1-1-1                
000000001002A094         GetModuleFileNameA                                                                                      api-ms-win-core-libraryloader-l1-2-0     
000000001002A098         GetModuleHandleExW                                                                                      api-ms-win-core-libraryloader-l1-2-0     
000000001002A0A0         FormatMessageW                                                                                          api-ms-win-core-localization-l1-2-1      
000000001002A0A8         ExpandEnvironmentStringsW                                                                               api-ms-win-core-processenvironment-l1-2-0
000000001002A0B0         GetCurrentThreadId                                                                                      api-ms-win-core-processthreads-l1-1-2    
000000001002A0B4         ResumeThread                                                                                            api-ms-win-core-processthreads-l1-1-2    
000000001002A0B8         GetCurrentProcessId                                                                                     api-ms-win-core-processthreads-l1-1-2    
000000001002A0BC         CreateProcessAsUserW                                                                                    api-ms-win-core-processthreads-l1-1-2    
000000001002A0C0         OpenProcessToken                                                                                        api-ms-win-core-processthreads-l1-1-2    
000000001002A0C4         TerminateProcess                                                                                        api-ms-win-core-processthreads-l1-1-2    
000000001002A0C8         IsProcessorFeaturePresent                                                                               api-ms-win-core-processthreads-l1-1-2    
000000001002A0CC         CreateProcessW                                                                                          api-ms-win-core-processthreads-l1-1-2    
000000001002A0D0         GetCurrentProcess                                                                                       api-ms-win-core-processthreads-l1-1-2    
000000001002A0D8         QueryPerformanceCounter                                                                                 api-ms-win-core-profile-l1-1-0           
000000001002A0E0         RegGetValueW                                                                                            api-ms-win-core-registry-l1-1-0          
000000001002A0E4         RegCloseKey                                                                                             api-ms-win-core-registry-l1-1-0          
000000001002A0E8         RegQueryValueExW                                                                                        api-ms-win-core-registry-l1-1-0          
000000001002A0F0         RegOpenKeyW                                                                                             api-ms-win-core-registry-l2-2-0          
000000001002A0F8         PathFileExistsW                                                                                         api-ms-win-core-shlwapi-legacy-l1-1-0    
000000001002A100         AcquireSRWLockExclusive                                                                                 api-ms-win-core-synch-l1-2-0             
000000001002A104         ReleaseSRWLockExclusive                                                                                 api-ms-win-core-synch-l1-2-0             
000000001002A10C         GetSystemTimeAsFileTime                                                                                 api-ms-win-core-sysinfo-l1-2-1           
000000001002A110         GetWindowsDirectoryW                                                                                    api-ms-win-core-sysinfo-l1-2-1           
000000001002A118         VerQueryValueW                                                                                          api-ms-win-core-version-l1-1-0           
000000001002A120         GetFileVersionInfoW                                                                                     api-ms-win-core-version-l1-1-1           
000000001002A124         GetFileVersionInfoSizeW                                                                                 api-ms-win-core-version-l1-1-1           
000000001002A12C         _o___std_exception_copy                                                                                 api-ms-win-crt-private-l1-1-0            
000000001002A130         _o___std_exception_destroy                                                                              api-ms-win-crt-private-l1-1-0            
000000001002A134         _o___std_type_info_destroy_list                                                                         api-ms-win-crt-private-l1-1-0            
000000001002A138         _o___std_type_info_name                                                                                 api-ms-win-crt-private-l1-1-0            
000000001002A13C         _o___stdio_common_vsnprintf_s                                                                           api-ms-win-crt-private-l1-1-0            
000000001002A140         _o___stdio_common_vswprintf                                                                             api-ms-win-crt-private-l1-1-0            
000000001002A144         _o__callnewh                                                                                            api-ms-win-crt-private-l1-1-0            
000000001002A148         _o__cexit                                                                                               api-ms-win-crt-private-l1-1-0            
000000001002A14C         _o__configure_narrow_argv                                                                               api-ms-win-crt-private-l1-1-0            
000000001002A150         _o__crt_atexit                                                                                          api-ms-win-crt-private-l1-1-0            
000000001002A154         _o__errno                                                                                               api-ms-win-crt-private-l1-1-0            
000000001002A158         _o__execute_onexit_table                                                                                api-ms-win-crt-private-l1-1-0            
000000001002A15C         _o__initialize_narrow_environment                                                                       api-ms-win-crt-private-l1-1-0            
000000001002A160         _o__initialize_onexit_table                                                                             api-ms-win-crt-private-l1-1-0            
000000001002A164         _o__invalid_parameter_noinfo                                                                            api-ms-win-crt-private-l1-1-0            
000000001002A168         _o__invalid_parameter_noinfo_noreturn                                                                   api-ms-win-crt-private-l1-1-0            
000000001002A16C         memmove                                                                                                 api-ms-win-crt-private-l1-1-0            
000000001002A170         _o__purecall                                                                                            api-ms-win-crt-private-l1-1-0            
000000001002A174         memcpy                                                                                                  api-ms-win-crt-private-l1-1-0            
000000001002A178         _o__register_onexit_function                                                                            api-ms-win-crt-private-l1-1-0            
000000001002A17C         _o__seh_filter_dll                                                                                      api-ms-win-crt-private-l1-1-0            
000000001002A180         _o__wcsicmp                                                                                             api-ms-win-crt-private-l1-1-0            
000000001002A184         _o_calloc                                                                                               api-ms-win-crt-private-l1-1-0            
000000001002A188         _o_free                                                                                                 api-ms-win-crt-private-l1-1-0            
000000001002A18C         _o_iswalnum                                                                                             api-ms-win-crt-private-l1-1-0            
000000001002A190         _o_malloc                                                                                               api-ms-win-crt-private-l1-1-0            
000000001002A194         _o_memset                                                                                               api-ms-win-crt-private-l1-1-0            
000000001002A198         _o_terminate                                                                                            api-ms-win-crt-private-l1-1-0            
000000001002A19C         _o_wcscpy_s                                                                                             api-ms-win-crt-private-l1-1-0            
000000001002A1A0         _except_handler4_common                                                                                 api-ms-win-crt-private-l1-1-0            
000000001002A1A4         _CxxThrowException                                                                                      api-ms-win-crt-private-l1-1-0            
000000001002A1A8         __CxxFrameHandler3                                                                                      api-ms-win-crt-private-l1-1-0            
000000001002A1AC         __std_terminate                                                                                         api-ms-win-crt-private-l1-1-0            
000000001002A1B0         __RTDynamicCast                                                                                         api-ms-win-crt-private-l1-1-0            
000000001002A1B8         _initterm                                                                                               api-ms-win-crt-runtime-l1-1-0            
000000001002A1BC         _initterm_e                                                                                             api-ms-win-crt-runtime-l1-1-0            
000000001002A1C4         wcsncmp                                                                                                 api-ms-win-crt-string-l1-1-0             
000000001002A1CC         EventActivityIdControl                                                                                  api-ms-win-eventing-provider-l1-1-0      
000000001002A1D0         EventSetInformation                                                                                     api-ms-win-eventing-provider-l1-1-0      
000000001002A1D4         EventUnregister                                                                                         api-ms-win-eventing-provider-l1-1-0      
000000001002A1D8         EventWriteTransfer                                                                                      api-ms-win-eventing-provider-l1-1-0      
000000001002A1DC         EventRegister                                                                                           api-ms-win-eventing-provider-l1-1-0      
000000001002A1E4         SetTokenInformation                                                                                     api-ms-win-security-base-l1-2-0          
000000001002A1E8         DuplicateTokenEx                                                                                        api-ms-win-security-base-l1-2-0          
000000001002A1F0         ?gbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXH@Z                                           msvcp_win                                
000000001002A1F4         ?pbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXH@Z                                           msvcp_win                                
000000001002A1F8         ??0?$basic_ios@GU?$char_traits@G@std@@@std@@IAE@XZ                                                      msvcp_win                                
000000001002A1FC         ??0?$basic_ostream@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N@Z msvcp_win                                
000000001002A200         ?_Osfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEXXZ                                              msvcp_win                                
000000001002A204         ?flush@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@XZ                                         msvcp_win                                
000000001002A208         ??1?$basic_ios@GU?$char_traits@G@std@@@std@@UAE@XZ                                                      msvcp_win                                
000000001002A20C         ?sputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE_JPBG_J@Z                                      msvcp_win                                
000000001002A210         ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXH_N@Z                                            msvcp_win                                
000000001002A214         ?id@?$ctype@G@std@@2V0locale@2@A                                                                        msvcp_win                                
000000001002A218         ?_BADOFF@std@@3_JB                                                                                      msvcp_win                                
000000001002A21C         ?uncaught_exception@std@@YA_NXZ                                                                         msvcp_win                                
000000001002A220         ?_Getgloballocale@locale@std@@CAPAV_Locimp@12@XZ                                                        msvcp_win                                
000000001002A224         ??0_Lockit@std@@QAE@H@Z                                                                                 msvcp_win                                
000000001002A228         ??1_Lockit@std@@QAE@XZ                                                                                  msvcp_win                                
000000001002A22C         ?_Syserror_map@std@@YAPBDH@Z                                                                            msvcp_win                                
000000001002A230         ?_Xlength_error@std@@YAXPBD@Z                                                                           msvcp_win                                
000000001002A234         ?_Winerror_map@std@@YAHH@Z                                                                              msvcp_win                                
000000001002A238         ?_Xbad_function_call@std@@YAXXZ                                                                         msvcp_win                                
000000001002A23C         ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@H@Z                                            msvcp_win                                
000000001002A240         ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@J@Z                                            msvcp_win                                
000000001002A244         ?setw@std@@YA?AU?$_Smanip@_J@1@_J@Z                                                                     msvcp_win                                
000000001002A248         ??Bid@locale@std@@QAEIXZ                                                                                msvcp_win                                
000000001002A24C         ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAE@XZ                                                msvcp_win                                
000000001002A250         ?widen@?$ctype@G@std@@QBEGD@Z                                                                           msvcp_win                                
000000001002A254         ?_Getcat@?$ctype@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z                                                 msvcp_win                                
000000001002A258         ?_Xbad_alloc@std@@YAXXZ                                                                                 msvcp_win                                
000000001002A25C         ?getloc@ios_base@std@@QBE?AVlocale@2@XZ                                                                 msvcp_win                                
000000001002A260         ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@K@Z                                            msvcp_win                                
000000001002A264         ?_Xout_of_range@std@@YAXPBD@Z                                                                           msvcp_win                                
000000001002A268         ?_Winerror_message@std@@YAKKPADK@Z                                                                      msvcp_win                                
000000001002A26C         ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@I@Z                                            msvcp_win                                
000000001002A270         ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z                    msvcp_win                                
000000001002A274         ??1?$basic_ostream@GU?$char_traits@G@std@@@std@@UAE@XZ                                                  msvcp_win                                
000000001002A278         ?imbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEXABVlocale@2@@Z                                msvcp_win                                
000000001002A27C         ?sync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHXZ                                             msvcp_win                                
000000001002A280         ?setbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEPAV12@PAG_J@Z                                 msvcp_win                                
000000001002A284         ?xsputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE_JPBG_J@Z                                     msvcp_win                                
000000001002A288         ?xsgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE_JPAG_J@Z                                     msvcp_win                                
000000001002A28C         ?uflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGXZ                                            msvcp_win                                
000000001002A290         ?showmanyc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE_JXZ                                       msvcp_win                                
000000001002A294         ?_Unlock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@UAEXXZ                                          msvcp_win                                
000000001002A298         ?_Lock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@UAEXXZ                                            msvcp_win                                
000000001002A29C         ?sputc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGG@Z                                           msvcp_win                                
000000001002A2A0         ??1?$basic_streambuf@GU?$char_traits@G@std@@@std@@UAE@XZ                                                msvcp_win                                
000000001002A2A8         RtlNtStatusToDosErrorNoTeb                                                                              ntdll                                    
000000001002A2AC         NtSetInformationJobObject                                                                               ntdll                                    
000000001002A2B0         NtQueryInformationJobObject                                                                             ntdll                                    
000000001002A2B4         NtAssignProcessToJobObject                                                                              ntdll                                    
000000001002A2B8         NtOpenSymbolicLinkObject                                                                                ntdll                                    
000000001002A2BC         NtQuerySymbolicLinkObject                                                                               ntdll                                    
000000001002A2C0         RtlInitUnicodeString                                                                                    ntdll                                    
000000001002A2C4         NtCreateFile                                                                                            ntdll                                    
000000001002A2C8         NtQuerySecurityObject                                                                                   ntdll                                    
000000001002A2CC         TpWaitForJobNotification                                                                                ntdll                                    
000000001002A2D0         TpAllocJobNotification                                                                                  ntdll                                    
000000001002A2D4         TpReleaseJobNotification                                                                                ntdll                                    
000000001002A2D8         NtSaveKeyEx                                                                                             ntdll                                    
000000001002A2DC         RtlStringFromGUIDEx                                                                                     ntdll                                    
000000001002A2E0         NtSetValueKey                                                                                           ntdll                                    
000000001002A2E4         NtCreateKey                                                                                             ntdll                                    
000000001002A2E8         NtLoadKeyEx                                                                                             ntdll                                    
000000001002A2EC         NtUnloadKey2                                                                                            ntdll                                    
000000001002A2F0         NtOpenKey                                                                                               ntdll                                    
000000001002A2F4         NtEnumerateKey                                                                                          ntdll                                    
000000001002A2F8         NtDeleteKey                                                                                             ntdll                                    
000000001002A2FC         RtlConnectToSm                                                                                          ntdll                                    
000000001002A300         RtlSendMsgToSm                                                                                          ntdll                                    
000000001002A304         NtOpenSection                                                                                           ntdll                                    
000000001002A308         NtMakeTemporaryObject                                                                                   ntdll                                    
000000001002A30C         NtCreateSymbolicLinkObject                                                                              ntdll                                    
000000001002A310         NtQueryDirectoryObject                                                                                  ntdll                                    
000000001002A314         NtOpenDirectoryObject                                                                                   ntdll                                    
000000001002A318         NtSetInformationSymbolicLink                                                                            ntdll                                    
000000001002A31C         NtOpenSemaphore                                                                                         ntdll                                    
000000001002A320         NtOpenEvent                                                                                             ntdll                                    
000000001002A324         NtCreateDirectoryObjectEx                                                                               ntdll                                    
000000001002A328         NtOpenMutant                                                                                            ntdll                                    
000000001002A32C         NtOpenFile                                                                                              ntdll                                    
{% endhighlight %}

以下是 container.dll 和 containerxml.dll 的导出函数
{% highlight text %}
// container.dll from IDA Pro
Name                                                                                                                                                                                                    Address          Ordinal
----                                                                                                                                                                                                    -------          -------
container::ContainerDescription::ContainerDescription(void)                                                                                                                                             0000000010006300 1      
container::Node::~Node(void)                                                                                                                                                                            0000000010011B90 2      
container::Filesystem::AccessMask(uint)                                                                                                                                                                 0000000010007210 3      
container::Registry2::AccessMask(uint)                                                                                                                                                                  0000000010008280 4      
container::Registry::AccessMask(uint)                                                                                                                                                                   0000000010007A20 5      
container::AddRuntimeVirtualKeysToContainer(void *,ulong,_WC_VKEY_INFO *)                                                                                                                               000000001000D590 6      
container::Registry::Alias(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                   0000000010007A60 7      
container::CleanupContainer(void *,ushort const *)                                                                                                                                                      000000001000C2C0 8      
container::ContainerDescription::Clear(void)                                                                                                                                                            00000000100063C0 9      
container::ObjectManager::CloneSd(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                            0000000010007880 10     
container::Registry::CloneSd(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                 0000000010007AA0 11     
container::Setup::CloneSd(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                    00000000100085C0 12     
container::Network::Compartment(void)                                                                                                                                                                   0000000010007850 13     
container::Registry2::ContainerPath(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                          00000000100082C0 14     
container::Registry::CopyOnWritePolicy(void)                                                                                                                                                            0000000010007BD0 15     
container::Job::Cpu(void)                                                                                                                                                                               00000000100076B0 16     
container::CreateContainer(void *,container::ContainerDescription const &,bool)                                                                                                                         000000001000C410 17     
container::Registry::Data(std::vector<uchar,std::allocator<uchar>> const &)                                                                                                                             0000000010007B20 18     
container::Registry::Data(ulong)                                                                                                                                                                        0000000010007AE0 19     
container::Registry::Data(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                    0000000010007B70 20     
container::Setup::Data(ulong)                                                                                                                                                                           0000000010008600 21     
container::Setup::Data(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                       0000000010008640 22     
container::Filesystem::DeleteSemantics(bool)                                                                                                                                                            0000000010007250 23     
container::Registry::DeleteSemantics(bool)                                                                                                                                                              0000000010007C00 24     
container::ObjectManager::Dir(void)                                                                                                                                                                     00000000100078C0 25     
container::Filesystem::Directives(void)                                                                                                                                                                 0000000010007290 26     
container::Registry::Directives(void)                                                                                                                                                                   0000000010007BB0 27     
container::Filesystem::Directory(void)                                                                                                                                                                  00000000100072B0 28     
container::Filesystem::File(void)                                                                                                                                                                       00000000100072D0 29     
container::Registry2::FilePath(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                               0000000010008440 30     
container::Registry::Flags(uint)                                                                                                                                                                        0000000010007C30 31     
container::GetComRegistryRoot(void *)                                                                                                                                                                   000000001000D480 32     
container::GetContainerIdentifierString(void *)                                                                                                                                                         000000001000D7B0 33     
container::GetContainerObjectRootPath(void *,std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>> &)                                                                               000000001000D8C0 34     
container::GetCurrentVolume(void)                                                                                                                                                                       000000001000CCD0 35     
container::GetRegistryRootPath(void *,std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>> const &,std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>> &)    000000001000D840 36     
container::Registry2::Hive(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                   00000000100083A0 37     
container::Registry2::HiveStack(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                              0000000010008340 38     
container::Registry2::HiveStack(void)                                                                                                                                                                   0000000010008380 39     
container::Registry2::HostHive(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                               0000000010008400 40     
container::Registry2::HostPath(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                               0000000010008300 41     
container::Filesystem2::Identifier(_GUID)                                                                                                                                                               00000000100075C0 42     
container::Registry2::Identifier(_GUID)                                                                                                                                                                 0000000010008480 43     
container::IsContainerQuiescent(void *)                                                                                                                                                                 000000001000CC20 44     
container::Filesystem::Junction(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                              00000000100072F0 45     
container::Registry::Junction(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                0000000010007D00 46     
container::Registry2::Key(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                    0000000010008540 47     
container::Registry::Key(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                     0000000010007D60 48     
container::Registry::Key(void)                                                                                                                                                                          0000000010007D40 49     
container::LaunchApplicationContainer(void *,ushort const *)                                                                                                                                            000000001000CD20 50     
container::LaunchContainer(void *,void *)                                                                                                                                                               000000001000D1C0 51     
container::Filesystem2::Layer(void)                                                                                                                                                                     0000000010007620 52     
container::Registry2::Layer(void)                                                                                                                                                                       00000000100083E0 53     
container::Job::Limit(unsigned __int64)                                                                                                                                                                 00000000100076E0 54     
container::Registry::LinkHive(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                0000000010007DA0 55     
container::Registry::Load(void)                                                                                                                                                                         0000000010007DE0 56     
container::LogException(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>> const &,std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>> const &,ulong,long) 000000001000E2C0 57     
container::Job::Memory(void)                                                                                                                                                                            0000000010007720 58     
container::Filesystem::Merge(void)                                                                                                                                                                      0000000010007330 59     
container::Registry::Merge(void)                                                                                                                                                                        0000000010007E00 60     
container::Setup::MkDir(void)                                                                                                                                                                           00000000100088C0 61     
container::Setup::MkHiv(void)                                                                                                                                                                           00000000100088E0 62     
container::Registry::MkKey(void)                                                                                                                                                                        0000000010007E20 63     
container::Setup::MkKey(void)                                                                                                                                                                           0000000010008900 64     
container::Registry::MkValue(void)                                                                                                                                                                      0000000010007E40 65     
container::Setup::MkValue(void)                                                                                                                                                                         0000000010008920 66     
container::Registry::MultiDelimitedStringData(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                0000000010007E60 67     
container::Setup::MultiDelimitedStringData(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                   0000000010008720 68     
container::Registry::MultiStringData(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                         0000000010008000 69     
container::Setup::MultiStringData(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                            0000000010008680 70     
container::Filesystem::Name(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                  0000000010007350 71     
container::ObjectManager::Name(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                               00000000100078F0 72     
container::Registry::Name(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                    00000000100080A0 73     
container::Setup::Name(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                       0000000010008940 74     
container::Filesystem2::Path(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                 0000000010007650 75     
container::Filesystem::Path(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                  0000000010007390 76     
container::ObjectManager::Path(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                               0000000010007930 77     
container::Registry::Path(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                    00000000100080E0 78     
container::Filesystem::Permit(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                00000000100073D0 79     
container::Registry::Permit(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                  0000000010008120 80     
container::Job::Rate(uint)                                                                                                                                                                              0000000010007750 81     
container::Registry2::ReadOnly(bool)                                                                                                                                                                    00000000100084E0 82     
container::Registry::ReadOnly(bool)                                                                                                                                                                     0000000010008160 83     
container::Registry2::RedirectionNode(void)                                                                                                                                                             0000000010008260 84     
container::RegisterForContainerTerminationNotification(void *,void (*)(void *,_WC_CONTAINER_TERMINATION_REASON,_WC_CONTAINER_NOTIFICATION *,void *),void *)                                             000000001001B960 85     
container::ReleaseContainerTerminationNotification(_WC_CONTAINER_NOTIFICATION *)                                                                                                                        000000001001BA60 86     
container::RemoveRuntimeVirtualKeysFromContainer(void *,ulong,_WC_VKEY_INFO *)                                                                                                                          000000001000D7A0 87     
container::ObjectManager::Scope(container::ObjectManager::SymlinkScope)                                                                                                                                 00000000100079B0 88     
container::SelectChildren(container::Node const &,bool,bool)                                                                                                                                            0000000010011C20 89     
container::SetRegistryFlushState(void *,uchar)                                                                                                                                                          000000001000D340 90     
container::ObjectManager::ShadowDirectory(container::ObjectManager::ShadowOption)                                                                                                                       0000000010007970 91     
container::Filesystem::ShortName(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                             0000000010007410 92     
container::Filesystem::Suppress(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                              0000000010007540 93     
container::Registry::Suppress(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                0000000010008190 94     
container::ObjectManager::Symlink(void)                                                                                                                                                                 00000000100079F0 95     
container::Registry2::Symlink(void)                                                                                                                                                                     0000000010008520 96     
container::Registry::Symlink(void)                                                                                                                                                                      00000000100081D0 97     
container::Registry2::Target(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                 0000000010008580 98     
container::Registry::Target(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                  00000000100081F0 99     
container::ThrowIfBasePackageHostMismatch(_WC_PACKAGE_LAYER *,ulong)                                                                                                                                    000000001000CAE0 100    
container::Registry::TransactionsDisabled(bool)                                                                                                                                                         0000000010008230 101    
container::Visit(container::Node const &,std::function<bool (container::Node const &,int)>,int)                                                                                                         0000000010011CB0 102    
container::Filesystem::Volume(std::basic_string<ushort,std::char_traits<ushort>,std::allocator<ushort>>)                                                                                                0000000010007580 103    
container::WaitForContainerTerminationNotification(_WC_CONTAINER_NOTIFICATION *)                                                                                                                        000000001001BB90 104    
container::Job::Weight(uint)                                                                                                                                                                            00000000100077D0 105    
WcAddRuntimeVirtualKeysToContainer(x,x,x)                                                                                                                                                               000000001000C1D0 106    
WcCleanupContainer(x,x)                                                                                                                                                                                 000000001000C050 107    
WcCreateContainer(x,x,x)                                                                                                                                                                                000000001000C090 108    
WcGetComRegistryRoot(x,x)                                                                                                                                                                               000000001000C230 109    
WcIsContainerQuiescent(x,x)                                                                                                                                                                             000000001000C0F0 110    
WcLaunchApplicationContainer(x,x,x)                                                                                                                                                                     000000001000C140 111    
WcLaunchContainer(x,x)                                                                                                                                                                                  000000001000C190 112    
WcRegisterForContainerTerminationNotification(x,x,x,x)                                                                                                                                                  000000001001B8D0 113    
WcReleaseContainerTerminationNotification(x)                                                                                                                                                            000000001001B920 114    
WcRemoveRuntimeVirtualKeysFromContainer(x,x,x)                                                                                                                                                          000000001000C220 115    
WcSetRegistryFlushState(x,x)                                                                                                                                                                            000000001000C280 116    
WcWaitForContainerTerminationNotification(x)                                                                                                                                                            000000001001B940 117    
DllEntryPoint                                                                                                                                                                                           000000001001CD60        
// container_xml.dll
void container::CreateDescriptionFromXml(void *,unsigned short const * *,unsigned long,bool,class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &,struct _WC_PACKAGE_LAYER *,unsigned long,class container::ContainerDescription &)
void container::xml::ReadFromXml(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &,bool,class container::xml::Listener *)
WcCreateDescriptionFromXml
WcDestroyDescription
//CCGLaunchPad.dll
CCGInitialize
CCGLaunch
CCGTeardown

{% endhighlight %}

默认情况下， wchar_t 在 Visual C++ 中是一种内置类型，std::wstring CharT 类型为 wchar_t,而在这些导出符号中，则是 unsigned short 。

## Resources

+ [WinDocks](http://windocks.com/blog-2/docker-windows)
+ [Windows Containers Documentation](https://msdn.microsoft.com/en-us/virtualization/windowscontainers/containers_welcome)
+ [Quick Nano Server PXE boot demo](https://channel9.msdn.com/Blogs/Regular-IT-Guy/Quick-Nano-Server-PXE-boot-demo/player)
+ [Library OS PDF](http://research.microsoft.com/pubs/141071/asplos2011-drawbridge.pdf)
+ [Graphene Library OS - Github](https://github.com/oscarlab/graphene)
+ [Containers: Docker, Windows and Trends](https://azure.microsoft.com/en-us/blog/containers-docker-windows-and-trends/)