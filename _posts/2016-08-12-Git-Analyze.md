---
layout: post
title:  "Git Analyze 工具实现与原理"
date:   2016-08-11 20:00:00
published: true
categories: git
---

# 前言

作为一个免费提供私有仓库的代码托管平台，码云时常要考虑利用现有的资源支持更多的用户，对于体积较大的存存储库，
由于 git 的分布式特性，服务器往往需要更多的硬件资源来支撑这些存储库的访问。

码云对 git 仓库的大小限制为 1GB，用户在本地可以使用如下命令查看存储库的大小。

>du -sh .git/objects

这个命令在 Git for Windows 中可以找到，也可以使用 www.sysinternals.com 提供的 du （Directory disk usage reporter）工具。

码云对文件的限制为 100 MB，超过 50 MB 会提出警告。一部分用户很容易将生成的二进制文件添加到版本控制之中，当推送到
码云上就被拒绝推送了。当用户需要检查或者回退就会感到非常麻烦，开发 git-analyze 的目的也就是为了解决这些用户的烦恼。

## Analyze

git-analyze 此工具的设计上是根据用户的输入，扫描存储库特定分支从哪次提交引入了体积超出限制的文件。

git 有多种实现，比如 Linus 的 git（官方 git），libgit2，jgit 等等，官方 git 是一个由多个子命令组成的程序集合。
但是，如果要新增一个工具到 git 官方还是比较麻烦，定制的 git 也容易带来兼容性问题，不利于用户体验。
JGIT 是 Java 实现的 git 类库，如果要实现这些工具，还要用户安装 JRE 或者携带 JRE，并且 Java 也不擅长做跨平台
命令。libgit2 是 C 实现的一个跨平台 git 协议实现库，并且提供多种语言的 banding，所以用 libgit2 再合适不过。

git-analyze 支持参数：

+ --limitsize 设置超限大小，可选，单位 MB,默认为 100，例如 --limitsize=72 或者 --limitsize 72。
+ --warnsize 设置警告大小，可选，单位 MB,默认为 50。
+ --timeout 设置超时大小，可选，默认未开启。
+ --all 检查所有分支
+ --who 显示超限文件提交信息和提交者

git-analyze 仓库参数为:

>git-analyze /path/to/repo master # 也可以是 引用全名，二者的相对顺序必须是先路径后引用，标签参数不做要求。

git-analyze 在用户输入参数后，使用 libgit2 打开存储库。目前只支持工作目录的根目录和 .git 目录。

git 的每一次提交都是文件快照，并不像 Subversion 一样每一个文件都有版本号。如果要知道是否有新的文件被添加或者
是被修改，则需要与上一个提交进行比较，通常就是当前的 commit 与 parent commit 比较，在 libgit2 中，并不能直接
比较，需要比较 commit 的根 tree。使用 git_commit_tree 得到 tree 对象，git_diff_tree_to_tree 比较 tree，git_diff_foreach
去遍历 diff 的内容，这里由于我们只需要查看文件修改，所以，git_diff_foreach binary_cb hunk_cb line_cb callback 设置
为空即可，git_diff_foreach 的 API 在下面：

