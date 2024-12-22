+++
title = "Windows 下载功能的实现 - C++ 篇"
date = "2016-11-03T20:00:00+08:00"
categories = "windows"
+++

# 前言

笔者计划开发一个自用的包管理工具，需要支持下载功能，笔者尝试了多种 Windows 下载 API，这里分享出来。

## URLDownloadToFile
自 Internet Explorer 3.0 开始，Urlmon.dll 中开始提供 URLDownloadToFile，支持从远程服务器上下载文件到本地。
URLDownloadToFile 会先将文件下载到 IE 缓存目录，然后再复制到设置的输出目录，如果第二次下载，就省去了下载时间。
Urlmon 还提供了下载到缓存目录的函数 URLDownloadToCacheFile，正因为 URLDownloadToFile 先下载到缓存目录，
就会出现缓存问题，可以使用 Wininet 中的 DeleteUrlCacheEntry 删除缓存。

使用 URLDownloadToFile 下载文件，下面有个简单的例子：

```cpp
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
```

在 ReactOS 中，URLDownloadToFile 是使用 WinINet 实现

## HttpClient

自 Windows 8 开始，微软推出了 Windows Runtime,Windows Runtime 基于 COM 实现，可以使用 C++/CX,C#,VB.Net,JavaScript 等，
拥有存储，网络，设备，UI，媒体等等。但是如果要使用现代的标准 C++，还是有一些麻烦。

