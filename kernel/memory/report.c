#include "mmu_status.h"
#include "console.h"
#include "diagnostic.h"
static void hex(unsigned x,unsigned y,uint64_t value) {
    char s[19]="0x0000000000000000";
    for(unsigned i=0;i<16;i++)s[2+i]="0123456789ABCDEF"[(value>>((15-i)*4))&15];
    console_text(x,y,s,0xffffff);
}
void mmu_failure_screen(void) {
    const struct mmu_status *s=mmu_status_get();
    static const char stages[][20]={"RESET","FEATURES","MEMORY MAP","ROOT","RAM","FRAMEBUFFER","GIC","TEXT","RODATA","GUARDS","TRANSITION","READY","DEVICE"};
    static const char reasons[][24]={"OK","PAGE ALLOCATION","TABLE CAPACITY","PHYSICAL ADDRESS","ADDRESS WIDTH","SIZE OVERFLOW","UEFI DESCRIPTOR","DESCRIPTOR OVERLAP","MAPPING CONFLICT","ALIGNMENT","4K GRANULE","IDENTITY COVERAGE","TABLE POINTER","FIRMWARE STATE"};
    console_rect(0,72,1024,650,0x300808);
    console_text(32,96,"CHOS MMU INIT FAILED / REPORT V1",0xff8080);
    console_text(32,132,"STAGE",0xffffff);console_number(176,132,s->stage);
    console_text(248,132,s->stage<13?stages[s->stage]:"UNKNOWN",0xffffff);
    console_text(32,160,"REASON",0xffffff);console_number(176,160,s->reason);
    console_text(248,160,s->reason<14?reasons[s->reason]:"UNKNOWN",0xffffff);
    console_text(32,196,"PHYSICAL",0xffffff);hex(272,196,s->address);
    console_text(32,224,"SIZE / ARGUMENT",0xffffff);hex(272,224,s->size);
    console_text(32,252,"DESCRIPTOR INDEX",0xffffff);console_number(272,252,s->descriptor_index);
    console_text(32,280,"DESCRIPTOR TYPE",0xffffff);console_number(272,280,s->descriptor_type);
    console_text(32,308,"PAGES REQUESTED",0xffffff);console_number(272,308,s->pages_requested);
    console_text(32,336,"TABLES / CAPACITY",0xffffff);console_number(272,336,s->tables);console_number(416,336,s->capacity);
    console_text(32,364,"PMM USED / TOTAL",0xffffff);console_number(272,364,s->physical_used);console_number(416,364,s->physical_total);
    console_text(32,392,"MAP ENTRIES",0xffffff);console_number(272,392,s->descriptors);
    console_text(32,420,"HIGHEST END",0xffffff);hex(272,420,s->highest_end);
    console_text(32,448,"MMFR0",0xffffff);hex(272,448,s->features);
    console_text(32,476,"FIRMWARE TCR",0xffffff);hex(272,476,s->old_tcr);
    console_text(32,504,"FIRMWARE TTBR0",0xffffff);hex(272,504,s->old_ttbr);
    console_text(32,532,"FIRMWARE SCTLR",0xffffff);hex(272,532,s->old_sctlr);
    console_text(32,560,"NEW TCR",0xffffff);hex(272,560,s->new_tcr);
    console_text(32,588,"DESCRIPTOR ATTR",0xffffff);hex(272,588,s->descriptor_attributes);
    console_text(32,616,"PMM ERROR / RANGES",0xffffff);console_number(272,616,s->physical_error);console_number(416,616,s->physical_ranges);
    console_text(32,656,"CAPTURE THIS SCREEN. NO INSTALLATION PERFORMED.",0x9ed7ff);
    diagnostic_value("MMU failure stage: ",s->stage,false);diagnostic_value("MMU failure reason: ",s->reason,false);
    diagnostic_value("MMU failure address: ",s->address,true);diagnostic_value("MMU failure size: ",s->size,true);
}
