+++
title = "Baulk - 一次有趣的尝试"
date = "2021-07-20T20:00:00+08:00"
categories = "toolset"
+++

## 前言

作为一个程序员，你认为你做过的最让你自豪的东西是什么？作为了一个菜鸡程序员，很遗憾，我拿不出什么像样的作品，工作上也只是站在前人的肩膀上，做了一些微小的改动。如果硬要说一个作品是我最得意的，我会选择 Baulk，它是一个极简的 Windows 包管理器，开发这个工具花费了我大量的业余时间，我很多有意思的构想也在开发 baulk 的过程中付诸实现了。Baulk `[bɔːk]` 其含义是`错误；失败；障碍（等于balk）` 或是 `阻止；错过；推诿（等于balk）`；对于绝大多数人来说，2020 年有一个坠落的魔幻开局，到了今年，新冠疫情的阴影任然笼罩全球，Baulk 诞生于 2020 年 3 月 9 日，那个时候我确实是挫败的吧。Baulk 还可以翻译成`梁木`，阴云将逐渐消散，baulk 也可以成为 `梁木`。

我认为 Baulk 是个有趣的工具，在这篇文章中，我将说一说它为什么会有趣。

## 为什么会有 Baulk

为什么会有 Baulk，我想到了一个词**文人相轻**，总觉得自己能够写出更好的软件，他人写的并不能满足我的需求，于是乎，也就有了 Baulk。

《典论·论文》- 曹丕： `文人相轻,自古而然.傅毅之于班固,伯仲之间耳.而固小之,与弟超书曰：“武仲以能属文，为兰台令史,下笔不能自休.”夫人善于自见.而文非一体,鲜能备善,是以各以所长,相轻所短.里语曰：“家有弊帚,享之千金.”斯不自见之患也。`

在开发 Baulk 很长一段时间，我使用 PowerShell 开发了一个简单的包管理软件 devi，这个软件最初用于 Clang on Windows 的构建工具的安装，升级。毕竟重复得一次次手动下载安装一些软件总显得有些愚蠢，就如同我们重复说同样的话会被别人觉得怪异。

使用 PowerShell 开发的 devi 缺点也很明显，慢，尽管我是一个 PowerShell 粉丝，但事实我也需要说出来，PowerShell 的加载速度慢，毕竟它需要加载 PowerShell 解析引擎，如果还加载一些启动配置文件，那个速度就很感人了。

2020 年上半年，我终于开始不再忍受基于 PowerShell 的 devi，于是就开始使用 C++17（后来升级为 C++20） 开发新的包管理器 baulk，在 Windows 上有 scoop、chocolatey 这样优秀的开源包管理器，但它们都是基于 PowerShell（C#） 开发，然后在环境变量和安装机制上也与 Baulk 有一些分歧。这些不同之处也是 baulk 存在的原因。

## Baulk 有哪些特色

在 baulk 的 ReadMe 页面，介绍了 baulk 的一些特色，但很繁杂，并不是很容易了解全，这里再次简单的介绍一下。

### Baulk 是一个绿色的包管理器

在 baulk 的设计哲学中，便携，绿色是其管理软件的宗旨，这衍生出来一些特性，baulk 在安装软件的过程中所执行的动作只有：

1.  解压压缩包
2.  移动到安装目录
3.  创建命令的符号链接或者启动器