[libgit2 API git_diff_foreach](https://libgit2.github.com/libgit2/#HEAD/group/diff/git_diff_foreach)

我们在 回调函数中，只响应 diff 类型为新增和修改的文件类型。

当出现合并时，我们的策略是，只比较第一个 parent commit，大文件引入行为归咎与合并者。

当遍历到初始提交时，parent commit 也就不存在了，所以，我们要使用 treewalk 遍历所有的文件，检测引入的大文件。

当使用 --all 参数时，git-analyze 会忽略引用参数，直接遍历所有本地分支对应的引用，然后逐一检测。

### CMake

libgit2 使用 CMake 作为构建文件，CMake 能够根据不同的平台生成不同类型的项目文件，如 Visual Studio 的 msbuild
项目文件，Makefile 文件 等，然后支持自动打包，例如下面的一些代码就可以支持生成 Windows 安装程序，Ubuntu DEB 包

{% highlight cmake %}
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

set(CPACK_PACKAGE_NAME "git-analyze")
set(CPACK_PACKAGE_VENDOR "OSChina.NET")
set(CPACK_PACKAGE_DESCRIPTION "This is git analyze tools")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GIT Analyze")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "git-analyze")
set(CPACK_PACKAGE_VERSION_MAJOR ${GITANALYZE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GITANALYZE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GITANALYZE_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DESCRIPTION "Git Analyze")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.3.1-6)")
set(CPACK_PACKAGE_CONTACT "admin@oschina.cn")
set(CPACK_DEBIAN_PACKAGE_SECTION T)
if(WIN32 AND NOT UNIX)
  set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "GitAnalyze")
  set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\cmake\\\\git.ico")
  set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\cmake\\\\git.ico")
  set(CPACK_NSIS_MODIFY_PATH "ON")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
  if( CMAKE_CL_64 )
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  endif()
endif()

include(CPack)

if(WIN32)
include(InstallRequiredSystemLibraries)
endif()
install(TARGETS git-analyze
    DESTINATION bin
)

{% endhighlight %}

CMake 也能自动识别程序资源源文件 (.rc 文件),程序清单 (.manifest) 。

{% highlight cmake %}
#C++ Based hook command

if(WIN32)
add_executable(git-analyze
  driver.cc
  analyze.cc
  environment.cc
  git-analyze.rc
  git-analyze.manifest
)
else()
add_executable(git-analyze
  driver.cc
  analyze.cc
  environment.cc
)
endif()
{% endhighlight %}

将 libgit2 作为一个依赖加入项目中，只需要在 CMakeLists.txt 中添加 **add_subdirectory(vendor/libgit2)** 可。



### Timer

UNIX® 系统支持信号 SIGALRM ，注册信号后, 然后可以使用 alarm 激活定时器，git-analyze 在非 Windows 平台
是同 alarm 实现定时器，不过 alarm 精度不高，如果要使用更高精度的可以使用 ualarm 。

WINDOW­S ® 系统的定时器有 CreateWaitableTimer timeSetEvent CreateTimerQueueTimer 等，分别应对不同的场景。
比如 timeSetEvent 实际上是使用 Windows Event 对象实现，内部还是开了线程，git-analyze 实现的 Timer 功能是启动
一个新的线程，然后 Sleep 后，运行 exit 退出进程，调用 exit 后会调用 ExitProcess 所以进程会退出，然后主进程结束时
也会调用 ExitProcess 退出。

## Rollback

在 Git 中， 有 revert 和 reset 命令，而 git-rollback 实现 git 特定分支的回滚， 只是一个直观简单的替代。需要使用高级功能
可以使用 git reset 或者 revert。

支持参数：

+ --git-dir
+ --backid
+ --backrev
+ --refname
+ --force


使用 --backid 参数时，git-rollback 先需要回溯检测 commit 是否在分支上，存在的时候会设置 refname (这个支持分支名和引用全名)
的 commit 为 --backid 的值，然后运行 git gc ，当添加 --force 时会清理掉那些悬空对象。

使用 --backrev 时， git-rollback 会回溯 commit，然后当回溯次数与 --backrev 值一致时，将当前 commit 的 oid 设置到引用上，与
--backid 的策略一致即可。

由于 libgit2 暂时并未提供 GC 功能，我们调用的是原生命令，在 UNIX 类系统中，我们先获得环境变量 PATH,然后遍历这些目录是否
存在 git ，存在后，使用 fork-execvp-wait 一系列 API 运行 git GC。

在 Windows 中，我们从 git-rollback 的当前目录，以及 git-rollback 进程所在目录，以及 PATH 中查找 git，如果没有找到，则从
注册表中查找 Git for Windows 的安装路径。部分的代码如下：

