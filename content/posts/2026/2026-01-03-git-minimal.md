+++
title = "在 Linux 构建和发行 Git 极简版"
date = "2026-01-03T20:00:00+08:00"
categories = "git"
+++

## 前言

去年 12 月底，我在 Gitee 看到了我 8 年前写的 [Git 构建脚本：git-dist](https://gitee.com/oscstudio/git-dist)，这个项目至今仍在维护，看起来 Gitee 仍然在使用这一脚本，其核心思路也没有太大变化。我现在对如何构建 Git 有了新的方法，似乎可以重新开始编写一个项目来在 Linux 上构建和打包 Git，我将它称之为 [git-minimal](https://github.com/baulk/git-minimal)。

[git-minimal](https://github.com/baulk/git-minimal) 尽管只是一个小项目，但还是花了我不少时间，到了今天才把这个事情弄完，现在制作了 [Git 2.52.0](https://github.com/baulk/git-minimal/releases/tag/v2.52.0) 的安装包 ，基本能覆盖到 Linux `x86_64/aarch64` 平台，在脚本中还预留了对龙芯平台的支持。

## 构建之前

git 最重要的依赖是 cURL，用于 Git over HTTP。在构建 git 之前，我们需要先准备好 cURL。几年前，我维护了一个 cURL 的构建仓库（[wincurl](https://github.com/fcharlie/wincurl)）。该仓库使用 MSVC 编译 cURL 的各个依赖并进行静态链接，同时开启了 HTTP/2 和 HTTP/3。后来我在公司内部构建 Git 发行版时，也参考了这一脚本。构建 cURL 需要依赖 zlib（现可由 zlib-ng 替代，且通常性能更好）、zstd、brotli、OpenSSL（也可以使用 LibreSSL 等）、nghttp2（开启 HTTP/2 支持）、nghttp3（开启 HTTP/3 支持，并可选用 ngtcp2 或 openssl 3.6 开启 QUIC 支持）、libssh2（开启 SCP 支持）。

一直以来，我更喜欢使用 PowerShell 而不是 POSIX Shell 编写脚本。PowerShell 更加现代化，PowerShell Core 的出现解决了 PowerShell 脚本的跨平台问题，这让我更乐意使用它。PowerShell 也有它固有的问题：运行较慢，不过对于构建脚本而言这是可以接受。PowerShell 还有另一个让我难以忍受的缺点：PowerShell Core 在 Linux 上动态链接了 libstdc++，这就导致无法在较老的系统上使用最新版本 PowerShell Core。在接触到 [Nushell](https://github.com/nushell/nushell) 之后，我曾考虑用它替代 PowerShell。最近我在公司内部也用 Nushell 重写了一些脚本，现在看来或许也可以用 Nushell 编写构建脚本了。

这样看来，我只要按照之前 wincurl 的思路再编写一个脚本，不就能轻松完成这件事情了吗？

## 构建的挑战

### 时间

这个项目是我个人业余的实践，这意味着我只能周末或假期才能编码，然而在周末我也只能像挤海绵一样的挤时间。我很想对年轻的程序员说，趁着年轻，遇到感兴趣的事情多去做一下。

### 交叉编译

在 Github Actions 可用的运行镜像只有 Windows/macOS/Ubuntu，当然还有一些手段可以运行其他的环境，但如果不想折腾的话，对于 `git-minimal` 可选的就只有 Ubuntu，可喜的是，Github 上有人构建了交叉编译工具链 [https://github.com/cross-tools/musl-cross](https://github.com/cross-tools/musl-cross)，编译了很多架构的 GCC+ musl libc 工具链，我们可以在 Github actions 脚本中下载并配置好工具链：

```shell
MUSL_CROSS_VERSION="20250929"
MOLD_VERSION="2.40.4"
case $BUILD_TARGET in
x86_64-unknown-linux-musl)
	curl -o x86_64-unknown-linux-musl.tar.xz -L "https://github.com/cross-tools/musl-cross/releases/download/$MUSL_CROSS_VERSION/x86_64-unknown-linux-musl.tar.xz" || exit
	tar -xf x86_64-unknown-linux-musl.tar.xz || exit
	sudo mv x86_64-unknown-linux-musl "$BUILD_TOOLS_DIR" || exit
	export PATH="$BUILD_TOOLS_DIR/x86_64-unknown-linux-musl/bin:$PATH"
	export CC="x86_64-unknown-linux-musl-gcc"
	export CXX="x86_64-unknown-linux-musl-g++"
	export AR="x86_64-unknown-linux-musl-gcc-ar"
	export RANLIB="x86_64-unknown-linux-musl-gcc-ranlib"
	;;
aarch64-unknown-linux-musl)
	curl -o aarch64-unknown-linux-musl.tar.xz -L "https://github.com/cross-tools/musl-cross/releases/download/$MUSL_CROSS_VERSION/aarch64-unknown-linux-musl.tar.xz" || exit
	tar -xf aarch64-unknown-linux-musl.tar.xz || exit
	sudo mv aarch64-unknown-linux-musl "$BUILD_TOOLS_DIR" || exit
	# TODO: It seems that the git Makefile does not recognize the STRIP environment variable, so we need to override strip.
	sudo ln -sf aarch64-unknown-linux-musl-strip "$BUILD_TOOLS_DIR/aarch64-unknown-linux-musl/bin/strip"
	export PATH="$BUILD_TOOLS_DIR/aarch64-unknown-linux-musl/bin:$PATH"
	export CC="aarch64-unknown-linux-musl-gcc"
	export CXX="aarch64-unknown-linux-musl-g++"
	export AR="aarch64-unknown-linux-musl-gcc-ar"
	export RANLIB="aarch64-unknown-linux-musl-gcc-ranlib"
	;;
loongarch64-unknown-linux-musl)
	curl -o loongarch64-unknown-linux-musl.tar.xz -L "https://github.com/cross-tools/musl-cross/releases/download/$MUSL_CROSS_VERSION/loongarch64-unknown-linux-musl.tar.xz" || exit
	tar -xf loongarch64-unknown-linux-musl.tar.xz || exit
	sudo mv loongarch64-unknown-linux-musl "$BUILD_TOOLS_DIR" || exit
	sudo ln -sf aarch64-unknown-linux-musl-strip "$BUILD_TOOLS_DIR/loongarch64-unknown-linux-musl/bin/strip"
	export PATH="$BUILD_TOOLS_DIR/loongarch64-unknown-linux-musl/bin:$PATH"
	export CC="loongarch64-unknown-linux-musl-gcc"
	export CXX="loongarch64-unknown-linux-musl-g++"
	export AR="loongarch64-unknown-linux-musl-gcc-ar"
	export RANLIB="loongarch64-unknown-linux-musl-gcc-ranlib"
	;;
*)
	wget -O- --timeout=10 --waitretry=3 --retry-connrefused --progress=dot:mega https://github.com/rui314/mold/releases/download/v${MOLD_VERSION}/mold-${MOLD_VERSION}-x86_64-linux.tar.gz | sudo tar -C /usr/local --strip-components=1 --no-overwrite-dir -xzf -
	export CC="gcc"
	export CXX="g++"
	export AR="gcc-ar"
	export RANLIB="gcc-ranlib"
	;;
esac
```

`cmake` 和 `autoconf` 基本都能正确识别工具链，构建的过程中都没有什么问题，但 `OpenSSL` 的构建体系较为独立，需要做一些调整：

```nushell
        if ($target | str starts-with "aarch64") {
            $opensslOptions = $opensslOptions | append "linux-aarch64"
        } else if ($target | str starts-with "loongarch64") {
            # loongarch64-unknown-linux-musl
            $opensslOptions = $opensslOptions | append "linux64-loongarch64"
        }
```

在构建 git 之前基本没有出什么问题，但运行 `configure` 阶段一直报告交叉编译的程序无法运行，看来 git 对这块做的不够好，我们可以使用 configure 缓存机制跳过这些运行性检查：

```nushell
    if not ($target | str starts-with $nu.os-info.arch) {
        # If git's configure is not handled properly, cross-compilation will fail, and the configure cache mechanism needs to be used to intercept it.
        # The following is the correct configuration for caching under musl libc (refer to alpine x86_64).
        let crossCompileCache = [
            "ac_cv_iconv_omits_bom='yes'"
            "ac_cv_fread_reads_directories='yes'"
            "ac_cv_snprintf_returns_bogus='no'"
        ]
        $crossCompileCache | str join (char newline) | save --append $"($GIT_SOURCE_DIR)/($target).cache"
        $gitOptions = $gitOptions | append [$"--host=($target)",$"--cache-file=($GIT_SOURCE_DIR)/($target).cache"] # FIXME: fix git check iconv check
    }
```

从构建的过程来看，如果使用 cmake 构建项目要好于自建工具和 configure 系统的，配置交叉工具也比较简单。在这个项目中，我使用的是 GCC + Binutils 作为交叉工具链，在去除 git 符号的时候遇到 strip 格式不支持的问题，设置 `STRIP=aarch64-unknown-linux-musl-strip` 并不生效，通过软链解决了问题，在这个场景，我们如果使用 LLVM+Clang 工具链要好一些，`llvm-strip` 可以作为 binutils strip 的替代，并且一个一个命令支持多个架构，另一方面，git 基于 configure + Makefile 的构建机制并没有适配好交叉编译。

musl-cross 的作者还提供了 [clang-cross](https://github.com/cross-tools/clang-cross)，但 clang-cross 也仅是使用 clang/clang++ 代替 gcc/g++，使用 lld/llvm-tools 代替 Binutils，但没有使用 libc++/libc++abi 代替 libstdc++，没有使用 libunwind/compiler-rt 代替 libgcc ，这不像 [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) 做的彻底（完全使用 llvm-tools/lld/clang/libc++/libc++abi/compiler-rt），而 llvm-mingw 还实现了一套工具链支持 Windows x86/amd64/arm64 三套架构。

实际上交叉编译还有一个选择，[`zig cc`: a Powerful Drop-In Replacement for GCC/Clang](https://andrewkelley.me/post/zig-cc-powerful-drop-in-replacement-gcc-clang.html)，zig 是一个年轻的通用编程语言，可以作为 C 的替代者，它最有趣的一点是可以支持 C/C++ 的交叉编译，它的做法是将 glibc 的 ABI list，musl 源码，libc++/libc++abi/libunwind/compiler-rt 源码，MinGW-w64，libSystem.tbd (darwin) 随工具链一起分发，zig 嵌入了一个 clang 编译器和 lld 链接器，从而实现了交叉编译的能力，尽管 clang 支持多系统多 CPU 架构，但它并不携带关键的 libc，而自身的 libc++/libc++abi/libunwind/compiler-rt 这些也不是随身携带的，其生态位是缺失的，zig 很机智地填补了这个生态位，Github 上也有为 CMake 适配了 Zig 做交叉编译工具的项目。当然 Zig 也并不是没有缺点，第一次编译因需要编译 libc 通常会慢一些。

而 llvm 还有个雄心勃勃但进展迟缓的项目 [libc](https://github.com/llvm/llvm-project/tree/main/libc)，如果 llvm 能做好交叉编译的事情，把配套做好，整个 C/C++ 的生态会好很多，现在流行的 Golang，Rust，在交叉编译上比这些前辈要好很多。

### 打包与分发

一般而言，制作 rpm 需要使用 rpmbuild，制作 deb 要使用 dh-make，如果你在运行在本机，制作本机安装包要容易的多，但如果是交叉编译，并且要保持较高的兼容性，那就没有那么容易，一来是打包工具可能存在版本兼容性问题，二来是这些工具并不总是容易安装。比如我曾经在 Kali Linux 制作了 rpm 包在 centos 7 上安装失败，Kali Linux 的 rpmbuild 版本较高，centos 7 不支持。我在编写 Golang 构建打包工具 [https://github.com/balibuild/bali](https://github.com/balibuild/bali) 曾经接触过 Google 员工开发的 [https://github.com/google/rpmpack](https://github.com/google/rpmpack) 可以制作 rpm 包;还使用了 [https://github.com/goreleaser/nfpm](https://github.com/goreleaser/nfpm) 制作 deb 包，在 git-minimal 这个场景我没有必要写打包工具，可以直接使用 nfpm 的能力，编写一个 YAML 的包即可：

```yaml
# https://nfpm.goreleaser.com/docs/configuration/
name: "git-minimal-musl"
arch: "${BUILD_ARCH}"
platform: linux
version: "2.52.0"
release: "${BUILD_RELEASE}"
maintainer: "J23 <fcharlie@users.noreply.github.com>"
vendor: Baulk
description: |
  "Unofficial Git minimal build"
homepage: "https://github.com/baulk/git-minimal"
license: GPLv2
rpm:
  group: Unspecified
  summary: Unofficial Git minimal build
  compression: xz
contents:
  - src: _out
    dst: /
    type: tree
```

使用 nfpm 我可以制作 rpm,deb,apk(alpine package) 等等包。


### RUNTIME_PREFIX 与启动器

使用包管理器安装 git-minimal 的时候可以指定 prefix 为 `/usr/local`，但如果用户想解压 git-minimal 直接在任意位置运行，那就歇菜了，虽然可以配置 `RUNTIME_PREFIX` 解决任意位置运行的问题，但我们还依赖 cURL CA-BUNDLE (`curl-ca-bundle.crt`)，它是 cURL 定期从 Mozilla 提取的，我们携带用来解决 SSL 证书验证问题，当然如果 git-minimal 解压到任意位置，可能会导致证书验证失败，但 Git 还留了一条路，即环境变量可以修改这个文件的路径，即 `GIT_SSL_CAINFO`。这个时候我可以参考 git-for-windows 的思路写一个启动器，配置好环境后运行 `execve` 启动对应的命令，在只使用较少工具链的基础上，我使用 C++23 编写了这个启动器，设置好环境变量后使用 execve 执行对应的命令：

```c++
#include <filesystem>
#include <string>
#include <ranges>
#include <format>
#include <vector>
#include <cstring>
#include <algorithm>
#include <optional>
#include <utility>
#include <unistd.h>
constexpr const char Separator = ':';
constexpr const std::string_view Separators = ":";

namespace bela {
template <class F> class final_act {
public:
  explicit final_act(F f) noexcept : f_(std::move(f)), invoke_(true) {}
  final_act(final_act &&other) noexcept : f_(std::move(other.f_)), invoke_(std::exchange(other.invoke_, false)) {}
  final_act(const final_act &) = delete;
  final_act &operator=(const final_act &) = delete;
  ~final_act() noexcept {
    if (invoke_) {
      f_();
    }
  }

private:
  F f_;
  bool invoke_{true};
};
// Final() - convenience function to generate a final_act
template <class F> inline final_act<F> Final(const F &f) noexcept { return final_act<F>(f); }
template <class F> inline final_act<F> Final(F &&f) noexcept { return final_act<F>(std::forward<F>(f)); }
} // namespace bela

constexpr size_t PathSizeMax = 4096;

constexpr int memcasecmp(const char *s1, const char *s2, size_t len) noexcept {
  for (size_t i = 0; i < len; i++) {
    const auto diff = std::tolower(s1[i]) - std::tolower(s2[i]);
    if (diff != 0) {
      return static_cast<int>(diff);
    }
  }
  return 0;
}

bool EqualsIgnoreCase(std::string_view piece1, std::string_view piece2) noexcept {
  return (piece1.size() == piece2.size() && memcasecmp(piece1.data(), piece2.data(), piece1.size()) == 0);
}

bool StartsWithIgnoreCase(std::string_view text, std::string_view prefix) noexcept {
  return (text.size() >= prefix.size()) && EqualsIgnoreCase(text.substr(0, prefix.size()), prefix);
}

std::string Executable() noexcept {
  char exePath[PathSizeMax];
  constexpr const char *procSelf = "/proc/self/exe";
  ssize_t len = readlink(procSelf, exePath, sizeof(exePath));
  if (len < 0) {
    return "";
  }
  len = std::min(len, ssize_t(sizeof(exePath) - 1));
  exePath[len] = '\0';
  // On Linux, /proc/self/exe always looks through symlinks. However, on
  // GNU/Hurd, /proc/self/exe is a symlink to the path that was used to start
  // the program, and not the eventual binary file. Therefore, call realpath
  // so this behaves the same on all platforms.
#if _POSIX_VERSION >= 200112 || defined(__GLIBC__)
  if (char *absPath = realpath(exePath, nullptr)) {
    std::string result = std::string(absPath);
    free(absPath);
    return result;
  }
#else
  char absPath[PATH_MAX];
  if (realpath(exePath, absPath)) {
    return std::string(exePath);
  }
#endif
  return "";
}

std::optional<std::string> search_bundle(const std::filesystem::path &prefix) {
  auto bundle = prefix / "share/git-minimal/curl-ca-bundle.crt";
  std::error_code ec;
  if (std::filesystem::exists(bundle, ec)) {
    return std::make_optional<>(bundle.string());
  }
  bundle = prefix / "share/curl-ca-bundle.crt";
  if (std::filesystem::exists(bundle, ec)) {
    return std::make_optional<>(bundle.string());
  }
  return std::nullopt;
}

std::filesystem::path search_root() {
  auto self = Executable();
  return std::filesystem::path(self).parent_path().parent_path(); // auto move
}

std::optional<std::filesystem::path> search_command(const std::filesystem::path &prefix, const char *arg0) {
  auto command = prefix / "bin" / std::filesystem::path(arg0).filename();
  std::error_code ec;
  if (std::filesystem::exists(command)) {
    return std::make_optional<>(std::move(command));
  }
  return std::nullopt;
}

// $prefix/cmd/git
// $prefix/cmd/git-upload-pack
// $prefix/cmd/git-receive-pack
int main(int argc, const char *argv[]) {
  if (argc <= 0) {
    fprintf(stderr, "git-minimal launcher fatal: missing args\n");
    return 1;
  }
  auto self = Executable();
  auto prefix = std::filesystem::path(self).parent_path().parent_path();
  auto command = search_command(prefix, argv[0]);
  if (!command) {
    auto filename = std::filesystem::path(argv[0]).filename().string();
    fprintf(stderr, "git-minimal launcher fatal: command '$prefix/bin/%s' not found\n", filename.c_str());
    return 1;
  }
  if (std::filesystem::equivalent(self, *command)) {
    fprintf(stderr, "git-minimal launcher fatal: self equivalent command\n");
    return 1;
  }
  auto basename = std::filesystem::path(argv[0]).filename().string();
  auto execPath = prefix / "libexec" / "git-core";
  std::vector<char *> newArgs;
  newArgs.push_back(strdup(basename.data()));
  for (int i = 1; i < argc; i++) {
    newArgs.push_back(strdup(argv[i]));
  }
  newArgs.push_back(nullptr);
  auto closer = bela::Final([&] {
    for (auto a : newArgs) {
      if (a != nullptr) {
        std::free(a);
      }
    }
  });
  std::vector<char *> newEnviron;
  std::string execEnv = std::format("GIT_EXEC_PATH={0}", execPath.string()); // GIT_EXEC_PATH
  newEnviron.push_back(execEnv.data());
  // https://en.cppreference.com/w/cpp/filesystem/path/formatter std::format support std::filesystem::path starts C++26
  std::string newPath = std::format("PATH={0}{1}{2}", command->parent_path().string(), Separator, getenv("PATH"));
  newEnviron.push_back(newPath.data());
  // https://git-scm.com/docs/git-config GIT_SSL_CAINFO
  std::string caBundle;
  if (auto bundle = search_bundle(prefix); bundle) {
    caBundle = std::format("GIT_SSL_CAINFO={0}", *bundle);
    newEnviron.push_back(caBundle.data());
  }
  constexpr std::string_view PathEnv = "PATH";
  constexpr std::string_view CAINFO = "GIT_SSL_CAINFO";
  constexpr std::string_view EXEC = "GIT_EXEC_PATH";
  if (environ != nullptr) {
    for (int i = 0;; i++) {
      auto e = environ[i];
      if (e == nullptr) {
        break;
      }
      std::string_view s(e);
      auto pos = s.find('=');
      if (pos == std::string_view::npos) {
        continue;
      }
      auto envKey = s.substr(0, pos);
      if (EqualsIgnoreCase(envKey, PathEnv) || EqualsIgnoreCase(envKey, CAINFO) || EqualsIgnoreCase(envKey, EXEC)) {
        continue;
      }
      newEnviron.push_back(e);
    }
  }
  newEnviron.push_back(nullptr);
  // int execve(const char *pathname, char *const argv[], char *const envp[]);
  auto commandPath = command->string();
  auto result = execve(commandPath.data(), &newArgs[0], &newEnviron[0]);
  if (result != 0) {
    fprintf(stderr, "git-minimal launcher execve failed\n");
    exit(EXIT_FAILURE);
  }
  // Not reached if execve succeeds
  return 0;
}
```

在制作 `tar.xz` 创建好符号链接：

```nushell
let CXX = $env | get CXX? | default "g++"
let BUILD_CXXFLAGS = [
    "-std=c++23","-O2",
    $BUILD_MARCH,
    "-flto",
    "-fuse-ld=mold",
    "-Wl,-O2,--as-needed,--gc-sections",
    "-static",
    $"($SOURCE_DIR)/cmd/git-minimal/main.cc",
    "-o",
    $"($DESTDIR)($prefix)/cmd/git-minimal"
]
if (Exec --cmd $CXX --args $BUILD_CXXFLAGS) == 0 {
    print -e "build git-minimal launcher success"
    Exec --cmd "ln" --args ["-sf","git-minimal",$"($DESTDIR)($prefix)/cmd/git"] | ignore
    Exec --cmd "ln" --args ["-sf","git-minimal",$"($DESTDIR)($prefix)/cmd/curl"] | ignore
    Exec --cmd "ln" --args ["-sf","git-minimal",$"($DESTDIR)($prefix)/cmd/git-receive-pack"] | ignore
    Exec --cmd "ln" --args ["-sf","git-minimal",$"($DESTDIR)($prefix)/cmd/git-shell"] | ignore
    Exec --cmd "ln" --args ["-sf","git-minimal",$"($DESTDIR)($prefix)/cmd/git-upload-archive"] | ignore
    Exec --cmd "ln" --args ["-sf","git-minimal",$"($DESTDIR)($prefix)/cmd/git-upload-pack"] | ignore
    Exec --cmd "ln" --args ["-sf","git-minimal",$"($DESTDIR)($prefix)/cmd/scalar"] | ignore
}
```

用户将 `$prefix/cmd` 目录添加到 PATH 或者直接用全路径就能正常运行了。

## 最后

祝大家新年快乐
