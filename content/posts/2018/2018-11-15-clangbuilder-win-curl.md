+++
title = "Clangbuilder 已支持自动构建 CURL"
date = "2018-11-15T10:00:00+08:00"
categories = "toolset"
+++
# cURL
cURL 是一个非常著名的开源 URL 数据传输工具，支持 `HTTP`,`HTTPS`,`FTP`,`SCP`,`SFTP`,`Telnet` 等协议。绝大多数操作系统都自带了，也包括 Windows 10 17134/17763。但系统自带的版本通常都不会及时更新到最新版本，而 `cURL` 是一个非常活跃的项目，大约2个月就会发布一个新版本。每一次更新都会修复大量 bug，新增很多新特性，比如最近增加了 `DOH` 更好的 `TLS1.3` 支持。

系统自带的 `curl -V` 输出：

```
curl 7.55.1 (Windows) libcurl/7.55.1 WinSSL
Release-Date: [unreleased]
Protocols: dict file ftp ftps http https imap imaps pop3 pop3s smtp smtps telnet tftp
Features: AsynchDNS IPv6 Largefile SSPI Kerberos SPNEGO NTLM SSL
```

实际上，在 cURL 下载页面 [https://curl.haxx.se/download.html](https://curl.haxx.se/download.html) Windows 平台有好几个二进制发行包，官方也提供了二进制下载：[https://curl.haxx.se/windows/](https://curl.haxx.se/windows/) 基于 `Mingw64` 构建。在 Github 上有构建项目：[https://github.com/curl/curl-for-win](https://github.com/curl/curl-for-win)。而这个发行版也是 curl 一个开发者个人项目纳入到 curl 中的。

我们还可以使用 `vcpkg` 安装 `curl`，但截至目前，`vcpkg` 的 `OpenSSL` 版本依然是 **1.0.2p** 无法支持 `TLS 1.3`。

笔者想要使用 `Visual C++` 构建静态链接支持 `TLS 1.3` 的 `curl`，最后发现都得自己一步一步来，于是就写了构建脚本。

话不多说，一切尽在代码中。Github 源代码地址：[sources/wincurl/wincurl.ps1](https://github.com/fstudio/clangbuilder/blob/526183ade63a4a9b8bd2f4b99872536c9da9e997/sources/wincurl/wincurl.ps1)

```powershell
#!/usr/bin/env pwsh
# Require Clangbuilder install perl

param(
    [String]$WD
)

# Import version info
. "$PSScriptRoot/version.ps1"

# thanks https://github.com/curl/curl-for-win


Write-Host "Download urls:
zlib: $ZLIB_URL
openssl: $OPENSSL_URL
brotli: $BROTLI_URL
libssh2: $LIBSSH2_URL
nghttp2: $NGHTTP2_URL
curl: $CURL_URL"

################################################## Found commands

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Import-Module -Name "$PSScriptRoot/Utility"

$clexe = Get-Command -CommandType Application "cl" -ErrorAction SilentlyContinue
if ($null -eq $clexe) {
    Write-Host -ForegroundColor Red "Please install Visual Studio 2017 or BuildTools (C++) and Initialzie DevEnv"
    exit 1
}
Write-Host "Find cl.exe: $($clexe.Version)"

$tarexe = Findcommand -Name "tar"
$decompress = "-xvf"
if ($null -eq $tarexe) {
    $tarexe = Findcommand -Name "cmake"
    if ($null -eq $tarexe) {
        Write-Host -ForegroundColor Red "Please install tar or cmake."
        exit 1
    }
    $decompress = "-E tar -xvf"
}
Write-Host -ForegroundColor Green "Use $tarexe as tar"


Function DecompressTar {
    param(
        [String]$URL,
        [String]$File,
        [String]$Hash
    )
    if (!(Test-Path $File)) {
        if (!(WinGet -URL $URL -O $File)) {
            return $false
        }
    }
    else {
        if ((Get-FileHash -Algorithm SHA256 $File).Hash -ne $Hash) {
            Write-Host -ForegroundColor Yellow "$File exists and hash not match $Hash."
            Remove-Item -Force $File
            if (!(WinGet -URL $URL -O $File)) {
                return $false
            }
        }
        else {
            Write-Host -ForegroundColor Yellow "$File exists and hash is match. use it."
        }
    }
    if ((Get-FileHash -Algorithm SHA256 $File).Hash -ne $Hash) {
        Remove-Item -Force $File
        if (!(WinGet -URL $URL -O $File)) {
            return $false
        }
    }

    if ((Exec -FilePath $tarexe -Argv "$decompress $File") -ne 0) {
        Write-Host -ForegroundColor Red "Decompress $File failed"
        return $false
    }
    return $true
}

$curlexe = Findcommand -Name "curl"
if ($null -eq $curlexe) {
    Write-Host -ForegroundColor Red "Please install curl or upgrade to Windows 10 17134 or Later."
    return 1
}
Write-Host -ForegroundColor Green "Find curl install: $curlexe"

$cmakeexe = Findcommand -Name "cmake"
if ($null -eq $cmakeexe) {
    Write-Host -ForegroundColor Red "Please install cmake."
    return 1
}
Write-Host -ForegroundColor Green "Find cmake install: $cmakeexe"


$Ninjaexe = Findcommand -Name "ninja"
if ($null -eq $Ninjaexe) {
    Write-Host -ForegroundColor Red "Please install ninja."
    return 1
}
Write-Host -ForegroundColor Green "Find cmake install: $Ninjaexe"

$Patchexe = Findcommand -Name "patch"
if ($null -eq $Patchexe) {

    $Gitexe = Findcommand -Name "git"
    if ($null -eq $Gitexe) {
        Write-Host -ForegroundColor Red "Please install git for windows (or PortableGit)."
        return 1
    }
    $gitinstall = Split-Path -Parent (Split-Path -Parent $gitexe)
    if ([String]::IsNullOrEmpty($gitinstall)) {
        Write-Host -ForegroundColor Red "Please install git for windows (or PortableGit)."
        return 1
    }
    $patchx = Join-Path $gitinstall "usr/bin/patch.exe"
    Write-Host "Try to find patch from $patchx"
    if (!(Test-Path $patchx)) {
        $xinstall = Split-Path -Parent $gitinstall
        if ([String]::IsNullOrEmpty($xinstall)) {
            Write-Host -ForegroundColor Red "Please install git for windows (or PortableGit)."
            return 1
        }
        $patchx = Join-Path  $xinstall "usr/bin/patch.exe"
        if (!(Test-Path $patchx)) {
            Write-Host -ForegroundColor Red "Please install git for windows (or PortableGit)."
            return 1
        }
    }
    $Patchexe = $patchx
}
Write-Host  -ForegroundColor Green "Found patch install: $Patchexe"

$Perlexe = Findcommand -Name "perl"
if ($null -eq $Perlexe) {
    Write-Host -ForegroundColor Red "Please add perl to your environment."
    exit 1
}
Write-Host  -ForegroundColor Green "Found perl install: $Perlexe"


########################################################## Check WD
if ([String]::IsNullOrEmpty($WD)) {
    $cbroot = Split-Path -Parent (Split-Path -Path $PSScriptRoot)
    $WD = Join-Path $cbroot "out/curl"
}
if (!(MkdirAll -Dir $WD)) {
    exit 1
}

Write-Host -ForegroundColor Cyan "Build curl on windows use"

Set-Location $WD

$Prefix = Join-Path $WD "build"
$CURLOUT = Join-Path $WD "out"

Write-Host "we will deploy curl to: $CURLPrefix"

if (!(MkdirAll -Dir $Prefix)) {
    exit 1
}

################################################## Zlib
if (!(DecompressTar -URL $ZLIB_URL -File "$ZLIB_FILENAME.tar.gz" -Hash $ZLIB_HASH)) {
    exit 1
}

$ZLIBDIR = Join-Path $PWD $ZLIB_FILENAME
$ZLIBBD = Join-Path $ZLIBDIR "build"

Write-Host -ForegroundColor Yellow "Apply zlib.patch ..."
$ZLIB_PACTH = Join-Path $PSScriptRoot "patch/zlib.patch"

$ec = Exec -FilePath $Patchexe -Argv "-Nbp1 -i `"$ZLIB_PACTH`"" -WD $ZLIBDIR
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "Apply $ZLIB_PACTH failed"
}

if (!(MkdirAll -Dir $ZLIBBD)) {
    exit 1
}

$cmakeflags = "-GNinja " + `
    "-DCMAKE_BUILD_TYPE=Release " + `
    "`"-DCMAKE_INSTALL_PREFIX=$Prefix`" " + `
    "-DSKIP_INSTALL_FILES=ON " + `
    "-DSKIP_BUILD_EXAMPLES=ON " + `
    "-DBUILD_SHARED_LIBS=OFF `"$ZLIBDIR`""


$ec = Exec -FilePath $cmakeexe -Argv $cmakeflags -WD $ZLIBBD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "zlib: create build.ninja error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "all" -WD $ZLIBBD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "zlib: build error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "install" -WD $ZLIBBD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "zlib: install error"
    return 1
}

Rename-Item -Path "$Prefix/lib/zlibstatic.lib"   "$Prefix/lib/zlib.lib"  -Force -ErrorAction SilentlyContinue
#Copy-Item -Path "$ZLIBDIR/LICENSE" 

##################################################### OpenSSL
Write-Host -ForegroundColor Yellow "Build OpenSSL $OPENSSL_VERSION"

if (!(DecompressTar -URL $OPENSSL_URL -File "$OPENSSL_FILE.tar.gz" -Hash $OPENSSL_HASH)) {
    exit 1
}

# Update env
$env:INCLUDE = "$Prefix\include;$env:INCLUDE"
$env:LIB = "$Prefix\lib;$env:LIB"

# perl Configure no-shared no-ssl3 enable-capieng -utf-8

$opensslflags = "Configure no-shared no-unit-test no-tests no-ssl3 enable-capieng -utf-8 " + `
    "VC-WIN64A `"--prefix=$Prefix`" `"--openssldir=$Prefix`""

$Nasmexe = Findcommand -Name "nasm"
if ($null -eq $Nasmexe) {
    Write-Host -ForegroundColor Yellow "Not found nasm, build openssl no-asm"
    $opensslflags += " no-asm"
}

$openssldir = Join-Path $WD $OPENSSL_FILE

$ec = Exec -FilePath $Perlexe -Argv $opensslflags -WD $openssldir
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "openssl: config error"
    return 1
}