为了实现解压各种格式的安装包，baulk 将 Golang 的 [archive/zip](https://github.com/golang/go/tree/master/src/archive/zip) 和 [archive/tar](https://github.com/golang/go/tree/master/src/archive/zip) 使用 C++20 重新实现，提供了 `.zip`、`tar.*` 等格式的解压能力。其中 zip 解压缩相比于 Minizip 新增支持 deflate64 解压缩能力，与 7z 的 zip 解压缩相比，支持 zstd，可以说 baulk 的解压缩能力还是不错的，另外，baulk 集成了 google 的 [Compact Encoding Detection(CED for short)](https://github.com/google/compact_enc_det) 支持文本编码的自动识别，而一些旧的 zip 文件名并非使用 UTF-8，导致在不同的代码也共享具有中文或者其他多字节编码的文件名的 zip 文件查看或者解压缩乱码，这个问题在 baulk 中被基本解决。

baulk 创新的使用了 Linux/macOS 的安装-软链接技术，这样能够合并 PATH 环境变量的条目数，降低操作系统在搜索环境变量的次数，对的你没看错，操作系统在查找 PATH 中的命令时需要一个目录去找，如果环境变量 PATH 组成的目录太多，会导致搜索次数太多，另外还有一个非常严重的问题，PATH 条目过多还存在 环境变量冲突的影响。baulk 的软链机制就很好的解决了这些问题。有些程序依赖自身目录下的动态链接库，并没有正确处理 GetModuleFilename，所以我们还增加了启动器的机制，当用户安装了 Visual Studio (c++ compiler exists)，我们会生成极简的启动源码，编译成启动器，如果没有则会使用 baulk-lnk.exe 代理，baulk-lnk.exe 需要分析安装信息，所以要慢一点点，只有一点点，不会太多。

## Baulk 的环境隔离特性

baulk 作为一个绿色的包管理器，一个很强的特性是环境隔离机制，为了支持并行安装同类软件的多个版本，我在 baulk 中引入了虚拟环境隔离机制，对于像 JDK/Golang/DMD/node.js 这样的软件，用户需要安装多个版本并且相互不影响，这个时候 baulk 就派上大用场了，baulk 也不需要像 RVM(ruby) 那样麻烦就能做到提供一套通用解决方案：

下面以 zulu（JDK 的著名发行版）为例，在这里我们支持的 venv 能够指定 category，这是一个大的分类，比如所有的 JDK 发行版的分类就应该是 java，而 `path` 则告知 baulk-exec 在加载虚拟环境时需要追加的 PATH 环境变量，而 env 则是除了 path 之外其他追加的环境变量，这些变量都支持推导，baulk 内置一些变量可以帮助开发者更好的展开环境变量。

```json
{
    "description": "Zulu is certified build of OpenJDK",
    "version": "16.0.2",
    "homepage": "https://www.azul.com/",
    "url64": "https://cdn.azul.com/zulu/bin/zulu16.32.15-ca-jdk16.0.2-win_x64.zip",
    "url64.hash": "SHA256:2b9ad008596c535c0b4da6ddabe56b35af3198cf00a933b1e60e074a30f31047",
    "urlarm64": "https://cdn.azul.com/zulu/bin/zulu16.30.17-ca-jdk16.0.1-win_aarch64.zip",
    "urlarm64.hash": "SHA256:5263cfc5f9526a5cb9520111d5abb506c6cad18bd1d1ea165d0f218cfd1318c3",
    "extension": "zip",
    "venv": {
        "category": "java",
        "path": "${BAULK_PKGROOT}\\bin",
        "env": [
            "JAVA_HOME=${BAULK_PKGROOT}"
        ]
    }
}
```

当然，baulk 还支持 `include`、`lib` 这类特殊的环境变量，这些单独抽出来是为了方便追加。另外 baulk 还支持 `dependencies` 机制，比如 kotlin-native 就依赖 zulu。

baulk 虚拟环境的精髓在 baulk-exec 中得以实现，其命令行如下：

```txt
baulk-exec - Baulk extend executor
Usage: baulk-exec [option] <command> [<args>] ...
  -h|--help            Show usage text and quit
  -v|--version         Show version number and quit
  -V|--verbose         Make the operation more talkative
  -C|--cleanup         Create clean environment variables to avoid interference
  -W|--cwd             Set the command startup directory
  -A|--arch            Select a specific arch, use native architecture by default
  -E|--venv            Choose to load a specific package virtual environment
  --vs                 Load Visual Studio related environment variables
  --vs-preview         Load Visual Studio (Preview) related environment variables
  --clang              Add Visual Studio's built-in clang to the PATH environment variable
  --time               Summarize command system resource usage

Example:
  baulk-exec -V --vs TUNNEL_DEBUG=1 pwsh

Built-in alias:
  winsh          A fake shell. It may be pwsh or powershell and cmd.
  pwsh           PowerShell Core
  pwsh-preview   PowerShell Core Preview

```

对了，如果用户觉得环境变量乱糟糟的，可以使用 `-C/--cleanup` 机制避免外部混乱的环境变量带来干扰，为了方便 C++ 开发，baulk 支持初始化 Visual C++ 环境变量，`--clang` 则可以初始化 Visual Studio 内置的 clang，当然你还可以使用 `--time` 去分析执行某个命令所消耗的时间。

```shell
 >$ baulk-exec --time baulk unzip .\DG520884_x64.zip
x DiskGenius\VPreview.dll
run command 'baulk unzip .\DG520884_x64.zip' use time:
Creation: 625.6us
Kernel:   125ms
User:     93.75ms
Exit:     231.0306ms
Wall:     231.9762ms
```

## Baulk 的其他功能

baulk 还实现了许多其他的功能，限于篇幅长度也就不一一说了，有兴趣可以访问 [[baulk/ReadMe.zh-CN.md at master · baulk/baulk (github.com)](https://github.com/baulk/baulk/issues)] 提出建议或者参与到项目的贡献当中。

