#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstring>
#include <vector>
#pragma comment(lib,"bcrypt.lib")
// TTWVR build 6AF40B94 / TTR1.1 only. Verified built-in default entry RVA 7443C8:
// {section="Reload", key="bManual", value="1"}. The cached setting then
// controls the original mod's reload path; we change only this default pointer.
namespace build_adapter {
inline void freeGameString(void* p) {
 // FalloutNV 1.4.0.525, runtime 040020D0; xNVSE GameAPI.cpp FormHeap_Free.
 reinterpret_cast<void(__cdecl*)(void*)>(0x00401030)(p);
}
inline bool verifiedTTWVR(HMODULE m) {
 wchar_t path[MAX_PATH];if(!GetModuleFileNameW(m,path,MAX_PATH))return false;
 FILE* f=nullptr;if(_wfopen_s(&f,path,L"rb"))return false;
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 DWORD objectSize=0,got=0;unsigned char digest[32]{};
 bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&objectSize,sizeof(objectSize),&got,0)>=0;
 std::vector<unsigned char> object(objectSize),buf(65536);
 if(ok)ok=BCryptCreateHash(alg,&hash,object.data(),objectSize,nullptr,0,0)>=0;
 size_t n;while(ok&&(n=fread(buf.data(),1,buf.size(),f))>0)ok=BCryptHashData(hash,buf.data(),static_cast<ULONG>(n),0)>=0;
 if(ferror(f))ok=false;fclose(f);
 if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);
 const unsigned char expected[32]={0xE4,0x28,0xC4,0xBD,0x12,0x09,0xD5,0x8D,0x21,0xF7,0x51,0x08,0x80,0x8C,0x86,0x04,0x9E,0x26,0xC7,0x6E,0x58,0xA1,0x64,0xAD,0x86,0x4A,0x61,0xB9,0x96,0x96,0xBD,0xAB};
 return ok&&memcmp(digest,expected,32)==0;
}
inline const char* disableManualReload() {
 HMODULE m=GetModuleHandleW(L"TTWVR.dll");if(!m)return "UNAVAILABLE: TTWVR module absent";
 if(!verifiedTTWVR(m))return "REFUSED: TTWVR hash differs";
 auto base=reinterpret_cast<unsigned char*>(m);
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
 if(nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt->FileHeader.TimeDateStamp!=0x6AF40B94)return "REFUSED: PE build mismatch";
 struct Entry {const char* section;const char* key;const char* value;};
 auto entry=reinterpret_cast<Entry*>(base+0x82B528);
 if(entry->section!=reinterpret_cast<char*>(base+0x82CF68)||entry->key!=reinterpret_cast<char*>(base+0x82CF70)||entry->value!=reinterpret_cast<char*>(base+0x82C1C4))return "REFUSED: default-table pointer mismatch";
 if(strcmp(entry->section,"Reload")||strcmp(entry->key,"bManual")||strcmp(entry->value,"1"))return "REFUSED: default-table text mismatch";
 static const char automatic[]="0";DWORD old=0;
 if(!VirtualProtect(&entry->value,sizeof(entry->value),PAGE_READWRITE,&old))return "FAILED: default-table protection";
 InterlockedExchangePointer((void* volatile*)(void*)&entry->value,const_cast<char*>(automatic));
 DWORD ignored;VirtualProtect(&entry->value,sizeof(entry->value),old,&ignored);
 return "APPLIED: hash-verified bManual default 1 -> 0 (memory only)";
}
inline const char* testDisableUpscaler() {
 char flag[8]{};if(!GetEnvironmentVariableA("SC_TEST_NO_UPSCALER",flag,sizeof(flag))||strcmp(flag,"1"))return "not requested";
 auto m=GetModuleHandleW(L"TTWVR.dll");if(!m||!verifiedTTWVR(m))return "refused build mismatch";
 struct Entry{const char* section;const char* key;const char* value;};
 auto e=reinterpret_cast<Entry*>(reinterpret_cast<unsigned char*>(m)+0x82B024);
 if(strcmp(e->section,"DLSS")||strcmp(e->key,"bEnabled")||strcmp(e->value,"1"))return "refused table mismatch";
 static const char off[]="0";DWORD old=0;if(!VirtualProtect(&e->value,4,PAGE_READWRITE,&old))return "protection failed";
 InterlockedExchangePointer((void* volatile*)(void*)&e->value,const_cast<char*>(off));DWORD unused;VirtualProtect(&e->value,4,old,&unused);
 return "DLSS bEnabled=0 (test only)";
}
inline const char* requestHandDraw(int hand=1) {
 auto m=GetModuleHandleW(L"TTWVR.dll");
 static HMODULE verified=nullptr;
 if(!m)return "refused module absent";
 if(verified!=m){if(!verifiedTTWVR(m))return "refused build mismatch";verified=m;}
 auto b=reinterpret_cast<unsigned char*>(m);
 // Verified original holster draw transition, RVA 1ED055..1ED0A6:
 // clear the released-weapon latch, choose the requested hand convention,
 // set draw timestamp/pending hand, and reset release timeout.
 *reinterpret_cast<void**>(b+0x14482E8)=nullptr;
 reinterpret_cast<void(__cdecl*)(bool,const char*)>(b+0x1EC130)(hand==0,"SeatedControls selection");
 *reinterpret_cast<ULONGLONG*>(b+0x14482F0)=GetTickCount64();
 *reinterpret_cast<int*>(b+0xEC51B8)=hand;
 *reinterpret_cast<unsigned char*>(b+0x14482C5)=0;
 *reinterpret_cast<float*>(b+0x1448300)=0;
 return "hand draw armed";
}
// Native held-item entry points for the exact hash above. These execute on the
// game's main thread. The selector does not clone aid meshes or consume items.
inline unsigned char* heldModule() {
 auto m=GetModuleHandleW(L"TTWVR.dll");static HMODULE verified=nullptr;
 if(!m)return nullptr;if(verified!=m){if(!verifiedTTWVR(m))return nullptr;verified=m;}
 return reinterpret_cast<unsigned char*>(m);
}
inline void* grabbedItem() {
 auto b=heldModule();if(!b)return nullptr;
 if(b[0x136CF04]&&*reinterpret_cast<int*>(b+0x136CF08)==1)return *reinterpret_cast<void**>(b+0x136CF0C);
 return b[0x136CE35]?*reinterpret_cast<void**>(b+0x136D2F0):nullptr;
}
inline int grabbedHand() {auto b=heldModule();if(!b)return -1;if(b[0x136CF04]&&*reinterpret_cast<int*>(b+0x136CF08)==1)return 1;return b[0x136CE35]?*reinterpret_cast<int*>(b+0xEC33E8):-1;}
inline int stimHand() {auto b=heldModule();return b?*reinterpret_cast<int*>(b+0xECD1BC):-1;}
inline void releaseGrab() {if(auto b=heldModule())reinterpret_cast<void(__cdecl*)(const char*)>(b+0x19DB30)("selector replacement");}
inline void clearStim(void* player) {if(auto b=heldModule())reinterpret_cast<void(__cdecl*)(void*,const char*)>(b+0x39FB40)(player,"selector replacement");}
inline bool stowSelectedGrab(void* player,void* wanted){
 auto b=heldModule();if(!b||!player||!wanted)return false;
 void* first=b[0x136CE35]?*reinterpret_cast<void**>(b+0x136D2F0):nullptr;
 void* second=b[0x136CF04]?*reinterpret_cast<void**>(b+0x136CF0C):nullptr;
 bool primary=first==wanted,secondary=second==wanted;
 if(!primary&&!secondary)return false;
 // Native IsSameGrabbedObject (194E00) recognizes a legitimate two-hand hold.
 // 197479..1974A3 releases both aliases before activating that reference once.
 bool shared=primary&&secondary;
 // The optional slot context may be null in both aliases; native equality permits it.
 if(shared&&*reinterpret_cast<void**>(b+0x136D2F4)!=*reinterpret_cast<void**>(b+0x136CF10))return false;
 // Exact native release-all routine 1930E0 uses this swap/release/swap sequence
 // to address slot two. Use it for the selected slot only, preserving the other.
 const unsigned char swapCode[]={0x81,0xEC,0xCC,0,0,0,0x56,0x57};
 if(memcmp(b+0x1A0710,swapCode,sizeof(swapCode)))return false;
 int otherHand=*reinterpret_cast<int*>(b+0xEC33E8);
 int hand=*reinterpret_cast<int*>(b+(secondary?0x136CF08:0xEC33E8));
 auto swap=reinterpret_cast<void(__cdecl*)()>(b+0x1A0710);
 auto release=reinterpret_cast<void(__cdecl*)(const char*)>(b+0x19DB30);
 if(shared){release("sticky shared item stow");swap();release("sticky shared item stow");swap();}
 else {if(secondary)swap();release("sticky item stow");if(secondary)swap();}
 auto cancelPull=reinterpret_cast<void(__cdecl*)(int,const char*)>(b+0x1966B0);
 cancelPull(hand,"sticky item stow");
 if(shared&&otherHand!=hand)cancelPull(otherHand,"sticky shared item stow");
 if((b[0x136CE35]&&*reinterpret_cast<void**>(b+0x136D2F0)==wanted)||(b[0x136CF04]&&*reinterpret_cast<void**>(b+0x136CF0C)==wanted))return false;
 reinterpret_cast<void(__thiscall*)(void*,void*,int,int,int)>(0x00573170)(wanted,player,0,0,1);
 return true;
}
inline void stowGrab(void* player) {
 auto b=heldModule();if(!b||!player)return;
 void* first=b[0x136CE35]?*reinterpret_cast<void**>(b+0x136D2F0):nullptr;
 void* second=b[0x136CF04]?*reinterpret_cast<void**>(b+0x136CF0C):nullptr;
 // The native dual-hand cleanup swaps/releases both slots and cancels pulls.
 // Releasing only the primary slot can leave an already-stowed ref in slot two.
 reinterpret_cast<void(__cdecl*)(const char*)>(b+0x19DAE0)("selector replacement");
 // Same TESObjectREFR::Activate call used by TTWVR's backpack at RVA 19C4DD.
 if(first)reinterpret_cast<void(__thiscall*)(void*,void*,int,int,int)>(0x00573170)(first,player,0,0,1);
 if(second&&second!=first)reinterpret_cast<void(__thiscall*)(void*,void*,int,int,int)>(0x00573170)(second,player,0,0,1);
}
inline int grabSelected(void* player,const float* head,void* item) {
 auto b=heldModule();if(!b||!player||!head||!item)return 0;
 // cdecl(player, world head position, right-hand bool, required reference).
 // Required reference restricts native proximity picking to the selected item.
 return reinterpret_cast<int(__cdecl*)(void*,const float*,bool,void*)>(b+0x1A5890)(player,head,true,item);
}
inline void* dropOneAid(void* player,void* form,void* extra) {
 if(!player||!form)return nullptr;
 // TESObjectREFR::RemoveItem virtual 0x17C, JIP LN GameObjects.h. Returns the
 // actual dropped reference; GetDroppedRefs is not populated for player aid.
 using Drop=void*(__thiscall*)(void*,void*,void*,unsigned,bool,bool,void*,unsigned,unsigned,bool,bool);
 auto vt=*reinterpret_cast<void***>(player);
 return reinterpret_cast<Drop>(vt[0x17C/4])(player,form,extra,1,true,true,nullptr,0,0,true,false);
}
inline bool selectStim(void* player) {
 auto b=heldModule();if(!b||!player)return false;
 if(reinterpret_cast<int(__cdecl*)(void*)>(b+0x90E8)(player)<=0)return false;
 auto bone=reinterpret_cast<void*(__cdecl*)(void*,int)>(b+0x39F380)(player,1);
 if(!bone||!reinterpret_cast<bool(__cdecl*)(void*,int)>(b+0x39F500)(bone,1))return false;
 // Reproduce only the successful native neck-selection transition (316641).
 *reinterpret_cast<ULONGLONG*>(b+0x17FA8A8)=GetTickCount64();
 *reinterpret_cast<int*>(b+0xECD1BC)=1;b[0x17FA805]=0;b[0x17FA806]=0;b[0xECD1C0]=1;
 *reinterpret_cast<int*>(b+0x17FABB0)=0;*reinterpret_cast<void**>(b+0x17FABB4)=nullptr;
 *reinterpret_cast<int*>(b+0x17FAC78)=0;*reinterpret_cast<void**>(b+0x17FAC7C)=nullptr;b[0x17FA807]=0;
 reinterpret_cast<void(__cdecl*)(bool,const void*,int)>(b+0xE066)(true,b+0x9EF3D0,0);
 return true;
}
}
