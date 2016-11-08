---
layout: post
title:  "Windows 下载功能的实现 - C++ 篇"
date:   2016-11-03 20:00:00
published: true
categories: windows
---

# 前言

笔者计划开发一个自用的包管理工具，需要支持下载功能，笔者尝试了多种 Windows 下载 API，这里分享出来。

## URLDownloadToFile
自 Internet Explorer 3.0 开始，Urlmon.dll 中开始提供 URLDownloadToFile，支持从远程服务器上下载文件到本地。
URLDownloadToFile 会先将文件下载到 IE 缓存目录，然后再复制到设置的输出目录，如果第二次下载，就省去了下
载时间。Urlmon 还提供了下载到缓存目录的函数 URLDownloadToCacheFile，正因为 URLDownloadToFile 先下载
到缓存目录，就会出现缓存问题，可以使用 Wininet 中的 DeleteUrlCacheEntry 删除缓存。

使用 URLDownloadToFile 下载文件，下面有个简单的例子：

{% highlight cpp %}
#include "stdafx.h"
#include <string>
#include <Urlmon.h>
#include <functional>
#pragma comment(lib,"Urlmon")


class DownloadStatus :public IBindStatusCallback {
public:
	ULONG presize{ 0 };
	STDMETHOD(OnStartBinding)(
		/* [in] */ DWORD dwReserved,
		/* [in] */ IBinding __RPC_FAR *pib)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(GetPriority)(
		/* [out] */ LONG __RPC_FAR *pnPriority)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(OnLowResource)(
		/* [in] */ DWORD reserved)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(OnProgress)(
		/* [in] */ ULONG ulProgress,
		/* [in] */ ULONG ulProgressMax,
		/* [in] */ ULONG ulStatusCode,
		/* [in] */ LPCWSTR wszStatusText);

	STDMETHOD(OnStopBinding)(
		/* [in] */ HRESULT hresult,
		/* [unique][in] */ LPCWSTR szError)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(GetBindInfo)(
		/* [out] */ DWORD __RPC_FAR *grfBINDF,
		/* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(OnDataAvailable)(
		/* [in] */ DWORD grfBSCF,
		/* [in] */ DWORD dwSize,
		/* [in] */ FORMATETC __RPC_FAR *pformatetc,
		/* [in] */ STGMEDIUM __RPC_FAR *pstgmed)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(OnObjectAvailable)(
		/* [in] */ REFIID riid,
		/* [iid_is][in] */ IUnknown __RPC_FAR *punk)
	{
		return E_NOTIMPL;
	}

	// IUnknown methods.  Note that IE never calls any of these methods, since
	// the caller owns the IBindStatusCallback interface, so the methods all
	// return zero/E_NOTIMPL.

	STDMETHOD_(ULONG, AddRef)()
	{
		return 0;
	}

	STDMETHOD_(ULONG, Release)()
	{
		return 0;
	}

	STDMETHOD(QueryInterface)(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return E_NOTIMPL;
	}
};

HRESULT DownloadStatus::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
	ULONG ulStatusCode, LPCWSTR wszStatusText)
{
	float ps = 0;
	if (ulProgressMax != 0) {
		ps = (float)ulProgress * 100 / ulProgressMax;
	}

	fprintf(stderr, "\rDownload %2.2f%%", ps);
	return S_OK;
}

bool DownloadFileWarp(const std::wstring &remoteFile, const std::wstring &localFile) {
	DownloadStatus ds;
	return URLDownloadToFileW(nullptr, remoteFile.c_str(), localFile.c_str(), 0, &ds) == S_OK;
}
{% endhighlight %}

在 ReactOS 中，URLDownloadToFile 是使用 WinINet 实现

## HttpClient

自 Windows 8 开始，微软推出了 Windows Runtime,Windows Runtime 基于 COM 实现，可以使用
C++/CX,C#,VB.Net,JavaScript 等，拥有存储，网络，设备，UI，媒体等等。但是如果要使用现代的标准 C++，
还是有一些麻烦。

