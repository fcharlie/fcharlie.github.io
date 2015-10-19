---
layout: post
title:  "Aeah of Time Compilation "
date:   2015-06-16 21:30:16
categories: compiler
---
#Ahead of Time
AOT 即Ahead of Time Compilation,即运行前编，与之对应的是JIT。众所周知，程序的源码并不能够被处理器直接执行，
编程语言基本上都是人类可读，编译器或者解释器就需要将源代码转变为 CPU 可以操作的指令。比如一个加法函数最终需要执行 
addl 汇编指令对应的机器码。   
add.c
{% highlight cpp %}
int add(int x,int y){
    return x+y;
}
{% endhighlight %}

>clang -S add.c

汇编文件 add.s
{% highlight asm %}
	.text
	.def	 add;
	.scl	2;
	.type	32;
	.endef
	.globl	add
	.align	16, 0x90
add:                                    # @add
.Ltmp0:
.seh_proc add
# BB#0:
	pushq	%rax
.Ltmp1:
	.seh_stackalloc 8
.Ltmp2:
	.seh_endprologue
	movl	%ecx, 4(%rsp)
	movl	%edx, (%rsp)
	movl	4(%rsp), %ecx
	addl	%ecx, %edx
	movl	%edx, %eax
	popq	%rdx
	retq
.Ltmp3:
	.seh_endproc


{% endhighlight %}

对于转变为机器码的时机，不同的语言有着不同的选择，或是完全转变为机器码后运行，或是在运行时转变为机器码。AOT 便是运行前转为机器码。
，事实上C/C++ D,Pascal,Fortran之类的语言本质上也是AOT,但本文讨论的AOT主要针对的是对于Java,NET等框架或语言的AOT。 

以 Java 为例，Java 源码被编译器 Java Vitrual Machine ByteCode,当需要执行的时候，将 JVM 指令一条一条的转变为对应处理器的指令，然后执行，（实际上
x86 上模拟执行 ARM 架构的程序也可以是这个套路。）但是这个效率并不高，而且不好优化，而 JIT 的做法是将字节码编译成对于处理器的指令后运行。
这比纯解释又快了许多。



