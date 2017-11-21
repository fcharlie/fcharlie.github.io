---
layout: post
title:  "Kisasum Hash 实用工具"
date:   2017-09-01 10:00:00
published: true
categories: developer
---
## Kisasum Hash 实用工具

Kismet 是笔者开发的一个计算 Hash 的图形化工具，有桌面版本 Kismet 和 UWP 版本 KismetUWP。 在前面的 Blog 中有介绍： 
[Kismet 杂谈](http://forcemz.net/developer/2017/05/14/Kismet/)


KismetUWP 截图如下：

![u1](https://github.com/fcharlie/KismetUWP/raw/master/docs/images/none.png)

当人们使用一系列命令完成工作时，这个时候优先需要命令行工具。

Hash 计算有什么命令行工具？无论是 PowerShell 还是 Unix Shell 都有一系列的 Hash 命令，比如 PowerShell，有 `Get-FileHash`， 命令格式如下

```powershell
Get-FileHash windows10.iso SHA256
```

也可以使用强制参数，具体帮助信息如下
```usage
NAME
    Get-FileHash

SYNTAX
    Get-FileHash [-Path] <string[]> [[-Algorithm] {SHA1 | SHA256 | SHA384 | SHA512 | MD5}]  [<CommonParameters>]

    Get-FileHash [-LiteralPath] <string[]> [[-Algorithm] {SHA1 | SHA256 | SHA384 | SHA512 | MD5}]  [<CommonParameters>]

    Get-FileHash [-InputStream] <Stream> [[-Algorithm] {SHA1 | SHA256 | SHA384 | SHA512 | MD5}]  [<CommonParameters>]


ALIASES
    None


REMARKS
    Get-Help cannot find the Help files for this cmdlet on this computer. It is displaying only partial help.
        -- To download and install Help files for the module that includes this cmdlet, use Update-Help.
        -- To view the Help topic for this cmdlet online, type: "Get-Help Get-FileHash -Online" or
           go to https://go.microsoft.com/fwlink/?LinkId=517145.

```

在 Unix Shell，也有 shasum sha256sum sha512sum sha384sum，第三方还有 sha3sum。

命令格式通常是：
```
shaNsum file
```

对于大多数人来说，这些命令都是极好的。但我并不满足，于是开发了 Kisasum Hash 实用工具，此工具支持输出 JSON 和 XML。

Kisasum 源码在 Kismet 的项目中 [Kisasum](https://github.com/fcharlie/Kismet/tree/master/Kisasum)。

Kisasum 的帮助信息如下：

```usage
OVERVIEW: kisasum 1.0
USAGE: kisasum [options] <input>
OPTIONS:
  -a, --algorithm  Hash Algorithm,support algorithm described below.
                   Algorithm Ignore case, default sha256
                   SHA1DC is SHA-1 collision.
  -f, --format     Return information about hash in a format described below.
  -h, -?, --help   Print usage and exit.
  -v, --version    Print version and exit.

Algorithm:
  MD5        SHA1       SHA1DC
  SHA224     SHA256     SHA384     SHA512
  SHA3-224   SHA3-256   SHA3-384   SHA3-512

Formats:
  text     format to text, support progress
  json     format to json
  xml      format to xml

```

Kisasum 支持的 Hash 算法与 Kismet 一致，Kisasum 支持输出普通文本，JSON，以及 XML。这样做的好处是，笔者可以使用 PowerShell 将 Kisasum 的输出转变为 PowerShell 对象，从而能够在 PowerShell 脚本中方便的使用 Kisasum。

```powershell
 $hash=.\Kisasum.exe $FilePath1 $FilePath2 --format=json -a sha3-256|ConvertFrom-JSON
 foreach($f in $hash.files){
   Write-Host "$($f.hash)    $($f.name)"
 }
```

例子：

```json
//D:\Utilities\CharlieKit>Kisasum Diego.exe Kisasum.exe -f json
{
  "algorithm": "sha256",
  "files":
  [
    {
      "name": "Diego.exe",
      "hash": "AEC1352B5A1123293EDE9A9B74665C013DE37453368909FC988E43862A86114B"
    },
    {
      "name": "Kisasum.exe",
      "hash": "596A02FF501883A194B283A40E6A1D046F6CEEEAF46FBAF6A98D0D1EBC74FA57"
    }
  ]
}
```

格式化输出的思想借鉴了 [`vswhere`](https://github.com/Microsoft/vswhere)，笔者的 `Clangbuilder` 正是使用 vswhere 作为 Visual Studio 检测工具，如果不是输出成 JSON 或者 XML，过程要麻烦得多。

Kisasum 格式化并没有使用第三方库，非常简单代码如下：

```json
int KisasumPrintXML(const KisasumResult &result) {
	std::wstring ws(LR"(<?xml version="1.0">)");
	ws.append(L"\n<root>\n  <algorithm>")
		.append(result.algorithm)
		.append(L"</algorithm>\n  <files>\n");
	for (const auto &e : result.elems) {
		ws.append(L"    <file>\n      <name>")
			.append(e.filename)
			.append(L"</name>\n      <hash>")
			.append(e.hash)
			.append(L"</hash>\n    </file>\n");
	}
	ws.append(L"  </files>\n</root>\n");
	console::WriteFormatted(ws.data(), ws.size());
	return 0;
}
int KisasumPrintJSON(const KisasumResult &result) {
	std::wstring ws(L"{\n"); /// start
	ws.append(L"  \"algorithm\": \"")
		.append(result.algorithm)
		.append(L"\",\n  \"files\": \n  [");
	for (const auto &e : result.elems) {
		ws.append(L"\n    {\n      \"name\": \"")
			.append(e.filename)
			.append(L"\",\n      \"hash\": \"")
			.append(e.hash)
			.append(L"\"\n    },");
	}
	if (ws.back() == L',') {
		ws.pop_back();
	}
	ws.append(L"\n  ]\n}\n");
	console::WriteFormatted(ws.data(), ws.size());
	return 0;
}
```

最后输出到控制台，控制台进行了封装。

## 最后

欢迎大家使用和提出宝贵意见。