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

将 libgit2 作为一个依赖加入项目中，只需要在 CMakeLists.txt 中添加 **add_subdirectory(vendor/libgit2)** 即可。



### Timer

UNIX® 系统支持信号 SIGALRM ，注册信号后, 然后可以使用 alarm 激活定时器，git-analyze 在非 Windows 平台
是同 alarm 实现定时器，不过 alarm 精度不高，如果要使用更高精度的可以使用 ualarm 。

## Rollback

在 Git 中， 有 revert 和 reset 命令，而 git-rollback 实现 git 特定分支的回滚， 只是一个直观简单的替代。需要使用高级功能
可以使用 git reset 或者 revert。

## Pal

libgit2 使用的是 UTF-8 编码,在 Windows 中转变为 UTF16 编码,使用 Windows API 完成一系列操作.

如果按照默认的 main 传递命令行参数,那么可能会发生错误,在 Windows 中, 创建进程是通过 CreateProcess 这样的 API 实现的, 
NT 内核将命令行参数写入的进程的 PEB 中, CRT 初始化时,根据启动函数类型执行不同的策略 (WinMain wWinMain main wmain) ,
比如 main , CRT 通过  GetCommandLineA 获得命令行参数,然后将 LPCSTR 转变成 char * Argv[] 的形式. GetCommandLineA 获得
的命令行参数也是由 PEB 的命令行参数转换编码过来的. main 命令行参数的编码即当前代码页的编码,也就是 CP_ACP ,
比如 Windows 下常见的 936 GBK。

这样一来,libgit2 传入非 西文字符 就会操作失败, 为了支持 Windows 平台,笔者使用 wmain ,然后将命令行参数依次转变为 UTF-8,
这样就可以解决不支持非西文字符的问题。