##1. LLVM 编译器基础设施的发迹   
数年前，LLVM的官网对于LLVM项目的介绍是: "Low Level Virtual Machine",低级虚拟机，而现在对LLVM的介绍是："The LLVM Compiler Infrastructure"，
即编译器基础设施。  在程序员圈子中对LLVM最深刻的影响来自于Clang，C 家族编译器(C/C++ /Objective-C/C++ Compiler)前端，Clang 是 LLVM 最成功的实现，
在平台支持上，Clang 短短几年达到了 GNU C Compliton (GCC) 20年的高度。 Clang 在编译速度，占用内存，以及整个框架的设计上都是可圈可点的，
对用户友好的开源许可证 *[The University of Illinois/NCSA Open Source License (NCSA)](http://opensource.org/licenses/UoI-NCSA.php)*. 
实际上就有商业编译器依赖Clang实现，比如：Embarcadero™ C++ Builder 的 Win64 编译器 bcc64 就是完全基于 Clang 实现（3.1 trunk）。
而 C++ Builder 前身是 Borland C/C++&Turbo C.     
下面bcc64的命令实例:       
>bcc64 -cc1 -D_RTLDLL -fborland-extensions -triple=x86_64-pc-win32-elf -emit-obj -std=c++11 -o Hello.o Hello.cpp   

看过**《C/C++圣战》** 大抵也知道 Borland C/C++ 曾经是多么的辉煌，而现在却选择了 Clang 来实现 Win64 工具链 （C++ Builder 10  32位也使用了 clang）。  
 
一方面，单从 C 语言家族来讲 Clang 基于库的模块化设计，易于 IDE 集成及其他用途的重用。比如 Sublime Text，VIM，Emacs 都有基于 Clang 实现 C/C++ 
代码自动补全，Clang 提供一个 libclang 的库，可以编译成动态也可以编译成静态库，SublimeText 的 C/C++ 插件 SublimeClang 就是使用 libclang.dll(so/dylib)。
其他的编译器对于 IDE 集成的支持是远远不及的，比如 Visual Studio IDE 对于 C++ 的智能提示是使用  [EDG C++ Frontend ](http://www.edg.com/index.php?location=c_frontend)   
目前 Clang 在 C++ 的标准上，远远优于其他主流编译器 Microsoft C++(cl),GCC (g++)。   
另一方面，LLVM 实现了一套可扩展的编译器实现方案，任何人需要实现一个语言，只需要实现一个前段，然后将源码编译成 LLVM 字节码，也就是 LLVM IR, 然后 LLVM llc 将
源码编译成不同平台的机器码，并且优化。比如最近正火的语言 Rust 后端也使用了 LLVM,以及 D 语言编译器 ldc，Go 语言编译器 llgo 等等。而 LLVM 不仅仅拥有 AOT 的能力，
而且还有 JIT 模块, [LLVM ExecutionEngine](http://llvm.org/svn/llvm-project/llvm/trunk/lib/ExecutionEngine/) ExecutionEngine 的 API 并不是非常稳定。


###传统的编译器  
传统编译器需要经过前端(Frontend)，优化(Optimizer)，后端(Backend)然后将源代码转变为机器码。    
![SimpleCompiler](http://www.aosabook.org/images/llvm/SimpleCompiler.png)   
                             Three Major Components of a Three-Phase Compiler                           
							                             
如果需要增加一种新的平台的支持，这种模型无法提供更多的可重用的代码。   

要添加其他语言的支持模型如下：      
![Retargetable](http://www.aosabook.org/images/llvm/RetargetableCompiler.png)                
                              Retargetablity


###基于 LLVM 的编译器  
基于 LLVM 的编译器架构如下：      
![LLVMCompiler1](http://www.aosabook.org/images/llvm/LLVMCompiler1.png)   
                              LLVM's Implementation of the Three-Phase Design                
							
基于 LLVM 的编译器前端将源码编译成 LLVM IR,然后在使用优化编译器编译成对应平台的机器码，一个很鲜明的对比是 D语言的编译器 DMD 与 ldc,DMD 是传统的编译器
而 ldc 是基于 LLVM 的编译器，DMD 目前依然只支持 x86/x86_64 架构处理器，而 ldc 可以生成 ARM64,PPC,PPC64, mips64 架构的机器码
[Dlang Compilers](http://wiki.dlang.org/Compilers#Comparison)   

LLVM IR 可以反汇编成人类可读的形式，LLVM IR 类似于 RSIC 指令。
> clang add.c -S -emit-llvm

add.ll
{% highlight llvm %}
; ModuleID = 'add.bc'
target datalayout = "e-m:w-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-w64-windows-gnu"

; Function Attrs: nounwind uwtable
define i32 @add(i32 %y, i32 %x) #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  store i32 %y, i32* %1, align 4
  store i32 %x, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  %4 = load i32, i32* %1, align 4
  %5 = add nsw i32 %3, %4
  ret i32 %5
}

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"clang version 3.7.0 (tags/RELEASE_370/final)"}

{% endhighlight %}


使用以下命令即可：    
> clang add.ll -c

也可以使用 llc 命令编译   

于2010年Chris Lattner 被 ACM 授予 "Programming Languages Software Award" 。2014年 Chris Lattner 作为苹果编译器开发团队的首席架构师，
在 Apple WWDC 2014 推出了Swift。而 Swift 就是基于 LLVM 的，使用如下命令编译 swift 代码，即可得到 LLVM IR 代码。   
>swiftc -S -emit-object hello.swift 

{% highlight swift %}
// hello.swift
print("Hello, world!");
{% endhighlight %}
 
[通过 LLVM 在 Android 上运行 Swift 代码](http://romain.goyet.com/articles/running_swift_code_on_android/)

整个 LLVM 项目推出了很多重量级工具，除了 Clang 之外，还有 LLVM 调试器  lldb, LLVM 连接器 lld,目前都可以在 Windows, Linux ,Mac ,以及 BSD 上运行，
目前 XCode 自带有这些工具，Windows 上，clang lld 都是能够安装集成到 Visual Studio 的。

很多公司贡献了代码到 LLVM 项目中，或使用 LLVM 的工具改善自己的产品，比如 Google ,Google NDK 以及 PNacl 都使用了 LLVM 的工具，而 LLVM 的许多特性就是 
Google 实现的，比如地址消毒剂 AddressSanitizer（GCC 目前也支持了）。还有 Intel OpenCL, Adobe, NVIDIA Nucda,Microsoft WinObjc。

##2. Android 与 AOT  
LLVM 优异的架构并没有被 Android 广泛使用。
Android最初由Andy Rubin开发作为数码相机的操作系统，使用Linux内核，后来发现市场需求不大被改造成智能手机操作系统反而获得了巨大成功。
Rubin 选择了具有很大争议的Java作为Android的应用开发语言，Java基于JVM，能够在支持JVM的平台上运行，Java的开发者非常多，
你可以在中国任何一个理工科大学找到学习Java的学生，漫天遍地的Java培训机构，这对于Android来说非常有利，从Google收购Android开始，这一切已然水到渠成。
Android 使用的是Dalvik的虚拟机，这与Java官方的JVM 技术上稍微有些差异，JVM是一种堆栈机器，
而Dalvik是[寄存器机](http://zh.wikipedia.org/wiki/%E5%AF%84%E5%AD%98%E5%99%A8%E6%9C%BA),孰优孰劣，也不太好评价，
正如CISC与RSIC的争议，实际上对于软件而言，架构，编码实现，编译器（解析器），都会给软件的性能带来巨大的影响，时常发现某某JavaScript升级换代，
性能增加一倍。


####Android Runtime
2014年6月，Google 推出 Android 5.0(Android Lollipop) ，ART 完全取代了 Dalvik。    
![ART View](https://upload.wikimedia.org/wikipedia/commons/2/25/ART_view.png)  
ART 本质上一个混合的 AOT 方案，它还实现了 JVM 解释器。

Andy Rubin 先后在苹果 微软 谷歌公司工作过。

##3. .NET 与 AOT    
说起.NET 就不得不谈到 [Anders Hejlsberg](http://zh.wikipedia.org/wiki/%E5%AE%89%E5%BE%B7%E6%96%AF%C2%B7%E6%B5%B7%E5%B0%94%E6%96%AF%E4%BC%AF%E6%A0%BC)此人，
他来自丹麦，Turbo Pascal最开始就是他开发的，Delphi/C#之父，C#&.NET 的首席架构师，TypeScript 的首席架构师，主持开发了.NET Framework，Visual Basic.NET，
以及最新的.NET 编译器 [Roslyn](http://msdn.microsoft.com/en-us/vstudio/roslyn.aspx) 。   
值得注意的是 TypeScript 完全基于 ECMAScript 6标准草案开发，Java 的流行以至于微软也坐不住，在上个世纪末，微软也开发了自己的Java虚拟机，最初微软推出的是Visual J++，
而在Anders加入微软后立即被委以重任，Visual J++在性能上甚至超越了Sun JVM，这个Sun带来了恐慌，Sun 以破坏兼容性将微软告上公堂，微软最终放弃了Java的开发，而C#与.NET也诞生了，
.NET在设计上确实借鉴了Java的很多理念，并且超越了Java，这也是 Anders 从 Borland 就存在心中的构想。

类似于 LLVM 的研究，微软很早就有，这个项目是：   
**Phoenix Compiler and Shared Source Common Language Infrastructure**

现在的 Microsoft Visual C++ 就有 Phoenix 编译器架构的技术积累。

Chris Lattner 曾于2004年在微软研究院实习，参与微软的 [Phoenix Compiler Framework](http://research.microsoft.com/en-us/collaboration/focus/cs/phoenix.aspx) 项目，
或许对于微软来说，应该感到遗憾，Chris Lattner 并没有最终加入微软，而是加入了苹果公司。很多时候技术是相互影响的，
好的技术最后都会殊途同归。          
在我刚进入大学的时候，刚刚学会编程，曾经下载过08版的 Phoenix Compiler 编译器工具，并且也试用过，不过到现在已经无法下载了。而 Phoenix Compiler Framework与LLVM的理念确实很相似，
并且可以得知的是，Phoenix 很多的技术被整合到微软的 Microsoft C/C++ Compiler，就技术上而言 Phoenix 与 LLVM 有许多相似之处，比如都能转变成 IR，拥有软件优化和分析框架，
然而具体的中间语言是不一样的。         

Phoenix 的架构师 Andy Ayers 本人也是 LLILC 的核心成员。     

>Phoenix不仅仅限于一个编译器，它还是一个软件优化和分析框架，能被其他编译器和工具使用。 它能生成二进制代码，也能输出MSIL程序集。源代码可以经过分析，
>并被表示为 IR（中间表示，Intermediate Representation）形式，这种形式可以在后期被各种工具分析和处理。    
                                                         ----InfoQ: [Phoenix编译器框架说明](http://www.infoq.com/cn/news/2008/05/Phoenix-Compiler-Framework)     

在 .NET 未开源时，微软研究院还提供了一个 .NET 的学习代码 “[Shared Source Common Language Infrastructure](http://www.microsoft.com/en-us/download/details.aspx?id=4917)”的源代码下载。

为什么说些无关的东西？实际上，微软的 .NET Native 实现离不开 Phoenix 编译器的技术研究。   


.NET Framework 三阶段图：      
![DotCLR](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/dotNet/CLR_diag.png)
                               .Net Three-Phase 


.NET Framewok Native & JIT 模型
![DotNetCoreCLR](https://raw.githubusercontent.com/fcharlie/site-res/master/compilers/dotnativecoreclr.png)


###.NET Compiler Platform ("Roslyn")
Roslyn 是 Microsoft 推出的新一代 C#/VB.NET 编译器,相对于传统的 .NET C# 编译器,整个生产流程结构非常清晰,
和 C++ 中的 clang 类比丝毫不为过,而 Visual Studio 2015 也充分利用了 Roslyn 的优秀特性.     
目前无论是 Microsoft 还是 Mono 都参与到了 Roslyn 的开发过程中,利用 Roslyn ,一些第三方的 C# AOT 解决方案迅速的发展起来.     

编译器管道:      
![Pipe](https://github.com/dotnet/roslyn/wiki/images/compiler-pipeline.png)

编译器管道及对应的 API:      
![API](https://github.com/dotnet/roslyn/wiki/images/compiler-pipeline-api.png)

编译器 API 和 服务:     
![svc](https://github.com/dotnet/roslyn/wiki/images/compiler-pipeline-lang-svc.png)

Roslyn APIs:    
![Roslyn](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/Roslyn.png)           
                                             


###.NET Native
.NET 的 AOT 解决方案在 Mono 中很早就出现了，Mono 平台支持 Android 以及 iOS 的 App 开发,由于 iOS 禁止第三方软件的 JIT 编译,
在iOS 平台,Mono 使用的就是 Full AOT 策略.       

Program.cs
{% highlight csharp %}
using System;

namespace hello
{
	class MainClass
	{
		public static void Main (string[] args)
		{
			Console.WriteLine ("Hello World!");
		}
	}
}
{% endhighlight %}  

使用 Mono 编译:      
>mcs Program.cs

然后使用 mono AOT 编译成机器码:     
>mono --aot=full,nrgctx-trampolines=8096,nimt-trampolines=8096,ntrampolines=4048 Program.exe

使用 objdump 反汇编:     
>objdump -d Program.exe.so >Program.s

这里只反汇编了执行段,Program.s:      
{% highlight cpp-objdump %}

Program.exe.so：     文件格式 elf64-x86-64


Disassembly of section .text:

0000000000001000 <hello_MainClass__ctor-0x10>:
    1000:	90                   	nop
    1001:	90                   	nop
    1002:	90                   	nop
    1003:	90                   	nop
    1004:	90                   	nop
    1005:	90                   	nop
    1006:	90                   	nop
    1007:	90                   	nop
    1008:	90                   	nop
    1009:	90                   	nop
    100a:	90                   	nop
    100b:	90                   	nop
    100c:	90                   	nop
    100d:	90                   	nop
    100e:	90                   	nop
    100f:	90                   	nop

0000000000001010 <hello_MainClass__ctor>:
    1010:	48 83 ec 08          	sub    $0x8,%rsp
    1014:	48 83 c4 08          	add    $0x8,%rsp
    1018:	c3                   	retq   
    1019:	00 00                	add    %al,(%rax)
    101b:	00 00                	add    %al,(%rax)
    101d:	00 00                	add    %al,(%rax)
	...

0000000000001020 <hello_MainClass_Main_string__>:
    1020:	48 83 ec 08          	sub    $0x8,%rsp
    1024:	49 8b 3d e5 24 00 00 	mov    0x24e5(%rip),%rdi        # 3510 <__bss_start+0x20>
    102b:	e8 20 00 00 00       	callq  1050 <plt_System_Console_WriteLine_string>
    1030:	48 83 c4 08          	add    $0x8,%rsp
    1034:	c3                   	retq   
    1035:	00 00                	add    %al,(%rax)
    1037:	00 90 90 90 90 00    	add    %dl,0x909090(%rax)
	...

0000000000001050 <plt_System_Console_WriteLine_string>:
    1050:	ff 25 ca 24 00 00    	jmpq   *0x24ca(%rip)        # 3520 <__bss_start+0x30>
    1056:	0e                   	(bad)  
	...

0000000000001060 <method_addresses>:
    1060:	e8 ab ff ff ff       	callq  1010 <hello_MainClass__ctor>
    1065:	e8 b6 ff ff ff       	callq  1020 <hello_MainClass_Main_string__>
    106a:	e8 f1 ff ff ff       	callq  1060 <method_addresses>
	...
{% endhighlight %}

.NET Framework 一直有一个工具, NGEN (Native Image Generator), NGEN 会将程序集简单的编译成机器码,在 C:\Windows\Microsoft.Net\assembly 
目录就是 NGEN 的镜像. NGEN 依然无法脱离 .NET Framework,任然需要 JIT,程序运行的时候往往是 MSIL 和 MachineCode 混合运行.       
Windows update 更新重启后,经常可以在任务管理器里面发现 NGEN 进程疯狂的执行任务.     
在没有 .NET Native 时, Windows Phone 中,.NET App 在安装后就会通过 NGEN 转变为机器码,以此来提升运行速度,降低功耗.
对于 .NET Native 的需求,随着 Microsoft 的 移动战略的实施变得尤为迫切.      
早在2013年就有传闻，.NET将推出.NET Native,时至今日,基于 .NET 的 Windows 10 通用应用程序,都开始开启 .NET Native 支持.    

.NET Native 基本的流程如下:    
>App IL + FX -> MCG　-> Interop.g.cs -> CSC -> Interop.dll -> Merge -> IL transform -> NUTC -> RhBind -> .EXE

.NET Native 工具链将所有依赖到的程序集反汇编成 C# 源码,使用 C# 编译器再编译成一个 dll, dll 再转 IR ,使用 nutc_driver 编译成机器码,
而 nutc_driver 代码是使用了 Microsoft C++ 后端代码. 最后生成一个 dll 和一个 Bootstrap 的 EXE, dll 导出的函数为:      
>RHBinder__ShimExeMain

使用 Visual C++ 工具 dumpbin 查看符号信息：  
>dumpbin /EXPORTS App2.dll

得到的结果如下：   
{% highlight cpp-objdump %}
Microsoft (R) COFF/PE Dumper Version 14.00.23303.0
Copyright (C) Microsoft Corporation.  All rights reserved.


Dump of file app2.dll

File Type: DLL

  Section contains the following exports for 

    00000000 characteristics
           0 time date stamp Thu Jan  1 08:00:00 1970
        0.00 version
           1 ordinal base
          20 number of functions
          20 number of names

    ordinal hint RVA      name

          1    0 00258498 $thread_static_index = TlsIndexSection
          2    1 00688FA0 AppendExceptionStackFrame = System::Exception.AppendExceptionStackFrame
          3    2 004D6CC0 CheckStaticClassConstruction = System::Runtime::CompilerServices::ClassConstructorRunner.EnsureClassConstructorRun
          4    3 0067B240 CreateCommandLine = System::Runtime::CommandLine.InternalCreateCommandLine
          5    4 00681710 CtorCharArray = System::String.CtorCharArray
          6    5 00681520 CtorCharArrayStartLength = System::String.CtorCharArrayStartLength
          7    6 00680FC0 CtorCharCount = System::String.CtorCharCount
          8    7 00681300 CtorCharPtr = System::String.CtorCharPtr
          9    8 00681110 CtorCharPtrStartLength = System::String.CtorCharPtrStartLength
         10    9 0067E450 FailFast = System::RuntimeExceptionHelpers.RuntimeFailFast
         11    A 0067B110 GenericLookup = System::Runtime::TypeLoaderExports.GenericLookup
         12    B 0067AFF0 GenericLookupAndAllocArray = System::Runtime::TypeLoaderExports.GenericLookupAndAllocArray
         13    C 00462220 GenericLookupAndAllocObject = System::Runtime::TypeLoaderExports.GenericLookupAndAllocObject
         14    D 0067B080 GenericLookupAndCallCtor = System::Runtime::TypeLoaderExports.GenericLookupAndCallCtor
         15    E 0067AEE0 GenericLookupAndCast = System::Runtime::TypeLoaderExports.GenericLookupAndCast
         16    F 0067AF60 GenericLookupAndCheckArrayElemType = System::Runtime::TypeLoaderExports.GenericLookupAndCheckArrayElemType
         17   10 0067E500 GetRuntimeException = System::RuntimeExceptionHelpers.GetRuntimeException
         18   11 0067B170 GetThreadStaticsForDynamicType = System::Runtime::TypeLoaderExports.GetThreadStaticsForDynamicType
         19   12 0067AE50 InitializeFinalizerThread = System::Runtime::FinalizerInitRunner.DoInitialize
         20   13 0027BF10 RHBinder__ShimExeMain = RHBinder__ShimExeMain

  Summary

       5A000 .data
      1FE000 .rdata
       66000 .reloc
        3000 .rsrc
      4F4000 .text
        1000 .tkd0
        1000 .tkd1
        1000 .tkd2
        1000 .tkd3
        1000 .tkd4
        1000 .tkd5
        1000 .tkd6
        1000 .tkd7
        1000 .tks0
        1000 .tks1
        1000 .tks2
        1000 .tks3
        1000 .tks4
        1000 .tks5
        1000 .tks6
        1000 .tks7

{% endhighlight %}

查看 App2.exe 导入的符号信息       
>dumpbin /IMPORTS App2.exe

输出如下：   
{% highlight cpp-objdump %}
Microsoft (R) COFF/PE Dumper Version 14.00.23303.0
Copyright (C) Microsoft Corporation.  All rights reserved.


Dump of file app2.exe

File Type: EXECUTABLE IMAGE

  Section contains the following imports:

    App2.dll
                402000 Import Address Table
                401028 Import Name Table
                     0 time date stamp
              FFFFFFFF Index of first forwarder reference

                    0 RHBinder__ShimExeMain

  Summary

        1000 .data
        1000 .rdata
        1000 .reloc
        3000 .rsrc
        1000 .text

{% endhighlight %}

.NET Native 的实现，在 IR 前期很大的程度上依赖 Roslyn 这类新型的编译器，而在 IR 后期，就得意于 Phoenix 编译器框架，
.NET Native 后端和 Visual C/C++ 共用一套后端优化编译器。

在 Microsoft Channel 9 有一个对 .NET Native 的介绍视频：
[.NET Native Deep Dive](https://channel9.msdn.com/Events/dotnetConf/2014/-NET-Native-Deep-Dive)

视频中的 PPT 可以下载：
[.NET Native PPTX](http://files.channel9.msdn.com/thumbnail/45d78758-8ab8-4e62-8a73-2e6a4027b49c.pptx)  

在 Visual Studio 2015 中，可以使用 NuGet 安装 .NET Native 的相关插件，以此来分析 .NET 引用能否被 .NET Native 支持。      
>Install-Package Microsoft.NETNative.Analyzer

对于 .NET Native, 大多数人并不会感到满意，大多数 .NET 开发者都希望 .NET Native 能够扩展到 桌面平台，能够支持 WPF ...
如果下面的这个项目能够成功，那么 .NET 的 AOT 也就指日可待。


###LLILC - LLVM-Based Compiler for .NET CoreCLR
在 .NET CoreCLR 开源后，.NET 开发团队也创建了基于 LLVM 的 .NET Core 编译器项目 LLILC，实际上，在之前已经有了 C# Native, 
SharpLang 之类的项目着手实现 .NET 的 AOT。然而这些项目大多是个人兴趣，支持有限。     
LLILC 的核心开发者是 Phoenix 编译器框架的架构师 [Andy Ayers](https://github.com/AndyAyersMS),  大神本人也会在 gitter.im 上回答人们对 LLILC 的疑问。
LLILC 包括 JIT 和 AOT ,不过目前 AOT 并没有编码实现。目前项目组的重心任然是 JIT 模块。     

LLILC 的 JIT 架构
![JIT](https://github.com/dotnet/llilc/raw/master/Documentation/Images/JITArch.png)

LLILC 的 AOT 架构
![AOT](https://github.com/dotnet/llilc/raw/master/Documentation/Images/AOTArch.png)

MRT 也就是 .NET Native Runtime ，专门为 .NET Native 实现的一个精简运行时。

LLILC 依然是非常的不完善，最后的究竟怎样仍需观望。

从 .NET 还是 JVM 或者是 LLVM 来看，很多东西都是相似的，技术也在互相影响和渗透。

##4. 探索的脚步 

###4.1 CSNative
永远不会有完全统一的意见，总会有人去创造新的轮子。不谈其他，重复的创造能对已有的东西带来技术革新，在 CodePlex上,
就有个伙计实现了自己的 .NET Native 方案：[C# Native](http://csnative.codeplex.com/)；他利用 Roslyn API 将 C# 编译成 MSIL，
然后将 MSIL 编译成 LLVM IR ,随后 'LLVM System compiler' llc 编译成 Native code ,用 GCC 将 Object 文件链接成 exe，
GC库是32位的 [libgc](http://www.hboehm.info/gc/)， 现在已经转变了策略，直接生成 C++ 代码，使用 G++ 编译成二进制。

{% highlight csharp %}
using System;

class X {
	public static int Main (string [] args)
	{
		Console.WriteLine ("Hello, World!");
		return 0;
	}
}
{% endhighlight %}

Il2c 是一个利用 Roslyn 实现的 C#/MSIL to C++ 的编译器       
>Il2c.exe helloworld.cs /corelib:CoreLib.dll

生成 helloworld.cpp, 然后使用 g++ 编译成 exe :    
>g++ -o helloworld.exe helloworld.cpp CoreLib.cpp -lstdc++ -lgcmt-lib -march=i686 -L .

直接生成 Exe:    
>Il2c.exe /exe helloworld.cs /corelib:CoreLib.dll

C# Native 作者 AlexDev 本人也是 Babylon 3D (C#/native port) 的作者。  

###4.2 SharpLang
同样的，在 Github上，也有一个基于 LLVM 的 C# Native 的解决方案: [SharpLang](https://github.com/xen2/SharpLang)。
在LLILC推出后，开发者 Virgile Bello 也就没有更新 SharpLang 了。

##5. 框架图
###.NET
实际上无论是JVM还是.NET Framework 已经LLVM Framework在结构上是非常相似的，如下图：     
![Framework](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/dotNet/CLR_diag.png)    
                                        

.NET 的功能演进:     
![dotNet](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/dotNet/DotNet.png)      
                                   

从源码到运行：     
![Step](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/dotNet/Overview_of_the_Common_Language_Infrastructure.png)         
                                                        

###JVM   
JVM 加载器：     
![JVM](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/JvmSpec7.png)        
                                                      

###ECMAScript PNacl Asm.js WebAssembly
**asm.js** 是一个非常容易优化的JavaScript子集
[asm.js AOT](https://blog.mozilla.org/luke/2014/01/14/asm-js-aot-compilation-and-startup-performance/)     
![asm.js-AOT](https://ffp4g1ylyit3jdyti1hqcvtb-wpengine.netdna-ssl.com/luke/files/2013/12/aot-diagram.png)  

**PNacl**   
本质上，PNaCl 通过编译本地的 C 和 C++ 代码到一个中间表示，而不是像在 Native Client 的特定于体系结构的表示。LLVM 类型的字节代码被包裹在一个可移植的执行体里面，这个执行体可以托管在一个 Web 服务器上，就像许多其它的网站资产一样。当该网站被访问的时候，Chrome 获取信息并将可移植的执行体转换成一个特定于体系结构的、便携式的、可执行的机器代码，直接为底层设备进行优化。这种转换方法意味着开发者不需要施行多次重新编译App，也可以在x86、ARM或MIPS设备上运行。

**WebAssembly**     
WebAssembly 是 Microsoft Google Mozille Apple 开发者合作开发的一项新技术，可以作为任何编程语言的编译目标，
使应用程序可以运行在浏览器或其它代理中。

[InfoQ WebAssembly：面向Web的通用二进制和文本格式](http://www.infoq.com/cn/news/2015/06/webassembly-wasm)


##备注  
1. LLVM [http://www.aosabook.org/en/llvm.html](http://www.aosabook.org/en/llvm.html)     
2. Embarcadero C++ Builder:    
BCC64.EXE, the C++ 64-bit Windows Compiler:     
[http://docwiki.embarcadero.com/RADStudio/XE6/en/BCC64.EXE,_the_C%2B%2B_64-bit_Windows_Compiler](http://docwiki.embarcadero.com/RADStudio/XE6/en/BCC64.EXE,_the_C%2B%2B_64-bit_Windows_Compiler)   
Clang-based C++ Compilers:     
[http://docwiki.embarcadero.com/RADStudio/XE6/en/Clang-based_C%2B%2B_Compilers](http://docwiki.embarcadero.com/RADStudio/XE6/en/Clang-based_C%2B%2B_Compilers)
3. Phoenix Compiler Framework Wiki:     
[http://en.wikipedia.org/wiki/Phoenix_(compiler_framework)](https://en.wikipedia.org/wiki/Phoenix_%28compiler_framework%29)           
4. Dalvik Wiki:       
[http://en.wikipedia.org/wiki/Dalvik_(software)](https://en.wikipedia.org/wiki/Dalvik_%28software%29)        
5. LLILC News:     
[InfoQ Microsoft Introduces LLILC, LLVM-based .NET/CoreCLR Compiler](http://www.infoq.com/news/2015/04/microsoft-llilc-llvm-compiler) 
