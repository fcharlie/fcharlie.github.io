---
layout: post
title:  "基于清单的启动器的实现"
date:   2015-11-27 21:30:16
categories: toolset
---
#Launcher
Launcher (启动器) 是一类非常有用的工具，这类工具的意义就在于设置好特定的环境以特定的参数启动特定的进程。
很多软件也用到了 launcher, 比如 Chrome，还有 Android Studio, 在 Windows 平台上，可见的是 studio.exe,
事实上，Android Studio 是基于 Intellij IDEA 开发的，IDE 代码是基于 Java 的，所谓的 studio.exe 其实就是个启动器，加载 jvm.dll 罢了。
IDEA WinLauncher 在 Github 上可以看到源码： [WinLauncher](https://github.com/JetBrains/intellij-community/tree/master/native/WinLauncher)    
Manifest 清单是一类记录程序运行需求的文件，比如 Chrome 就有启动清单：   
{% highlight xml %}
<Application>
  <VisualElements
      DisplayName='Google Chrome'
      Logo='46.0.2490.80\VisualElements\Logo.png'
      SmallLogo='46.0.2490.80\VisualElements\SmallLogo.png'
      ForegroundText='light'
      BackgroundColor='#323232'>
    <DefaultTile ShowName='allLogos'/>
    <SplashScreen Image='46.0.2490.80\VisualElements\splash-620x300.png'/>
  </VisualElements>
</Application>
{% endhighlight %}

Chrome 的清单主要是设置了启动画面以及 Logo 信息。

从 Windows XP 开启，Windows 引入了 manifest 文件，格式基于 XML,记录了一些 依赖组件，隔离信息等，比如下面的一个清单，
requestedPrivileges 表示运行的权限，asInvoker 表示同父进程一样，如果是 requireAdministrator，如果父进程不是管理员则需触发 UAC 提权。
而 dpiAware 则是 DPI 缩放所需，实际上很多 Windows 应用开发商对这个一无所知。assemblyIdentity 则是一些依赖组件的信息了，
这里要求一些基本的控件，如 MessageBox Button, Edit, Combbox 等需要支持 Vista 以后的风格。   
{% highlight xml %}
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
	<noInherit></noInherit>
	<assemblyIdentity processorArchitecture="*" type="win32" name="Force.Charlie.OSChina.Robot" version="1.0.0.0"></assemblyIdentity>
	<description>Robot</description>
	<trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
		<security>
			<requestedPrivileges>
				<requestedExecutionLevel level="asInvoker" uiAccess="false"></requestedExecutionLevel>
			</requestedPrivileges>
		</security>
	</trustInfo>
	<asmv3:application xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
		<asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">
			<dpiAware>true</dpiAware>
		</asmv3:windowsSettings>
	</asmv3:application>
	<dependency optional="yes">
		<dependentAssembly>
			<assemblyIdentity type="win32" name="Microsoft.Windows.Common-Controls" version="6.0.1.0" publicKeyToken="6595b64144ccf1df" language="*" processorArchitecture="*"></assemblyIdentity>
		</dependentAssembly>
	</dependency>
	<compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
		<application>
			<!--The ID below indicates application support for Windows 7 -->
			<supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}"></supportedOS>
		</application>
	</compatibility>
</assembly>
{% endhighlight %}

还有 .NET 应用程序，常常带有 app.exe.config 这类应该说是混合清单了，.config 文件除了保留了清单功能，（比如这里有指定 .NET 版本）
还支持配置信息，应用程序可以通过 ConfigurationManager 读取配置信息。   
{% highlight xml %}
<?xml version ="1.0"?>
<configuration>
	<!--
		Mixed mode assemblies (C++/CLI) will not load into a newer CLR by default. Expression disables this
		so user projects can load when they have mixed mode dependencies that target older frameworks.
	-->
	<startup useLegacyV2RuntimeActivationPolicy="true">
		<supportedRuntime version="v4.0" sku=".NETFramework,Version=v4.5"/>
	</startup>

    <runtime>
		<designerNamespaceResolution enabled="true" />
		<assemblyBinding xmlns="urn:schemas-microsoft-com:asm.v1">
			<probing privatePath="PublicAssemblies;PrivateAssemblies" />
		</assemblyBinding>
    </runtime>
