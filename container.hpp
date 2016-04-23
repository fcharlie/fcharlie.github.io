//
//
//
#pragama once

namespace container {
void AddRuntimeVirtualKeysToContainer(HANDLE lpContainer, DWORD flags,_WC_VKEY_INFO *vkey);
void CleanupContainer(HANDLE lpContainer, LPCWSTR containerName);
class ContainerDescription {
public:
  ContainerDescription(void);
  void Clear(void);
};
void CreateContainer(HANDLE lpContainer, const ContainerDescription &des,
                     bool bv);
class Node {};
class FileSystem2{
  public:
  Node &Identifier(GUID );
  Node &Layer(void);
  Node &Path(std::wstring);
};
class FileSystem{};
}

extern "C"{
    void WINAPI WcAddRuntimeVirtualKeysToContainer(HANDLE lpContainer,DWORD flags,_WC_VKEY_INFO *vkey);
    void WINAPI WcCleanupContainer(HANDLE lpContainer,LPCWSTR containerName);
}