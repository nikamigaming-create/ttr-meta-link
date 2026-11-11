#pragma once
// Exact TTWVR build adapters; included inside LocalMetaLink implementation namespace.
// All original-mod RVAs and instruction guards live here, not in the seated addon.
void patchTtrApi(){
 if(!metaReady||apiPatched||apiRefused)return;
 auto module=GetModuleHandleW(L"TTWVR.dll");if(!module)return;
 if(!build_adapter::verifiedTTWVR(module)){apiRefused=true;log("API REFUSED: unsupported TTWVR file hash");return;}
 // Exact build 6AF40B94: two immediate stores construct XrVersion 1.0.49.
 // Meta 208's feature gate ignores the minor version and activates its
 // nominal 1.1.49 aim change without the matching 1.1.53 rotation change.
 // Matched probes: 1.0.49 => 177 degrees; 1.1.59 => corrected ~60 degrees.
 // Verified installed Meta reports API 1.1.62. Requesting 1.1.59 enables both
 // aim fixes and the later PC stage-pose/visibility fixes. Extension negotiation
 // and the native controller/eye transforms remain unchanged.
 auto* context=reinterpret_cast<unsigned char*>(module)+0x6144F5;
 const unsigned char expected[]={0xC7,0x85,0x44,0xFE,0xFF,0xFF,0x31,0,0,0,0xC7,0x85,0x48,0xFE,0xFF,0xFF,0,0,1,0};
 if(memcmp(context,expected,sizeof(expected))){apiRefused=true;log("API REFUSED: XrVersion instruction guard mismatch");return;}
 DWORD old{};
 if(!VirtualProtect(context,sizeof(expected),PAGE_EXECUTE_READWRITE,&old)){apiRefused=true;log("API REFUSED: protection error=%lu",GetLastError());return;}
 context[6]=0x3B;context[16]=0x01;
 BOOL flushed=FlushInstructionCache(GetCurrentProcess(),context,sizeof(expected));DWORD ignored{};BOOL restored=VirtualProtect(context,sizeof(expected),old,&ignored);
 apiPatched=true;
 log("API APPLIED: TTWVR verified hash E428C4BD1209D58D21F75108808C86049E26C76E58A164AD864A61B99696BDAB RVA=0x6144FB/0x614505 request 1.0.49 -> 1.1.59 flush=%d protectionRestored=%d; native poses/bindings unchanged",flushed,restored);
}
void patchTtrProjection(){
 if(!metaReady||!apiPatched||projectionHandled)return;
 projectionHandled=true;auto module=GetModuleHandleW(L"TTWVR.dll");
 if(!module||!build_adapter::verifiedTTWVR(module)){log("FOV REFUSED: unsupported TTWVR hash");return;}
 auto* base=reinterpret_cast<unsigned char*>(module);auto* site=base+0x3B4A59;auto* target=base+0x3B4AD7;
 const unsigned char expected[]={0x0F,0x2F,0xD8,0x76,0x45,0x0F,0x28,0xC5};
 const unsigned char prefix[]={0x0F,0x5A,0xC6};
 const unsigned char stores[][4]={{0xF3,0x0F,0x11,0x35},{0xF3,0x0F,0x11,0x15},{0xF3,0x0F,0x11,0x25},{0xF3,0x0F,0x11,0x0D}};
 bool valid=!memcmp(site,expected,8)&&!memcmp(target,prefix,3);
 for(unsigned i=0;i<4&&valid;++i){u32 address{};memcpy(&address,target+7+8*i,4);valid=!memcmp(target+3+8*i,stores[i],4)&&address==reinterpret_cast<uintptr_t>(base+0xECD1FC+4*i);}
 if(!valid){log("FOV REFUSED: branch/destination instruction guard mismatch");return;}
 // Meta reports fovMutable=0. Bypass aspect-driven expansion BEFORE camera
 // publication, preserving the original XR tangents for rendering, replay,
 // capture and submission. Do not rewrite submission metadata alone.
 const unsigned char replacement[]={0xE9,0x79,0,0,0,0x90,0x90,0x90};DWORD old{};
 if(!VirtualProtect(site,8,PAGE_EXECUTE_READWRITE,&old)){log("FOV REFUSED: protection error=%lu",GetLastError());return;}
 memcpy(site,replacement,8);BOOL flushed=FlushInstructionCache(GetCurrentProcess(),site,8);DWORD ignored{};BOOL restored=VirtualProtect(site,8,old,&ignored);
 log("FOV APPLIED: verified TTWVR RVA=0x3B4A59 -> 0x3B4AD7; original OpenXR FOV used by camera/replay/submission; flush=%d protectionRestored=%d",flushed,restored);
}
