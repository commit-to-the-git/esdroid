#include "../include/os_utilities.h"
#include "../include/path.h"
#include <string>
#if defined(__ANDROID__)
  #include <unistd.h>
  #include <sys/stat.h>
  extern "C" const char *esdroid_get_files_dir();
#else
  #define NOMINMAX
  #include <Windows.h>
#endif
dbasic::Path dbasic::GetModulePath() {
#if defined(__ANDROID__)
    const char *dir=esdroid_get_files_dir();
    if(dir==nullptr||dir[0]==0) return dbasic::Path(std::wstring());
    std::wstring wdir;
    for(const char *p=dir;*p;++p) wdir.push_back((wchar_t)(unsigned char)*p);
    return dbasic::Path(wdir);
#else
    wchar_t path[MAX_PATH];
    DWORD result=GetModuleFileName(NULL,path,MAX_PATH);
    Path fullPath=Path(path); Path parentPath;
    fullPath.GetParentPath(&parentPath);
    return parentPath;
#endif
}
