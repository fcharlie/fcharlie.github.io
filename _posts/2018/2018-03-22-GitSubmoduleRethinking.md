---
layout: post
title:  "Git Submodule 的反思"
date:   2018-03-22 10:00:00
published: true
categories: git
---

## Git Submodule 介绍

Git Submodule 翻译成中文叫做`子模块`，将一个存储库（如 B）作为另一个（如 A）存储库的子目录，这个存储库就可以称之为 B 是 A 的子模块。
在 ProGit2: [https://git-scm.com/book/en/v2/Git-Tools-Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules), Git-SCM 官网：[https://www.git-scm.com/docs/gitsubmodules](https://www.git-scm.com/docs/gitsubmodules)，对 submodule 有非常详细的介绍。

Git Submodule 的引入主要是为了管理项目依赖或者将项目进行拆分，在不同的团队开发然后在发布的时候再进行合并。Git Submodule 是 Git 比较重要的功能之一，包括像 [Boost](https://github.com/boostorg/boost) [gPRC](https://github.com/grpc/grpc) [Protobuf](https://github.com/google/protobuf) [MSIX-Packaging](https://github.com/Microsoft/msix-packaging) [wxWidgets](https://github.com/wxWidgets/wxWidgets) [PowerShell](https://github.com/PowerShell/PowerShell) [AvaloniaUI](https://github.com/AvaloniaUI/Avalonia) [LDC](https://github.com/ldc-developers/ldc) [nghttp2](https://github.com/nghttp2/nghttp2) 这样的项目都在使用 Submodule 功能，可以说 Submodule 的使用面还是比较广的。在码云内部，一些基础设施项目都是使用 Submodule 管理项目依赖。用户只需要在工作目录运行下面命令就可以轻松将依赖克隆下来并且 checkout 到规定的 commit。

```shell
git submodule init
git submodule update
```

Git Submodule 的好处显而易见，可以简化项目依赖管理。比如在 Ubuntu 系统中，Ubuntu 包管理器 apt-get 渠道的依赖库一般都不会及时更新，但开发者需要使用最新版以来，这个时候就需要使用源码构建了，如果将指定的存储库作为子模块添加到存储库，然后修改构建文件，比如 `CMakeLists.txt` 这样的话构建起来就非常的简单。另外在编写不同的程序时，如果以来类似的功能，我们可以将类似的功能实现单独存储，然后在不同的程序源码中添加子模块，这样就实现了功能的拆分和抽象。

## Git Submodule 的代价

在使用 Submodule 的过程中我们很快可以发现，Submodule 的缺陷非常明显，由于 Git 是分布式版本控制系统，这通常意味着其 Submodule 也是一个独立的存储库，虽然我们可以使用 `--depth=1` `--single-branch`  `--shallow-since` 的方式去克隆存储库，但对于 Submodule 而言，无论是 `git clone --recursive(--recurse-submodules)` 还是 `git submodule int ;git submodule update` 都写命令都会将子模块完整的克隆下来。受限于 Github 的网络状况，一些 Submodule 在 Github 上项目子模块经常的克隆失败，笔者在开发中也遇到很多次这种情况。

虽然 git submodule 能够接受 `--depth,--single-branch,--shallow-since` 但用户又怎么能确定存储库树上的 commit 存在于这些克隆方式拉去的对象中呢？倘若不在，用户实质上是会检出失败的。

有的人喜欢 Submodule，但是 Submodule 的问题也让人避之不及。有的项目干脆将依赖完全放入自己目录而不是用 Submodule，比如 [NodeJS](https://github.com/nodejs/node)，[ChakraCore](https://github.com/Microsoft/ChakraCore)，[CMake](https://github.com/Kitware/CMake)。

也有的也是将源码放入自身目录，但一些其他依赖用其包管理器管理，如：[CoreCLR](https://github.com/dotnet/coreclr) [CoreRT](https://github.com/dotnet/coreclr)。

还有的在构建的时候下载自身的依赖，如：[Proxygen](https://github.com/facebook/proxygen)
，也有的要开发者一步一步操作：[fbthrift](https://github.com/facebook/fbthrift/tree/master/build/deps/github_hashes/facebook)，或者设置布局：[Swift](https://github.com/apple/swift)

Git 不能完全解决存储库克隆大量数据的问题，Submodule 同样不能解决。

## 特定提交的克隆和下载

话说回来，Git 这种缺点是由其默认不支持检出克隆特定的 commit 造成的。即无法指定特定 commit 进行浅表克隆，任何克隆都需要从发现引用开始。然后从引用反向计算。查找特定 commit 并不是一个耗时的操作，在 git 中，如果对象存在在松散对象目录中，我们可以马上找到，反之如果存在在包文件中，使用二分法，最多每个包 8 次就可以知道 commit 对象是否存在，找到 commit 后，然后构建传输包即可。

如果我们设置了 `uploadpack.allowReachableSHA1InWant=true` 然后执行

```
git init
git remote add origin <url>
git fetch --depth 1 origin <sha1>
git checkout FETCH_HEAD
```

参考 [git-fetch-pack.txt#L122-L124](https://github.com/git/git/blob/f8edeaa05d8623a9f6dad408237496c51101aad8/Documentation/git-fetch-pack.txt#L122-L124)

实际上，Git UploadArchive 同样不支持打包特定提交的压缩包，除非设置了 `uploadArchive.allowUnreachable=true`


这样拒绝也是有原因的，如果一个对象被删除了，但没有被 GC 清理掉，Upload-Pack UploadArchive 这时候都可以获得这些数据，这就存在一个安全问题：[git-upload-archive.txt#L26-L31](https://github.com/git/git/blob/565301e41670825ceedf75220f2918ae76831240/Documentation/git-upload-archive.txt#L26-L31)

由于 HTTP Archive 通常用 `git-archive` 包装实现，因此支持下载特定的 commit。

而对于代码托管平台来说并不一定会开启这几个参数，所以对 Submodule 而言并没什么效果，当然我也非常乐意 Git 在这方面改进的。

## 备注

1. [How to shallow clone a specific commit with depth 1?](https://stackoverflow.com/questions/31278902/how-to-shallow-clone-a-specific-commit-with-depth-1)