{% highlight c++ %}
class WCharacters {
private:
  wchar_t *wstr;

public:
  WCharacters(const char *str) : wstr(nullptr) {
    if (str == nullptr)
      return;
    int unicodeLen = ::MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (unicodeLen == 0)
      return;
    wstr = new wchar_t[unicodeLen + 1];
    if (wstr == nullptr)
      return;
    wstr[unicodeLen] = 0;
    ::MultiByteToWideChar(CP_UTF8, 0, str, -1, (LPWSTR)wstr, unicodeLen);
  }
  const wchar_t *Get() {
    if (!wstr)
      return nullptr;
    return const_cast<const wchar_t *>(wstr);
  }
  ~WCharacters() {
    if (wstr)
      delete[] wstr;
  }
};

inline bool PathFileIsExistsU(const std::wstring &path) {
  auto i = GetFileAttributesW(path.c_str());
  return INVALID_FILE_ATTRIBUTES != i;
}

inline bool PathRemoveFileSpecU(wchar_t *begin, wchar_t *end) {
  for (; end > begin; end--) {
    if (*end == '/' || *end == '\\') {
      *end = 0;
      return true;
    }
  }
  return false;
}

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
BOOL IsRunOnWin64() {
  BOOL bIsWow64 = FALSE;
  LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
      GetModuleHandleW(L"kernel32"), "IsWow64Process");
  if (NULL != fnIsWow64Process) {
    if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
      // handle error
    }
  }
  return bIsWow64;
}
BOOL WINAPI FindGitInstallationLocation(std::wstring &location) {
  // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Git_is1
  // InstallLocation
  HKEY hInst = nullptr;
  LSTATUS result = ERROR_SUCCESS;
  const wchar_t *git4win =
      LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Git_is1)";
  const wchar_t *installKey = L"InstallLocation";
  WCHAR buffer[4096] = {0};
#if defined(_M_X64)
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, git4win, 0, KEY_READ, &hInst) !=
      ERROR_SUCCESS) {
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, git4win, 0,
                      KEY_READ | KEY_WOW64_32KEY, &hInst) != ERROR_SUCCESS) {
      // Cannot found msysgit or Git for Windows install
      return FALSE;
    }
  }
#else
  if (IsRunOnWin64()) {
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, git4win, 0,
                      KEY_READ | KEY_WOW64_64KEY, &hInst) != ERROR_SUCCESS) {
      if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, git4win, 0, KEY_READ, &hInst) !=
          ERROR_SUCCESS) {
        // Cannot found msysgit or Git for Windows install
        return FALSE;
      }
    }
  } else {
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, git4win, 0, KEY_READ, &hInst) !=
        ERROR_SUCCESS) {
      return FALSE;
    }
  }
#endif
  DWORD type = 0;
  DWORD dwSize = 4096 * sizeof(wchar_t);
  result = RegGetValueW(hInst, nullptr, installKey, RRF_RT_REG_SZ, &type,
                        buffer, &dwSize);
  if (result == ERROR_SUCCESS) {
    location.assign(buffer);
  }
  RegCloseKey(hInst);
  return result == ERROR_SUCCESS;
}
////

// ////
// bool search_git_from_path(std::wstring &gitbin) {
//   ///
//   WCHAR buffer[4096] = {0};
//   DWORD dwLength = 0;
//   ////
//   if ((dwLength =
//            SearchPathW(nullptr, L"git", L".exe", 4096, buffer, nullptr)) > 0)
//            {
//     gitbin.assign(buffer, dwLength);
//     return true;
//   }
//   return false;
// }

bool SearchGitForWindowsInstall(std::wstring &gitbin) {
  //
  if (!FindGitInstallationLocation(gitbin))
    return false;
  gitbin.push_back(L'\\');
  gitbin.append(L"git.exe");
  if (PathFileIsExistsU(gitbin))
    return true;
  return false;
}