$ec = Exec -FilePath nmake -Argv "-f makefile" -WD $openssldir
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "openssl: build error"
    return 1
}

$ec = Exec -FilePath nmake -Argv "-f makefile install_sw" -WD $openssldir
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "openssl: install_sw error"
    return 1
}
# build brotli static
######################################################### Brotli
Write-Host -ForegroundColor Yellow "Build brotli $BROTLI_VERSION"
if (!(DecompressTar -URL $BROTLI_URL -File "$BROTLI_FILE.tar.gz" -Hash $BROTLI_HASH)) {
    exit 1
}

$BDIR = Join-Path $WD $BROTLI_FILE
$BBUILD = Join-Path $BDIR "out"
$BPATCH = Join-Path $PSScriptRoot "patch/brotli.patch"

if (!(MkdirAll -Dir $BBUILD)) {
    exit 1
}

$ec = Exec -FilePath $Patchexe -Argv "-Nbp1 -i `"$BPATCH`"" -WD $BDIR
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "Apply $BPATCH failed"
}


$brotliflags = "-GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " + `
    "-DBROTLI_DISABLE_TESTS=ON `"-DCMAKE_INSTALL_PREFIX=$Prefix`" .."


$ec = Exec -FilePath $cmakeexe -Argv $brotliflags -WD $BBUILD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "brotli: create build.ninja error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "all" -WD $BBUILD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "brotli: build error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "install" -WD $BBUILD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "brotli: install error"
    return 1
}

## Fix curl not exists
Move-Item -Path "$Prefix/lib/brotlicommon-static.lib"   "$Prefix/lib/brotlicommon.lib"  -Force -ErrorAction SilentlyContinue
Move-Item -Path "$Prefix/lib/brotlidec-static.lib"   "$Prefix/lib/brotlidec.lib"  -Force -ErrorAction SilentlyContinue
Move-Item -Path "$Prefix/lib/brotlienc-static.lib"   "$Prefix/lib/brotlienc.lib"  -Force -ErrorAction SilentlyContinue

######################################################### Nghttp2
Write-Host -ForegroundColor Yellow "Build nghttp2 $NGHTTP2_VERSION"
if (!(DecompressTar -URL $NGHTTP2_URL -File "$NGHTTP2_FILE.tar.gz" -Hash $NGHTTP2_HASH)) {
    exit 1
}

$NGDIR = Join-Path $WD $NGHTTP2_FILE
$NGBUILD = Join-Path $NGDIR "build"
$NGPATCH = Join-Path $PSScriptRoot "patch/nghttp2.patch"

if (!(MkdirAll -Dir $NGBUILD)) {
    exit 1
}

$ec = Exec -FilePath $Patchexe -Argv "-Nbp1 -i `"$NGPATCH`"" -WD $NGDIR
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "Apply $NGPATCH failed"
}

