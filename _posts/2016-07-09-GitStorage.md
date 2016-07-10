---
layout: post
title:  "Git 存储格式与运用"
date:   2016-07-09 20:00:00
published: true
categories: git
---

# GIT 存储格式与运用

在 GIT 的实现规范中，存储格式是非常简单而且高效的，而一个代码托管平台通常需要
基于这些特性实现一非常有意思的功能。在本文中，将介绍基于 GIT 存储库格式实现的
仓库体积限制与大文件检查。

## 存储库的布局

正常的 GIT 存储库布局应当遵循 GIT 规范 [Git Repository Layout](https://github.com/git/git/blob/master/Documentation/gitrepository-layout.txt)
一个 GIT 仓库包括如下两种风格：

+ `.git` 目录存在于工作目录的根目录中。
+ `<project>.git` 这种是一个裸仓库，没有工作目录，服务器上存储的就是这种。

特别注意的是，如果是一个 **子模块 （submodule）** `.git` 回是一个文件，文件内容为 `gitdir:/path/to/gitdir`

下文是一个表格，关于目录结构和描述信息。

| 路径 | 目录（F）\ 文件 （F） | 描述 |
|--------|-------|----------|
| objects | D | 松散对象和包文件 |
| refs | D | 引用，包括头引用，标签引用，和远程引用 |
| packed-refs | F | 打包的引用，通常运行 `git gc` 后产生 |
| HEAD | F | 当前指向的引用或者 oid，例如 refs/heads/master |
| config | F |存储库的配置，可以覆盖全局配置 |
| branches | D | -|
| hooks |D | 请查看 Documentation/githooks.txt |
| index | F| git index file, Documentation/technical/index-format.txt |
| `sharedindex.<SHA-1>`|F|-|
| info | D | 存储库信息,哑协议依赖 info/refs |
| remotes |D|-|
| logs| D| 运行 git log 可以查看提交记录 |
| shallow | F|-|
| commondir| D|-|
| modules |D| 子模块的 git 目录 |
| worktrees| D | 工作目录，更新后的文档与 git 多个工作目录有关 |

对于一些接触到非常少的路径，我就没有添加说明了。

## 松散文件格式

在 objects 目录中，有 00,01,...ff 这样的目录，目录下存储着文件名长度为38的二进制文件，这些文件就是松散文件，
git 创建提交时，修改的文件，更新的目录树，以及提交内容会被压缩后写入到这些目录中，成为一个个松散文件，当需要传输
或者运行 gc 时，这些文件就会被写入到 pack 文件中。

对象文件使用的压缩算法是 deflate，增加一个新的对象文件时，先要计算这个文件的 hash 值（长度 20，16 进制 40 个字符），
这个根据这个值查找文件是否存在 这些目录或者 pack 文件中，不存在则创建前缀目录（hash 值 16 进制字符串前两个），然后、
将原始文件的类型以及长度信息以及内容一起压缩，写入到磁盘，文件名是 hash 值的 16 进制的后 38 个字符。

通过解压缩可以得到下面格式的文件：

>type SP digest NUL body

其中类型是 commit blob tree 而长度则是 10 进制的数字，以字节为单位。后面的 body 就是各种类型文件的内容。

blob 就是真实的文件，而 tree 就是将文件按目录结构和属性组织起来，在 tree 中每一个 tree entry 可能是 blob，
也可能是 tree，也有可能是 commit，在有 submodule 的情况下就有 commit。

{% highlight shell %}
100644 blob 33d07c06bd90833ce56bc64c13bdc08c1997c3fb    .gitattributes
100644 blob 6483b21cbfc73601602d628a2c609d3ca84f9e53    .gitignore
100755 blob a88b6824b908d89ee185b84ed92b9c122b0118dd    GIT-VERSION-GEN
100644 blob 4f00bdd3d69babe8a58c4989406eaa6fb5f36a50    Makefile
100755 blob 4277f30c4116faf2788243af4ec23f1d077698e8    git-gui--askpass
100755 blob 11048c7a0e94f598b168de98d18fda9aea420c7d    git-gui.sh
040000 tree 1ead6a96af286100752067ea1849d49b35ce1d35    lib
040000 tree 452280d7fa4a155bd311a7cce7e327964267b792    macosx
040000 tree 2294a6a975b861ecdb5b03d091877c14ec696621    po
040000 tree ae99a38593d127e47f956d96abf3d6a40d3aff66    windows
{% endhighlight %}

在 Git-SCM 中，有例图显示了这种结构：

![Data-Model](https://git-scm.com/book/en/v2/book/10-git-internals/images/data-model-1.png)

如何解压对象文件？大多数语言都绑定了 zlib，放心去使用即可。

比如 C# 有 System.IO.Compression 有 System.IO.Compression.DeflateStream 类，就可以拿过来使用。
如果是 C++ 直接使用 zlib 中 z_stream 即可，Linux Unix 都带了，然后 Visual Studio 可以使用 NuGet 安装
倒项目中，可以使用。

那么计算 HASH 呢？OpenSSL 提供了 hash 函数，大多数 Linux 和 Unix 都带了，可以使用，在 Windows 中也可以使用 OpenSSL,
当然也可以使用 Windows 自带的加密算法动态库 bcrypt.dll，你也可以在网上找到一个 SHA-1 算法的实现。

## Pack 文件格式

Pack 文件的设计使得 git 仓库可以更好的节省磁盘空间，有利于服务器之间传输数据。

## 仓库大小限制和文件大小检测

在 Unix/Linux 或者 Bash On Windows 中，可以使用下面这个例子

{% highlight cpp %}
class ScanningFolder {
public:
  bool FolderSizeResolve(const std::string &dir__) {
    DIR *dir = opendir(dir__.c_str());
    if (dir == nullptr) {
      fprintf(stderr, "opendir: %s\n", strerror(errno));
      return false;
    }
    // folder self
    size_ += 4096;
    dirent *dirent_ = nullptr;
    while ((dirent_ = readdir(dir))) {
      if (dirent_->d_type & DT_REG) {
        std::string file = dir__ + "/" + dirent_->d_name;
        struct stat stat_;
        if (stat(file.c_str(), &stat_) != 0) {
          fprintf(stderr, "ERROR: %s\n", strerror(errno));
          closedir(dir);
          return false;
        } else {
          // S_BLKSIZE 512
          size_ += stat_.st_blocks * S_BLKSIZE;
        }
      } else if (dirent_->d_type & DT_DIR) {
        if (strcmp(dirent_->d_name, ".") == 0 ||
            strcmp(dirent_->d_name, "..") == 0) {
          continue;
        }
        std::string newdir = dir__ + "/" + dirent_->d_name;
        if (!FolderSizeResolve(newdir)) {
          closedir(dir);
          return false;
        }
      }
    }
    closedir(dir);
    return true;
  }
  uint64_t Size() const { return this->size_; }

private:
  uint64_t size_ = 0;
};
{% endhighlight %}

在 Windows 中，遍历目录可以使用 FindFirstFile/FindNextFile 这个两个 API。

{% highlight cpp %}
class FolderSize {
public:
  FolderSize(const std::wstring &dir) : size_(-1) {}
  int64_t Size() const { return size_; }

private:
  bool TraverseFolder(const std::wstring &dir) {
    WIN32_FIND_DATAW find_data;
    HANDLE hFind = FindFirstFileW(dir.c_str(), &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
      return false;
    }
    while (true) {
      if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (wcscmp(find_data.cFileName, L".") != 0) {
          std::wstring xdir = dir;
          xdir.push_back('\\');
          xdir.append(find_data.cFileName);
          TraverseFolder(xdir);
        }
      } else {
        size_ +=
            ((int64_t)find_data.nFileSizeHigh << 32 + find_data.nFileSizeLow);
      }
      if (!::FindNextFileW(hFind, &find_data)) {
        break;
      }
    }
    FindClose(hFind);
    return true;
  }
  int64_t size_;
};
{% endhighlight %}