//
bool GitExecutePathSearchAuto(const wchar_t *cmd, std::wstring &gitbin) {
  //// Self , Path Env,
  if (PathFileIsExistsU(cmd)) {
    gitbin.assign(cmd);
    return true;
  }
  std::wstring Path;
  Path.reserve(0x8000); /// 32767
  ///
  auto len = GetModuleFileNameW(nullptr, &Path[0], 32767);
  if (len > 0) {
    auto end = &Path[0] + len;
    PathRemoveFileSpecU(&Path[0], end);
    gitbin.assign(&Path[0]);
    gitbin.push_back(L'\\');
    gitbin.append(cmd);
    if (PathFileIsExistsU(gitbin))
      return true;
    gitbin.clear();
  }
  ///
  GetEnvironmentVariableW(L"PATH", &Path[0], 32767);
  auto iter = &Path[0];
  for (; *iter; iter++) {
    if (*iter == ';') {
      gitbin.push_back(L'\\');
      gitbin.append(cmd);
      if (PathFileIsExistsU(gitbin)) {
        return true;
      }
      gitbin.clear();
    } else {
      gitbin.push_back(*iter);
    }
  }
  return false;
}

/// First search git from path.
bool GitGCInvoke(const std::string &dir, bool forced) {
  ///
  WCharacters wstr(dir.c_str()); /// convert to UTF16
  std::wstring gitbin;
  if (!GitExecutePathSearchAuto(L"git.exe", gitbin)) {
    if (!SearchGitForWindowsInstall(gitbin)) {
      BaseErrorMessagePrint(
          "Not Found git in your PATH environemnt variable and Registry !");
      return false;
    }
  }
  /////////////////////////////////////////////////////////
  std::wstring cmdline;
  cmdline.reserve(0x8000);
  _snwprintf_s(&cmdline[0], 32767, 32767, LR"("%s" gc )", gitbin.c_str());
  if (forced) {
    wcscat_s(&cmdline[0], 32767, L"--prune=now --force");
  }
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (!CreateProcessW(nullptr, &cmdline[0], nullptr, nullptr, FALSE, 0, nullptr,
                      wstr.Get(), &si, &pi)) {
    return false;
  }
  bool result = false;
  if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0) {
    DWORD dwExit = 0;
    if (GetExitCodeProcess(pi.hProcess, &dwExit) && dwExit == 0) {
      result = true;
    }
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return result;
}
{% endhighlight %}

## Compatibility

libgit2 使用的是 UTF-8 编码,在 Windows 中转变为 UTF16 编码,使用 Windows API 完成一系列操作.

如果按照默认的 main 传递命令行参数,那么可能会发生错误,在 Windows 中, 创建进程是通过 CreateProcess 这样的 API 实现的, 
NT 内核将命令行参数写入的进程的 PEB 中, CRT 初始化时,根据启动函数类型执行不同的策略 (WinMain wWinMain main wmain) ,
比如 main , CRT 通过  GetCommandLineA 获得命令行参数,然后将 LPCSTR 转变成 char * Argv[] 的形式. GetCommandLineA 获得
的命令行参数也是由 PEB 的命令行参数转换编码过来的. main 命令行参数的编码即当前代码页的编码,也就是 CP_ACP ,
比如 Windows 下常见的 936 GBK。

这样一来,libgit2 传入非 西文字符 就会操作失败, 为了支持 Windows 平台,笔者使用 wmain ,然后将命令行参数依次转变为 UTF-8,
这样就可以解决不支持非西文字符的问题。然后 POSIX 平台依然使用 main。

