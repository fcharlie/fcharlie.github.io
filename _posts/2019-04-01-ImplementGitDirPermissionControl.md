---
layout: post
title:  "实现 Git 目录权限控制"
date:   2019-04-01 10:00:00
published: true
categories: git
---

# 前言
 
 Git 与 Subversion 有诸多不同，最核心的一点是前者属于分布式版本控制工具，后者属于集中式版本控制工具。前者的提交行为是离线的，本地的，后者的提交是在线的，需要与远程中央服务器通信，在线创建提交。基于这种现实，Git 和 Subversion 在原生提供的附加功能也存在很大的差别。比如目录权限控制。Git 原生并不支持目录权限控制，而 Subversion 支持。


## Subversion 的目录权限控制

 用户接入远程服务器上的 Subversion 存储库通常可以使用 HTTP 协议 SVN 协议以及 SVN+SSH 协议，HTTP 协议本质上是 HTTP 客户端与 Apache httpd 服务器通信，此时，请求由 `mod_dav_svn.so` 模块处理，然后调用 subversion 的核心模块，包括文件系统和存储库模块。使用 HTTP 访问 Subversion 存储库时，可以如下：

 ```shell
 svn co https://llvm.org/svn/llvm-project/llvm/trunk
 ```

 还可以直接检出存储库的子目录：

 ```shell
 svn co https://llvm.org/svn/llvm-project/llvm/trunk/include/
 ```

使用 SVN 协议与 SVN+SSH 协议本质上都是与远程服务器上的 `svnserve` 通信，前者是 svnserve 监听 3690 TCP 端口，后者是在远程服务器上运行 `svnserve -t`，但内部的细节都是一致的。检出代码如下：

 ```shell
 svn co svn://llvm.org/svn/llvm-project/llvm/trunk
 ```

 还可以直接检出存储库的子目录：

 ```shell
 svn co svn://llvm.org/svn/llvm-project/llvm/trunk/include/
 ```

Subversion 的提交是在线的，以 SVN 协议为例，当用户提交代码时，svn 客户端将发送 `commit` 命令消息给 svnserve，然后在这个命令中将修改的文件发送到远程服务器，远程服务器上作出一些列操作，然后创建提交。在这个过程中，只要对文件相对存储库的路径与目录权限规则作出匹配，就可以知道用户是否由能力修改相应文件。

Subversion 的原理也就使其很容易支持细粒度的权限控制，可以精确到文件。svnserve 可以修改 svnserve.conf 实现，而 HTTP 则与 apache mod_dav_svn 模块有关。

如果要禁止用户读取，则在 checkout 的时候禁止检出即可，实现起来并不困难。

## Git 不支持目录权限控制

像 Git 这样的分布式版本控制系统，获得远程存储库的行为叫做 `clone`，常规情况下获得远程存储库的所有数据，但这种方式毕竟需要获得大量数据，因此 Git 还提供浅表克隆，还有 VFSForGit 这样的方式只获得目录结构，不获取文件内容，还有规划中的部分克隆。

在服务器上，要控制存储库的数据按目录粒度提供给用户读取，通常无法通过 Git 客户端实现，这是由 Git 传输协议决定的，Git 在克隆远程存储库时需先 '发现引用列表'，然后按需求获得引用列表的提交 ID，服务器将这些 Commit 以及相关的 Tree Blob 等等回溯打包发送给客户端。Git 服务器最多能实现的是在在 '引用广播' 时隐藏特定的引用实现对用户的禁止读取，但要做到目录级别的禁止读取，目前由于 Git 协议限制暂时无法实现。

分布式版本控制系统的提交行为发生在本地，当人们运行 `git commit -m "message"` 时，就会在本地创建一个提交。无论是 Subversion 还是 Git，提交在存储库中具有唯一性，提交创建后，目录结构，文件内容，提交信息也就已经确定了，如果再次修改就不再是相同的提交。在 Subversion 中提交占据唯一的一个修订号(Revision)，在 Git 中则是唯一的 commitID (目前为 SHA1，未来计划转变为 SHA-256)。在本地提交后，Git 存储库目录结构已经确定，因此推送到服务器并不能改变其目录结构，这就意味着，在服务器上 Git 不支持目录级别粒度的写入控制。