$ngflags = "-GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " + `
    "-DENABLE_LIB_ONLY=ON -DENABLE_ASIO_LIB=OFF `"-DCMAKE_INSTALL_PREFIX=$Prefix`" .."

$ec = Exec -FilePath $cmakeexe -Argv $ngflags -WD $NGBUILD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "nghttp2: create build.ninja error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "all" -WD $NGBUILD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "nghttp2: build error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "install" -WD $NGBUILD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "nghttp2: install error"
    return 1
}

############################################################# Libssh2
Write-Host -ForegroundColor Yellow "Build libssh2 $LIBSSH2_VERSION"
if (!(DecompressTar -URL $LIBSSH2_URL -File "$LIBSSH2_FILE.tar.gz" -Hash $LIBSSH2_HASH)) {
    exit 1
}
$LIBSSH2DIR = Join-Path $WD $LIBSSH2_FILE
$LIBSSH2BUILD = Join-Path $LIBSSH2DIR "build"
$LIBSSH2PATCH = Join-Path $PSScriptRoot "patch/libssh2.patch"

if (!(MkdirAll -Dir $LIBSSH2BUILD)) {
    exit 1
}

$ec = Exec -FilePath $Patchexe -Argv "-Nbp1 -i `"$LIBSSH2PATCH`"" -WD $LIBSSH2DIR
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "Apply $LIBSSH2PATCH failed"
}

$libssh2flags = "-GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " + `
    "-DBUILD_EXAMPLES=OFF " + `
    "-DBUILD_TESTING=OFF " + `
    "-DENABLE_ZLIB_COMPRESSION=ON " + `
    "`"-DCMAKE_INSTALL_PREFIX=$Prefix`" .."

