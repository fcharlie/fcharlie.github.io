---
layout: post
title:  "Windows 命令行转义杂谈"
date:   2019-07-16 22:00:00
published: false
categories: windows
---

## 背景

2019 年五月的 Microsoft Build 大会，微软宣布了 Windows Terminal，并在 Github  上开源：[https://github.com/microsoft/terminal](https://github.com/microsoft/terminal)。我作为技术爱好者，肯定要尝鲜一番。

<!--https://github.com/rprichard/win32-console-docs-->

<!--
https://github.com/microsoft/terminal/blob/master/src/server/Entrypoints.cpp
https://github.com/microsoft/terminal/blob/master/src/cascadia/TerminalConnection/ConptyConnection.cpp
https://github.com/microsoft/terminal/blob/master/src/server/DeviceComm.cpp
https://github.com/microsoft/terminal/blob/master/src/cascadia/TerminalConnection/ConhostConnection.cpp
https://github.com/microsoft/terminal/blob/master/src/inc/conpty.h
-->

```c++
__declspec(noinline) inline HRESULT CreateConPty(const std::wstring& cmdline,
                                                 const unsigned short w,
                                                 const unsigned short h,
                                                 HANDLE* const hInput,
                                                 HANDLE* const hOutput,
                                                 HANDLE* const hSignal,
                                                 PROCESS_INFORMATION* const piPty)
{
    // Create some anon pipes so we can pass handles down and into the console.
    // IMPORTANT NOTE:
    // We're creating the pipe here with un-inheritable handles, then marking
    //      the conhost sides of the pipes as inheritable. We do this because if
    //      the entire pipe is marked as inheritable, when we pass the handles
    //      to CreateProcess, at some point the entire pipe object is copied to
    //      the conhost process, which includes the terminal side of the pipes
    //      (_inPipe and _outPipe). This means that if we die, there's still
    //      outstanding handles to our side of the pipes, and those handles are
    //      in conhost, despite conhost being unable to reference those handles
    //      and close them.
    // CRITICAL: Close our side of the handles. Otherwise you'll get the same
    //      problem if you close conhost, but not us (the terminal).
    HANDLE outPipeConhostSide;
    HANDLE inPipeConhostSide;
    HANDLE signalPipeConhostSide;

    SECURITY_ATTRIBUTES sa;
    sa = { 0 };
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = nullptr;

    CreatePipe(&inPipeConhostSide, hInput, &sa, 0);
    CreatePipe(hOutput, &outPipeConhostSide, &sa, 0);

    // Mark inheritable for signal handle when creating. It'll have the same value on the other side.
    sa.bInheritHandle = TRUE;
    CreatePipe(&signalPipeConhostSide, hSignal, &sa, 0);

    SetHandleInformation(inPipeConhostSide, HANDLE_FLAG_INHERIT, 1);
    SetHandleInformation(outPipeConhostSide, HANDLE_FLAG_INHERIT, 1);

    std::wstring conhostCmdline = L"conhost.exe";
    conhostCmdline += L" --headless";
    std::wstringstream ss;
    if (w != 0 && h != 0)
    {
        ss << L" --width " << (unsigned long)w;
        ss << L" --height " << (unsigned long)h;
    }

    ss << L" --signal 0x" << std::hex << HandleToUlong(signalPipeConhostSide);
    conhostCmdline += ss.str();
    conhostCmdline += L" -- ";
    conhostCmdline += cmdline;

    STARTUPINFO si = { 0 };
    si.cb = sizeof(STARTUPINFOW);
    si.hStdInput = inPipeConhostSide;
    si.hStdOutput = outPipeConhostSide;
    si.hStdError = outPipeConhostSide;
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::unique_ptr<wchar_t[]> mutableCommandline = std::make_unique<wchar_t[]>(conhostCmdline.length() + 1);
    if (mutableCommandline == nullptr)
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = StringCchCopy(mutableCommandline.get(), conhostCmdline.length() + 1, conhostCmdline.c_str());
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    bool fSuccess = !!CreateProcessW(
        nullptr,
        mutableCommandline.get(),
        nullptr, // lpProcessAttributes
        nullptr, // lpThreadAttributes
        true, // bInheritHandles
        0, // dwCreationFlags
        nullptr, // lpEnvironment
        nullptr, // lpCurrentDirectory
        &si, // lpStartupInfo
        piPty // lpProcessInformation
    );

    CloseHandle(inPipeConhostSide);
    CloseHandle(outPipeConhostSide);
    CloseHandle(signalPipeConhostSide);

    return fSuccess ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}
```