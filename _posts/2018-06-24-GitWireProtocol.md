---
layout: post
title:  "Git Wire 协议杂谈"
date:   2018-06-24 10:00:00
published: true
categories: git
---

# 前言

注意本文的 **GIT 传输协议** 仅代表 **智能传输协议**

美国当地时间 5月18日 Google 开发者发布了一篇博客 [Introducing Git protocol version 2](https://opensource.googleblog.com/2018/05/introducing-git-protocol-version-2.html) 宣布了 Git v2 Protocol，v2 协议又叫做 Git Wire Protocol，新协议旨在改进 Git 的传输过程。Git 不是有个好好的传输协议，为什么要重新弄一个？

## Git 传输协议的缺陷

抛开 `git-upload-archive` 不说，Git 传输协议的核心实际上是 `git-fetch-pack`/`git-upload-pack`，`git-send-pack`/`git-receive-pack` 是两组命令之间交换输出输出，这就代表一个传输周期就是对应命令的生存周期。

无论是 fetch 还是 push，传输开始都有一个服务器发现引用的过程，在这个过程中，服务器上的 `git-upload-pack(git-receive-pack)` 将服务器上的所有引用按照特定格式发送给客户端。当引用数目较多的时候，这个过程就比较缓慢了，在没有 GC 时，引用的分布布局为 
`$gitdir/refs/heads/$bracnh_name`,
`$gitdir/refs/tags/$tag_name`,
`$gitdir/refs/(pull/merge/others)/$refname`，

引用的 id 在文件中，如果有上万个引用，这就意味着服务器需要进行一些目录遍历，并且进行上万次文件读取，一旦服务器上的并发较大时，引用发现的过程就比较缓慢了，但很多时候，用户可能只是需要某一个引用罢了。

另外一方面，传输协议时一组命令的输入输出交换，这种机制的可扩展性非常差，比如，git 有浅表克隆，这是比较晚出现的功能，git 需要协商需要的 commit 深度，由于 HTTP 协议是无状态协议，请求流程是 Request-Response ，因此在第一个 HTTP post 请求时，fetch-pack 需要输出 depth 的值，然后等待 upload-pack 计算此上限 commit id，fetch-pack 再次发送请求告知 upload-pack 需要的上限 commitid，在 v1 协议中只能别扭的实现此功能，Http Git 服务器开发者也需要注意此处需要及时的让 git-upload-pack 退出，否则会出现竞争长时间挂起。

## Git Wire  协议的改进

在 Wire 协议中，传输协议的机制发生了比较大的改变，即在 `git-upload-pack/git-receive-pack` 的传输过程中，客户端可以指定特定的 git 子命令，服务器端执行获取输出即可，对于前面的第一个缺陷，在 Wire 协议中可以使用 `ls-refs` 获取引用，`ls-refs` 是一个内置函数实现，[{ "ls-refs", always_advertise, ls_refs }](https://github.com/git/git/blob/e144d126d74f5d2702870ca9423743102eec6fcd/serve.c#L57)，当 `ls-refs` 使用参数 `ref-prefix` 即可获取只需要的引用。`ls-refs` 参数如下：

```
symrefs
    In addition to the object pointed by it, show the underlying ref
    pointed by it when showing a symbolic ref.
peel
    Show peeled tags.
ref-prefix <prefix>
    When specified, only references having a prefix matching one of
    the provided prefixes are displayed.
```

对于浅表克隆，使用 Wire 协议也要好多了，这时使用的是 `fetch` 命令，fetch 命令中可以直接指定 `deepen N` 而不用像 v1 协议一样要多一次协商（对于 HTTP 来说是多一次请求。）

命令允许的参数如下：

```
want <oid>
    Indicates to the server an object which the client wants to
    retrieve.  Wants can be anything and are not limited to
    advertised objects.
have <oid>
    Indicates to the server an object which the client has locally.
    This allows the server to make a packfile which only contains
    the objects that the client needs. Multiple 'have' lines can be
    supplied.
done
    Indicates to the server that negotiation should terminate (or
    not even begin if performing a clone) and that the server should
    use the information supplied in the request to construct the
    packfile.
thin-pack
    Request that a thin pack be sent, which is a pack with deltas
    which reference base objects not contained within the pack (but
    are known to exist at the receiving end). This can reduce the
    network traffic significantly, but it requires the receiving end
    to know how to "thicken" these packs by adding the missing bases
    to the pack.
no-progress
    Request that progress information that would normally be sent on
    side-band channel 2, during the packfile transfer, should not be
    sent.  However, the side-band channel 3 is still used for error
    responses.
include-tag
    Request that annotated tags should be sent if the objects they
    point to are being sent.
ofs-delta
    Indicate that the client understands PACKv2 with delta referring
    to its base by position in pack rather than by an oid.  That is,
    they can read OBJ_OFS_DELTA (ake type 6) in a packfile.
```

浅表克隆允许的参数

```
shallow <oid>
    A client must notify the server of all commits for which it only
    has shallow copies (meaning that it doesn't have the parents of
    a commit) by supplying a 'shallow <oid>' line for each such
    object so that the server is aware of the limitations of the
    client's history.  This is so that the server is aware that the
    client may not have all objects reachable from such commits.
deepen <depth>
    Requests that the fetch/clone should be shallow having a commit
    depth of <depth> relative to the remote side.
deepen-relative
    Requests that the semantics of the "deepen" command be changed
    to indicate that the depth requested is relative to the client's
    current shallow boundary, instead of relative to the requested
    commits.
deepen-since <timestamp>
    Requests that the shallow clone/fetch should be cut at a
    specific time, instead of depth.  Internally it's equivalent to
    doing "git rev-list --max-age=<timestamp>". Cannot be used with
    "deepen".
deepen-not <rev>
    Requests that the shallow clone/fetch should be cut at a
    specific revision specified by '<rev>', instead of a depth.
    Internally it's equivalent of doing "git rev-list --not <rev>".
    Cannot be used with "deepen", but can be used with
    "deepen-since".
```

过滤器参数：

```
filter <filter-spec>
    Request that various objects from the packfile be omitted
    using one of several filtering techniques. These are intended
    for use with partial clone and partial fetch operations. See
    `rev-list` for possible "filter-spec" values.
```

另外 Wire 协议中的过滤器功能在以后支持部分克隆等功能时可能会变得非常重要。

## 如何支持 Wire 协议

支持 Wire 协议需要客户端和服务器同时支持，Git 通常可以使用 HTTP，SSH，Git 协议进行传输，针对不同协议，需要特定的修改。

HTTP 协议需要检测请求头中是否有 Git-Protocol 然后 GET 请求时不再输出 `# service=git-upload-pack` 这样的标志，添加环境变量 `GIT_PROTOCOL=version=2` 去执行 `git-upload-pack/git-receive-pack`.

SSH 协议支持 SetEnv 修改环境变量，所以只要接受客户端的 SetEnv 请求，然后和 HTTP 一样即可，SSH 没有 `# service=xx` 因此也没有那一不需要修改。

Git 协议需要重新解析数据头，判断是否存在 `version=2`，类似头部 `003egit-upload-pack /project.git\0host=myserver.com\0\0version=2\0`。

值得注意的是，在 SSH 协议中，如果是 SSH 服务器并不是直接启动 git 命令而是代理到其他服务器上时，需要区别对待 v1 与 Wire 协议的传输流程进行少量修改。

## 如何使用

用户需要安装 git 2.18 . 然后找到一个支持 v2 的服务器，目前主流的 Git 托管平台都暂不支持（除了 Google 的），官方的 git-http-backend 支持，这里有一个笔者贡献的 git-http-backend 已经支持：[https://github.com/asim/git-http-backend](https://github.com/asim/git-http-backend)。 需要注意服务器上的 git 也需要是最新版，否则无法使用。

```sh
git -c protocol.version=2 clone http://domian/repo.git
```

## 其他

Wire 协议的细节：[git/docs/technical/protocol-v2.html](https://mirrors.edge.kernel.org/pub/software/scm/git/docs/technical/protocol-v2.html)

这里有一个 Wire 协议与 Smart 协议抓包对比：[Git Wire Protocol vs Git Smart Protocol](https://gitee.com/ipvb/codes/y1b4ew3vqfgom5298iznh69)