著名 MSDN 专栏作家，Microsoft MVP，Kenny Kerr 近一两年致力于 WinRT 对现代 C++ 的支持,先推出了
[Modern C++ for the Windows Runtime](https://github.com/kennykerr/modern),最近又以微软官方的名义推出了
[cppwinrt](https://github.com/microsoft/cppwinrt). 这些项目都是致力于现代 C++ 使用 Windows Runtime API。

在下载文件的时候，我们可以使用 HttpClient 下载文件，下面是一个简单的实例：

```cpp
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
```

完全使用 cppwinrt 还是有一些问题，比如，StorageFolder 的目录权限问题，然后就是重定向，如果要解决重定向，
还要添加一些复杂的代码，比如 npm.taobao.org 使用 HTTPS 下载重定向 HTTP 就会触发异常，然后异常捕获还是有点麻烦。

当然我是非常期待 cppwinrt 的进一步改进。

Kenny Kerr 在 [MSDN](https://msdn.microsoft.com/magazine/mt149362?author=Kenny%20Kerr) 上发布了很多优秀的文章，
Windows 平台上原生程序开发人员可以去查看此链接。

## Background Intelligent Transfer Service

[BITS](https://msdn.microsoft.com/en-us/library/windows/desktop/aa362708(v=vs.85).aspx) - Background Intelligent Transfer Service
(后台智能传输服务) 是 Windows 的一个重要功能.

笔者在开发 [Clangbuilder](https://github.com/fstudio/clangbuilder) 时,需要使用 PowerShell 下载软件使用的 cmdlet 是 Start-BitsTransfer,
Start-BitsTransfer 实际上使用的是 Windows 的 BITS 服务,而 Windows 更新功能也是使用的 BITS, Chrome 同步扩展也是使用的 BITS.

IBackgroundCopyJob

## WinHTTP

WinHTTP 实现下载功能还算比较简单，通常就是发送 HTTP GET 请求，然后创建一个空文件，从 HTTP 响应中读取返回包体，
写入到文件中，直至读取完毕。如果要下载超过 4GB 的文件，那么在获取 HTTP 头部 Content-Length 时需要获取字符串，而不是整数。

```c++
#include "stdafx.h"
#include <Shlwapi.h>
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "console.h"

#pragma comment(lib,"WinHTTP")

#define MinWarp(a,b) ((b<a)?b:a)


struct RequestURL {
	int nPort;
	int nScheme;
	std::wstring scheme;
	std::wstring host;
	std::wstring path;
	std::wstring username;
	std::wstring password;
	std::wstring extrainfo;
	bool Parse(const std::wstring &url) {
		URL_COMPONENTS urlComp;
		DWORD dwUrlLen = 0;
		ZeroMemory(&urlComp, sizeof(urlComp));
		urlComp.dwStructSize = sizeof(urlComp);
		urlComp.dwSchemeLength = (DWORD)-1;
		urlComp.dwHostNameLength = (DWORD)-1;
		urlComp.dwUrlPathLength = (DWORD)-1;
		urlComp.dwExtraInfoLength = (DWORD)-1;

		if (!WinHttpCrackUrl(url.data(), dwUrlLen, 0, &urlComp)) {
			return false;
		}
		scheme.assign(urlComp.lpszScheme, urlComp.dwSchemeLength);
		host.assign(urlComp.lpszHostName, urlComp.dwHostNameLength);
		path.assign(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
		nPort = urlComp.nPort;
		nScheme = urlComp.nScheme;
		if (urlComp.lpszUserName) {
			username.assign(urlComp.lpszUserName, urlComp.dwUserNameLength);
		}
		if (urlComp.lpszPassword) {
			password.assign(urlComp.lpszPassword, urlComp.dwPasswordLength);
		}
		if (urlComp.lpszExtraInfo) {
			extrainfo.assign(urlComp.lpszExtraInfo,
				urlComp.dwExtraInfoLength);
		}
		return true;
	}
};


#define DEFAULT_USERAGENT L"WindowsGet/1.0"

class InternetObject {
public:
	InternetObject(HINTERNET hInternet):hInternet_(hInternet) {
	}
	operator HINTERNET() {
		return hInternet_;
	}
	operator bool() {
		return hInternet_ != nullptr;
	}
	InternetObject() {
		if (hInternet_) {
			WinHttpCloseHandle(hInternet_);
		}
	}
private:
	HINTERNET hInternet_;
};

bool DownloadFileUseWinHTTP(const std::wstring &url, const std::wstring &localFile,ProgressCallback *callback) {
	RequestURL zurl;
	if (!zurl.Parse(url)) {
		BaseErrorMessagePrint(L"Wrong URL: %s\n",url.c_str());
		return false;
	}
	InternetObject hInternet =
		WinHttpOpen(DEFAULT_USERAGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hInternet) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"WinHttpOpen(): %s", err.message());
		return false;
	}
	//	WinHttpSetOption(hInternet, WINHTTP_OPTION_REDIRECT_POLICY, &dwOption,sizeof(DWORD));
	/// https://msdn.microsoft.com/en-us/library/windows/desktop/aa384066(v=vs.85).aspx
	/// WINHTTP_PROTOCOL_FLAG_HTTP2
	DWORD dwOption = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
	if (!WinHttpSetOption(hInternet, WINHTTP_OPTION_REDIRECT_POLICY,
		&dwOption, sizeof(DWORD))) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"WINHTTP_OPTION_REDIRECT_POLICY: %s", err.message());
	}
	dwOption = WINHTTP_PROTOCOL_FLAG_HTTP2;
	if (!WinHttpSetOption(hInternet, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &dwOption, sizeof(dwOption))) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL: %s", err.message());
	}
	InternetObject hConnect = WinHttpConnect(hInternet, zurl.host.c_str(),
		(INTERNET_PORT)zurl.nPort, 0);
	if (!hConnect) {
		BaseErrorMessagePrint(L"Server unable to connect: %s", zurl.host.c_str());
		return false;
	}
	DWORD dwOpenRequestFlag =
		(zurl.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
	InternetObject hRequest = WinHttpOpenRequest(
		hConnect, L"GET", zurl.path.c_str(), nullptr, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, dwOpenRequestFlag);
	if (!hRequest) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"Open Request failed: %s", err.message());
		return false;
	}
	if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"Send Request failed: %s", err.message());
		return false;
	}
	if (WinHttpReceiveResponse(hRequest, NULL) == FALSE) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"Receive Response failed: %s", err.message());
		return false;
	}
	DWORD dwStatusCode = 0;
	DWORD dwXsize = sizeof(dwStatusCode);
	if (!WinHttpQueryHeaders(hRequest,
		WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwXsize,
		WINHTTP_NO_HEADER_INDEX)) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"WinHttpQueryHeaders failed: %s", err.message());
		return false;
	}
	if (dwStatusCode != 200 && dwStatusCode != 201) {
		BaseErrorMessagePrint(L"Reponse status code: %d\n",dwStatusCode);
		return false;
	}
	uint64_t dwContentLength=0;
	wchar_t conlen[36];
	dwXsize = sizeof(conlen);
	if (WinHttpQueryHeaders(hRequest,
		WINHTTP_QUERY_CONTENT_LENGTH,
		WINHTTP_HEADER_NAME_BY_INDEX,conlen, &dwXsize,
		WINHTTP_NO_HEADER_INDEX)) {
	}
	wchar_t *cx = nullptr;
	dwContentLength=wcstoull(conlen, &cx, 10);
	wprintf(L"File size: %lld\n", dwContentLength);
	std::wstring tmp = localFile + L".part";
	HANDLE hFile =
		CreateFileW(tmp.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	///
	uint64_t total = 0;
	DWORD dwSize = 0;
	char fixedsizebuf[16384];
	///
	if (callback) {
		callback->impl(0, callback->userdata);
	}
	do {
		// Check for available data.
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
			break;
		}
		total += dwSize;
		if (dwContentLength > 0) {
			if (callback) {
				callback->impl(total * 100 / dwContentLength, callback->userdata);
			}
		}
		auto dwSizeN = dwSize;
		while (dwSizeN > 0) {
			DWORD dwDownloaded = 0;
			if (!WinHttpReadData(hRequest, (LPVOID)fixedsizebuf, MinWarp(sizeof(fixedsizebuf), dwSizeN), &dwDownloaded)) {
				break;
			}
			dwSizeN = dwSizeN - dwDownloaded;
			DWORD wmWritten;
			WriteFile(hFile, fixedsizebuf, dwSize, &wmWritten, NULL);
		}
	} while (dwSize > 0);
	if (callback) {
		callback->impl(100, callback->userdata);
	}
	std::wstring npath = localFile;
	int i = 1;
	while (PathFileExistsW(npath.c_str())) {
		auto n = localFile.find_last_of('.');
		if (n != std::wstring::npos) {
			npath = localFile.substr(0, n) + L"(";
			npath += std::to_wstring(i);
			npath += L")";
			npath += localFile.substr(n);
		}
		else {
			npath = localFile + L"(" + std::to_wstring(i) + L")";
		}
		i++;
	}
	CloseHandle(hFile);
	MoveFileExW(tmp.c_str(), npath.c_str(), MOVEFILE_COPY_ALLOWED);
	return true;
}
```

### HTTP2 支持

自 Windows 10 1607 起，WinHTTP 允许开发者通过 WinHttpSetOption 开启 HTTP2 支持。
上述源码中就有关于 HTTP2.0 的设置。

## Wininet

Wininet 流程和 WinHTTP 类似，下面是实现代码：

```c++
#include "stdafx.h"
#include <Windows.h>
#include <WinInet.h>
#include <Shlwapi.h>
#include "console.h"

#pragma comment(lib, "WinInet.lib")
//https://msdn.microsoft.com/en-us/library/windows/desktop/aa385328(v=vs.85).aspx
//INTERNET_OPTION_ENABLE_HTTP_PROTOCOL
//HTTP_PROTOCOL_FLAG_HTTP2
struct WinINetRequestURL {
	int nPort=0;
	int nScheme=0;
	std::wstring scheme;
	std::wstring host;
	std::wstring path;
	std::wstring username;
	std::wstring password;
	std::wstring extrainfo;
	bool Parse(const std::wstring &url) {
		URL_COMPONENTS urlComp;
		DWORD dwUrlLen = 0;
		ZeroMemory(&urlComp, sizeof(urlComp));
		urlComp.dwStructSize = sizeof(urlComp);
		urlComp.dwSchemeLength = (DWORD)-1;
		urlComp.dwHostNameLength = (DWORD)-1;
		urlComp.dwUrlPathLength = (DWORD)-1;
		urlComp.dwExtraInfoLength = (DWORD)-1;

		if (!InternetCrackUrlW(url.data(), dwUrlLen, 0, &urlComp)) {
			return false;
		}
		scheme.assign(urlComp.lpszScheme, urlComp.dwSchemeLength);
		host.assign(urlComp.lpszHostName, urlComp.dwHostNameLength);
		path.assign(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
		nPort = urlComp.nPort;
		nScheme = urlComp.nScheme;
		if (urlComp.lpszUserName) {
			username.assign(urlComp.lpszUserName, urlComp.dwUserNameLength);
		}
		if (urlComp.lpszPassword) {
			password.assign(urlComp.lpszPassword, urlComp.dwPasswordLength);
		}
		if (urlComp.lpszExtraInfo) {
			extrainfo.assign(urlComp.lpszExtraInfo,
				urlComp.dwExtraInfoLength);
		}
		return true;
	}
};


class WinINetObject {
public:
	WinINetObject(HINTERNET hInternet) :hInternet_(hInternet) {
	}
	operator HINTERNET() {
		return hInternet_;
	}
	operator bool() {
		return hInternet_ != nullptr;
	}
	WinINetObject() {
		if (hInternet_) {
			InternetCloseHandle(hInternet_);
		}
	}
private:
	HINTERNET hInternet_;
};

bool DownloadFileUseWininet(const std::wstring &url, const std::wstring &localFile, ProgressCallback *callback) {
	WinINetRequestURL zurl;
	if (!zurl.Parse(url)) {
		BaseErrorMessagePrint(L"Wrong URL: %s\n", url.c_str());
		return false;
	}
	DeleteUrlCacheEntryW(url.c_str());
	WinINetObject hInet = InternetOpenW(L"WindowsGet", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (!hInet) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"InternetOpenW(): %s", err.message());
		return false;
	}
	DWORD  dwOption = HTTP_PROTOCOL_FLAG_HTTP2;
	InternetSetOptionW(hInet, INTERNET_OPTION_ENABLE_HTTP_PROTOCOL,&dwOption,sizeof(dwOption));

	WinINetObject hConnect = InternetConnectW(hInet, zurl.host.c_str(),
		(INTERNET_PORT)zurl.nPort, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, NULL);
	if (!hConnect) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"InternetConnectW(): %s", err.message());
		return false;
	}
	DWORD dwOpenRequestFlags = INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
		INTERNET_FLAG_KEEP_CONNECTION |
		INTERNET_FLAG_NO_AUTH |
		INTERNET_FLAG_NO_COOKIES |
		INTERNET_FLAG_NO_UI |
		INTERNET_FLAG_SECURE |
		INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
		INTERNET_FLAG_RELOAD;

	DWORD64 dwContentLength=1;
	WinINetObject hRequest = InternetOpenUrlW(hInet, url.c_str(), nullptr, 0,
		dwOpenRequestFlags, 0);
	if (zurl.nScheme == INTERNET_SCHEME_HTTP
		|| zurl.nScheme == INTERNET_SCHEME_HTTPS) {
		DWORD dwStatusCode = 0;
		DWORD dwSizeLength = sizeof(dwStatusCode);
		if (HttpQueryInfoW(hRequest,
			HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
			&dwStatusCode, &dwSizeLength, nullptr)) {
			return false;
		}
		wchar_t szbuf[20];
		dwSizeLength = sizeof(szbuf);
		HttpQueryInfoW(hRequest,HTTP_QUERY_CONTENT_LENGTH, 
			szbuf, &dwSizeLength, nullptr);
		wchar_t *cx;
		dwContentLength = wcstoull(szbuf, &cx, 10);
		fprintf(stderr, "Content-Length: %llu\n", dwContentLength);
	}
	else if(zurl.nScheme==URL_SCHEME_FTP) {
		DWORD highSize=0;
		auto loSize=FtpGetFileSize(hRequest, &highSize);
		dwContentLength = ((DWORD64)highSize << 32)+loSize ;
	}
	//InternetQueryDataAvailable
	if (!hRequest) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"InternetOpenUrlW(): %s", err.message());
		return false;
	}

	// lpszVersion ->nullptr ,use config
	std::wstring tmp = localFile + L".part";
	HANDLE hFile =
		CreateFileW(tmp.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	///
	BYTE fixedsizebuf[16384];
	//DWORD64 rdsize = 0;
	DWORD dwReadSize = 0;
	DWORD dwWriteSize = 0;
	uint64_t rdsize = 0;
	if (callback) {
		callback->impl(0, callback->userdata);
	}
	BOOL result=true;
	do {
		result=InternetReadFile(hRequest, fixedsizebuf, sizeof(fixedsizebuf), &dwReadSize);
		if (!result) {
			ErrorMessage err(GetLastError());
			BaseErrorMessagePrint(L"HttpOpenRequestW(): %s", err.message());
		}
		WriteFile(hFile, fixedsizebuf, dwReadSize, &dwWriteSize, nullptr);
		rdsize += dwReadSize;
		if (callback) {
			callback->impl(rdsize *100/ dwContentLength, callback->userdata);
		}
	} while (result&&dwReadSize);
	if (callback) {
		callback->impl(100, callback->userdata);
	}
	std::wstring npath = localFile;
	int i = 1;
	while (PathFileExistsW(npath.c_str())) {
		auto n = localFile.find_last_of('.');
		if (n != std::wstring::npos) {
			npath = localFile.substr(0, n) + L"(";
			npath += std::to_wstring(i);
			npath += L")";
			npath += localFile.substr(n);
		}
		else {
			npath = localFile + L"(" + std::to_wstring(i) + L")";
		}
		i++;
	}
	CloseHandle(hFile);
	MoveFileExW(tmp.c_str(), npath.c_str(), MOVEFILE_COPY_ALLOWED);
	return true;
}
```

### HTTP2 支持

自 Windows 10 1507 起， Wininet 便允许开发者通过设置参数开启 HTTP2 支持


# 如何选择

除了以上的解决方案外，在 Windows 系统中实现下载功能还有很多其他的选择，比如试用 libcurl，Poco Net，cpp-netlib，
cpprestsdk，QNetworkRequest 等等。就 HTTP URL 的下载来说，如果不想使用现成的 HTTP 库实现下载，还可以自己实现
HTTP 库，可以使用原生的 socket，也可以使用 Boost.Asio，libuv，libev，ACE 来实现 HTTP 客户端。