著名 MSDN 专栏作家，Microsoft MVP，Kenny Kerr 近一两年致力于 WinRT 对现代 C++ 的支持,先推出了
[Modern C++ for the Windows Runtime](https://github.com/kennykerr/modern),最近又以微软官方的
名义推出了 [cppwinrt](https://github.com/microsoft/cppwinrt). 这些项目都是致力于现代 C++ 使用
Windows Runtime API。

在下载文件的时候，我们可以使用 HttpClient 下载文件，下面是一个简单的实例：

{% highlight cpp %}
#include "stdafx.h"
#include <Shlwapi.h>
#include <winrt/base.h>
#include <winrt/Windows.Web.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Filters.h>
#include <winrt/Windows.Storage.Streams.h>

#pragma comment(lib, "windowsapp")
#pragma comment(lib,"Shlwapi")

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Web::Http;
using namespace Windows::Web::Http::Filters;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;


//https://msdn.microsoft.com/en-us/library/windows/apps/xaml/windows.web.http.httpclient.aspx


IAsyncAction DownloadFileInternal(hstring_ref remoteFile,hstring_ref localFile) {
	Uri uri(remoteFile);
	HttpBaseProtocolFilter baseFilter;
	baseFilter.AllowAutoRedirect(true);
	HttpClient client(baseFilter);
	StorageFolder folder=KnownFolders::DocumentsLibrary();
	{
		auto file = co_await folder.CreateFileAsync(localFile);
		auto stream = co_await file.OpenAsync(FileAccessMode::ReadWrite);
		auto result = co_await client.GetAsync(uri);
		auto content = co_await result.Content().WriteToStreamAsync(stream);
	}
}

class RoInitializeWarp {
public:
	RoInitializeWarp() {
		initialize();
	}
	~RoInitializeWarp() {
		uninitialize();
	}
};

bool WinRTDownloadFile(const std::wstring &remoteFile, const std::wstring &localFile) {
	initialize();
	try {
		DownloadFileInternal(remoteFile.c_str(), localFile.c_str()).get();
	}
	catch (hresult_error const  &e) {
		printf("Error: %ls\n", e.message().c_str());
		uninitialize();
		return false;
	}
	catch (...) {
		return false;
	}
	uninitialize();
	return true;
}
{% endhighlight %}

完全使用 cppwinrt 还是有一些问题，比如，StorageFolder 的目录权限问题，然后就是重定向，如果要解决重定向，还要添加一些
复杂的代码，比如 npm.taobao.org 使用 HTTPS 下载重定向 HTTP 就会触发异常，然后异常捕获还是有点麻烦。

当然我是非常期待 cppwinrt 的进一步改进。

Kenny Kerr 在 [MSDN](https://msdn.microsoft.com/magazine/mt149362?author=Kenny%20Kerr) 上发布了
很多优秀的文章，Windows 平台上原生程序开发人员可以去查看此链接。

## Background Intelligent Transfer Service

[BITS](https://msdn.microsoft.com/en-us/library/windows/desktop/aa362708(v=vs.85).aspx) - Background Intelligent Transfer Service 
(后台智能传输服务) 是 Windows 的一个重要功能.

笔者在开发 [Clangbuilder](https://github.com/fstudio/clangbuilder) 时,需要使用 PowerShell 下载软件使用的 cmdlet 是 Start-BitsTransfer,
Start-BitsTransfer 实际上使用的是 Windows 的 BITS 服务,而 Windows 更新功能也是使用的 BITS, Chrome 同步扩展也是使用的 BITS.

IBackgroundCopyJob

## WinHTTP

WinHTTP 实现下载功能还算比较简单，通常就是发送 HTTP GET 请求，然后创建一个空文件，从 HTTP 响应中
读取返回包体，写入到文件中，直至读取完毕。

### HTTP2 支持

自 Windows 10 1607 起，WinHTTP 允许开发者通过 WinHttpSetOption 开启 HTTP2 支持。

## Wininet

### HTTP2 支持

自 Windows 10 1507 起， Wininet 便允许开发者通过设置参数开启 HTTP2 支持


# 如何选择

除了以上的解决方案外，在 Windows 系统中实现下载功能还有很多其他的选择，比如试用 libcurl，Poco Net，cpp-netlib，
cpprestsdk，QNetworkRequest 等等。就 HTTP URL 的下载来说，如果不想使用现成的 HTTP 库实现下载，还可以自己实现
HTTP 库，可以使用原生的 socket，也可以使用 Boost.Asio，libuv，libev，ACE 来实现 HTTP 客户端。
