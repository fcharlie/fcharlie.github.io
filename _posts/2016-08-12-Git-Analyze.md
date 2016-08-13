---
layout: post
title:  "Git Analyze 工具实现与原理"
date:   2016-08-12 20:00:00
published: true
categories: git
---

# 前言



## Analyze

git-analyze 此工具的设计上是根据用户的输入，扫描存储库特定分支从哪次提交引入了体积超出限制的文件。

git 有多种实现，比如 Linus 的 git（官方 git），libgit2，jgit 等等，官方 git 是一个由多个子命令组成的程序集合。
但是，如果要新增一个工具到 git 官方还是比较麻烦，定制的 git 也容易带来兼容性问题，不利于用户体验。
JGIT 是 Java 实现的 git 类库，如果要实现这些工具，还要用户安装 JRE 或者携带 JRE，并且 Java 也不擅长做跨平台
命令。libgit2 是 C 实现的一个跨平台 git 协议实现库，并且提供多种语言的 banding，所以用 libgit2 再合适不过。

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

CMake 也能自动识别程序资源源文件 (.rc 文件),程序清单 (.manifest) 即可。

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

将 libgit2 作为一个依赖加入项目中，只需要在 CMakeLists.txt 中添加 **add_subdirectory(vendor/libgit2)** 即可。



### Timer

UNIX® 系统支持信号 SIGALRM ，注册信号后, 然后可以使用 alarm 激活定时器，git-analyze 在非 Windows 平台
是同 alarm 实现定时器，不过 alarm 精度不高，如果要使用更高精度的可以使用 ualarm 。

WINDOW­ ® 系统的定时器有 CreateWaitableTimer timeSetEvent CreateTimerQueueTimer 等，分别应对不同的场景。
比如 timeSetEvent 实际上是使用 Windows Event 对象实现，内部还是开了线程，git-analyze 实现的 Timer 功能是启动
一个新的线程，然后 Sleep 后，运行 exit 退出进程，调用 exit 后会调用 ExitProcess 所以进程会退出，然后主进程结束时
也会调用 ExitProcess 退出。

## Rollback

在 Git 中， 有 revert 和 reset 命令，而 git-rollback 实现 git 特定分支的回滚， 只是一个直观简单的替代。需要使用高级功能
可以使用 git reset 或者 revert。

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
#ifdef _MSC_VER
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
  ///
  char *value{nullptr};
  size_t len;
  if (_dupenv_s(&value, &len, "TERM") != 0 || value == nullptr)
    return false;
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

在 Unix 或者 MSYS2 中，可以在输出中加入 **\e[31m** **\33[31m** 这样的字符控制终端文字颜色。

更多的代码请查看 [git-analyze](http://git.oschina.net/oscstudio/git-analyze/blob/master/lib/Pal/Console.cc)

