---
layout: post
title:  "使用 PowerShell 管理构建你的软件"
date:   2016-01-21 21:30:16
published: true
categories: toolset
---
#前言
远古时，Windows 用户使用 Batch 或者 VB Script 等脚本来实现一些自动化的任务，Batch 和 Unix Shell Script 具有相似的地位。   
由于技术人员日益增长的工具需求与 Batch 自身局限性的矛盾，Microsoft 推出了 Windows PowerShell，基于 .NET 的 Shell Script 环境。  
PowerShell 利用自身特性保持了 与 cmd 命令 的兼容，通过 Alias 特性，实现了与 Bash 基本命令一致的命令，cat 是 Get-Content 的别名，
ls 是 Get-ChildItem 的别名 

>Get-Alias

在脚本语法上充分吸收了 Shell Script 的优异特性，并将其改进，比如函数 Function , if 分支语句 

{% highlight powershell %}
Function foo($va){
    Write-Output $va
}

Function{
    param(
        [String]$va
    )
    Write-Output $va
}

if($null -eq $va){
    Write-Output $true
}else{
    Write-Output $false
}

$Str="PowerShell"

if($Str -match "Po*"){
    #to do
}else if($Str -match "ZZ*"){
    #to do
}else{
    #to do
}

{% endhighlight%}

与严格缩进的语言相比，这种由花括号包裹的代码快能够避免很大一部分因格式带来的错误。

PowerShell 可以非常方便的利用 .NET 运行时，实现一些模块和功能，比如实现一个 解压 zip 文件的 cmdlet:

{% highlight powershell %}
Function Expand-ZipPackage
{
    param(
        [Parameter(Position=0,Mandatory=$True,HelpMessage="Unzip sources")]
        [ValidateNotNullorEmpty()]
        [String]$ZipSource,
        [Parameter(Position=1,Mandatory=$True,HelpMessage="Output Directory")]
        [ValidateNotNullorEmpty()]
        [String]$Destination
    )
    [System.Reflection.Assembly]::LoadWithPartialName('System.IO.Compression.FileSystem')|Out-Null
    [System.IO.Compression.ZipFile]::ExtractToDirectory($ZipSource, $Destination)
}

{% endhighlight %}



