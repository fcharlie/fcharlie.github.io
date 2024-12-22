+++
title = "七月的技术分享"
date = "2019-07-09T10:00:00+08:00"
categories = "talk"
+++

## 前言

写代码是一个不断积累的过程，将一些好的想法转变为解决实际问题的程序通常让程序员感到愉悦。而最近我也有两个还算好的想法，在本文中分享给大家。

## 支持环境变量展开的配置解析

在 Gitee 的基础服务组件中，像 Basalt (git ssh 服务器)，git-diamond (Gitee 内部的 git 协议服务器)，git-srv (Gitee 分布式 git 传输后端) 都支持这样形式的配置： `${APPDIR}/run/git-srv.pid` 。在运行过程中，`APPDIR` 被解释成相应组件的安装根目录，在配置文件中，解析到 `${APPDIR}/run/git-srv.pid` 后，使用推导函数，将其展开为 `/home/git/oscstudio/run/git-srv.pid`。这样一来，默认配置情况下，Gitee 的这些组件都支持安装到任意位置，而无需在编译时设置 `--prefix`。而像 nginx 这样的软件，在构建时，使用 `--prefix` 指定了安装目录后，如果不使用 `-p` 指定 `prefix`，是无法安装到任意位置的。

除此之外，环境变量展开还可以做很多事情，但环境变量展开是如何实现的？

