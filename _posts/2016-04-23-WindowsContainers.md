---
layout: post
title:  "Windows Containers 初窥 (一)"
date:   2016-04-23 21:30:16
published: false
categories: builder
---

# Windows Containers

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