#关于 ClangBuilder
[ClangBuilder](https://github.com/fstudio/clangbuilder) 是一个基于 PowerShell 实现的 Windows 平台 自动化构建 Clang 的环境，
本文将结合 ClangBuilder 来讨论 PowerShell 的功能与特性。


#PowerShell 实现在线安装
在 PowerShell 中，有个 Cmdlet Invoke-Expression (Alias iex ) 可以将字符串当做 PowerShell Script 执行，这样可以从网上下载安装脚本并执行：  

{% highlight powershell %}
iex ((new-object net.webclient).DownloadString('https://raw.githubusercontent.com/fstudio/clangbuilder/master/bin/Installer/WebInstall.ps1'))
{% endhighlight%}

当然也可以设置代理：    
 {% highlight powershell %}
 &{$wc=New-Object System.Net.WebClient;$wc.Proxy=[System.Net.WebRequest]::DefaultWebProxy;$wc.Proxy.Credentials=[System.Net.CredentialCache]::DefaultNetworkCredentials;Invoke-Expression ($wc.DownloadString('https://raw.githubusercontent.com/fstudio/clangbuilder/master/bin/Installer/WebInstall.ps1'))}
 {% endhighlight%}
 
 执行 WebInstall 时， 安装脚本会要求用户设置安装目录，这一功能由一下代码实现
 
{% highlight powershell %}
 Function Get-InstallPrefix{
    param(
        [Parameter(Position=0,Mandatory=$True,HelpMessage="Enter Clangbuilder Install Folder: ")]
        [ValidateNotNullorEmpty()]
        [String]$Prefix
    )
   return $Prefix
}

$ClangBuilderInstallRoot=Get-InstallPrefix
{% endhighlight%}
 
在 PowerShell 中，函数的格式有多种类型，可以使用 param 限定函数参数，ValidateNotNullorEmpty() 将限制函数的参数不为空，为空的情况下，
PowerShell 会要求用户输入。

在 PowerShell 中，上传和下载文件可以使用 Start-BitsTransfer, 这个命令实际上是基于 BITS 服务，而 Windows Update 就是基于 BITS 实现的。
相对于其他如 System.Net.WebClient Start-BitsTransfer 能够显示下载进度条，Start-BitsTransfer 是 PowerShell 3.0 才有的，Windows 7 
可以安装 PowerShell 3.0，所以这个限制不算问题。

{% highlight powershell %}
Set-StrictMode -Version latest
Import-Module -Name BitsTransfer

Function Get-RegistryValue($key, $value) {
    (Get-ItemProperty $key $value).$value
}

Function Get-DownloadFile
{
    param
    (
        [Parameter(Position=0,Mandatory=$True,HelpMessage="Enter a Internet File Full Url")]
        [ValidateNotNullorEmpty()]
        [String]$FileUrl,
        [String]$FileSavePath
    )

    IF($FileSavePath -eq $null)
    {
        $NposIndex=$FileUrl.LastIndexOf("/")+1
        IF($NposIndex -eq $FileUrl.Length)
        {
            return $False
        }
        $DownloadFd=Get-RegistryValue 'HKCU:Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders' '{374DE290-123F-4565-9164-39C4925E467B}'
        $FileSigName=$FileUrl.Substring($NposIndex,$FileUrl.Length - $NposIndex)
        $FileSavePath="{0}\{1}" -f $DownloadFd,$FileSigName
    }
    Start-BitsTransfer $FileUrl  $FileSavePath
}

Get-DownloadFile -FileUrl $OfficaUrl -FileSavePath $DownloadInstallPackage
{% endhighlight %} 
 
在这段代码中， $DownloadFd 是 TEMP 目录，不像 Batch 查询注册表 使用 reg 命令非常麻烦，PowerShell 查询注册表可以像查询文件系统一样非常方便。
 
整个安装脚本源码： [WebInstall.ps1](https://github.com/fstudio/clangbuilder/blob/master/bin/Installer/WebInstall.ps1)
 
#PowerShell 绿色安装
Restore Packages [RestorePackages.ps1](https://github.com/fstudio/clangbuilder/blob/master/Packages/RestorePackages.ps1)   
PowerShell 操作 JSON 非常方便，可以将 JSON 文本转变为  Object 对象，也可以将 Object 对象转变为 JSON 文本。
{% highlight powershell %}

$PackageMap=@{}
$PackageMap["CMake"]="3.4.2"
$PackageMap["Subversion"]="1.9.3"
$PackageMap["Python"]="2.7.11"
$PackageMap["NSIS"]="3.0b3"
$PackageMap["GNUWin"]="1.0"

if(Test-Path "$SelfFolder/Package.lock.json"){
    $PackageLockJson=Get-Content -TotalCount -1 -Path "$SelfFolder/Package.lock.json" |ConvertFrom-Json
}

if($PackageLockJson -ne $null){
    if($PackageLockJson.CMake -ne $PackageMap["CMake"] -and (Test-Path "$SelfFolder\CMake") ){
        Rename-Item "$SelfFolder\CMake" "$SelfFolder\CMake.bak"
    }
    if($PackageLockJson.Subversion -ne $PackageMap["Subversion"] -and (Test-Path "$SelfFolder\Subversion")){
        Rename-Item "$SelfFolder\Subversion" "$SelfFolder\Subversion.bak"
    }
    if($PackageLockJson.Python -ne $PackageMap["Python"] -and (Test-Path "$SelfFolder\Python")){
        Rename-Item "$SelfFolder\Python" "$SelfFolder\Python.bak"
    }
    if($PackageLockJson.NSIS -ne $PackageMap["NSIS"] -and (Test-Path "$SelfFolder\NSIS")){
        Rename-Item "$SelfFolder\NSIS" "$SelfFolder\NSIS.bak"
    }
    if($PackageLockJson.GNUWin -ne $PackageMap["GNUWin"] -and (Test-Path "$SelfFolder\GNUWin")){
        Rename-Item "$SelfFolder\GNUWin" "$SelfFolder\GNUWin.bak"
    }
}

ConvertTo-Json $PackageMap |Out-File -Force -FilePath "$SelfFolder\Package.lock.json"
{% endhighlight %}
 
 利用这一特性，我们可以实现依赖文件的版本管理。出于控制复杂度的目的，并没有更精细的设置。
 
 对于 Zip 文件，可以用前文中的 Expend-ZipPackage ，而解包 msi 安装包可以使用如下语句:   
 
{% highlight powershell %}
 Start-Process -FilePath msiexec -ArgumentList "/a `"$SelfFolder\Subversion.msi`" /qn TARGETDIR=`"$SelfFolder\Subversion`"" -NoNewWindow -Wait
{% endhighlight %}
 
 -NoNewWindow 表示不启动新窗口，-Wait 表示等待执行完毕。
 
#PowerShell 加载 Visual Studio 环境    
前文中 param 可以是限制函数的参数，也可以限制脚本文件的参数。比如在下面脚本中，ValidateSet 表示参数只能是这四种。   
{% highlight powershell %}
 param (
    [ValidateSet("x86", "x64", "ARM", "ARM64")]
    [String]$Arch="x64"
)
{% endhighlight%}
 
如果没有 param 限制参数，在脚本中获取参数需要处理 $argv 数组， $argv.Count 表示 参数个数, 以下面命令为例，param 处理参数后，脚本中变量 $Arch 的值就是参数的 x64。 

>./show.ps1 -Arch x64

如果不用 param, 就得自己解析 $argv 了。

关于 Visual Studio 环境脚本在 [bin/Model](https://github.com/fstudio/clangbuilder/blob/master/bin/Model) 

#PowerShell 其他资料

[Windows PowerShell: Build a Better Function](https://technet.microsoft.com/en-us/magazine/hh360993.aspx)
