---
layout: post
title:  "构建恰当的 Git SSH Server"
date:   2019-03-15 18:00:00
published: true
categories: git
---

## 前言

相对于 HTTP(HTTPS) 协议，Git 在使用 SSH 协议操作远程存储库时，因为省去了输入用户名密码的环节，往往要更方便一些，并且，在 Gitlab 这样的代码托管服务中，SSH 在时长上更具优势，早期 Gitlab 使用了 Grack 提供 Git HTTP 访问支持，由于 Unicron+Grack 固定数目多线程同步模型导致服务器上的 HTTP 超时不得不设置非常小，而 SSH fork 多线程同步模型反而能够支持更大的访问时长。实际情况中，Gitee 平台里 Git 接入的最大份额也是 SSH。构建恰当的 Git SSH Server 对于整个 Gitee 平台也就非常重要。

## Git SSH 基于 OpenSSH

在 OpenSSH 的 [`sshd_config`](https://linux.die.net/man/5/sshd_config) 中有一些关键配置项 `AuthorizedKeysFile`  (通常为 `.ssh/authorized_keys`)  `AuthorizedKeysCommand`。Git SSH 的授权也是由这些特性实现的，具体的实现可以参考 [gitlab-shell](https://github.com/gitlabhq/gitlab-shell)

>git pull/push over ssh -> gitlab-shell -> API call to gitlab-rails (Authorization) -> accept or decline -> execute git command


早在 2015 年，Gitee 由 NFS 架构转向为目前的分布式架构之初，我们面临了一个难题，如何使得 Git SSH 的流量分发到不同的存储后端，最初的想法是还是使用 OpenSSH，基于端口转发，但端口转发的流量在内网中仍然需要重复加密解密，这非常消耗 CPU 资源。并且验证流程不是很清晰，需要启动命令完成验证，这对于体量较大的代码托管平台来说是不能忍受的。

## Gitee 的 SSH Server

在 Gitee 分布式架构演进的过程中，我们开发了 git-srv，最初实现了 git-upload-pack，git-receive-pack 这样的命令，这些命令会取代前端服务器上的 git-upload-pack/git-receive-pack，当被打开时，会查询存储服务器路由并与存储服务器上的 git-srv 通讯，最终在存储服务器上运行 git-upload-pack/git-receive-pack。这种方案与目前 Gitlab gitlay 的架构类似。但此方案仍然有很多不足，每来一个请求，则需要启动一个子进程。子进程还要经历一些复杂的操作才能与后端建立连接，这并不高效。

因此我们直接将 兼容的 git-receive-pack/git-upload-pack 这一中间组件拿掉，实现一个 SSH 服务与 Git-SRV 通信。于是我们基于 libssh 开发了 Basalt SSHD。由于 libssh 基本上是一个同步阻塞的库，（libssh2 没有服务端 API）, sshd 也就只能使用 pre-fork 模型，这与 OpenSSH 类似。

但 Gitee 的 SSH Server 并不需要 fork 子进程执行，pre-fork 模型也就显得非常鸡肋。后来我们又使用 Golang [gliderlabs/ssh](https://github.com/gliderlabs/ssh) 实现了一个 SSH Server。目前 Gitee 运行的 SSH 服务有两个版本，一个是基于 libssh 的 Basalt v1，一个是基于 Golang 的 Basalt v2。要查看 Gitee 使用的 SSH Server 版本，可以运行 `ssh -Tv git@gitee.com`。

```shell
# libssh
debug1: Remote protocol version 2.0, remote software version Basalt-1.2
# Golang
debug1: Remote protocol version 2.0, remote software version Basalt-2.5.2

```

## SSH Server 的选型比较

SSH 协议指的是 [安全 Shell (Secure Shell)](https://en.wikipedia.org/wiki/Secure_Shell) 协议，SSH 用于替代不安全的 Telnet 协议和 Shell 协议，所有的流量均通过加密传输，协议的主要版本有 SSH-1，SSH-2。SSH 规范如下：

* RFC 4250, The Secure Shell (SSH) Protocol Assigned Numbers
* RFC 4251, The Secure Shell (SSH) Protocol Architecture
* RFC 4252, The Secure Shell (SSH) Authentication Protocol
* RFC 4253, The Secure Shell (SSH) Transport Layer Protocol
* RFC 4254, The Secure Shell (SSH) Connection Protocol
* RFC 4255, Using DNS to Securely Publish Secure Shell (SSH) Key Fingerprints
* RFC 4256, Generic Message Exchange Authentication for the Secure Shell Protocol (SSH)
* RFC 4335, The Secure Shell (SSH) Session Channel Break Extension
* RFC 4344, The Secure Shell (SSH) Transport Layer Encryption Modes
* RFC 4345, Improved Arcfour Modes for the Secure Shell (SSH) Transport Layer Protocol

修改和扩展：
* RFC 4419, Diffie-Hellman Group Exchange for the Secure Shell (SSH) Transport Layer Protocol (March 2006)
* RFC 4432, RSA Key Exchange for the Secure Shell (SSH) Transport Layer Protocol (March 2006)
* RFC 4462, Generic Security Service Application Program Interface (GSS-API) Authentication and Key Exchange for the Secure Shell (SSH) Protocol (May 2006)
* RFC 4716, The Secure Shell (SSH) Public Key File Format (November 2006)
* RFC 4819: Secure Shell Public Key Subsystem (March 2007)
* RFC 5647: AES Galois Counter Mode for the Secure Shell Transport Layer Protocol (August 2009)
* RFC 5656, Elliptic Curve Algorithm Integration in the Secure Shell Transport Layer (December 2009)
* RFC 6187: X.509v3 Certificates for Secure Shell Authentication (March 2011)
* RFC 6239: Suite B Cryptographic Suites for Secure Shell (SSH) (May 2011)
* RFC 6594: Use of the SHA-256 Algorithm with RSA, Digital Signature Algorithm (DSA), and Elliptic Curve DSA (ECDSA) in SSHFP Resource Records
* RFC 6668, SHA-2 Data Integrity Verification for the Secure Shell (SSH) Transport Layer Protocol (July 2012)
* RFC 7479: [[Ed25519]] SSHFP Resource Records

虽然 SSH 服务器和客户端比较多<sup>1</sup>，但基于现成的 SSH 库开发 SSH Server 的选择并不多，在 C 或者 C++ 中，基本上只有 libssh 可选，libssh 被用来实现 Git SSH Server 的有 Gitee 和 Github。但目前 Github 的 SSH Server 标识变成了 `babeld-9d924d26`，目前是否切换到其他实现不得而知。

Libssh 的缺点也比较明显，异步非阻塞支持比较弱，另外开发者比较少，长期只有 Andreas Schneider 一个人提交。并且 Bug 比较多，Gitee 线上的 Libssh 也是需要应用特定的 Patch 才行。维护 SSHD 需要经常登陆 [https://bugs.libssh.org/](https://bugs.libssh.org/) 查看 libssh 动态。

其他语言可以使用 libssh 的绑定来开发 SSH 服务器，也可以使用对应语言的 SSH 库实现，下面是一些常见的语言与对应的 SSH 库实现：

|语言|库|特点|
|---|---|---|
|Go|[crypto/ssh](https://github.com/golang/crypto/tree/master/ssh)|异步，Golang 官方支持，使用广泛|
|Java|[Apache Mina SSHD](https://github.com/apache/mina-sshd)|高性能，功能完整|
|Rust|[Thrussh](https://pijul.org/thrussh/)|异步 IO，缓冲区安全|
|C#|[FxSsh](https://github.com/Aimeast/FxSsh)|主要用于 GitCandy，功能有限|

基于 Golang `crypto/ssh` 实现的 SSH Server 比较多，而基于 Golang 的开源代码托管平台 Gogs/Gitea 均使用了 `crypto/ssh`。

Apache Mina SSHD 在 Java 社区里被广泛使用，比如 Jenkins CI 就有个 [SSH Server](https://mvnrepository.com/artifact/org.jenkins-ci.modules/sshd) 模块。基于 Scala (JVM) 的代码托管平台 [https://github.com/gitbucket/gitbucket](https://github.com/gitbucket/gitbucket) 使用了  Apache Mina SSHD 实现 Git SSH Server。

而 Thrussh 则主要被用于基于 Rust 的版本控制工具 [pijul](https://pijul.org/) 提高 SSH 协议支持。

还有一些商业上的 SSH Server 库，使用较少。

## Git SSH Server 技术细节

实现 Git SSH Server 与实现常规的 SSH Server 的区别主要是，Git SSH Server 只是一个严格的子集，session 阶段只需要实现 `env` `exec` 两种请求。不需要实现 `sftp` 和端口转发。

因此简单的实现一个 Git SSH Server 即可如下：

```go
package main

import (
	"errors"
	"fmt"
	"log"
	"net"
	"os"
	"os/exec"
	"os/user"
	"path"
	"path/filepath"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/gliderlabs/ssh"
)

// Error
var (
	ErrPublicKeyNotFound  = errors.New("Public key not found")
	ErrEncodeHandshake    = errors.New("Handshake encode error")
	ErrRepositoryNotFound = errors.New("Repository Not Found")
	ErrUnreachablePath    = errors.New("Path is unreachable")
)

// StrSplitSkipEmpty skip empty string suggestcap is suggest cap
func StrSplitSkipEmpty(s string, sep byte, suggestcap int) []string {
	sv := make([]string, 0, suggestcap)
	var first, i int
	for ; i < len(s); i++ {
		if s[i] != sep {
			continue
		}
		if first != i {
			sv = append(sv, s[first:i])
		}
		first = i + 1
	}
	if first < len(s) {
		sv = append(sv, s[first:])
	}
	return sv
}

// RepoPathClean todo
func RepoPathClean(p string) (string, error) {
	xp := path.Clean(p)
	pv := StrSplitSkipEmpty(xp, '/', 4)
	if len(pv) != 2 || len(pv[0]) < 2 {
		return "", ErrUnreachablePath
	}
	return pv[0] + "/" + strings.TrimSuffix(pv[1], ".git"), nil
}

// GetSessionEnv env
func GetSessionEnv(s ssh.Session, key string) string {
	prefix := key + "="
	kl := len(prefix)
	for _, str := range s.Environ() {
		if kl >= len(str) {
			continue
		}
		if strings.HasPrefix(str, prefix) {
			return str[kl:]
		}
	}
	return ""
}

// RepoPathStat stat repo
func RepoPathStat(repo string) (string, error) {
	r := filepath.Join("/home/git/root", repo[0:2], repo)
	if !strings.HasSuffix(r, ".git") {
		r += ".git"
	}
	if _, err := os.Stat(r); err != nil {
		return "", ErrRepositoryNotFound
	}
	return r, nil
}

// GitCommand exe git-*
func GitCommand(s ssh.Session, scmd string, repo string) int {
	if s.User() != "git" {
		// filter unallowed user
		fmt.Fprintf(s.Stderr(), "Permission denied, user: \x1b[31m'%s'\x1b[0m\n", s.User())
		return 127
	}

	pwn, err := RepoPathClean(repo)
	if err != nil {
		fmt.Fprintf(s.Stderr(), "Permission denied: \x1b[31m%v\x1b[0m\n", err)
		return 127
	}
	// TODO AUTH
	diskrepo, err := RepoPathStat(pwn)
	if err != nil {
		fmt.Fprintf(s.Stderr(), "Access deined: \x1b[31m%v\x1b[0m\n", err)
		return 127
	}
	version := GetSessionEnv(s, "GIT_PROTOCOL")
	cmd := exec.Command("git", scmd, diskrepo)
	ProcAttr(cmd)
	cmd.Env = append(environ, "GL_ID=key-"+strconv.FormatInt(1111, 10)) /// to set envid
	if len(version) > 0 {
		cmd.Env = append(environ, "GIT_PROTOCOL="+version) /// to set envid
	}
	cmd.Env = append(environ, s.Environ()...) /// include other
	cmd.Stderr = s.Stderr()
	cmd.Stdin = s
	cmd.Stdout = s
	// FIXME: check timeout
	if err = cmd.Start(); err != nil {
		fmt.Fprintln(s.Stderr(), "Server internal error, unable to run git-", scmd)
		log.Printf("Server internal error, unable to run git-%s error: %v", scmd, err)
		return 127
	}
	var exitcode int
	exitChain := make(chan bool, 1)
	go func() {
		if err = cmd.Wait(); err != nil {
			exitcode = 127
		}
		exitChain <- true
	}()
	for {
		select {
		case <-exitChain:
			return exitcode
		case <-s.Context().Done():
			cmd.Process.Kill()
			return 0
		}
	}
}

func sshAuth(ctx ssh.Context, key ssh.PublicKey) bool {
	/// TODO auth

	return true
}

func sessionHandler(s ssh.Session) {
	cmd := s.Command()
	if len(cmd) < 2 || !strings.HasPrefix(cmd[0], "git-") {
		s.Stderr().Write([]byte("bad command " + cmd[0] + "\n"))
		return
	}
	GitCommand(s, strings.TrimPrefix(cmd[0], "git-"), cmd[1])
}

// Userid get
var Userid uint32

// Groupid in
var Groupid uint32

// NeedSetsid is
var NeedSetsid bool

// ProcAttr is
func ProcAttr(cmd *exec.Cmd) {
	if NeedSetsid {
		cmd.SysProcAttr = &syscall.SysProcAttr{
			Credential: &syscall.Credential{
				Uid: Userid,
				Gid: Groupid,
			},
			Setsid: true,
		}
	}
}

//InitializeUtils ini
func InitializeUtils() error {
	if syscall.Getuid() != 0 {
		NeedSetsid = false
		Userid = uint32(syscall.Getuid())
		Groupid = uint32(syscall.Getgid())
		return nil
	}
	//
	user, err := user.Lookup("git")
	if err != nil {
		return err
	}
	xid, err := strconv.ParseUint(user.Uid, 10, 32)
	if err != nil {
		return err
	}
	environ = os.Environ()
	for i, s := range environ {
		if strings.HasPrefix(s, "HOME=") {
			environ[i] = fmt.Sprintf("HOME=%s", user.HomeDir)
		}
	}

	Userid = uint32(xid)
	zid, err := strconv.ParseUint(user.Gid, 10, 32)
	if err != nil {
		return err
	}
	Groupid = uint32(zid)
	NeedSetsid = true
	return nil
}

var environ []string

func main() {
	srv := &ssh.Server{
		Handler:          sessionHandler,
		PublicKeyHandler: sshAuth,
		MaxTimeout:       time.Second * 180,
		IdleTimeout:      time.Second * 3600,
		Version:          "Basalt-2.0-Single",
	}
	InitializeUtils()
	log.Println("starting ssh server on port 2222...")
	ln, err := net.Listen("tcp", ":2222")
	if err != nil {
		return
	}
	srv.Serve(ln)
}

```

这个项目非常简单，但足以说明实现 Git SSH Server 的细节。

要生成 SSH Key 也非常简单，下面是 ECDSA RSA ED25519 SSH Key 生成示例：

```go
package main

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"encoding/pem"
	"io/ioutil"
	mrand "math/rand"

	"golang.org/x/crypto/ed25519"
	"golang.org/x/crypto/ssh"
)

// Thanks https://github.com/mikesmitty/edkey

// MarshalED25519PrivateKey todo
/* Writes ed25519 private keys into the new OpenSSH private key format.
I have no idea why this isn't implemented anywhere yet, you can do seemingly
everything except write it to disk in the OpenSSH private key format. */
func MarshalED25519PrivateKey(key ed25519.PrivateKey) []byte {
	// Add our key header (followed by a null byte)
	magic := append([]byte("openssh-key-v1"), 0)

	var w struct {
		CipherName   string
		KdfName      string
		KdfOpts      string
		NumKeys      uint32
		PubKey       []byte
		PrivKeyBlock []byte
	}

	// Fill out the private key fields
	pk1 := struct {
		Check1  uint32
		Check2  uint32
		Keytype string
		Pub     []byte
		Priv    []byte
		Comment string
		Pad     []byte `ssh:"rest"`
	}{}

	// Set our check ints
	ci := mrand.Uint32()
	pk1.Check1 = ci
	pk1.Check2 = ci

	// Set our key type
	pk1.Keytype = ssh.KeyAlgoED25519

	// Add the pubkey to the optionally-encrypted block
	pk, ok := key.Public().(ed25519.PublicKey)
	if !ok {
		//fmt.Fprintln(os.Stderr, "ed25519.PublicKey type assertion failed on an ed25519 public key. This should never ever happen.")
		return nil
	}
	pubKey := []byte(pk)
	pk1.Pub = pubKey

	// Add our private key
	pk1.Priv = []byte(key)

	// Might be useful to put something in here at some point
	pk1.Comment = ""

	// Add some padding to match the encryption block size within PrivKeyBlock (without Pad field)
	// 8 doesn't match the documentation, but that's what ssh-keygen uses for unencrypted keys. *shrug*
	bs := 8
	blockLen := len(ssh.Marshal(pk1))
	padLen := (bs - (blockLen % bs)) % bs
	pk1.Pad = make([]byte, padLen)

	// Padding is a sequence of bytes like: 1, 2, 3...
	for i := 0; i < padLen; i++ {
		pk1.Pad[i] = byte(i + 1)
	}

	// Generate the pubkey prefix "\0\0\0\nssh-ed25519\0\0\0 "
	prefix := []byte{0x0, 0x0, 0x0, 0x0b}
	prefix = append(prefix, []byte(ssh.KeyAlgoED25519)...)
	prefix = append(prefix, []byte{0x0, 0x0, 0x0, 0x20}...)

	// Only going to support unencrypted keys for now
	w.CipherName = "none"
	w.KdfName = "none"
	w.KdfOpts = ""
	w.NumKeys = 1
	w.PubKey = append(prefix, pubKey...)
	w.PrivKeyBlock = ssh.Marshal(pk1)

	magic = append(magic, ssh.Marshal(w)...)

	return magic
}

// AuthenticationKeyGenerationRSA RSA
func AuthenticationKeyGenerationRSA(keyPath string) error {
	privKey, err := rsa.GenerateKey(rand.Reader, 2048)
	if err != nil {
		return err
	}

	// generate public key
	publicKey, err := ssh.NewPublicKey(&privKey.PublicKey)
	if err != nil {
		return err
	}

	pemKey := &pem.Block{
		Type:  "RSA PRIVATE KEY",
		Bytes: x509.MarshalPKCS1PrivateKey(privKey),
	}
	privateKey := pem.EncodeToMemory(pemKey)
	authorizedKey := ssh.MarshalAuthorizedKey(publicKey)
	if err = ioutil.WriteFile(keyPath, privateKey, 0600); err != nil {
		return err
	}
	return ioutil.WriteFile(keyPath+".pub", authorizedKey, 0644)
}

// AuthenticationKeyGenerationECDSA ecdsa
// by default we gen 256
func AuthenticationKeyGenerationECDSA(keyPath string) error {
	privKey, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return err
	}
	publicKey, err := ssh.NewPublicKey(&privKey.PublicKey)
	if err != nil {
		return err
	}
	pembyte, err := x509.MarshalECPrivateKey(privKey)
	if err != nil {
		return err
	}
	// EC PRIVATE KEY
	pemKey := &pem.Block{
		Type:  "EC PRIVATE KEY",
		Bytes: pembyte,
	}
	privateKey := pem.EncodeToMemory(pemKey)
	authorizedKey := ssh.MarshalAuthorizedKey(publicKey)
	if err = ioutil.WriteFile(keyPath, privateKey, 0600); err != nil {
		return err
	}
	return ioutil.WriteFile(keyPath+".pub", authorizedKey, 0644)
}

// AuthenticationKeyGenerationED25519 ED25519 keygen
func AuthenticationKeyGenerationED25519(keyPath string) error {
	pubKey, privKey, err := ed25519.GenerateKey(rand.Reader)
	if err != nil {
		return err
	}
	publicKey, err := ssh.NewPublicKey(pubKey)
	if err != nil {
		return err
	}

	pemKey := &pem.Block{
		Type:  "OPENSSH PRIVATE KEY",
		Bytes: MarshalED25519PrivateKey(privKey),
	}
	privateKey := pem.EncodeToMemory(pemKey)
	authorizedKey := ssh.MarshalAuthorizedKey(publicKey)

	if err = ioutil.WriteFile(keyPath, privateKey, 0600); err != nil {
		return err
	}
	return ioutil.WriteFile(keyPath+".pub", authorizedKey, 0644)
}

```

## SVN+SSH 实现

Gitee 目前提供  `svn+ssh` 协议访问，而此协议实际上是通过在服务器上运行 `svnserve -t`，从而读写存储库数据，而 Gitee 的 svn 功能基于 git-as-svn ，我们只需在请求命令为 `svnserve` 时，将流量代理转发到 git-as-svn 上即可。在 Gitee 上通过 ssh+svn 访问存储库需要用户对存储库具有写权限，这是特意而为之。



## 附录

1.  [Comparison of SSH clients](https://en.wikipedia.org/wiki/Comparison_of_SSH_clients) , [Comparison of SSH servers](https://en.wikipedia.org/wiki/Comparison_of_SSH_servers)