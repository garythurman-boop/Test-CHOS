#include "app.h"
static unsigned pressed;
void about_key(unsigned key) {if(key)pressed++;}
void about_paint(void) {
    begin(0xe5eef8);label(22,"CHOS Desktop - pre-alpha");label(54,"Original ARM64 operating system");label(86,"Build 5. Native EL0 applications.");
    struct chos_info info;
    if(api(SYS_INFO,(uintptr_t)&info,0,0)==0){char value[21];number(value,info.total_pages*4);label(118,"Managed conventional RAM KiB:");text(370,118,value,0x204b72);}
    label(150,"Press a key to test native input.");box(20,184,80,32,pressed?0x328c69:0x326bb0);
    status("No installer yet. VMware Fusion unverified.");
}
