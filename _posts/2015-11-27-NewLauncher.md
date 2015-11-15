---
layout: post
title:  "基于清单的 Linux 服务启动器的实现"
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



支持环境变量的解析
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

{% highlight toml %}
#launcher.manifest
# OWNERDIR is process image directory
[Launcher]
LibraryPath="/opt/boost/lib"
Path="${OWNERDIR}/../bin"
Binary="launcher_child"
{% endhighlight %}


##Java Native Launcher


