---
layout: post
title:  "Windows 下载功能的实现 - C++ 篇"
date:   2016-11-03 20:00:00
published: true
categories: windows
---

# 前言

笔者计划开发一个自用的包管理工具，需要支持下载功能，

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

Windows Runtime -cppwinrt

## IBackgroundCopyJob (BITS)

[BITS](https://msdn.microsoft.com/en-us/library/windows/desktop/aa362708(v=vs.85).aspx) - Background Intelligent Transfer Service 
(后台智能传输服务) 是 Windows 的一个重要功能.
笔者在开发 [Clangbuilder](https://github.com/fstudio/clangbuilder) 时,需要使用 PowerShell 下载软件使用的 cmdlet 是 Start-BitsTransfer,
Start-BitsTransfer 实际上使用的是 Windows 的 BITS 服务,而 Windows 更新功能也是使用的 BITS, Chrome 同步扩展也是使用的 BITS.

## WinHTTP

## Wininet
