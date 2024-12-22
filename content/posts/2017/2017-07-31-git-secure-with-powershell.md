+++
title = "基于 Powershell Core 的 Git 存储库加密方案"
date = "2017-07-31T10:00:00+08:00"
categories = "git"
+++
## 背景

虽然码云提供免费私有存储库，但一些用户还是认为网站管理员可以看到他们的源码，认为私有库也不太安全。而且这些用户也没有私有化部署的打算。如何消除他们的疑虑？使用笔者开发的 **Git-Secure** 就可以实现存储库的加密。项目开源地址：[Git-Secure](https://gitee.com/oscstudio/git-secure)


```usage
Git Secure utilies 1.0
Usage: git-secure cmd args
       add         add file contents to the index
       clone       clone a encrypted repository
       config      config your secure repository
       commit      create a commit
       diff        show commit changes between commit worktree
       init        initialize a secure repository
       key         create a aes key
       pull        Fetch from and integrate with another repository or a local branch
       push        Update remote refs along with associated objects
       remote      set remote for secure repositroy
       status      Show the working tree status
       help        print help message
```

## 其他方案

笔者在开发 Git-Secure 之前也去了解过有什么方案支持存储库加密，在 Github 上有一个简单的项目，将所有的文件压缩后创建单文件存储库，加密后提交，然后推送到远程服务器。这样有一些缺点，第一，无法通过 diff 查看文件变更；第二，由于 git 的快照特性，存储库的膨胀会很严重。因此笔者还是决定自己动手开发。

## PowerShell

在实现一系列的 git 功能时，我们可以直接使用 git 命令，通过一些 shell 脚本将 git 命令组合来实现这些功能，也可以使用 libgit2 或者 libgit 调用函数来实现这些功能。第一个开发起来比较简单，于是笔者采用了第一种。得益于 PowerShell Core 的开源和跨平台，笔者作为一个熟练使用 PowerShell 脚本的开发者，肯定优先使用 PowerShell。当然相对于 Bash Shell 之类的 Shell 脚本， PowerShell 的功能更加强大，可以无缝调用 .Net 类库，比如，我们加密使用的是 AES 算法，就可以调用 `System.Security.Cryptography.AesManaged` 来实现 AES 加密。

## Git-Secure 原理

Git-Secure 原理非常简单，即本地存在两个存储库，一个是用户可见的存储库，用户使用 git-secure 的 commit, push 这些操作时，实际上是转换到隐藏的真实的存储库，用户 commit 时，将修改的文件加密后提交到隐藏的加密存储库，推送时，将隐藏存储库推送到远程服务器。

而 Git-Secure 的 fecth,pull 等操作则是上述操作的逆过程。

commit 源码： [ps/commit.ps1](https://gitee.com/oscstudio/git-secure/blob/master/ps/commit.ps1)

pull 源码：[ps/pull.ps1](https://gitee.com/oscstudio/git-secure/blob/master/ps/pull.ps1)

Git-Secure 加密使用了 AES-256，加密模块源码地址：[Modules/AesProvider/AesProvider.psm1](https://gitee.com/oscstudio/git-secure/blob/master/Modules/AesProvider/AesProvider.psm1)

```powershell
function New-AesKey {
    Param(
        [Parameter(Mandatory = $false, Position = 1, ValueFromPipeline = $true)]
        [Int]$KeySize = 256
    )

    try {
        $AESProvider = New-Object "System.Security.Cryptography.AesManaged"
        $AESProvider.KeySize = $KeySize
        $AESProvider.GenerateKey()
        return [System.Convert]::ToBase64String($AESProvider.Key)
    }
    catch {
        Write-Error $_
    }
}

function New-AesFile {
    param(
        [Parameter(Position = 0, Mandatory = $True, HelpMessage = "Enter your need encrypt file")]
        [System.IO.FileInfo]$File,
        [string]$Key,
        [string]$Destination
    )
    if ([System.String]::IsNullOrEmpty($Destination)) {
        $Destination = $Path + ".crypt"
    }
    try {
        $EncryptionKey = [System.Convert]::FromBase64String($Key)
        $KeySize = $EncryptionKey.Length * 8
        $AESProvider = New-Object 'System.Security.Cryptography.AesManaged'
        $AESProvider.Mode = [System.Security.Cryptography.CipherMode]::CBC
        $AESProvider.BlockSize = 128
        $AESProvider.KeySize = $KeySize
        $AESProvider.Key = $EncryptionKey
    }
    Catch {
        Write-Error 'Unable to configure AES, verify you are using a valid key.'
        Return
    }
    Write-Verbose "Encryping $($File.FullName) with the $KeySize-bit key $Key"
    Try {
        $FileStreamReader = New-Object System.IO.FileStream($File.FullName, [System.IO.FileMode]::Open)
    }
    Catch {
        Write-Error "Unable to open $($File.FullName) for reading.`n$_"
        return 1
    }
    Try {
        $FileStreamWriter = New-Object System.IO.FileStream($Destination, [System.IO.FileMode]::Create)
    }
    Catch {
        Write-Error "Unable to open $Destination for writing."
        $FileStreamReader.Close()
        return 1
    }
    $AESProvider.GenerateIV()
    $FileStreamWriter.Write([System.BitConverter]::GetBytes($AESProvider.IV.Length), 0, 4)
    $FileStreamWriter.Write($AESProvider.IV, 0, $AESProvider.IV.Length)
    Write-Verbose "Encrypting $($File.FullName) with an IV of $([System.Convert]::ToBase64String($AESProvider.IV))"
    try {
        $Transform = $AESProvider.CreateEncryptor()
        $CryptoStream = New-Object System.Security.Cryptography.CryptoStream($FileStreamWriter, $Transform, [System.Security.Cryptography.CryptoStreamMode]::Write)
        [Int]$Count = 0
        [Int]$BlockSizeBytes = $AESProvider.BlockSize / 8
        [Byte[]]$Data = New-Object Byte[] $BlockSizeBytes
        Do {
            $Count = $FileStreamReader.Read($Data, 0, $BlockSizeBytes)
            $CryptoStream.Write($Data, 0, $Count)
        }
        While ($Count -gt 0)
    
        #Close open files
        $CryptoStream.FlushFinalBlock()
        $CryptoStream.Close()
        $FileStreamReader.Close()
        $FileStreamWriter.Close()

        Write-Verbose "Successfully encrypted $($File.FullName)"
    }
    catch {
        Write-Error "Failed to encrypt $($File.FullName)."
        $CryptoStream.Close()
        $FileStreamWriter.Close()
        $FileStreamReader.Close()
        Remove-Item $Destination
        return 1
    }
    return 0
}
function Restore-AesFile {
    param(
        [System.IO.FileInfo]$File,
        [string]$Key,
        [string]$Destination
    )
    try {
        $EncryptionKey = [System.Convert]::FromBase64String($Key)
        $KeySize = $EncryptionKey.Length * 8
        $AESProvider = New-Object 'System.Security.Cryptography.AesManaged'
        $AESProvider.Mode = [System.Security.Cryptography.CipherMode]::CBC
        $AESProvider.BlockSize = 128
        $AESProvider.KeySize = $KeySize
        $AESProvider.Key = $EncryptionKey
    }
    Catch {
        Write-Error 'Unable to configure AES, verify you are using a valid key.'
        Return
    }
    Write-Verbose "Encryping $($File.FullName) with the $KeySize-bit key $Key"

    #Open file to decrypt
    Try {
        $FileStreamReader = New-Object System.IO.FileStream($File.FullName, [System.IO.FileMode]::Open)
    }
    Catch {
        Write-Error "Unable to open $($File.FullName) for reading."
        return 1
    }
    
    Try {
        $FileStreamWriter = New-Object System.IO.FileStream($Destination, [System.IO.FileMode]::Create)
    }
    Catch {
        Write-Error "Unable to open $DestinationFile for writing."
        $FileStreamReader.Close()
        return 1
    }

    #Get IV
    try {
        [Byte[]]$LenIV = New-Object Byte[] 4
        $FileStreamReader.Seek(0, [System.IO.SeekOrigin]::Begin) | Out-Null
        $FileStreamReader.Read($LenIV, 0, 3) | Out-Null
        [Int]$LIV = [System.BitConverter]::ToInt32($LenIV, 0)
        [Byte[]]$IV = New-Object Byte[] $LIV
        $FileStreamReader.Seek(4, [System.IO.SeekOrigin]::Begin) | Out-Null
        $FileStreamReader.Read($IV, 0, $LIV) | Out-Null
        $AESProvider.IV = $IV
    }
    catch {
        Write-Error 'Unable to read IV from file, verify this file was made using the included New-AesFile function.'
        return 1
    }

    Write-Verbose "Decrypting $($File.FullName) with an IV of $([System.Convert]::ToBase64String($AESProvider.IV))"

    #Decrypt
    try {
        $Transform = $AESProvider.CreateDecryptor()
        [Int]$Count = 0
        [Int]$BlockSizeBytes = $AESProvider.BlockSize / 8
        [Byte[]]$Data = New-Object Byte[] $BlockSizeBytes
        $CryptoStream = New-Object System.Security.Cryptography.CryptoStream($FileStreamWriter, $Transform, [System.Security.Cryptography.CryptoStreamMode]::Write)
        Do {
            $Count = $FileStreamReader.Read($Data, 0, $BlockSizeBytes)
            $CryptoStream.Write($Data, 0, $Count)
        }
        While ($Count -gt 0)

        $CryptoStream.FlushFinalBlock()
        $CryptoStream.Close()
        $FileStreamWriter.Close()
        $FileStreamReader.Close()

        Write-Verbose "Successfully decrypted $($File.FullName)"
    }
    catch {
        Write-Error "Failed to decrypt $($File.FullName)."
        $CryptoStream.Close()
        $FileStreamWriter.Close()
        $FileStreamReader.Close()
        Remove-Item $Destination
        return 1
    }        
    return 0
}
```

所以说这工具并没有什么技术含量，主要是功能的整合。我在使用 PowerShell 管道的解析 git 的输出时感觉比 Shell 脚本方便多了。

## 最后关于 PowerShell

非常推荐 PowerShell。

## 其他

相关新闻链接 [码云存储库加密工具 1.0 正式发布](https://www.oschina.net/news/87432/git-1-0-released)