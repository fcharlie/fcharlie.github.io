---
layout: post
title:  "Windows 命令行转义杂谈"
date:   2019-07-20 20:00:00
published: true
categories: windows
---

## 背景

2019 年五月的 Microsoft Build 大会，微软宣布了 Windows Terminal，并在 Github  上开源：[https://github.com/microsoft/terminal](https://github.com/microsoft/terminal)。我作为技术爱好者，肯定要尝鲜一番。

使用截图：

![](https://user-images.githubusercontent.com/6904176/58634006-b05e6080-82d9-11e9-9e3f-2715647edf1b.png)

在使用的过程中，多标签，亚克力的窗口背景和 `Colour Emojis` 都让我非常满意，而且使用 Direct2D 绘制的 emoji，看起来要比 Mintty 使用 GDI+PNG 的 emoji 方案好的多（PNG 的 emoji 无论是放大和缩小都更容易失真，Mintty 并没有 Segoe UI 字体的 emoji 风格，其他风格我并不太喜欢）。不过我在使用的过程中发现了一个 BUG，即 [Bug Report: The conhost command line is not properly escaped #1090 ](https://github.com/microsoft/terminal/issues/1090)，后来我又提交了一个 PR 修复了此问题，在研究其他软件源码时，我发现这个问题并不是个例，因此有必要对此问题进行一次讨论，便有了此文。

## Windows Terminal 的命令行转义错误

在 Windows Termianl 开源之处，我便安装了 Windows Termianl，并且每个周末都可能会拉取最新代码构建 Windows Termianl。但在自定义 `profile.json` 时，我发现 Windows Termianl 启动某个 `profiles` 会与预期不一致，相关的配置如下：

```json
        {
            "acrylicOpacity": 0.75,
            "closeOnExit": false,
            "colorScheme": "Campbell",
            "commandline": "\"C:\\Program Files\\PowerShell\\7-preview\\pwsh.exe\" -NoExit -Command \"$Host.UI.RawUI.WindowTitle=\\\"Windows Pwsh 💙 (7 Preview)\\\"\"",
            "cursorColor": "#FFFFFF",
            "cursorShape": "bar",
            "fontFace": "Consolas",
            "fontSize": 12,
            "guid": "{08a0be98-ff68-4e3a-a054-0fbd3969d3bb}",
            "historySize": 9001,
            "icon": "ms-appdata:///roaming/pwsh-32.png",
            "name": "Windows Pwsh \ud83d\udc99 (7 Preview)",
            "padding": "0, 0, 0, 0",
            "snapOnInput": true,
            "startingDirectory": "C:\\Users\\CharlieInc",
            "useAcrylic": true
        }
```

其中的 `commandline` 是启动 Shell 的命令，这里的目的是启动 `PowerShell 7 Preview` 并将其标题设置成 `Emoji`, 这里 `\ud83d\udc99` 实际上是 `U+1F499` 即 '💙'，你可以使用 `echo -e "\U0001F499"` 输出。但 Windows Terminal 中，报告了如下错误：

![](https://user-images.githubusercontent.com/6904176/58741634-e7558300-844d-11e9-9b7e-dcdff905ffa3.png)

查看任务管理器命令行得到如下命令：

![](https://user-images.githubusercontent.com/6904176/58741666-38657700-844e-11e9-8711-e813c22bc137.png)

我们可以看到 Pwsh 进程的命令行与 JSON 配置中的已经不一致了：

```batch
:: want
"C:\Program Files\PowerShell\7-preview\pwsh.exe" -NoExit -Command "$Host.UI.RawUI.WindowTitle=\"Windows Pwsh 💙 (7 Preview)\""
:: got
"C:\Program Files\PowerShell\7-preview\pwsh.exe" -NoExit -Command $Host.UI.RawUI.WindowTitle="Windows Pwsh 💙 (7 Preview)"
```
按照 Windows 命令行解析规范 [Parsing C++ Command-Line Arguments ](https://docs.microsoft.com/zh-cn/previous-versions/17w5ykft(v=vs.85))，pwsh 在解析命令行时， 获得到的命令行会编成如下数组：

```txt
ARGV0: C:\Program Files\PowerShell\7-preview\pwsh.exe
ARGV1: -NoExit
ARGV2: -Command
ARGV3: $Host.UI.RawUI.WindowTitle=Windows Pwsh 💙 (7 Preview)
```

但实际上我们预期的命令行应该是：

```txt
ARGV0: C:\Program Files\PowerShell\7-preview\pwsh.exe
ARGV1: -NoExit
ARGV2: -Command 
ARGV3: $Host.UI.RawUI.WindowTitle="Windows Pwsh 💙 (7 Preview)"
```

这样一来在 Pwsh 解析命令行后设置 `$Host.UI.RawUI.WindowTitle` 肯定就会出错，那么问题是怎么产生的？

我们查看进程树，发现 Pwsh 的父进程是 OpenConsole，也就是图中的 `conhost.exe`, 即 Windows 控制台窗口主机的开源版本。

通过分析 OpenConsole 的源码，我发现问题出现在合成子进程命令行出错了：

```c++
[[nodiscard]]
HRESULT ConsoleArguments::_GetClientCommandline(_Inout_ std::vector<std::wstring>& args, const size_t index, const bool skipFirst)
{
    auto start = args.begin()+index;

    // Erase the first token.
    //  Used to get rid of the explicit commandline token "--"
    if (skipFirst)
    {
        // Make sure that the arg we're deleting is "--"
        FAIL_FAST_IF(!(CLIENT_COMMANDLINE_ARG == start->c_str()));
        args.erase(start);
    }

    _clientCommandline = L"";
    size_t j = 0;
    for (j = index; j < args.size(); j++)
    {
        _clientCommandline += args[j];
        if (j+1 < args.size())
        {
            _clientCommandline += L" ";
        }
    }
    args.erase(args.begin()+index, args.begin()+j);

    return S_OK;
}
```

在这段代码中，`_clientCommandline` 仅仅只是简单相加，没有做任何的转义，一旦某一个参数中需要转义却未转义，则会导致子进程命令行与预期不相符，于是我便打开了 Issue: [Bug Report: The conhost command line is not properly escaped #1090 ](https://github.com/microsoft/terminal/issues/1090)，由于我在实现 [Privexec/wsudo](https://github.com/M2Team/Privexec)，已经有过此类经验，还在此 Issue 中添加了一段代码告知如何转义，但最初 Windows Terminal 的意见是直接使用 `ConPTY` 替换 `Conhost`，也就是省去了使用 Conhost 作为 ConPTY，好吧，这有点绕，目前 Windows Terminal 依然使用 Conhost 创建 ConPTY, 然后，WindowsTerminal.exe 通过三条管道连接 Conhost.exe 分别是输入输出和信号管道。创建 [Conhost-ConPTY](https://github.com/microsoft/terminal/blob/master/src/inc/conpty.h) 的代码如下：

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

但由于 [Switch from ConhostConnection to the official ConPTY API #1131](https://github.com/microsoft/terminal/issues/1131) 依赖 [`winconpty.c`](https://github.com/microsoft/terminal/issues/1130)，但 `winconpty.c` 迟迟没有开源，因此，此问题一直没有得到解决，后来随着 Windows Terminal 的公开预览，此问题也不断由用户报告。于是我提交了一个 [Fix The conhost command line is not properly escaped](https://github.com/microsoft/terminal/pull/1815), 试图修复这个问题，经过一个星期的等待，PR 终于被合并，目前使用最新源码构建的 Windows Terminal 已经能够按照预期工作，我的 Windows Terminal 配置文件也已经更新：[https://gist.github.com/fcharlie/7530d36175bc5249f1ae92be536238cd](https://gist.github.com/fcharlie/7530d36175bc5249f1ae92be536238cd)

这个问题在 conhost 中都能出现，可能并不是个例，那么应该很多软件都会出现命令行正确转义。

## Windows UCRT 的进程启动函数

在 Windows 中，通常编写 C 程序启动进程除了使用 `CreateProcess` API 之外，还可以使用 `_spawn*` `_exec*` 之类的函数，但在 Windows 1903（10.0.18362.0）UCRT 的源码中，都是调用 `common_spawnvp` 实现，而命令行合成的代码是通过如下函数实现：

```c++
// "C:\Program Files (x86)\Windows Kits\10\Source\10.0.18362.0\ucrt\exec\cenvarg.cpp" +24

// Converts a main()-style argv arguments vector into a command line.  On success,
// returns a pointer to the newly constructed arguments block; the caller is
// responsible for freeing the string.  On failure, returns null and sets errno.
template <typename Character>
static errno_t __cdecl construct_command_line(
    Character const* const* const argv,
    Character**             const command_line_result
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    *command_line_result = nullptr;

    // Compute the number of bytes required to store the arguments in argv in a
    // command line string (including spaces between arguments and a terminator):
    size_t const command_line_count = [&]
    {
        size_t n = 0;
        for (Character const* const* it = argv; *it; n += traits::tcslen(*it++) + 1) { }

        // If there were no arguments, return 1 so that we can return an empty
        // string:
        return __max(n, 1);
    }();

    __crt_unique_heap_ptr<Character> command_line(_calloc_crt_t(Character, command_line_count));
    if (!command_line)
    {
        __acrt_errno_map_os_error(ERROR_NOT_ENOUGH_MEMORY);
        return errno = ENOMEM;
    }

    Character const* const* source_it = argv;
    Character*              result_it = command_line.get();

    // If there are no arguments, just return the empty string:
    if (*source_it == '\0')
    {
        *command_line_result = command_line.detach();
        return 0;
    }

    // Copy the arguments, separated by spaces:
    while (*source_it != '\0')
    {
        _ERRCHECK(traits::tcscpy_s(result_it, command_line_count - (result_it - command_line.get()), *source_it));
        result_it += traits::tcslen(*source_it);
        *result_it++ = ' ';
        ++source_it;
    }

    // Replace the last space with a terminator:
    result_it[-1] = '\0';

    *command_line_result = command_line.detach();
    return 0;
}
```

实际上这个 UCRT 出现此问题，修复的可能性要小于 Windows Terminal，毕竟没啥渠道。这和 SSH 的命令行转义一样难修。还有一些其他软件也存在这个问题，比如 [reproc](https://github.com/DaanDeMeyer/reproc/issues/18)。

## 说不完的转义

转义问题不仅仅是命令行需要注意的，在查询数据库，解析网络请求，方方面面都需要注意到转义问题，一些安全漏洞也是有缺乏转义导致的。写代码虽易，写好代码难。

这个问题在 Windows 中发生频率较高的原因之一是在 Windows 进程 PEB 中，命令行与 POSIX 的命令行保存格式差异，Windows 命令行参数是 `CommandLine` 形式，而 POSIX 以 Linux 为例是 `Argv`。内存中如下：

```c++
const wchar_t *WinCommandLine=L"\"C:\\Program Files\\PowerShell\\7-preview\\pwsh.exe\" -NoExit -Command \"$Host.UI.RawUI.WindowTitle=\\\"Windows Pwsh 💙 (7 Preview)\\\"\"\0";

const char *argvblock="pwsh\0-NoExit\0-Command\0\"$Host.UI.RawUI.WindowTitle=\"Windows Pwsh 💙 (7 Preview)\"\0";
//char *const argv[]={0x0100,...};
```

因此，以 Argv 的这种命令行形式在使用系统调用真正启动进程时，Windows 仍需要一次转换，这就容易带来问题，这也是命令行转义问题在 Windows 平台太比较多的原因。

## 最后

Windows Terminal 开源还是非常不错的，参与到其中改进在 Windows 系统的开发体验感觉很不错，而其中的 OpenConsole 源码也让我更加深入了解了 Windows Console 的一些原理。

OpenConsole 目前可以开启 注册表选项 `HKCU\Console\UseDx`=`DWORD(1)` 切换到使用 DirectWrite 渲染字体，但我目前（2019-07-20）测试虽然使用 DirectWrite 渲染，但 Emoji 无法显示，这个问题我将进一步跟踪。

控制台团队还在 Windows Terminal 源码中添加了一个 Console 控件，这样一来，第三方程序中集成控制台窗口将更加容易。Visual Studio [WhackWhackTerminal](https://github.com/microsoft/WhackWhackTerminal) 的作者 Daniel Griffen 还增加了一些 PR, 实现 WPF 控制台控件，这可以预见，可能将新的控制台集成到 Visual Studio。

下面是一些控制台的文档和关键代码：

+   WinPTY 作者的控制台文档： [Console Handles and Standard Handles](https://github.com/rprichard/win32-console-docs)
+   Conhost 启动进程并创建 ConDrv 设备：[https://github.com/microsoft/terminal/blob/master/src/server/Entrypoints.cpp](https://github.com/microsoft/terminal/blob/master/src/server/Entrypoints.cpp)
+   Conhost 读写内核对象(读写控制台程序的输入输出): [https://github.com/microsoft/terminal/blob/master/src/server/DeviceComm.cpp](https://github.com/microsoft/terminal/blob/master/src/server/DeviceComm.cpp)
+    Windows Terminal 连接器 [ConptyConnection](https://github.com/microsoft/terminal/blob/master/src/cascadia/TerminalConnection/ConptyConnection.cpp) 及 [ConhostConnection](https://github.com/microsoft/terminal/blob/master/src/cascadia/TerminalConnection/ConhostConnection.cpp)
