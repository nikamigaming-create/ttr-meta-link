#pragma once
// Minimal Win32 ABI declarations verified against xNVSE PluginAPI.h.
// Runtime target: FalloutNV 1.4.0.525, xNVSE 6.4.9.
#include <cstdint>
using u32=uint32_t;
struct Script;
struct Form { void* vtable; uint8_t type; uint8_t pad[3]; u32 flags,id; };
struct alignas(8) Element {
 union { double number; void* array; Form* form; char* string; } value{};
 uint8_t type=0;
};
static_assert(sizeof(Element)==16 && offsetof(Element,type)==8);
struct NVSE {
 u32 version,runtime,editorVersion,isEditor;
 void* registerCommand; void* setOpcodeBase;
 void* (__cdecl* queryInterface)(u32);
 u32 (__cdecl* getPluginHandle)();
 void* registerTypedCommand;
 const char* (__cdecl* runtimeDirectory)();
};
struct PluginInfo {u32 infoVersion;const char* name;u32 version;};
struct Message {const char* sender;u32 type,len;void* data;};
struct Messaging {u32 version;bool (__cdecl* listen)(u32,const char*,void(__cdecl*)(Message*));void* dispatch;};
struct Scripts {
 bool (__cdecl* call)(Script*,Form*,Form*,Element*,uint8_t,...);
 void* params;void* extract;void* extractFormat;
 bool (__cdecl* callAlt)(Script*,Form*,uint8_t,...);
 Script* (__cdecl* compile)(const char*);
 Script* (__cdecl* expression)(const char*);
};
struct Arrays {
 void* createArray;void* createStringMap;void* createMap;void* assign;void* set;void* append;
 u32 (__cdecl* size)(void*);
 void* (__cdecl* lookup)(u32);
 bool (__cdecl* get)(void*,const Element&,Element&);
 bool (__cdecl* elements)(void*,Element*,Element*);
};
struct Console {u32 version;bool(__cdecl* run)(const char*,Form*);bool(__cdecl* runEx)(const char*,Form*,bool);};
// Public xNVSE PluginAPI.h / InventoryRef.h ABI.
struct DataInterface {u32 version;void*(__cdecl* singleton)(u32);void*(__cdecl* function)(u32);};
struct InventoryRef {Form* type;void* entry;void* extra;Form* container;Form* temp;unsigned char pad[24];bool validate,removed;unsigned char tail[2];};
static_assert(sizeof(InventoryRef)==0x30&&offsetof(InventoryRef,removed)==0x2D);
enum { PostLoad=0,ExitGame=1,ExitMenu=2,LoadGame=3,SaveGame=4,PreLoadGame=6,PostLoadGame=8,PostPostLoad=9,NewGame=14,DeferredInit=18,MainLoop=20 };