$ec = Exec -FilePath $cmakeexe -Argv $libssh2flags -WD $LIBSSH2BUILD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "libssh2: create build.ninja error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "all" -WD $LIBSSH2BUILD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "libssh2: build error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "install" -WD $LIBSSH2BUILD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "libssh2: install error"
    return 1
}

############################################################## CURL

Write-Host -ForegroundColor Yellow "Final build curl $CURL_VERSION"
if (!(DecompressTar -URL $CURL_URL -File "$CURL_FILE.tar.gz" -Hash $CURL_HASH)) {
    exit 1
}

$CURLDIR = Join-Path $WD $CURL_FILE
$CURLBD = Join-Path $CURLDIR "build"
$CURLPATCH = Join-Path $PSScriptRoot "patch/curl.patch"
$CURLICON = Join-Path $PSScriptRoot "patch/curl.ico"

if (!(MkdirAll -Dir $CURLBD)) {
    exit 1
}

# copy icon to path
Copy-Item $CURLICON -Destination "$CURLDIR/src"-Force -ErrorAction SilentlyContinue

$ec = Exec -FilePath $Patchexe -Argv "-Nbp1 -i `"$CURLPATCH`"" -WD $CURLDIR
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "Apply $CURLPATCH failed"
}

#https://github.com/curl/curl/blob/master/CMake/FindBrotli.cmake
$BROTLIDEC_LIBRARY = Join-Path $Prefix "lib/brotlidec-static.lib"
$BROTLICOMMON_LIBRARY = Join-Path $Prefix "lib/brotlicommon-static.lib"
$BROTLI_INCLUDE_DIR = Join-Path $Prefix "include"

## Use codepage 1252

$curlflags = "-GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " + `
    "-DUSE_NGHTTP2=ON -DBUILD_TESTING=OFF " + `
    "-DBUILD_CURL_EXE=ON " + `
    "-DCURL_STATIC_CRT=ON " + `
    "-DCMAKE_USE_OPENSSL=ON " + `
    "-DCMAKE_USE_WINSSL=ON " + `
    "-DCMAKE_USE_LIBSSH2=ON " + `
    "-DCURL_BROTLI=ON " + `
    "-DCMAKE_RC_FLAGS=-c1252 " + `
    "`"-DBROTLI_DIR=$Prefix`" " + `
    "`"-DCMAKE_INSTALL_PREFIX=$CURLOUT`" .."

$ec = Exec -FilePath $cmakeexe -Argv $curlflags -WD $CURLBD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "curl: create build.ninja error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "all" -WD $CURLBD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "curl: build error"
    return 1
}

$ec = Exec -FilePath $Ninjaexe -Argv "install" -WD $CURLBD
if ($ec -ne 0) {
    Write-Host -ForegroundColor Red "curl: install error"
    return 1
}

# download curl-ca-bundle.crt

$CA_BUNDLE = Join-Path $CURLOUT "bin/curl-ca-bundle.crt"

if (!(WinGet -URL $CA_BUNDLE_URL -O $CA_BUNDLE)) {
    Write-Host -ForegroundColor Red "download curl-ca-bundle.crt  error"
}

Write-Host -ForegroundColor Green "curl: build completed"
```

构建的 `curl -v` 输出：

```
$ curl -V
curl 7.62.0 (Windows) libcurl/7.62.0 OpenSSL/1.1.1 (WinSSL) zlib/1.2.11 brotli/1.0.7 libssh2/1.8.0 nghttp2/1.34.0
Release-Date: 2018-10-31
Protocols: dict file ftp ftps gopher http https imap imaps ldap pop3 pop3s rtsp scp sftp smtp smtps telnet tftp
Features: AsynchDNS IPv6 Largefile SSPI Kerberos SPNEGO NTLM SSL libz brotli HTTP2 HTTPS-proxy MultiSSL
```