综合来看，在服务器方，Git 是完全不支持目录权限控制的。

## Git 实现目录权限控制的一种途径

**Git 的目录级别粒度的禁止读取并不在本文的研究范围，本文所述的目录权限控制仅指 '目录的只读'。**

在设计此功能前，我们需要了解 Git 的一些原理，比如，目录权限控制发生在服务端，因此，处理的时机为 Git 推送代码代远程存储库时，一旦发现推送中修改了只读文件（目录），则拒绝推送。用户被拒绝推送后需要回退特定的提交，然后重新提交，提交完成后推送到远程服务器，实现协作共享，这样就达到了目录权限控制的目的。

实现目录权限控制核心操作就是检测目录结构的修改，然后按照特定的规则进行拦截，在不修改 Git 源码的基础上要实现这些功能通常可以使用服务端钩子。服务器上的主要的钩子有三种，`pre-receive`，`update`,`post-receive` ，相关钩子的原理在 [Git 原生钩子的深度优化](https://forcemz.net/git/2017/11/22/GitNativeHookDepthOptimization/) 一文中有介绍。要实现本节所述功能首先得排除 `post-receive` 钩子，此钩子的成功与否不会影响引用的更新，所以并不合适，而 `pre-receive` 钩子目前使用了 [环境隔离： Quarantine Environment](https://git-scm.com/docs/git-receive-pack#_quarantine_environment) 机制，目前 libgit2 并不支持此特性，所以要实现目录只读也不能直接使用 `pre-receive` 钩子。

最终，只有 `update` 钩子可用。

在实现目录权限检测时，我们应当先初始化规则，在 [git-analyze](https://gitee.com/oscstudio/git-analyze) 项目中，我添加了一个 `update` 钩子作为目录权限检测的原型，规则文件如下：

```json
{
  "master":{
    "dirs":[
      "lib/",
      "build/"
    ],
    "regex":[
      "build$"
    ]
  }
}

```

规则按分支名划分，当相关的分支并没有设置只读目录时，则查找 `.all` 配置，其中 `.all` 表示所有分支，`.all` 不能被用于 git 分支或者引用名称<sup>1</sup>，因此代表所有的分支。规则有目录级别匹配和正则匹配，`lib` 表示 `lib` 文件或者目录不可被修改。而 `regex` 则表示目录符合正则表达式的路径不可被修改。

对于正则表达式，我们使用的是 [RE2](https://github.com/google/re2)，这是一个非常高效的正则表达式库，由 Google 开发。拥有多种语言绑定。

无论是目录匹配还是正则表达式，我们都需要设置规则数量的上限，避免过多的规则导致计算量过大。目录匹配目前的限制为 256，正则限制为 64。

在确定好规则后，我们需要获得目录树的修改细节，包括文件的修改，文件的删除，文件的添加，以及文件的重命名，其中文件的 UNIX 属性修改应当属于文件的修改。update 钩子的命令行为 `refname oldrev newrev`，我们可以使用 `git diff oldrev newrev` 这样的思路获得文件修改细节。在 libgit2 中，可以使用 `git_diff_tree_to_tree` 去获得目录的差异。如果 oldrev 或者 newrev 二者中有一个是 `zeroid`，此时只需要使用 treewalk 遍历目录即可，当然，二者不可能同时为零 ID。核心代码如下：

```cpp
////////
#include "executor.hpp"
#include <git2.h>

struct commit_t {
  commit_t() = default;
  ~commit_t() {
    if (tree != nullptr) {
      git_tree_free(tree);
    }
    if (commit != nullptr) {
      git_commit_free(commit);
    }
  }
  git_commit *commit{nullptr};
  git_tree *tree{nullptr};
  bool lookup(git_repository *repo, const git_oid *id) {
    if (git_commit_lookup(&commit, repo, id) != 0) {
      return false;
    }
    return git_commit_tree(&tree, commit) == 0;
  }
};

class executor_base {
public:
  executor_base() {
    //
    git_libgit2_init();
  }
  ~executor_base() {
    if (repo_ != nullptr) {
      git_repository_free(repo_);
    }
    git_libgit2_shutdown();
  }
  bool open(std::string_view path) {
    return git_repository_open(&repo_, path.data()) == 0;
  }
  git_repository *repo() { return repo_; }

private:
  git_repository *repo_{nullptr};
};

Executor::Executor() {
  // initialzie todo
  base = new executor_base();
}
Executor::~Executor() {
  // delete
  delete base;
}

bool Executor::InitializeRules(std::string_view sv, std::string_view ref) {
  if (ref.compare(0, sizeof("refs/heads/") - 1, "refs/heads/") != 0) {
    return true;
  }
  auto branch = ref.substr(sizeof("refs/heads/") - 1);
  return engine.PreInitialize(sv, branch);
}

////////////////////////////////////////////////
int git_treewalk_impl(const char *root, const git_tree_entry *entry,
                      void *payload) {
  auto e = reinterpret_cast<Executor *>(payload);
  std::string name = root;
  name.append(git_tree_entry_name(entry));
  if (e->FullMatch(name)) {
    fprintf(stderr, "Path %s is readonly\n", name.c_str());
    return 1;
  }
  return 0;
}
bool Executor::ExecuteTreeWalk(std::string_view rev) {
  git_oid oid;
  if (git_oid_fromstrn(&oid, rev.data(), rev.size()) != 0) {
    return false;
  }
  commit_t commit;
  if (!commit.lookup(base->repo(), &oid)) {
    return false;
  }
  return (git_tree_walk(commit.tree, GIT_TREEWALK_PRE, git_treewalk_impl,
                        this) == 0);
}

int git_diff_callback(const git_diff_delta *delta, float progress,
                      void *payload) {
  (void)progress;
  auto e = reinterpret_cast<Executor *>(payload);
  switch (delta->status) {
  case GIT_DELTA_ADDED:
    /*fallthrough*/
  case GIT_DELTA_MODIFIED:
  /*fallthrough*/
  case GIT_DELTA_COPIED: // copy to new path
    if (e->FullMatch(delta->new_file.path)) {
      fprintf(stderr, "Path '%s' is readonly\n", delta->old_file.path);
      return 1;
    }
    break;
  case GIT_DELTA_DELETED:
    if (e->FullMatch(delta->old_file.path)) {
      fprintf(stderr, "Path '%s' is readonly\n", delta->old_file.path);
      return 1;
    }
    break;
  default:
    // ex. GIT_DELTA_RENAMED
    if (e->FullMatch(delta->new_file.path)) {
      fprintf(stderr, "Path '%s' is readonly\n", delta->old_file.path);
      return 1;
    }
    if (e->FullMatch(delta->old_file.path)) {
      fprintf(stderr, "Path '%s' is readonly\n", delta->old_file.path);
      return 1;
    }
    break;
  }
  return 0;
}

struct diff_t {
  ~diff_t() {
    if (p != nullptr) {
      git_diff_free(p);
    }
  }
  git_diff *p{nullptr};
};

bool Executor::Execute(std::string_view path, std::string_view oldrev,
                       std::string_view newrev) {
  if (engine.Empty()) {
    // Engine rules empty so return
    return true;
  }
  if (!base->open(path)) {
    return false;
  }
  constexpr const char zerooid[] = "0000000000000000000000000000000000000000";
  if (oldrev == zerooid) {
    return ExecuteTreeWalk(newrev);
  }
  if (newrev == zerooid) {
    return ExecuteTreeWalk(oldrev);
  }
  git_oid ooid, noid;
  if (git_oid_fromstrn(&ooid, oldrev.data(), oldrev.size()) != 0) {
    return false;
  }
  if (git_oid_fromstrn(&noid, newrev.data(), newrev.size()) != 0) {
    return false;
  }
  commit_t oldcommit, newcommit;
  if (!oldcommit.lookup(base->repo(), &ooid)) {
    return false;
  }
  if (!newcommit.lookup(base->repo(), &noid)) {
    return false;
  }
  diff_t diff;
  git_diff_options opts;
  git_diff_init_options(&opts, GIT_DIFF_OPTIONS_VERSION);
  if (git_diff_tree_to_tree(&diff.p, base->repo(), oldcommit.tree,
                            newcommit.tree, &opts) != 0) {
    return false;
  }
  return git_diff_foreach(diff.p, git_diff_callback, nullptr, nullptr, nullptr,
                          this) == 0;
}

```

update 工具源码地址为：[https://gitee.com/oscstudio/git-analyze/tree/master/tools/update](https://gitee.com/oscstudio/git-analyze/tree/master/tools/update)。

在本方案中，我们直接比较 oldrev-newrev 的差异，而不是回溯递归去判断 commit 与 parent-commit 的差异，这有以下考虑：
+  我们需要限制的是最终的结果而无需考虑中间的过程，无论中间环节如何变化，只要最终的只读目录，文件未发生修改即可。未发生修改表示 blob 不变，tree 不变。
+  我们只 diff oldrev-newrev 两者，避免校验中间环境能够降低只读文件功能造成代码推送的响应速率下降。

这实际上与使用 `update` 钩子实现大文件检测的策略有很大的不同，大文件检测如果不回溯去检测，则可能会忽略中间 commit 引入的大文件，这些大文件会一直存在于存储库中，导致存储库体积较大，打包困难，传输时间长。这也是我使用 pre-receive 钩子基于环境隔离，使用 Pack-Index 文件检测大文件取代 update 钩子的原因之一。

在 git diff 的文件修改中，文件路径为相对路径，如果是在 worktree 中，diff 的输出结果与当前目录有关，而在裸存储库中，diff 的文件路径与是相对于 存储库 worktree 根目录的路径。那么我们在生成路径校验规则时，则需对文件路径进行处理以符合相对路径。并且 git diff 的文件路径以 `/` 分割，要判断一个路径是否为只读文件或者属于只读目录下的文件，只需要判断文件路径被包含在只读路径之中即可。

我们先将路径规范化，也就是去除多余的 `/`，然后比较文件路径以只读路径开头并且存在如下两种情况：

+  文件路径长度与只读路径长度相符，文件路径为只读文件路径。
+  只读路径长度为 l，文件路径为符号 P，其中 `P[l]='/'`，此时路径为只读路径的子路径。

路径包含判断代码如下：

```c++
// when sub startswith parant
// 1.  size equal --> path equal
// 2.  path[p.size()]=='/' At this point, 'path' is a subpath of p
inline bool IsPathContains(std::string_view parent, std::string_view sub) {
  return (parent.size() <= sub.size() &&
          memcmp(parent.data(), sub.data(), parent.size()) == 0 &&
          (sub.size() == parent.size() || sub[parent.size()] == '/'));
}
```

如果使用 `update` 钩子实现 commit 私有邮箱过滤，为了避免私有邮箱逃逸，则需要回溯。

## 反思

虽然本文讲述了实现 Git 目录权限控制的实现的一种途径，但笔者比较反对过分使用 Git 服务端钩子实现过多额外的功能。一方面，这些功能并非不可替代，大多数都能可以通过规范的协作方式实现相同的目的。比如我们可以使用 fork-RP 模型参与协作开发，然后在主存储库中设置保护分支，严格限制修改，我们还可以实现只读存储库，即修改只能通过 PR 修改存储库，拒绝从客户端推送等。

另一方面，如果在钩子中实现的功能过多，这势必会导致服务器上存储库响应时间变长，服务器并发降低，这时候加机器也效果有限，特别是硬件设施不足的影响更大。功能过多也会导致开发变得异常复杂，钩子除了做这个还要做那个，整个流程处理流程一环扣一环，这对开发者要求比较高。如果出于效率原因修改 git 源码实现这些功能则可能带来自行维护 git 分支的困境，特别是 git 也是在不断发展的，比如 git 计划实施的部分克隆，SHA1 到 SHA-256 的转换，v2 协议的进一步发展，以及 VFSforGit 移植到 Linux。如果没有充足的人力去维护这些修改，很容易就跟不上 git 的步伐，则很容易在代码托管平台的竞争中落后。


## 备注

1. Git 引用名格式检查：[https://git-scm.com/docs/git-check-ref-format](https://git-scm.com/docs/git-check-ref-format)