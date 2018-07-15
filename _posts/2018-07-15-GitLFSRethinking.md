---
layout: post
title:  "Git LFS 的反思"
date:   2018-07-15 10:00:00
published: true
categories: git
---

# 前言

在一年多以前，笔者曾经写过一文： [《Git LFS 服务器实现杂谈》](https://forcemz.net/git/2017/04/16/Moses/)，最近笔者开发基于**对象存储**的 LFS 服务器又有了一些心得，这里分享给大家。

# 关于 Git LFS

Git LFS 即 Git Large File Storage （大文件存储），即将 git 存储库中的体积较大的，不利于打包的，修改不太频繁的文件单独存储到特定的服务器上，以减小存储库体积，加快用户的克隆拉取体验。其中的原理在 [《Git LFS 服务器实现杂谈》](https://forcemz.net/git/2017/04/16/Moses/) 都有说明，如果需要进一步的了解还可以去参考 Git LFS 技术规范： [ spec.md](https://github.com/git-lfs/git-lfs/blob/master/docs/spec.md)。

按照 LFS 规范，只要实现了 Git LFS API 以及相应的接口，LFS 客户端并不会关心大文件存储在那个地方。

# Git LFS 存储

LFS 对象可以存储到代码托管平台的服务器上，也可以存储到对象存储服务器上，不同的平台有不同的选择。这里选择几个分析一下。

## Github LFS 存储

Github 很早就与 Amazon 有深度的合作，比如 Github 的附件，Release 都是将文件存储到 AWS S3 上，然后时候共享密钥的方式下载，Github 将这些文件放在 S3 上，减轻了 Github 自身服务器的负载。文件备份等都不需要 Github 自己管理，这无疑降低了开发的难度。
 
 所以在实现 Git LFS 时，Github 也将对象存储到 AWS S3 上，我们可以通过如下命令去调试 Github 的对象存储：

```shell
GIT_TRACE_PACKET=1 GIT_TRACE=1 GIT_CURL_VERBOSE=1 git push
```

下面是 **upload** 数据：

```json
{
    "objects": [
        {
            "oid": "42e44ce1337ecd9bab99e78f42df5f73ae1a7e5bda505054e04cd4f36ed7bf21",
            "size": 2097152,
            "actions": {
                "upload": {
                    "href": "https://github-cloud.s3.amazonaws.com/alambic/media/180597487/42/e4/42e44ce1337ecd9bab99e78f42df5f73ae1a7e5bda505054e04cd4f36ed7bf21?actor_id=6904176",
                    "header": {
                        "Authorization": "AWS4-HMAC-SHA256 Credential=AKIAIMWPLRQEC4XCWWPA/20180628/us-east-1/s3/aws4_request,SignedHeaders=host;x-amz-content-sha256;x-amz-date,Signature=SignatureString",
                        "x-amz-content-sha256": "42e44ce1337ecd9bab99e78f42df5f73ae1a7e5bda505054e04cd4f36ed7bf21",
                        "x-amz-date": "20180628T032201Z"
                    },
                    "expires_at": "2018-06-28T03:37:01Z",
                    "expires_in": 900
                },
                "verify": {
                    "href": "https://lfs.github.com/lfstest/lfstest/objects/42e44ce1337ecd9bab99e78f42df5f73ae1a7e5bda505054e04cd4f36ed7bf21/verify",
                    "header": {
                        "Authorization": "RemoteAuth Token",
                        "Accept": "application/vnd.git-lfs+json"
                    }
                }
            }
        }
    ]
}
```

将对象存储到 S3 上，存在一个缺点，即当 **upload** 成功后，Github 是无法被动感知的，S3 目前并没有 Update-Callback 这样的功能，Git LFS 有一个 `verify` 字段，即通知服务器去验证 **upload** 是否成功。


## Bitbucket LFS 存储

Bitbucket 也是一个比较大的代码托管平台，Bitbucket 的 LFS 做的比较精致，下面有一篇 Tutorials 详细介绍了如何使用 LFS： [Git LFS Tutorials](https://www.atlassian.com/git/tutorials/git-lfs)

下面是 Bitbucket LFS 的管理界面。

![](https://wac-cdn.atlassian.com/dam/jcr:46218516-f4aa-490a-9afc-c36ca863c98f/09.png)

我们可以使用 Git 调试模式获得 Bitbucket 的一些细节，下面是 **download** 的数据：

```json
{
	"objects": [{
		"oid": "e29b4e1206ef04580f63cedd043834296fd56ad777d1903a0cab8b6178f6a6e0",
		"actions": {
			"download": {
				"header": {
					"X-Client-ID": "uuid-token",
					"Authorization": "Bearer BearerToken"
				},
				"href": "https://api.media.atlassian.com/file/6d4949ce-2442-424b-9875-983008b943d0/binary"
			}
		},
		"size": 1048576
	}]
}
```

从 JSON 数据中，我们可以知道 Bitbucket 并没有直接将对象存储到云服务对象存储上，并且 Bitbucket 使用了 OAuth 的验证方式。

Bitbucket 这种设计在实现精致的 LFS 管理功能时很有帮助，而 Github 将对象存储到 S3 上， 一来无法直接检验对象的 SHA256，另一方面无法检测对象的 `Mime`，也就无法做到像 Bitbucket 这样的效果。Bitbucket 可以查看 LFS 对象的 Mime(File Type)，上传时间，还可以通过删除比较旧的对象，从而降低 LFS 文件的配额占用。从这些方面来看 Bitbucket 更胜一筹。但也无法享受对象存储的好处了。


## Moses to LFSOSS

码云在去年开发 LFS 功能时使用的是和 Bitbucket 一样的机制，没有使用对象共享，即当 Sha256 一致时，并不支持共享，这无疑增加了服务器存储占用。

最近，随着需求的改变，我们要将 LFS 对象存储到 OSS 上，于是笔者也就开发了 LFSOSS 了，支持多种后端，只需要修改特定 URL 的生成即可支持。而不同的云存储的对象存储也有一些优缺点。

# 对象存储的对比

码云在支持 LFS for OSS 时，考察了几个对象存储。如 Azure Blob，Amazon S3，Aliyun OSS，Tencent COS。

|Object Storage|Shared Key|Content Verify|
|---|---|---|
|Azure Blob|Hmac Sha256|Content-MD5|
|Amazon S3|Hmac Sha256|Content-MD5, x-amz-content-sha256|
|Aliyun OSS|Hmac Sha1|Content-MD5, x-oss-callback|
|Tencent COS|Hmac Sha1|Content-MD5|

选择不同的云服务，不但需要考虑这些云服务的功能，还需要考虑到一些经济因素。我们可以看到在安全上，AWS S3 无疑是最好的，Azure 次之，其他的安全意识并不能算非常好，毕竟 MD5/SHA1 早已经被破解。如果考虑到对象存储服务器上使用 Hash 作为对象的索引，SHA1 无疑是不合适的，尽管 Aliyun OSS 并不能完美的切合 LFS，但由于国内的一些现状，码云仍旧基于 Aliyun 做出了一些设计上的让步，避免 LFS 在上传到对象存储时出现对象污染的情况。

回过头来看，Git LFS 更像是转为 AWS S3 定制的一样，S3 上传支持 `x-amz-content-sha256` 而 Git LFS 对象的 Hash 算法也是选择的 `SHA256`。

# 反思

对象存储的安全意识要跟上，避免平台的局限性而导致服务设计的妥协。

# 其他

## 共享密钥下载

共享密钥实际上就是将请求的一些数据与过期时间等合成一个 Slat，使用云平台生成的 AccessKeySecret 作为 Key，基于 Hmac 生成的签名，其中不同的平台支持不同的 Hmac 安全算法。大致的原理实际上可以用如下伪代码表示：

```
Slat=PlatfromMethod(ReqData,Expires,....)
Signature=Hmac(Secret,Slat)
```

