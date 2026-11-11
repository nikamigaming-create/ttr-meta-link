#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include "../build_adapter.hpp"
#include "../src/scope_surface_adapter.hpp"
namespace {
using u32=uint32_t;
bool metaReady=true,apiPatched=false,apiRefused=false,projectionHandled=false;
void log(const char* fmt,...){va_list a;va_start(a,fmt);vprintf(fmt,a);va_end(a);puts("");}
#include "../src/ttwvr_meta_adapter.hpp"
#include "../src/ttwvr_shadow_adapter.hpp"
}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;
 auto m=LoadLibraryExW(argv[1],nullptr,DONT_RESOLVE_DLL_REFERENCES);
 if(!m||!build_adapter::verifiedTTWVR(m))return 3;
 patchTtrApi();if(!apiPatched||apiRefused)return 4;
 patchTtrProjection();auto b=reinterpret_cast<unsigned char*>(m);
 if(b[0x3B4A59]!=0xE9||b[0x6144FB]!=0x3B||b[0x614505]!=1)return 5;
 shadowsRequested=true;patchTtrShadows();
 if(*reinterpret_cast<float*>(b+0x9F9F58)!=1.0f)return 6;
 if(!scope_surface_adapter::install(b))return 7;
 puts("PASS exact TTR1.1 mapped-image API/FOV/shadow/scope guards and patches; no TTR code executed");
 return 0;
}
