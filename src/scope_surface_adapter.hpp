#pragma once
#include <d3d9.h>
#include <atomic>

// Private presentation candidate. The native scope renderer draws into the
// back buffer and copies that image into its lens texture before returning.
// Preserve the caller's colour surface across that transaction. Do not change
// its camera, lens mask, eye assignment, or scope activation policy.
namespace scope_surface_adapter {
inline unsigned char* base=nullptr;
inline std::atomic<bool> enabled{true};
inline std::atomic<unsigned> attempts{0},restored{0},failures{0};
inline std::atomic<unsigned> width{0},height{0},samples{0};
inline std::atomic<long> lastResult{0};
using Render=bool(__cdecl*)(void*,void*,IDirect3DDevice9*,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
template<class T> struct Com {
 T* value=nullptr;
 ~Com(){if(value)value->Release();}
 T** put(){return &value;}
};
inline bool __cdecl render(void* manager,void* context,IDirect3DDevice9* device,
 uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8){
 const auto original=reinterpret_cast<Render>(base+0x3B6020);
 // Avoid any allocation/copy when the native scope isn't active.
 if(!enabled||!device||!base[0xECCD68])return original(manager,context,device,a3,a4,a5,a6,a7,a8);
 ++attempts;
 Com<IDirect3DSurface9> buffer,backup;
 D3DSURFACE_DESC desc{};
 HRESULT status=device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,buffer.put());
 if(SUCCEEDED(status))status=buffer.value->GetDesc(&desc);
 width=desc.Width;height=desc.Height;samples=desc.MultiSampleType;
 // A failed or unsupported copy leaves the original native route untouched.
 // Per-call ownership also prevents a DEFAULT-pool reference blocking Reset.
 if(SUCCEEDED(status))status=device->CreateRenderTarget(desc.Width,desc.Height,
   desc.Format,desc.MultiSampleType,desc.MultiSampleQuality,FALSE,backup.put(),nullptr);
 if(SUCCEEDED(status))status=device->StretchRect(buffer.value,nullptr,backup.value,nullptr,D3DTEXF_NONE);
 const bool saved=SUCCEEDED(status);
 const bool result=original(manager,context,device,a3,a4,a5,a6,a7,a8);
 if(saved)status=device->StretchRect(backup.value,nullptr,buffer.value,nullptr,D3DTEXF_NONE);
 lastResult=status;
 if(SUCCEEDED(status))++restored;else ++failures;
 return result;
}
inline bool install(unsigned char* module){
 if(base)return true;
 const unsigned char expected[]={0xE8,0x20,0x2E,0xFF,0xFF};
 const unsigned char prologue[]={0x81,0xEC,0x9C,0,0,0,0x56};
 if(!module||memcmp(module+0x3C31FB,expected,5)||memcmp(module+0x3B6020,prologue,7))return false;
 DWORD protection{};
 if(!VirtualProtect(module+0x3C31FB,5,PAGE_EXECUTE_READWRITE,&protection))return false;
 base=module;
 const int rel=int(reinterpret_cast<unsigned char*>(&render)-(module+0x3C3200));
 memcpy(module+0x3C31FC,&rel,4);
 FlushInstructionCache(GetCurrentProcess(),module+0x3C31FB,5);
 DWORD ignored{};VirtualProtect(module+0x3C31FB,5,protection,&ignored);
 return true;
}
}