在 Golang 的源码中，有一个 [`os.ExpandEnv`](https://github.com/golang/go/blob/06ef108cec98b3dfc0fba3f49e733a18eb9badd5/src/os/env.go#L50) 函数可以使用环境变量替换输入的字符串中所有以格式 `${var}` 和 `$var` 的字符串 ，而这个函数实际上是 `os.Expand` 使用 `os.GetEnv` 的特例，因此，你完全可以封装自己的 `GetEnv`。在 [`oscstudio/gitenv`](https://gitee.com/oscstudio/gitenv/blob/master/envctx.go) 中，我们就有一个 `Envcontext` 用于实现上述 `APPDIR` 这样的环境变量解析。

```go
package gitenv

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
)

// Envcontext is
type Envcontext struct {
	Env map[string]string
}

// NewEnvcontext todo
func NewEnvcontext() (*Envcontext, error) {
	exe, err := os.Executable()
	if err != nil {
		return nil, err
	}
	ec := &Envcontext{Env: make(map[string]string)}
	exedir := filepath.Dir(exe)
	if filepath.Base(exedir) == "bin" {
		ec.Env["APPDIR"] = filepath.Dir(exedir)
	} else {
		ec.Env["APPDIR"] = exedir
	}
	return ec, nil
}

// Append env
func (ec *Envcontext) Append(key, value string) error {
	if len(key) == 0 || len(value) == 0 {
		return errors.New("Empty key value")
	}
	ec.Env[key] = value
	return nil
}

// Delete Env
func (ec *Envcontext) Delete(key string) error {
	if _, ok := ec.Env[key]; ok {
		delete(ec.Env, key)
		return nil
	}
	return fmt.Errorf("'%s' not exists", key)
}

// Expand callback
func (ec *Envcontext) Expand(s string) string {
	if _, ok := ec.Env[s]; ok {
		return ec.Env[s]
	}
	return os.Getenv(s)
}

// Expandenv is
func (ec *Envcontext) Expandenv(s string) string {
	return os.Expand(s, ec.Expand)
}
```

上述 `Envcontext` 是基于 Golang 的，但 Gitee 很多服务是基于 C++ 编写，因此，我们需要一个 C++ 版本，为了支持异构查找，我们使用 `absl::flat_hash_map` 存储自定义的环境变量，这样也就避免了修改进程的环境变量，这里有一个 **header-only** 版本（借鉴了 Golang 的思路）：

```c++
////////
#ifndef EXPAND_ENV_HPP
#define EXPAND_ENV_HPP
#include <string>
#include <string_view>
#include <absl/strings/str_format.h>
#include <absl/container/flat_hash_map.h>

namespace env {

class Derivative {
public:
  Derivative() = default;
  Derivative(const Derivative &) = delete;
  Derivative &operator=(const Derivative &) = delete;
  bool AddBashCompatible(int argc, char *const *argv);
  bool EraseEnv(std::string_view key);
  bool SetEnv(std::string_view key, std::string_view value, bool force = false);
  bool PutEnv(std::string_view nv, bool force = false);
  [[nodiscard]] std::string_view GetEnv(std::string_view key) const;
  bool ExpandEnv(std::string_view raw, std::string &w,
                 bool disableos = false) const;

private:
  bool AppendEnv(std::string_view key, std::string &w) const;
  absl::flat_hash_map<std::string, std::string> envblock;
};

namespace env_internal {
inline bool is_shell_specia_var(char ch) {
  return (ch == '*' || ch == '#' || ch == '$' || ch == '@' || ch == '!' ||
          ch == '?' || ch == '-' || (ch >= '0' && ch <= '9'));
}

inline bool is_alphanum(char ch) {
  return (ch == '_' || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') ||
          (ch >= 'A' && ch <= 'Z'));
}

inline std::string_view resovle_shell_name(std::string_view s, size_t &off) {
  off = 0;
  if (s.front() == '{') {
    if (s.size() > 2 && is_shell_specia_var(s[1]) && s[2] == '}') {
      off = 3;
      return s.substr(1, 2);
    }
    for (size_t i = 1; i < s.size(); i++) {
      if (s[i] == '}') {
        if (i == 1) {
          off = 2;
          return "";
        }
        off = i + 1;
        return s.substr(1, i - 1);
      }
    }
    off = 1;
    return "";
  }
  if (is_shell_specia_var(s[0])) {
    off = 1;
    return s.substr(0, 1);
  }
  size_t i = 0;
  for (; i < s.size() && is_alphanum(s[i]); i++) {
    ;
  }
  off = i;
  return s.substr(0, i);
}

inline bool os_expand_env(const std::string &key, std::string &value) {
  auto v = ::getenv(key.data());
  if (v == nullptr) {
    return false;
  }
  value.append(v);
  return true;
}
} // namespace env_internal

inline bool Derivative::AddBashCompatible(int argc, char *const *argv) {
  // $0~$N
  for (int i = 0; i < argc; i++) {
    envblock.emplace(absl::AlphaNum(i).Piece(), argv[i]);
  }
  envblock.emplace("$",
                   absl::AlphaNum(::getpid()).Piece()); // current process PID
  return true;
}

inline bool Derivative::EraseEnv(std::string_view key) {
  return envblock.erase(key) != 0;
}

inline bool Derivative::SetEnv(std::string_view key, std::string_view value,
                               bool force) {
  if (force) {
    // envblock[key] = value;
    envblock.insert_or_assign(key, value);
    return true;
  }
  return envblock.emplace(key, value).second;
}

inline bool Derivative::PutEnv(std::string_view nv, bool force) {
  auto pos = nv.find('=');
  if (pos == std::wstring_view::npos) {
    return SetEnv(nv, "", force);
  }
  return SetEnv(nv.substr(0, pos), nv.substr(pos + 1), force);
}

[[nodiscard]] inline std::string_view
Derivative::GetEnv(std::string_view key) const {
  auto it = envblock.find(key);
  if (it == envblock.end()) {
    return "";
  }
  return it->second;
}

inline bool Derivative::AppendEnv(std::string_view key, std::string &w) const {
  auto it = envblock.find(key);
  if (it == envblock.end()) {
    return false;
  }
  w.append(it->second);
  return true;
}

// Expand Env string to normal string only support  Unix style'${KEY}'
inline bool Derivative::ExpandEnv(std::string_view raw, std::string &w,
                                  bool disableos) const {
  w.reserve(raw.size() * 2);
  size_t i = 0;
  for (size_t j = 0; j < raw.size(); j++) {
    if (raw[j] == '$' && j + 1 < raw.size()) {
      w.append(raw.substr(i, j - i));
      size_t off = 0;
      auto name = env_internal::resovle_shell_name(raw.substr(j + 1), off);
      if (name.empty()) {
        if (off == 0) {
          w.push_back(raw[j]);
        }
      } else {
        if (!AppendEnv(name, w)) {
          if (!disableos) {
            env_internal::os_expand_env(std::string(name), w);
          }
        }
      }
      j += off;
      i = j + 1;
    }
  }
  w.append(raw.substr(i));
  return true;
}

} // namespace env

#endif

```

这个 `Derivative` 类是我借鉴 `Golang` 先在 bela 中实现的，在 bela 中，我们使用了 [parallel-hashmap](https://github.com/greg7mdp/parallel-hashmap) 作为环境变量容器，并且提供了支持 `std::wstring` 异构查找的[补丁](https://github.com/fcharlie/bela/blob/master/include/bela/phmap/std_wstring.patch)。并且还使用 `parallel_flat_hash_map` 实现了线程安全的 `DerivativeMT`，测试无误后将其移植到到 Gitee 的项目中。

**2019-07-10** [Gregory Popovitch](https://github.com/greg7mdp) 接受了我的 [PR](https://github.com/greg7mdp/parallel-hashmap/pull/15)，目前已经使用官方的 `parallel-hashmap` 作为 Bela 的环境变量容器。

因此，如果你需要在 Windows 中使用 `Derivative`，建议使用 [`bela`](https://github.com/fcharlie/bela/blob/master/include/bela/env.hpp)，其他环境可以使用这个 `header-only` 版本。

## 可回滚的 Shell 自解压安装包

我在 Gitee 开发一些基础组件，除了要为 Gitee 公有云提供技术支持，同时也需要为私有化提供技术支持，因此，这些基础组件如若能静态编译，解压后直接运行是最简单不过的，但是当处于维护模式，升级软件时，却不得不考虑配置是否覆盖，如何支持二进制回滚的问题。

当我们使用 cmake 作为构建系统时，cmake 拥有打包工具 `cpack`，在 Linux 中，`cpack` 可以打包一个 `STGZ` 文件，这个文件有些特别，文件前部是一个脚本，后面则是一个 `tar.gz` 文件，当执行此文件前部的脚本时，脚本会读取文件中 `.tar.gz` 文件的偏移，以管道的方式调用 `tar` 解压。这样就实现了安装。`cmake` 使用一个名为：[CPack.STGZ_Header.sh.in](https://github.com/Kitware/CMake/blob/master/Modules/CPack.STGZ_Header.sh.in) 的模板，如果要安装后执行特定的配置文件，我们则可以在项目文件中添加一个修改后的 `CPack.STGZ_Header.sh.in` 覆盖 cmake 自身的模板即可。

在 cmake 中，配置文件的安装支持 `RENAME`，但 `target` 暂不支持 `RENAME`，因此，我们可以将 target 修改为原来的 `$TARGET_NAME.new`，在解压到安装目录后，自定义配置文件检测相应的 target 是否已经存在，存在则重命名，然后将 `$TARGET_NAME.new` 重命名为 `$TARGET_NAME`，这样便能够支持二进制回滚。配置文件配置也是类似，我们还可以运行 diff 去检测配置文件哪里发生了修改，提示用户更新。

在 Gitee 中，有一些项目基于 Golang 编写，而 cmake 目前并不支持 golang，虽然使用 cmake 可以打包，但是还是有一些麻烦，实际上编写 stgz 构建脚本非常简单，相应脚本如下：

主构建脚本（bali：我使用 [https://github.com/fcharlie/bali](https://github.com/fcharlie/bali) 作为 Golang 项目的构建软件）：

```powershell
#!/usr/bin/env pwsh

param(
    [ValidateSet("linux", "drawin", "windows")]
    [Alias("T")]
    [String]$Target = "linux",
    [String]$Arch = "amd64",
    [Alias("h")]
    [Switch]$Help
)

if ($Help) {
    Write-Host "charlie's mkstgz script tools
  -T|--target      target name, Linux, macOS, Windows
  -h|--help        print usage and exit.
  "
    exit 0
}

$BALIUTILS = Get-Command -CommandType Application bali -ErrorAction SilentlyContinue 
if ($null -eq $BALIUTILS) {
    Write-Host -ForegroundColor Red "Please install bali to allow mkstgz"
    exit 1
}

$AppDir = Split-Path -Parent -Path $PSScriptRoot

$result = Start-Process -FilePath "bali" -ArgumentList "-t $Target -Arch $Arch" -WorkingDirectory $AppDir -NoNewWindow -Wait -PassThru

if ($result.ExitCode -ne 0) {
    Write-Host -ForegroundColor Red "bali build MyPackage failed"
    exit 1
}

$Baliobj = Get-Content -Path "$AppDir/bali.json" | ConvertFrom-Json -ErrorAction SilentlyContinue
if ($null -eq $Baliobj) {
    Write-Host -ForegroundColor Red "parse $AppDir/bali.json failed"
    exit 1
}

if ($null -eq $Baliobj.version) {
    $version = "0.0.1"
}
else {
    $version = $Baliobj.version
}


Write-Host "The version of MyPackage detected is: $version"

Get-ChildItem -Path "$AppDir/build/bin" | ForEach-Object {
    Rename-Item -Path $_.FullName -NewName "$($_.FullName).new"
}

Get-ChildItem -Path "$AppDir/build/config" | ForEach-Object {
    Rename-Item -Path $_.FullName -NewName "$($_.FullName).template"
}

Copy-Item -Path "$PSScriptRoot/post_install.sh" -Destination "$AppDir/build/bin/post_install.sh"


$TarDistPath = "$AppDir/MyPackage-$Target-$Arch-$version.tar.gz"
$StgzDistPath = "$AppDir/MyPackage-$Target-$Arch-$version.sh"
$StgzFileName = "MyPackage-$Target-$Arch-$version.sh"

Write-Host "Compress MyPackage to $TarDistPath"
$TarArg = "-czf `"$TarDistPath`" ."
$TarStatus = Start-Process -FilePath "tar" -ArgumentList "$TarArg" -WorkingDirectory "$AppDir/build" -Wait -NoNewWindow -PassThru
if ($TarStatus.ExitCode -ne 0) {
    exit $TarStatus.ExitCode
}

Copy-Item -Path "$PSScriptRoot/stgz.sh" -Destination "$StgzDistPath" 
Write-Host "Create a self-extracting file for MyPackage`: $StgzDistPath"

try {
    $writer = New-Object System.IO.FileStream($StgzDistPath, [System.IO.FileMode]::Append)
}
catch {
    Write-Host -ForegroundColor Red "unable open $StgzDistPath"
    exit 1
}

try {
    $reader = New-Object System.IO.FileStream($TarDistPath, [System.IO.FileMode]::Open)
}
catch {
    $writer.Close()
    Write-Host -ForegroundColor Red "unable open $TarDistPath"
    exit 1
}

try {
    $reader.CopyTo($writer)
}
finally {
    $writer.Close()
    $reader.Close()
}

[char]$Esc = 0x1b
Write-Host "$Esc[32mPackaged successfully$Esc[0m
Your can run '$StgzFileName --prefix=/path/to/MyPackage' to install MyPackage"

```

STGZ 文件头部（stgz.sh）：

```sh
#!/usr/bin/env bash

# Display usage
stgz_usage() {
    cat <<EOF
Usage: $0 [options]
Options: [defaults in brackets after descriptions]
  --help            print this message
  --version         print cmake installer version
  --prefix=dir      directory in which to install
EOF
    exit 1
}

stgz_fix_slashes() {
    echo "$1" | sed 's/\\/\//g'
}

stgz_echo_exit() {
    echo "$1"
    exit 1
}

for a in "$@"; do
    if echo "$a" | grep "^--prefix=" >/dev/null 2>/dev/null; then
        stgz_prefix_dir="${a/--prefix=\///}"
        stgz_prefix_dir=$(stgz_fix_slashes "${stgz_prefix_dir}")
    fi
    if echo "$a" | grep "^--help" >/dev/null 2>/dev/null; then
        stgz_usage
    fi
done

echo "This is a self-extracting archive."
toplevel=$(pwd)
if [[ "x${stgz_prefix_dir}" != "x" ]]; then
    toplevel="${stgz_prefix_dir}"
fi

echo "The archive will be extracted to: ${toplevel}"

if [ ! -d "${toplevel}" ]; then
    mkdir -p "${toplevel}" || exit 1
fi

echo
echo "Using traget directory: ${toplevel}"
echo "Extracting, please wait..."
echo ""

ARCHIVE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' "$0")
tail "-n+$ARCHIVE" "$0" | tar xzvm -C "$toplevel" >/dev/null 2>&1 3>&1

if [[ -f "${toplevel}/bin/post_install.sh" ]]; then
    chmod +x "${toplevel}/bin/post_install.sh"
    bash "${toplevel}/bin/post_install.sh"
fi

exit 0
#This line must be the last line of the file
__ARCHIVE_BELOW__

```


安装后执行的 `post-install.sh`：

```sh
#!/usr/bin/env bash

BINPATH=$(dirname "$0")
BINPATH=$(realpath "$BINPATH")
CONFIGPATH=$(realpath "$BINPATH/../config")
toplevel=$(realpath "$BINPATH/../")

stgz_apply_target() {
    echo "apply target $1 $2"
    NEWNAME=$(basename "$1")
    NAME="${NEWNAME:0:0-4}"
    NBDIR="$2/bin"
    TARGETFILE="$NBDIR/$NAME"
    if [[ ! -d "$NBDIR/old" ]]; then
        mkdir -p "$NBDIR/old"
    fi
    if [[ -f "$NBDIR/old/$NAME.3" ]]; then
        rm "$NBDIR/old/$NAME.3"
    fi
    if [[ -f "$NBDIR/old/$NAME.2" ]]; then
        mv "$NBDIR/old/$NAME.2" "$NBDIR/old/$NAME.3"
    fi
    if [[ -f "$NBDIR/old/$NAME.1" ]]; then
        mv "$NBDIR/old/$NAME.1" "$NBDIR/old/$NAME.2"
    fi

    if [[ -f "$NBDIR/$NAME.old" ]]; then
        mv "$NBDIR/$NAME.old" "$NBDIR/old/$NAME.1"
    fi
    ###
    if [[ -f "$TARGETFILE" ]]; then
        mv "$TARGETFILE" "$TARGETFILE.old"
    fi
    mv "$TARGETFILE.new" "$TARGETFILE"
}

stgz_apply_config() {
    echo "Install config $1 to $2"
    NEWNAME=$(basename "$1")
    NAME="${NEWNAME:0:0-9}"
    NCDIR="$2/config"
    if [[ ! -d "$NCDIR" ]]; then
        mkdir -p "$NCDIR"
    fi
    if [[ -f "$NCDIR/$NAME" ]]; then
        echo "File $NAME exists in $NCDIR"
        git --no-pager diff --no-index "$1" "$NCDIR/$NAME"
    else
        echo "rename $1 to $NCDIR/$NAME"
        mv "$1" "$NCDIR/$NAME"
    fi
}

for file in "$BINPATH"/*.new; do
    echo "apply ${file}"
    stgz_apply_target "${file}" "$toplevel"
done

for file in "$CONFIGPATH"/*.template; do
    echo "apply $file"
    stgz_apply_config "$file" "$toplevel"
done

rm "$BINPATH/post_install.sh"

```

以上三个脚本就可以像 bali 构建的项目打包成 `STGZ` 文件，运行 `MyPackage-$Target-$Arch-$version.sh --prefix=/path/to/MyPackage` 后就可以了，不会覆盖配置还支持二进制回滚，这对于需要平滑重启的业务来说还是有一些帮助的。

## 最后

没什么可说的了。