{% highlight c++ %}
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <Windows.h>
//// To convert Utf8
char *CopyToUtf8(const wchar_t *wstr) {
  auto l = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  char *buf = (char *)malloc(sizeof(char) * l + 1);
  if (buf == nullptr)
    throw std::runtime_error("Out of Memory ");
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buf, l, NULL, NULL);
  return buf;
}
int wmain(int argc, wchar_t **argv) {
  std::vector<char *> Argv_;
  auto Release = [&]() {
    for (auto &a : Argv_) {
      free(a);
    }
  };
  try {
    for (int i = 0; i < argc; i++) {
      Argv_.push_back(CopyToUtf8(argv[i]));
    }
  } catch (const std::exception &e) {
    BaseErrorMessagePrint("Exception: %s\n", e.what());
    Release();
    return -1;
  }
  AnalyzeArgs analyzeArgs;
  ProcessArgv((int)Argv_.size(), Argv_.data(), analyzeArgs);
  if (ProcessAnalyzeTask(analyzeArgs)) {
    BaseConsoleWrite("git-analyze: Operation completed !\n");
  } else {
    BaseErrorMessagePrint("git-analyze: Operation aborted !\n");
  }
  Release();
  return 0;
}
#else

int main(int argc, char **argv) {
  AnalyzeArgs analyzeArgs;
  ProcessArgv(argc, argv, analyzeArgs);
  if (ProcessAnalyzeTask(analyzeArgs)) {
    BaseConsoleWrite("git-analyze: Operation completed !\n");
  } else {
    BaseErrorMessagePrint("git-analyze: Operation aborted !\n");
  }
  return 0;
}
#endif
{% endhighlight%}

另外一个问题，由于参数和 libgit2 都是使用的 UTF8 编码，默认情况下，Windows 控制台的代码页在输出 UTF8 编码
字符的情况下可能会乱码，libgit2 并没有去调整，而控制台的代码页如果手动调整，可能会导致其他程序乱码。
当然可以调用 SetConsoleOutputCP 去修改代码页，笔者并未测试，笔者采用的是和 git 官方一样的策略，
检测程序当前的标准输出标准错误是否是字符设备，这个可以使用 _isatty 来检测，当然也可以使用下面的代码
来实现检测：

{% highlight c++ %}
bool IsUnderConhost(FILE *fp) {
  HANDLE hStderr = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fp)));
  return GetFileType(hStderr) == FILE_TYPE_CHAR;
}
{% endhighlight %}

但是，重要的一点，MSYS2 的终端模拟器 Mintty 编码是 UTF8 ，_isatty 并不会将 Mintty 识别为字符设备，这是由于
MSYS2 或者 Cygwin 中，使用的是管道的方式读取程序的输出渲染到 Mintty，不过 MSYS2 的环境变量中会存在 TERM
这样的变量，就可以用下面的代码去识别：

{% highlight c++ %}
bool IsWindowsTTY() {
  if (GetEnvironmentVariableW(L"TERM", NULL, 0) == 0) {
    if (GetLastError() == ERROR_ENVVAR_NOT_FOUND)
      return false;
  }
  return true;
}
{% endhighlight %}

在输出错误的时候，我们可以修改输出颜色，在控制台中，可以使用 SetConsoleTextAttribute，使用 GetConsoleScreenBufferInfo
获得控制台的颜色，控制台是 256 色的，其中高 4位是背景色，低四位是前景色，所以可以使用下面的代码实现色彩输出：

{% highlight c++ %}
int BaseErrorWriteConhost(const char *buf, size_t len) {
  // TO set Foreground color
  HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(hConsole, &csbi);
  WORD oldColor = csbi.wAttributes;
  WORD newColor = (oldColor & 0xF0) | FOREGROUND_INTENSITY | FOREGROUND_RED;
  SetConsoleTextAttribute(hConsole, newColor);
  DWORD dwWrite;
  WCharacters wstr(buf, len);
  WriteConsoleW(hConsole, wstr.Get(), wstr.Length(), &dwWrite, nullptr);
  SetConsoleTextAttribute(hConsole, oldColor);
  return dwWrite;
}
{% endhighlight %}

在 Unix 或者 MSYS2 中，可以在输出中加入 **\e[31m** (GCC) **\33[31m** (MSVC) 这样的字符控制终端文字颜色。

更多的代码请查看 [git-analyze](http://git.oschina.net/oscstudio/git-analyze/blob/master/lib/Pal/Console.cc)

