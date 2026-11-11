#pragma once
// Private graphics policy for the exact TTWVR build. Included in the helper namespace.
bool shadowsRequested{},shadowsHandled{};
void patchTtrShadows(){
 if(!shadowsRequested||shadowsHandled)return;
 auto module=GetModuleHandleW(L"TTWVR.dll");if(!module)return;
 shadowsHandled=true;
 if(!build_adapter::verifiedTTWVR(module)){log("SHADOWS REFUSED: unsupported TTWVR hash");return;}
 auto* base=reinterpret_cast<unsigned char*>(module);
 // RVA457410 consumes 33 {Setting*,isFloat,floatValue,label} records, writes
 // Setting+4, and converts the requested float to integer when isFloat=0.
 const unsigned char writer[]={0xF3,0x0F,0x2C,0x03,0x39,0x06,0x74,0x04,0x89,0x06};
 if(memcmp(base+0x4DF652,writer,sizeof(writer))){log("SHADOWS REFUSED: settings writer guard");return;}
 struct Record{u32 setting;u32 isFloat;float value;const char* label;};
 static_assert(sizeof(Record)==16,"Exact PE32 table layout");
 struct Change{u32 rva,setting;float before,after;const char* label;};
 const Change changes[]={
  {0x9F9F50,0x11C74D4,0,1,"bDrawShadows (Enable Shadows: off)"},
  {0x9F9F60,0x11C7520,256,1024,"iShadowMapResolution (Shadow Quality: Low)"},
  {0x9F9F70,0x11C76CC,0,2,"iShadowFilter (Shadow Filtering: Low)"},
  {0x9F9F80,0x11C72D8,0,2,"iActorShadowCountInt"},
  {0x9F9F90,0x11C72E4,0,2,"iActorShadowCountExt"}
 };
 for(const auto& c:changes){
  auto* r=reinterpret_cast<Record*>(base+c.rva);
  if(r->setting!=c.setting||r->isFloat||r->value!=c.before||strcmp(r->label,c.label)){
   log("SHADOWS REFUSED: table guard at %08X",c.rva);return;
  }
 }
 DWORD old{};auto* first=base+changes[0].rva;constexpr size_t bytes=5*sizeof(Record);
 if(!VirtualProtect(first,bytes,PAGE_READWRITE,&old)){log("SHADOWS REFUSED: protection error=%lu",GetLastError());return;}
 for(const auto& c:changes)reinterpret_cast<Record*>(base+c.rva)->value=c.after;
 DWORD ignored{};BOOL restored=VirtualProtect(first,bytes,old,&ignored);
 log("SHADOWS APPLIED: native actor shadows on, map=1024 filter=2 interior/exterior=2; TTR preset table only; protectionRestored=%d",restored);
}
