---
layout: post
title:  "Windows Containers 初窥 (一)"
date:   2016-04-23 18:30:00
published: true
categories: container
---

# Windows Containers

随着 Windows 10 Redstone 和 Windows Server 2016 发布日益临近,一些重要的功能也逐渐披露到公众面前。
比如 Windows Container ，Container 功能应该是 Windows 8 AppContainer 以来，Windows 应用隔离技术的的一个大的补充。

从 Windows 8 开始，Windows 推出了 Store App, Store App 运行在一个 AppContainer 中， 操作系统通过 AppContainer 技术
实现了进程权限的隔离，由于 UAC 的限制，App 无法访问大多数的文件系统目录，也不能进行一些重要的操作，许多系统对进程 UAC 
权限存在依赖的 API 也无法被直接调用。一些系统的 Store App 只得依赖各种 Runtime Broker 通过 LPC 来实现重要的操作。

对于 Store App 来说，使用全新的 API 和不同以往的安全策略，在 AppContainer 中运行恰到好处，而 Win32 App 同样可以运行在
AppContainer 中 (在以前的文章中，我曾经写过一个如何启动基于 Windows AppConatiner 的传统 Win32 应用程序：
[Windows AppContainer 降权，隔离与安全](http://forcemz.net/container/2015/06/12/AppContainer/) )，不过，依赖传统配置和文件系统的应用程序如若使用 AppConatiner ，大量的功能将无法使用，这就使得 Windows 面临
一个困境，经历二十多年的积累，Win32 应用的存量已经非常可观，然而 Win32 应用却无法在 Store 中运行，Store 中 App 稀少。



在 Windows 10 Redstone Insider 14316 中，只存在于 Windows Server 2016 TP 的 Container 作为一个可选的功能被添加到了 企业版镜像中，
在 Windows 10 Redstone Insider 14328 中，Container 被添加到专业版功能当中。笔者才有幸去了解其中的奥秘。




## Windows Container 窥视

对于一个合格的 容器技术来说，既要做到开箱即用又要做到移除无痕，并且能够对应用的资源做出一个合理的限制和隔离。

从 Office 2013 起，Office 的安装采用 VFS 机制

在 Linux 中，Linux containers 的实现离不开 Namespace， 
对于 Windows Container ，Microsoft 目前公布的资料非常少，通过各种报道和 'Drawbridge'

C:\Windows\System32\containers

Windows Container Credential Guard Server
CCG.exe CCGLaunchPad.dll

在 WOW64 系统中，同样存在 Container.dll ，说明 Container 技术在 Win32 中是存在的。

## 附注

以下是 container.dll 和 containerxml.dll 的导出函数
{% highlight text %}
// container.dll
void container::AddRuntimeVirtualKeysToContainer(void *,unsigned long,struct _WC_VKEY_INFO *)
void container::CleanupContainer(void *,unsigned short const *)
void container::ContainerDescription::Clear(void)
container::ContainerDescription::ContainerDescription(void)
void container::CreateContainer(void *,class container::ContainerDescription const &,bool)
class container::Node & container::Filesystem2::Identifier(struct _GUID)
class container::Node & container::Filesystem2::Layer(void)
class container::Node & container::Filesystem2::Path(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Filesystem::AccessMask(unsigned int)
class container::Node & container::Filesystem::DeleteSemantics(bool)
class container::Node & container::Filesystem::Directives(void)
class container::Node & container::Filesystem::Directory(void)
class container::Node & container::Filesystem::File(void)
class container::Node & container::Filesystem::Junction(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Filesystem::Merge(void)
class container::Node & container::Filesystem::Name(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Filesystem::Path(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Filesystem::Permit(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Filesystem::ShortName(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Filesystem::Suppress(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Filesystem::Volume(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
void * container::GetComRegistryRoot(void *)
class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > container::GetContainerIdentifierString(void *)
void container::GetContainerObjectRootPath(void *,class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > &)
class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > container::GetCurrentVolume(void)
void container::GetRegistryRootPath(void *,class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &,class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > &)
unsigned char container::IsContainerQuiescent(void *)
class container::Node & container::Job::Cpu(void)
class container::Node & container::Job::Limit(unsigned __int64)
class container::Node & container::Job::Memory(void)
class container::Node & container::Job::Rate(unsigned int)
class container::Node & container::Job::Weight(unsigned int)
void * container::LaunchApplicationContainer(void *,unsigned short const *)
void container::LaunchContainer(void *,void *)
void container::LogException(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &,class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &,unsigned long,long)
class container::Node & container::Network::Compartment(void)
container::Node::~Node(void)
class container::Node & container::ObjectManager::CloneSd(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::ObjectManager::Dir(void)
class container::Node & container::ObjectManager::Name(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::ObjectManager::Path(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::ObjectManager::Scope(enum container::ObjectManager::SymlinkScope)
class container::Node & container::ObjectManager::ShadowDirectory(enum container::ObjectManager::ShadowOption)
class container::Node & container::ObjectManager::Symlink(void)
struct _WC_CONTAINER_NOTIFICATION * container::RegisterForContainerTerminationNotification(void *,void (*)(void *,enum _WC_CONTAINER_TERMINATION_REASON,struct _WC_CONTAINER_NOTIFICATION *,void *),void *)
class container::Node & container::Registry2::AccessMask(unsigned int)
class container::Node & container::Registry2::ContainerPath(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry2::FilePath(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry2::Hive(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry2::HiveStack(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry2::HiveStack(void)
class container::Node & container::Registry2::HostHive(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry2::HostPath(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry2::Identifier(struct _GUID)
class container::Node & container::Registry2::Key(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry2::Layer(void)
class container::Node & container::Registry2::ReadOnly(bool)
class container::Node & container::Registry2::RedirectionNode(void)
class container::Node & container::Registry2::Symlink(void)
class container::Node & container::Registry2::Target(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::AccessMask(unsigned int)
class container::Node & container::Registry::Alias(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::CloneSd(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::CopyOnWritePolicy(void)
class container::Node & container::Registry::Data(class std::vector<unsigned char,class std::allocator<unsigned char> > const &)
class container::Node & container::Registry::Data(unsigned long)
class container::Node & container::Registry::Data(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::DeleteSemantics(bool)
class container::Node & container::Registry::Directives(void)
class container::Node & container::Registry::Flags(unsigned int)
class container::Node & container::Registry::Junction(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::Key(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::Key(void)
class container::Node & container::Registry::LinkHive(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::Load(void)
class container::Node & container::Registry::Merge(void)
class container::Node & container::Registry::MkKey(void)
class container::Node & container::Registry::MkValue(void)
class container::Node & container::Registry::MultiDelimitedStringData(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::MultiStringData(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::Name(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::Path(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::Permit(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::ReadOnly(bool)
class container::Node & container::Registry::Suppress(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::Symlink(void)
class container::Node & container::Registry::Target(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Registry::TransactionsDisabled(bool)
void container::ReleaseContainerTerminationNotification(struct _WC_CONTAINER_NOTIFICATION *)
void container::RemoveRuntimeVirtualKeysFromContainer(void *,unsigned long,struct _WC_VKEY_INFO *)
class std::vector<class container::Node *,class std::allocator<class container::Node *> > container::SelectChildren(class container::Node const &,bool,bool)
void container::SetRegistryFlushState(void *,unsigned char)
class container::Node & container::Setup::CloneSd(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Setup::Data(unsigned long)
class container::Node & container::Setup::Data(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Setup::MkDir(void)
class container::Node & container::Setup::MkHiv(void)
class container::Node & container::Setup::MkKey(void)
class container::Node & container::Setup::MkValue(void)
class container::Node & container::Setup::MultiDelimitedStringData(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Setup::MultiStringData(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
class container::Node & container::Setup::Name(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >)
void container::ThrowIfBasePackageHostMismatch(struct _WC_PACKAGE_LAYER *,unsigned long)
bool container::Visit(class container::Node const &,class std::function<bool (class container::Node const &,int)>,int)
void container::WaitForContainerTerminationNotification(struct _WC_CONTAINER_NOTIFICATION *)
WcAddRuntimeVirtualKeysToContainer
WcCleanupContainer
WcCreateContainer
WcGetComRegistryRoot
WcIsContainerQuiescent
WcLaunchApplicationContainer
WcLaunchContainer
WcRegisterForContainerTerminationNotification
WcReleaseContainerTerminationNotification
WcRemoveRuntimeVirtualKeysFromContainer
WcSetRegistryFlushState
WcWaitForContainerTerminationNotification
// containerxml.dll
void container::CreateDescriptionFromXml(void *,unsigned short const * *,unsigned long,bool,class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &,struct _WC_PACKAGE_LAYER *,unsigned long,class container::ContainerDescription &)
void container::xml::ReadFromXml(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &,bool,class container::xml::Listener *)
WcCreateDescriptionFromXml
WcDestroyDescription
{% endhighlight %}

默认情况下， wchar_t 在 Visual C++ 中是一种内置类型，std::wstring CharT 类型为 wchar_t,而在这些导出符号中，则是 unsigned short 。


## Resources

+ [windocks](http://www.windocks.com/)
+ [Quick Nano Server PXE boot demo](https://channel9.msdn.com/Blogs/Regular-IT-Guy/Quick-Nano-Server-PXE-boot-demo/player)
+ [Library OS PDF](http://research.microsoft.com/pubs/141071/asplos2011-drawbridge.pdf)
+ [Graphene Library OS - Github](https://github.com/oscarlab/graphene)