---
layout: post
title:  "AOT与现代软件的杂谈 "
date:   2015-06-16 21:30:16
categories: compiler
---
#AOT与现代软件的杂谈 
AOT 即Ahead of Time,即运行前编译，事实上C/C++ D,Pascal,Fortran之类的语言本质上也是AOT,但本文讨论的AOT主要针对的是对于Java,NET等框架&语言的AOT。 

##1. LLVM的发迹   
三四年前，LLVM的官网对于LLVM项目的介绍是: "Low Level Virtual Machine",低级虚拟机，而现在对LLVM的介绍是："The LLVM Compiler Infrastructure"，即编译器基础设施。  
在程序员圈子中对LLVM最深刻的影响来自于Clang，C 家族编译器(C/C++ Objective-C/C++ Compiler)前端，Clang是LLVM最成功的实现，在平台支持上，Clang短短几年达到了GNU C Compliton (GCC) 20年的高度。 
Clang 在编译速度，占用内存，以及整个框架的设计上都是可圈可点的，对用户友好的开源许可证 *[The University of Illinois/NCSA Open Source License (NCSA)](http://opensource.org/licenses/UoI-NCSA.php)*. 实际上就有商业编译器依赖Clang实现，比如：Embarcadero™ C++ Builder的Win64编译器 bcc64就是完全基于Clang 实现（3.1 trunk）。而C++ Builder前身是Borland C/C++&Turbo C.  
下面bcc64的命令实例:    
>"C:\Program Files (x86)\Embarcadero\Studio\14.0\bin\bcc64.exe" -cc1 -D_RTLDLL -isystem "C:\Program Files (x86)\Embarcadero\Studio\14.0\include" -isystem "C:\Program Files (x86)\Embarcadero\Studio\14.0\include\dinkumware" -isystem "C:\Program Files (x86)\Embarcadero\Studio\14.0\include\windows\crtl" -fborland-extensions -triple=x86_64-pc-win32-elf -emit-obj -std=c++11 -o Hello.o Hello.cpp   

看过**《C/C++圣战》** 大抵也知道Borland C/C++曾经是多么的辉煌，而现在却选择了Clang来实现Win64工具链。    
Clang基于库的模块化设计，易于 IDE 集成及其他用途的重用。比如Sublime Text VIM Emacs都有基于Clang实现C/C++代码自动补全，Clang提供一个libclang的库，可以编译成动态也可以编译成静态库，SublimeText 的C/C++插件SublimeClang就是使用libclang.dll(libclang.so)。   
目前Clang在C++的标准上，远远优于其他主流编译器Microsoft C++(cl.exe),GCC (g++)。  
Clang如此优秀，备受开源界推崇。《程序员》杂志， 在介绍Mac OS背后的故事，专门有一篇介绍LLVM工具链，其中着重的赞扬了Clang,链接如下：[Mac OS X 背后的故事（八）三好学生Chris Lattner的LLVM编译工具链](http://www.programmer.com.cn/9436/) 。 
而Clang仅仅只是LLVM针对C家族语言的实现。 
说到LLVM就不得不谈到Chris Lattner，Chris Lattner其人生于1978年，于2000年进入著名的伊利诺伊大学厄巴纳-香槟分校(University of Illinois at Urbana-Champaign), 而LLVM正是源于Chris Lattner 与  [Vikram Adve](http://academic.research.microsoft.com/Author/2313275/vikram-s-adve)的研究项目。

传统的编译器  

![SimpleCompiler](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/llvm/SimpleCompiler.png)   
                                                 

LLVM  

![LLVMCompiler1](https://github.com/fstudio/Beaot/raw/master/doc/Images/llvm/LLVMCompiler1.png)   
                          

![Retargetable](https://github.com/fstudio/Beaot/raw/master/doc/Images/llvm/RetargetableCompiler.png)                
                               

![InstallTime](https://github.com/fstudio/Beaot/raw/master/doc/Images/llvm/InstallTime.png)    
                                                        

LTO:  

![LTO](https://github.com/fstudio/Beaot/raw/master/doc/Images/llvm/LTO.png)     


![PassLinkage](https://github.com/fstudio/Beaot/raw/master/doc/Images/llvm/PassLinkage.png)   


![X86](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/llvm/X86Target.png)    



于2010年Chris Lattner 被ACM授予 "Programming Languages Software Award" 。2014年Chris Lattner作为苹果编译器开发团队的首席架构师，在Apple WWDC 2014 推出了Swift。实际上Swift也是基于LLVM的。LLVM的优秀设计也是Apple能够迅速推出Swift的基础。

可以说，如果没有LLVM支撑着苹果的软件生态，靠着GCC对Objective-C低优先级的优化，苹果是很难打响一起翻身仗的，也很难用1GB RAM的iPhone单挑Android诸强。

然而不得不说，LLVM的发扬光大同样离不开Google，Apple系对于LLVM的贡献多少有点不足，比如跨平台，在Windows上LLVM很早几乎是无法使用的，Google贡献了大量代码，如项目msbuild，以及clang-cl命令，clang-cl十分接近Visual C/C++的cl命令，能够支持VisualStudio，一般安装后，就可以在VisualStudio选择Clang来开发Windows程序。

##2. Android壮士断腕   
Android最初由Andy Rubin开发作为数码相机的操作系统，使用Linux内核，后来发现市场需求不大被改造成智能手机操作系统反而获得了巨大成功。
Rubin 选择了具有很大争议的Java作为Android的应用开发语言，Java基于JVM，能够在支持JVM的平台上运行，Java的开发者非常多，你可以在中国任何一个理工科大学找到学习Java的学生，漫天遍地的Java培训机构，这对于Android来说非常有利，从Google收购Android开始，这一切已然水到渠成。
Android 使用的是Dalvik的虚拟机，这与Java官方的JVM 技术上稍微有些差异，JVM是一种堆栈机器，而Dalvik是[寄存器机](http://zh.wikipedia.org/wiki/%E5%AF%84%E5%AD%98%E5%99%A8%E6%9C%BA),孰优孰劣，也不太好评价，正如CISC与RSIC的争议，实际上对于软件而言，架构，编码实现，编译器（解析器），都会给软件的性能带来巨大的影响，时常发现某某JavaScript升级换代，性能增加一倍。


####Android ART
2014年6月，Google推出Android 5.0(Android Lollipop) ，ART完全取代了Dalvik。


Oracle将习性一并收购了Sun，当Android已成气候，Oracle便由API侵权来收割。这也迫使Google开始转变，或许Android ART最终的目的只是为了兼容，以后出现Go来开发Android,Java也少见了，Plan9换皮重生。

Andy Rubin 先后在苹果 微软 谷歌公司工作过。

##3. .NET Native的背水一战    
说起.NET 就不得不谈到[Anders Hejlsberg](http://zh.wikipedia.org/wiki/%E5%AE%89%E5%BE%B7%E6%96%AF%C2%B7%E6%B5%B7%E5%B0%94%E6%96%AF%E4%BC%AF%E6%A0%BC)此人，他来自丹麦，Turbo Pascal最开始就是他开发的，Delphi/C#之父，C#&.NET的首席架构师，TypeScript的首席架构师，主持开发了.NET Framework，Visual Basic.NET，以及最新的.NET编译器[Roslyn](http://msdn.microsoft.com/en-us/vstudio/roslyn.aspx) 。   
值得注意的是TypeScript 完全基于ECMAScript 6标准草案开发，
Java的流行以至于微软也坐不住，在上个世纪末，微软也开发了自己的Java虚拟机，最初微软推出的是Visual J++，而在Anders加入微软后立即被委以重任，Visual J++在性能上甚至超越了Sun JVM，这个Sun带来了恐慌，Sun以破坏兼容性将微软告上公堂，微软最终放弃了Java的开发，而C#与.NET也诞生了，.NET在设计上确实借鉴了Java的很多理念，并且超越了Java，这也是Anders从Borland就存在心中的构想。


>*Phoenix Compiler and Shared Source Common Language Infrastructure*

Chris Lattner 曾于2004年在微软研究院实习，参与微软的[Phoenix Compiler Framework](http://research.microsoft.com/en-us/collaboration/focus/cs/phoenix.aspx) 项目，在我刚进入大学的时候，刚刚学会编程，曾经下载过08版的Phoenix Compiler编译器工具，并且也试用过，不过到现在已经无法下载了。而Phoenix Compiler Framework与LLVM的理念确实很相似，并且可以得知的是，Phoenix很多的技术被整合到微软的Microsoft C/C++ Compiler，就技术上而言Phoenix与LLVM有许多相似之处。 
>Phoenix不仅仅限于一个编译器，它还是一个软件优化和分析框架，能被其他编译器和工具使用。 它能生成二进制代码，也能输出MSIL程序集。源代码可以经过分析，并被表示为IR（中间表示，Intermediate Representation）形式，这种形式可以在后期被各种工具分析和处理。    
                                                         ----InfoQ: [Phoenix编译器框架说明](http://www.infoq.com/cn/news/2008/05/Phoenix-Compiler-Framework)     

微软研究院还提供了一个“[Shared Source Common Language Infrastructure](http://www.microsoft.com/en-us/download/details.aspx?id=4917)”的源代码下载。

![DotCLR](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/dotNet/CLR_diag.png)

![DotNetCoreCLR](https://raw.githubusercontent.com/fcharlie/site-res/master/compilers/dotnativecoreclr.png)

####.NET Compiler Platform ("Roslyn")

![Images](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/Roslyn.png )           
                                             


放心 Roslyn是开源的基于C#的，Mono会移植到其他平台的。
####.NET Native
早在2013年就有传闻，.NET将推出.NET Native,.NET本就有一个NGEN工具，负责将.NET程序集一股脑的生成本机镜像。但NGEN依然无法脱离.NET平台，并且有大量的JIT，小型程序一般不会出现严重的性能问题，但是，当项目体积变得巨大时，类似于Visual Studio之类的工具，程序启动就会非常缓慢。必要的优化显得尤为重要。
>App IL + FX -> MCG　-> Interop.g.cs -> CSC -> Interop.dll -> Merge -> IL transform -> NUTC -> RhBind -> .EXE

.NET Native的实现，在IR前期很大的程度上依赖Roslyn这类新型的编译器，而在IR后期，就得意于Phoenix编译器框架，.NET Native后端和Visual C/C++共用一套后端优化编译器。


或许对于微软来说，应该感到遗憾，Chris Lattner 并没有最终加入微软，而是加入了苹果公司。

##4. 折腾，永不止步  

####4.1 CSNative
永远不会有完全统一的意见，总会有人去创造新的轮子。不谈其他，重复的创造能对已有的东西带来技术革新，在[codeplex.com](http://csnative.codeplex.com/)上,就有个伙计利用Roslyn API将C#编译成MSIL，然后将MSIL编译成LLVM IR,随后'LLVM System compiler' llc编译成Native code,用GCC将Object文件链接成exe，GC库是32位的 [libgc](http://www.hboehm.info/gc/)


####4.2 SharpLang
同样的，在Githu上，也有一个基于LLVM的C# Native的解决方案:[SharpLang](https://github.com/xen2/SharpLang)。在LLILC推出后，开发者 Virgile Bello 也就没有更新 SharpLang 了。


####.NET
实际上无论是JVM还是.NET Framework 已经LLVM Framework在结构上是非常相似的，如下图：     
![Framework](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/dotNet/CLR_diag.png)    
                                        

Developer History:     

![dotNet](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/dotNet/DotNet.png)      
                                   


![Step](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/dotNet/Overview_of_the_Common_Language_Infrastructure.png)         
                                                        

####JVM   

![JVM](https://raw.githubusercontent.com/fstudio/Beaot/master/doc/Images/JvmSpec7.png)        
                                                      

####ECMAScript PNacl Asm.js WebAssembly

[asm.js AOT](https://blog.mozilla.org/luke/2014/01/14/asm-js-aot-compilation-and-startup-performance/)  

![asm.js-AOT](https://ffp4g1ylyit3jdyti1hqcvtb-wpengine.netdna-ssl.com/luke/files/2013/12/aot-diagram.png)  

####LLILC
在.NET CoreCLR开源后，.NET开发团队也创建了基于LLVM的.NET Core编译器项目，包括JIT和AOT,不过目前AOT并没有编码实现。  

![AOT](https://raw.githubusercontent.com/dotnet/llilc/master/Documentation/Images/AOTArch.png)


##备注  
1. Embarcadero C++ Builder:    
BCC64.EXE, the C++ 64-bit Windows Compiler:     
[http://docwiki.embarcadero.com/RADStudio/XE6/en/BCC64.EXE,_the_C%2B%2B_64-bit_Windows_Compiler](http://docwiki.embarcadero.com/RADStudio/XE6/en/BCC64.EXE,_the_C%2B%2B_64-bit_Windows_Compiler)   
Clang-based C++ Compilers:     
[http://docwiki.embarcadero.com/RADStudio/XE6/en/Clang-based_C%2B%2B_Compilers](http://docwiki.embarcadero.com/RADStudio/XE6/en/Clang-based_C%2B%2B_Compilers)
2. Phoenix Compiler Framework Wiki:
[http://en.wikipedia.org/wiki/Phoenix_(compiler_framework)](http://en.wikipedia.org/wiki/Phoenix_(compiler_framework))           
3. Dalvik Wiki:
[http://en.wikipedia.org/wiki/Dalvik_(software)](http://en.wikipedia.org/wiki/Dalvik_(software))        
 
