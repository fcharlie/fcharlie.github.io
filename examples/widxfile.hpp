#ifndef WIDXFILE_HPP
#define WIDXFILE_HPP
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include <climits>
#include <Windows.h>

namespace git{
struct ObjectIndex {
  /// DON't Modify
  bool operator<(const ObjectIndex &o) { return offset > o.offset; }
  uint32_t offset{0};
  uint32_t index{0};
};

/// DON't adjust layout
struct ObjectIndexLarge {
  /// DON't Modify
  bool operator<(const ObjectIndexLarge &o) { return offset > o.offset; }
  uint64_t offset{0};
  uint32_t index{0};
};
enum GitidxValue { MaxNumberOfDetails = 7 };

struct FileInfo {
  std::wstring path;
  off64_t size;
};

struct Wfs {
  std::vector<FileInfo> files;
  std::size_t counts{0};
  std::size_t limits{MaxNumberOfDetails};
};

inline wchar_t *sha1_to_hex_r(wchar_t *buffer, const unsigned char *sha1) {
  static const char hex[] = "0123456789abcdef";
  wchar_t *buf = buffer;
  int i;
  for (i = 0; i < 20; i++) {
    unsigned int val = *sha1++;
    *buf++ = hex[val >> 4];
    *buf++ = hex[val & 0xf];
  }
  *buf = '\0';
  return buffer;
}
inline const wchar_t *Sha1FromIndex(FILE *fp, wchar_t *buf, std::uint32_t i) {
  unsigned char sha1__[20];
  constexpr int offsetbegin = 4 + 4 + 4 + 255 * 4;
  fseek(fp, offsetbegin + i * 20, SEEK_SET);
  if (fread(sha1__, 1, 20, fp) != 20) {
    return L"unkown";
  }
  sha1_to_hex_r(buf, sha1__);
  return buf;
}

inline size_t Filesize(const std::wstring &file) {
    auto hFile=CreateFileW(file.c_str(),GENRIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hFile==INVALID_HANDLEVALUE){
        return 0;
    }
    LARGE_INTEGER fsz; 
    GetFileSizeEx(hFile, &fsz); 
    CloseHandle(hFile);
  return fsz.QuadPart;
}

class Gitidx {
public:
  Gitidx(const Gitidx &) = delete;
  Gitidx &operator=(const Gitidx &) = delete;
  Gitidx(Wfs &wfs_) : wfs(wfs_) {}
  ~Gitidx() {
    if (fp != nullptr) {
      fclose(fp);
    }
  }
  bool verify(const std::wstring &pkfile, size_t pksz = 0); /// open and verify
  bool review(std::size_t limitsize, std::size_t warnsize);

private:
  bool reviewsmall(std::size_t limitsize, std::size_t warnsize);
  bool reviewlarge(std::size_t limitsize, std::size_t warnsize);
  FILE *fp{nullptr};
  std::size_t idxsize{0};
  std::size_t pksize{0};
  Wfs &wfs;
  std::uint32_t nr{0};
  std::uint32_t lnr{0};
};

};

#endif