</configuration>

{% endhighlight %}

[ConfigurationManager](https://msdn.microsoft.com/zh-cn/library/system.configuration.configurationmanager)      
[Using System.Configuration.ConfigurationManager Example (C#)](http://blogs.msdn.com/b/aspnetue/archive/2008/10/02/system-configuration-configurationmanager-source-c.aspx)     

##LD 补全的 Launcher
一般而言，Linux 进程依赖的 so 文件，如果不是通过 dlopen 动态加载的，都需要放到默认的 library 目录，也就是 /usr/lib, /usr/local/lib,
或者是通过 export 命令设置 LD PATH，然后从 Shell 或 Shell 脚本启动进程。    

>export LD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/lib

比如 google chrome , p4merge 等大多是写一个 launcher 脚本。新的启动器基于 C++ 实现，使用 TOML 格式文件作为清单文件。 

###TOML 格式清单
TOML (Tom's Obvious, Minimal Language) 是 Github 联合创始人 Tom Preston-Werner 设计的一种极简的配置文件，格式类似于 ini, 但比 ini 严格，
支持整数，浮点，字符串，数组，布尔值，表格，时间日期。解析起来非常方便。主页 [Github TOML](https://github.com/toml-lang/toml)    

TOML 格式清单如下：    
{% highlight toml %}
#launcher.manifest
# OWNERDIR is process image directory
[Launcher]
LibraryPath="/opt/boost/lib"
Path="${OWNERDIR}/../bin"
Binary="launcher_child"
{% endhighlight %}

这里只设置了 LibraryPath Path Binary 。

###清单的环境变量解析
在上面的清单中，Path="${OWNERDIR}/../bin", 这需要解析，OWNERDIR 代表一个环境变量，这里是内置的，表示程序 launcher 自身的目录。 
环境变量的解析如下：  
{% highlight cpp %}
/**
*
*
**/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <unordered_map>

bool GetProcessImageFilePath(char *buffer, size_t bufSize) {
  auto sz = readlink("/proc/self/exe", buffer, bufSize - 1);
  if (sz == 0)
    return false;
  buffer[sz] = 0;
  return true;
}

bool GetProcessImageFileFolder(char *buffer, size_t bufSize) {
  auto sz = readlink("/proc/self/exe", buffer, bufSize - 1);
  if (sz == 0)
    return false;
  buffer[sz] = 0;
  while (--sz) {
    if (buffer[sz] == '/') {
      if (sz == 0)
        buffer[1] = 0;
      else
        buffer[sz] = 0;
      return true;
    }
  }
  buffer[1] = 0;
  return true;
}

/*
* Match Resolve and Replace profile support environment value
*/

enum KEnvStateMachine : int {
  kClearReset = 0,
  kEscapeAllow = 1,
  kMarkAllow = 2,
  kBlockBegin = 3,
  kBlockEnd = 4
};

class EnvironmentValues {
private:
  std::unordered_map<std::string, std::string> ev;
  EnvironmentValues() {
    char buffer[4096];
    if (GetProcessImageFileFolder(buffer, 4096)) {
      ev.insert(
          std::pair<std::string, std::string>("OWNERDIR", std::string(buffer)));
    }
    ev.insert(std::pair<std::string, std::string>("SEPARATOR", "/"));
    ev.insert(std::pair<std::string, std::string>("TMP", "/tmp"));
  }

public:
  bool Push(std::string key, std::string value, bool over = false) {
    if (!over) {
      if (ev.find(key) != ev.end())
        return false;
    }
    ev.insert(std::pair<std::string, std::string>(key, value));
    return true;
  }
  EnvironmentValues &operator=(const EnvironmentValues &) = delete;
  EnvironmentValues(const EnvironmentValues &) = delete;
  static EnvironmentValues &Instance() {
    static EnvironmentValues env;
    return env;
  }
  const decltype(ev) &get() { return ev; }
};

// ${HOME}/${USER}/sss/dir

bool OverwriterEnvironmentKv(std::string k, std::string v) {
  return EnvironmentValues::Instance().Push(k, v, true);
}

bool PushEnvironmentKv(std::string k, std::string v) {
  return EnvironmentValues::Instance().Push(k, v);
}

static bool ResolveEnvironment(const std::string &k, std::string &v) {
  for (auto &i : EnvironmentValues::Instance().get()) {
    if (i.first.compare(k) == 0) {
      v = i.second;
      return true;
    }
  }
  const char *s = nullptr;
  if ((s = getenv(k.c_str())) != nullptr) {
    v = s;
    return true;
  }
  return false;
}

bool BaseEnvironmentExpend(std::string &va) {
  if (va.empty())
    return false;
  std::string ns, ks;
  auto p = va.c_str();
  auto n = va.size();
  int pre = 0;
  size_t i = 0;
  KEnvStateMachine state = kClearReset;
  for (; i < n; i++) {
    switch (p[i]) {
    case '`': {
      switch (state) {
      case kClearReset:
        state = kEscapeAllow;
        break;
      case kEscapeAllow:
        ns.push_back('`');
        state = kClearReset;
        break;
      case kMarkAllow:
        state = kEscapeAllow;
        ns.push_back('$');
        break;
      case kBlockBegin:
        continue;
      default:
        ns.push_back('`');
        continue;
      }
    } break;
    case '$': {
      switch (state) {
      case kClearReset:
        state = kMarkAllow;
        break;
      case kEscapeAllow:
        ns.push_back('$');
        state = kClearReset;
        break;
      case kMarkAllow:
        ns.push_back('$');
        state = kClearReset;
        break;
      case kBlockBegin:
      case kBlockEnd:
      default:
        ns.push_back('$');
        continue;
      }
    } break;
    case '{': {
      switch (state) {
      case kClearReset:
      case kEscapeAllow:
        ns.push_back('{');
        state = kClearReset;
        break;
      case kMarkAllow: {
        state = kBlockBegin;
        pre = i;
      } break;
      case kBlockBegin:
        ns.push_back('{');
        break;
      default:
        continue;
      }
    } break;
    case '}': {
      switch (state) {
      case kClearReset:
      case kEscapeAllow:
        ns.push_back('}');
        state = kClearReset;
        break;
      case kMarkAllow:
        state = kClearReset;
        ns.push_back('$');
        ns.push_back('}');
        break;
      case kBlockBegin: {
        ks.assign(&p[pre + 1], i - pre - 1);
        std::string v;
        if (ResolveEnvironment(ks, v))
          ns.append(v);
        state = kClearReset;
      } break;
      default:
        continue;
      }
    } break;
    default: {
      switch (state) {
      case kClearReset:
        ns.push_back(p[i]);
        break;
      case kEscapeAllow:
        ns.push_back('`');
        ns.push_back(p[i]);
        state = kClearReset;
        break;
      case kMarkAllow:
        ns.push_back('$');
        ns.push_back(p[i]);
        state = kClearReset;
        break;
      case kBlockBegin:
      default:
        continue;
      }
    } break;
    }
  }
  va = ns;
  return true;
}
{% endhighlight %}

###启动器的实现
启动器启动后，查找清单文件，清单文件文件名为 launcher.manifest , 要作为其他进程的启动器，只需要重命名和修改清单文件即可。   
Launcher 随后解析清单文件，并读取 LibraryPath, Path, Binray 等属性，设置好环境变量，最后通过 execvp 启动进程，输入的参数就是启动器的全部参数。   
{% highlight cpp %}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <iostream>
#include <cpptoml.h>

bool GetProcessImageFilePath(char *buffer, size_t bufSize);
bool BaseEnvironmentExpend(std::string &va);

bool AppendEnvironment(const char *name, const char *value,
                       bool front = false) {
  if (name == nullptr || value == nullptr)
    return false;
  const char *ev = nullptr;
  if ((ev = getenv(name)) == nullptr) {
    setenv(name, value, 1);
    return true;
  }
  auto l = strlen(value) + strlen(ev) + 2;
  auto s = new char[l];
  if (front) {
    strcpy(s, value);
    strcat(s, ":");
    strcat(s, ev);
  } else {
    strcpy(s, ev);
    strcat(s, ":");
    strcat(s, value);
  }
  setenv(name, s, 1);
  delete[] s;
  return true;
}

bool Pathcombine(const char *folder, const char *path, std::string &cmd) {
  if (folder == nullptr || path == nullptr)
    return false;
  if (*path == '/') {
    if (access(path, R_OK) != 0)
      return false;
    cmd = path;
    return true;
  }
  cmd = std::string(folder);
  if (cmd.back() != '/' && *path != '/')
    cmd.push_back('/');
  cmd.append(path);
  if (access(cmd.c_str(), R_OK) == 0)
    return true;
  return false;
}

bool PathRemoveFileSpec(char *dir) {
  auto sz = strlen(const_cast<const char *>(dir));
  while (--sz) {
    if (dir[sz] == '/') {
      if (sz == 0)
        dir[1] = 0;
      else
        dir[sz] = 0;
      return true;
    }
  }
  dir[1] = 0;
  return true;
}
//// launcher must find default
bool SearchManifest(std::string &cmd) {
  char selfPath[4096];
  if (!GetProcessImageFilePath(selfPath, 4096))
    return false;
  std::string manifest = std::string(selfPath) + std::string(".manifest");
  if (!PathRemoveFileSpec(selfPath)) {
    std::cout << "cannot resolve path " << selfPath << std::endl;
    return false;
  }
  if (access(manifest.c_str(), R_OK) != 0) {
    std::cerr << "Not found launcher manifest, path is: " << manifest
              << std::endl;
    return false;
  }
  std::shared_ptr<cpptoml::table> g;
  try {
    g = cpptoml::parse_file(manifest);
  } catch (const cpptoml::parse_exception &e) {
    std::cerr << "Failed to parse manifest: " << manifest << ": " << e.what()
              << std::endl;
    return false;
  }
  auto insider = [&](const char *key, const char *evar) {
    if (!g->contains_qualified(key))
      return;
    auto s = g->get_qualified(key)->as<std::string>()->get();
    if (!BaseEnvironmentExpend(s)) {
      std::cout << "cannot resolve env: " << s << std::endl;
      return;
    }
    if (!AppendEnvironment(evar, s.c_str(), true))
      std::cout << "cannot append " << s << " to env " << evar << std::endl;
    // std::cout<<"Append success "<<s<<" to env "<<evar<<std::endl;
    // std::cout<<getenv(evar)<<std::endl;
  };
  insider("Launcher.Path", "PATH");
  insider("Launcher.LibraryPath", "LD_LIBRARY_PATH");
  if (!g->contains_qualified("Launcher.Binary"))
    return false;
  auto sbinary = g->get_qualified("Launcher.Binary")->as<std::string>()->get();
  if (!BaseEnvironmentExpend(sbinary))
    return false;
  if (!Pathcombine(selfPath, sbinary.c_str(), cmd)) {
    std::cout << "Cannot found path: " << cmd << std::endl;
    return false;
  }
  return true;
}

int main(int argc, char *const argv[]) {
  std::string cmd;
  if (!SearchManifest(cmd))
    return -1;
  auto ret = execvp(cmd.c_str(), argv);
  // std::cout<<"Command is: "<<cmd<<std::endl;
  std::cout << "Subprocess result is: " << ret << std::endl;
  return 0;
}
{% endhighlight %}


##Java Service Native Launcher
由于工作需要，我曾经写过一个 Shell 的 Java 服务启动器。如果不用第三方启动器，直接使用 JVM 官方启动器 java，
需要输入很长的命令。比如运行 hello.jar 并传递参数，如下：       

>java -jar hello.jar world



如果运行的是一些服务，则可能是这样的：

>java -server -Xms512m -Xmx512m -jar com.fuck.service.jar --poolsize=24 --ip=192.168.1.12

很多时候，Java 开发者会使用 shell(batch) 脚本，或者第三方启动器来规避这些麻烦。

###Java Service Manifest
绝大多数人并不喜欢冗长的命令，我也不例外。   
在设计 Java Launcher 的时候，我采用 TOML 格式作为启动器清单格式，文件格式如下      
{% highlight toml %}
#Launcher
# OWNERDIR is process image directory
[Runing]
# limit 16 bytes
Title="JSrv"
IsService=true


[Runtime]
JDKPath="/opt/jdk"
Package="${OWNERDIR}/app.jar"
VMOptions=["-Xms512m","-Xmx512m"]
ClassPath=["vendors"]

[Service]
IsDaemon=false
# default -s --signal support stop restart
EnableSignal=true
PidFile="/tmp/jsrv.pids"
Stdout="/opt/jsrv.stdout.log"
Stderr="/opt/jsrv.stderr.log"
DefaultArgs=["--config","config.example","--show-config"]
{% endhighlight %}

在这个清单文件中，我设计了两种模式，第一个是**基础模式**，也就是 IsService 为 false 的时候，这个时候，程序只会读取 Runtime 一节
的所有配置信息。    
* JDKPath 读取 jdk 所在目录，并查找到 libjvm.so 的位置。    
* Package 是运行的 Jar 包的路径，也就是 MainClass 所在的Jar 包。
* VMoptions 是 Java 虚拟机 运行参数， ClassPath 是 Jar 包所在目录。   

在 基础模式中，所有输入的参数除了 Argv~0, 其他的参数都会被传递给 Java MainClass 。

> jsrv helloworld

假如 Package 是 hello.jar, 等价于：

>java -Xms512m -Xmx512m -jar hello.jar helloworld

另一种是 **服务模式** ，这种模式并不支持从命令行输入参数。当 IsService 为 true 时，启动器读取 Service 节的信息.    
* IsDaemon 表示启动器是否以 守护进程的模式运行     
* EnableSignal 表示开启 -s stop -s restart 参数    
* PidFile 是记录守护进程 pid 的临时文件    
* Stdout 是 stdout 重定向。  
* Stderr 是 stderr 重定向     
* DefaultArgs 这个数组是要传给 MainClass 的参数。    

启动一个进程作为守护进程时，你需要将日志输出到文件。  

一般而言，启动一个守护进程，先需要 fork 出一个子进程，然后退出父进程。 
通过 setsid() 函数调用，让 init 进程收养它，使用 umask 设置默认权限，后面重定向标准输入输出，关闭文件句柄即可.       
{% highlight cpp %}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>

int create_daemon() {
  int fd;
  switch (fork()) {
  case -1:
    printf("fork() failed\n");
    return -1;
  case 0:
    break;
  default:
    exit(0);
  }
  if (setsid() == -1) {
    return -1;
  }

  umask(0);
  fd = open("/dev/null", O_RDWR);
  if (fd == -1) {
    // open /dev/null failed
    return -1;
  }

  if (dup2(fd, STDIN_FILENO) == -1) {
    // dup2(STDIN) failed
    return -1;
  }

  if (dup2(fd, STDOUT_FILENO) == -1) {
    // dup2(STDOUT) failed
    return -1;
  }
  if (fd > STDERR_FILENO) {
    if (close(fd) == -1) {
      return -1;
    }
  }
  return 0;
}
{% endhighlight %}

如果在 Windows 上运行一个守护进程，除了作为一个 Windows Service 还可以通过 WinMain 启动，在后台运行，如果从 main 启动，
就需要关闭控制台，代码如下：   
{% highlight cpp %}
#include <cstdio>
#include <Windows.h>

int create_daemon()
{
  //do some thing and set io
  FILE *fp;
  if(freopen_s(&fp,"nul","w+b",stdout)!=0)
    return -1;
  fclose(fp);
  if(freopen_s(&fp,"nul","w+b",stderr)!=0)
    return -1;
  fclose(fp);
  FreeConsole();
  return 0;
}
{% endhighlight %}

启动器的处理流程可以借鉴上面的 LD 补全 启动器，在读取清单时，解析完 Runtime 一节后，读取 Runing.IsService 的信息，
如果不为真，则返回，这时候执行的是基础模式，否则，继续读取 Service 一节的信息。  
返回值如下：    
{% highlight cpp %}
enum KLauncherChannel {
  kBasicVMRuntime = 0,
  kServiceVMRuntime = 1,
  kUnknownVMRuntime
};
{% endhighlight %}

如果是服务模式还需要解析命令行参数，参数帮助信息如下：   
{% highlight cpp %}
static const char *kUsage =
    "Usage: %s [options] <input> \n"
    "    Flags:\n"
    "        -h [-?|--help]\tprint jsrv usage information and exit\n"
    "        -v [--version]\tprint jsrv version and exit\n"
    "        -s [--signal]\tsend a signal to jsrv,argument: stop|restart  "
    "\n";
{% endhighlight %}

解析完毕后，最后启动 JVM,执行代码并没有多少不同，执行 MainClass 的函数分别如下：   
{% highlight cpp%}
 int Exe(int Argc, char **Argv); /// Base mode
 int Exe(std::vector<std::string> Args); /// Service mode
{% endhighlight %}

###JVM 的启动流程
各个平台上的 java 以及 Windows 平台的 javaw 依然也只是 JVM 的一个启动器，这类程序需要通过调用 jvm.dll 或者 libjvm.so 导出的
函数来启动 JVM。

清单中设置了 JDKPath 信息，在 Linux 中，以 jdk8 为例，64位系统 libjvm.so 是 $JDKPath/jre/lib/amd64/server/libjvm.so，
如果找不到 libjvm.so, 启动器还会从 /usr/lib/libjvm.so 。

找到 libjvm.so 后，可以使用 dlopen 打开一个动态库句柄，dlsym 查找 Symbol, 获得函数地址，在 Windows 中 分别是 LoadLibrary 和 GetProcAddress    
函数大致如下：   
{% highlight cpp %}
typedef jint (*VMInitMethord)(JavaVM **pvm, JNIEnv **env, void *args);

static bool SearchJDKPath(const std::string &jdkPath, std::string &libjvm) {
#ifdef __x86_64__
  libjvm = jdkPath + "/jre/lib/amd64/server/libjvm.so";
#else
  libjvm = jdkPath + "/jre/lib/server/libjvm.so";
#endif
  if (access(libjvm.c_str(), X_OK) == 0)
    return true;
 // JDK9
#ifdef __x86_64__
  libjvm = jdkPath + "/lib/amd64/server/libjvm.so";
#else
  libjvm = jdkPath + "/lib/server/libjvm.so";
#endif
  if (access(libjvm.c_str(), X_OK) == 0)
    return true;
  if (access("/usr/lib/libjvm.so", X_OK) != 0)
    return false;
  libjvm = "/usr/lib/libjvm.so";
  return true;
}

bool VMRuntime::VMRuntimeBind() {
  std::string libjvm;
  if (!SearchJDKPath(rc_.jdkdir, libjvm)) {
    std::cerr << "Cannot found libjvm.so, jdk path is: " << rc_.jdkdir
              << std::endl;
    return false;
  }
  if ((vmHandle = dlopen(libjvm.c_str(), RTLD_NOW + RTLD_GLOBAL)) == nullptr) {
    std::cerr << "Failure: dlopen open libjvm.so failed. " << std::endl;
    return false;
  }
  if ((vmcall = (VMInitMethord)dlsym(vmHandle, "JNI_CreateJavaVM")) ==
      nullptr) {
    std::cerr << "Failure: cannot resolve JNI_CreateJavaVM !" << std::endl;
    dlclose(vmHandle);
    vmHandle = nullptr;
    return false;
  }
  return true;
}
{% endhighlight %}

成功得到 JNI_CreateJavaVM 函数指针后，就该设置 JavaVMOption 类型变量，上面的 清单中的 VMOptions 要依次绑定到 JavaVMOption,
ClassPath 目录下的 jar 包，以及 Package 要绑定到 -Djava.class.path 上， 然后调用 JNI_CreateJavaVM 函数指针，获得一个 JavaVM 对象和
JNIEnv 对象。

Jar 包是基于 zip 格式的文件，内部的 META-INF/MANIFEST.MF 这一清单文件记录了 清单版本，Main-Class 以及 Class-Path, 可以从 Jar 包里面读取
MainClass 。   
我使用了 Github 用户 slyfoxza [slyfoxza/minecraftd](https://github.com/slyfoxza/minecraftd/blob/master/src/JarReader.cpp) 的 JarReader来获取
MainClass,不过遗憾的是，通过 JarReader 得到的 MainClass 是用 '.' 区分的，而 FindClass 需要 '/' 区分。 我是用了一个帮助函数来替换所有的 点。  
{% highlight cpp %}
static void Replace(std::string &s, char c1, char c2) {
  for (auto &c : s) {
    if (c == c1)
      c = c2;
  }
}
{% endhighlight %}

使用 env->FindClass 获取 MainClass, 类型为 jclass, 使用 env->GetStatuucMethodID 获取 MainClass 的方法 main, 后面就是将 参数转成 JVM String[] 类型
随后调用静态方法即可， 大致如下：     
{% highlight cpp %}
int VMRuntime::Exe(std::vector<std::string> Args) {
  if (!InitVMEnv()) {
    std::cout << "Initialize VM failed !" << std::endl;
    return 1;
  }
  JarReader jarReader(rc_.package);
  auto mainClassString = jarReader.getMainClassName();
  Replace(mainClassString, '.', '/');
  jclass mainClass = env->FindClass(mainClassString.c_str());
  if (!mainClass) {
    std::cout << __LINE__ << " Cannot find MainClass: " << mainClassString
              << std::endl;
    return 2;
  }
  jmethodID mainMethod =
      env->GetStaticMethodID(mainClass, "main", "([Ljava/lang/String;)V");
  if (!mainMethod) {
    std::cout << __LINE__ << "Cannot GetStaticMetodID " << std::endl;
    return 3;
  }
  jclass stringClass = env->FindClass("java/lang/String");
  jobjectArray args = env->NewObjectArray(Args.size(), stringClass, NULL);
  int i = 0;
  for (auto &Arg : Args) {
    jstring jni_arg = env->NewStringUTF(Arg.c_str());
    env->SetObjectArrayElement(args, i++, jni_arg);
    env->DeleteLocalRef(jni_arg);
  }
  env->CallStaticVoidMethod(mainClass, mainMethod, args);
  // env->ExceptionDescribe();
  jthrowable exc = env->ExceptionOccurred();
  if (exc) {

    env->ExceptionDescribe();
    env->ExceptionClear();
    jclass newExcCls = env->FindClass("java/lang/IllegalArgumentException");
    if (!newExcCls) {
      env->ThrowNew(newExcCls, "Throw Exception");
    }
    return 4;
  }
  return 0;
}
{% endhighlight %}

程序退出的时候需要析构这些对象。

实际上一个大致的 JVM Launcher 也就这样了。

