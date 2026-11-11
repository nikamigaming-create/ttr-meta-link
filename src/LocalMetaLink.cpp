#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
#include "../nvse_api.hpp"
#include "../build_adapter.hpp"
#include "scope_surface_adapter.hpp"
#pragma comment(lib,"bcrypt.lib")

namespace {
HMODULE selfModule{}, retainedIpc{};
bool metaReady{}, apiPatched{}, apiRefused{};
bool projectionHandled{},scopeGuardRequested{},scopeGuardHandled{};
std::wstring logPath;
constexpr wchar_t section[]=L"LocalMetaLink";
constexpr wchar_t verifiedMetaManifest[]=L"C:\\Program Files\\Oculus\\Support\\oculus-runtime\\oculus_openxr_32.json";
constexpr char expectedHash[]="d072611e9ba99efc51fa5c9aad9eecb6e1ba9fdda8069ec96142241f75a57531";
constexpr size_t patchRva=0xE9474;
void log(const char* format,...) {
 FILE* file{}; if(logPath.empty()||_wfopen_s(&file,logPath.c_str(),L"a"))return;
 va_list args;va_start(args,format);vfprintf(file,format,args);va_end(args);fputc('\n',file);fclose(file);
}
std::wstring fullPath(const std::wstring& path) {
 if(path.size()<3||!((path[1]==L':'&&(path[2]==L'\\'||path[2]==L'/'))||(path[0]==L'\\'&&path[1]==L'\\')))return {};
 wchar_t value[32768]{};DWORD size=GetFullPathNameW(path.c_str(),32768,value,nullptr);
 return size&&size<32768?std::wstring(value):std::wstring{};
}
std::wstring setting(const std::wstring& ini,const wchar_t* key,const wchar_t* fallback) {
 wchar_t value[32768]{};GetPrivateProfileStringW(section,key,fallback,value,32768,ini.c_str());return fullPath(value);
}
bool patch(const std::wstring& path) {
 HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE){log("REFUSED: cannot open IPC DLL error=%lu",GetLastError());return false;}
 LARGE_INTEGER size{};bool readOk=GetFileSizeEx(file,&size)&&size.QuadPart==3106040;
 std::vector<unsigned char> data(readOk?static_cast<size_t>(size.QuadPart):0);
 DWORD count{};if(readOk)readOk=ReadFile(file,data.data(),static_cast<DWORD>(data.size()),&count,nullptr)&&count==data.size();
 if(!readOk){CloseHandle(file);log("REFUSED: IPC file size/read guard");return false;}
 BCRYPT_ALG_HANDLE algorithm{};unsigned char digest[32]{};
 NTSTATUS status=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0);
 if(status>=0){status=BCryptHash(algorithm,nullptr,0,data.data(),static_cast<ULONG>(data.size()),digest,32);BCryptCloseAlgorithmProvider(algorithm,0);}
 char hash[65]{};for(unsigned i=0;i<32;++i)sprintf_s(hash+i*2,3,"%02x",digest[i]);
 if(status<0||strcmp(hash,expectedHash)){CloseHandle(file);log("REFUSED: SHA256=%s expected=%s status=%08lx",hash,expectedHash,status);return false;}
 const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(data.data());
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||static_cast<size_t>(dos->e_lfanew)>data.size()-sizeof(IMAGE_NT_HEADERS32)){CloseHandle(file);log("REFUSED: DOS/PE bounds");return false;}
 const auto* pe=reinterpret_cast<const IMAGE_NT_HEADERS32*>(data.data()+dos->e_lfanew);
 if(pe->Signature!=IMAGE_NT_SIGNATURE||pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||pe->OptionalHeader.SizeOfImage<patchRva+3){CloseHandle(file);log("REFUSED: expected PE32 x86 image");return false;}
 if(GetModuleHandleW(L"RuntimeIPCServiceClient_32.dll")){CloseHandle(file);log("REFUSED: IPC DLL already loaded; initialization order not safe");return false;}
 HMODULE module=LoadLibraryExW(path.c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 CloseHandle(file);
 if(!module){log("REFUSED: IPC preload error=%lu",GetLastError());return false;}
 wchar_t actual[32768]{};DWORD length=GetModuleFileNameW(module,actual,32768);
 if(!length||length>=32768||_wcsicmp(fullPath(actual).c_str(),path.c_str())){log("REFUSED: loaded IPC module path mismatch");FreeLibrary(module);return false;}
 auto* code=reinterpret_cast<unsigned char*>(module)+patchRva;
 const unsigned char expected[]{0x83,0xC4,0x08};
 if(memcmp(code,expected,3)){log("REFUSED: RVA 0x%zX expected 83 C4 08; actual %02X %02X %02X",patchRva,code[0],code[1],code[2]);FreeLibrary(module);return false;}
 DWORD protection{};
 if(!VirtualProtect(code,3,PAGE_EXECUTE_READWRITE,&protection)){log("REFUSED: VirtualProtect error=%lu",GetLastError());FreeLibrary(module);return false;}
 memset(code,0x90,3);
 const BOOL flushed=FlushInstructionCache(GetCurrentProcess(),code,3);
 DWORD unused{};const BOOL restored=VirtualProtect(code,3,protection,&unused);
 // Keep the module alive after modifying its code, including diagnostic failures.
 retainedIpc=module;
 log("APPLIED: SHA256=%s RVA=0x%zX 83 C4 08 -> %02X %02X %02X flush=%d protectionRestored=%d (process memory only)",hash,patchRva,code[0],code[1],code[2],flushed,restored);
 return flushed&&restored;
}
bool supported(const NVSE* nvse){return nvse&&!nvse->isEditor&&nvse->runtime==0x040020D0&&nvse->version>=6;}
#include "ttwvr_meta_adapter.hpp"
#include "ttwvr_shadow_adapter.hpp"
void __cdecl onMessage(Message* message){
 if(message&&(message->type==PostLoad||message->type==PostPostLoad||message->type==DeferredInit)){
  patchTtrShadows();patchTtrApi();patchTtrProjection();
  if(message->type==DeferredInit&&metaReady&&apiPatched&&scopeGuardRequested&&!scopeGuardHandled){
   scopeGuardHandled=true;auto b=build_adapter::heldModule();
   log("SCOPE colour isolation: %s",b&&scope_surface_adapter::install(b)?"installed (native lens capture preserved)":"refused build/call guard");
  }
 }
 if(message&&message->type==ExitGame&&scopeGuardHandled)
  log("SCOPE restores=%u attempts=%u failures=%u lastHRESULT=%08lX surface=%ux%u samples=%u",scope_surface_adapter::restored.load(),scope_surface_adapter::attempts.load(),scope_surface_adapter::failures.load(),scope_surface_adapter::lastResult.load(),scope_surface_adapter::width.load(),scope_surface_adapter::height.load(),scope_surface_adapter::samples.load());
}
}

BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){selfModule=module;DisableThreadLibraryCalls(module);}return TRUE;}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Query(const NVSE* nvse,PluginInfo* info){
 if(!info)return false;info->infoVersion=1;info->name="LocalMetaLink";info->version=1;return supported(nvse);
}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Load(const NVSE* nvse){
 if(!supported(nvse))return false;
 wchar_t path[32768]{};DWORD length=GetModuleFileNameW(selfModule,path,32768);if(!length||length>=32768)return false;
 std::wstring own(path);const size_t slash=own.find_last_of(L"\\/");if(slash==std::wstring::npos)return false;
 const auto directory=own.substr(0,slash+1);const auto ini=directory+L"LocalMetaLink.ini";logPath=directory+L"LocalMetaLink.log";
 log("LOAD: LocalMetaLink 1 runtime=%08X xNVSE=%u PID=%lu",nvse->runtime,nvse->version,GetCurrentProcessId());
 if(!GetPrivateProfileIntW(section,L"Enabled",0,ini.c_str())){log("DISABLED: Enabled=0 (default)");return true;}
 scopeGuardRequested=GetPrivateProfileIntW(section,L"ScopeSurfaceGuard",0,ini.c_str())!=0;
 shadowsRequested=GetPrivateProfileIntW(section,L"NativeShadows",0,ini.c_str())!=0;
 auto messaging=static_cast<Messaging*>(nvse->queryInterface(2));
 if(messaging)log("API/graphics listener registered=%d",messaging->listen(nvse->getPluginHandle(),"NVSE",onMessage));
 patchTtrShadows();
 const auto manifest=setting(ini,L"RuntimeJson",verifiedMetaManifest);
 const auto ipc=setting(ini,L"IpcDll",L"C:\\Program Files\\Oculus\\Support\\oculus-runtime\\RuntimeIPCServiceClient_32.dll");
 if(manifest.empty()||ipc.empty()){log("REFUSED: configuration paths must be absolute");return true;}
 // This private build supports this verified local Meta installation only.
 // A configurable path must never relabel a simulator or SteamVR manifest as Meta.
 if(_wcsicmp(manifest.c_str(),verifiedMetaManifest)){log("REFUSED: RuntimeJson is not the verified local Meta32 manifest: %ls",manifest.c_str());return true;}
 const DWORD required=GetEnvironmentVariableW(L"XR_RUNTIME_JSON",nullptr,0);
 // An empty inherited value needs one wchar including its terminator. MO2
 // preserves empty environment entries; that is not a runtime override.
 const bool explicitRuntime=required>1;
 if(explicitRuntime){
  std::vector<wchar_t> env(required);DWORD count=GetEnvironmentVariableW(L"XR_RUNTIME_JSON",env.data(),required);
  if(!count||count>=required){log("SKIPPED: runtime environment changed while reading");return true;}
  auto selected=fullPath(env.data());
  if(selected.empty()||_wcsicmp(selected.c_str(),manifest.c_str())){log("SKIPPED: explicit XR_RUNTIME_JSON=%ls",env.data());return true;}
 }
 const DWORD attributes=GetFileAttributesW(manifest.c_str());
 if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_DIRECTORY)){log("REFUSED: Meta manifest unavailable: %ls",manifest.c_str());return true;}
 log("SELECTED: Meta manifest=%ls explicitEnvironment=%d IPC=%ls",manifest.c_str(),explicitRuntime,ipc.c_str());
 if(!patch(ipc)){log("REFUSED: compatibility guard failed; runtime environment unchanged");return true;}
 if(!explicitRuntime&&!SetEnvironmentVariableW(L"XR_RUNTIME_JSON",manifest.c_str())){log("ERROR: process-local runtime selection failed error=%lu",GetLastError());return true;}
 metaReady=true;
 patchTtrApi();
 patchTtrProjection();
 log("READY: process-local Meta runtime selected; no registry or installed DLL changes");return true;
}
