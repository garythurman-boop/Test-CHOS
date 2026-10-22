#include "mmu.h"
#define VALID 1ULL
#define TABLE 3ULL
#define AF (1ULL<<10)
#define SH_INNER (3ULL<<8)
#define READONLY (1ULL<<7)
#define PXN (1ULL<<53)
#define UXN (1ULL<<54)
#define ADDRESS UINT64_C(0x0000fffffffff000)
#define TABLE_PAGES 1024
static uint64_t *pool,*root;
static unsigned tables;
static bool active;
static void clean_tables(void);
static uint64_t *new_table(void) {
    if(tables==TABLE_PAGES) return 0;
    uint64_t *t=pool+512*tables++;
    for(unsigned i=0;i<512;i++) t[i]=0;
    return t;
}
static uint64_t *descend(uint64_t *parent,unsigned index,unsigned shift) {
    uint64_t old=parent[index];
    if((old&3)==TABLE) return (void *)(uintptr_t)(old&ADDRESS);
    if(active && (old&VALID)) return 0; /* live block splitting requires an alias */
    uint64_t *next=new_table();
    if(!next) return 0;
    if(old&VALID) {
        /* Split a block while preserving physical mapping and attributes. */
        uint64_t base=old&ADDRESS,flags=old&~ADDRESS;
        for(unsigned i=0;i<512;i++) next[i]=(base+((uint64_t)i<<shift))|flags|(shift==12?TABLE:VALID);
    }
    parent[index]=(uint64_t)(uintptr_t)next|TABLE;
    return next;
}
static bool map(uint64_t address,uint64_t size,uint64_t flags) {
    if(address%PAGE_SIZE || size%PAGE_SIZE || address>=(1ULL<<39) || size>(1ULL<<39)-address) return false;
    while(size) {
        uint64_t *l2=descend(root,(address>>30)&511,21);
        if(!l2) return false;
        unsigned index=(address>>21)&511;
        if((flags&(1ULL<<2)) && !(address&((1ULL<<21)-1)) && size>=(1ULL<<21) && !(l2[index]&VALID)) {
            l2[index]=address|flags|VALID;
            address+=1ULL<<21; size-=1ULL<<21;
        } else {
            uint64_t *l3=descend(l2,index,12);
            if(!l3) return false;
            l3[(address>>12)&511]=address|flags|TABLE;
            address+=PAGE_SIZE; size-=PAGE_SIZE;
        }
    }
    return true;
}
static void clean_tables(void) {
    uint64_t ctr; __asm__ volatile("mrs %0, ctr_el0":"=r"(ctr));
    uint64_t line=4ULL<<((ctr>>16)&15);
    uintptr_t end=(uintptr_t)pool+tables*PAGE_SIZE;
    for(uintptr_t p=(uintptr_t)pool;p<end;p+=line) __asm__ volatile("dc cvac, %0"::"r"(p):"memory");
    __asm__ volatile("dsb sy":::"memory");
}
bool mmu_guard(uint64_t address) {
    if(!root || address%PAGE_SIZE || address>=(1ULL<<39)) return false;
    uint64_t *l2=descend(root,(address>>30)&511,21);
    if(!l2) return false;
    uint64_t *l3=descend(l2,(address>>21)&511,12);
    if(!l3) return false;
    l3[(address>>12)&511]=0;
    if(active) {
        clean_tables();
        __asm__ volatile("tlbi vmalle1; dsb ish; isb":::"memory");
    }
    return true;
}
extern void arch_mmu_switch(uint64_t,uint64_t,uint64_t);
bool mmu_init(struct pmm *physical,const struct chos_boot_info *boot,uint64_t gicd,uint64_t gicc) {
    uint64_t base=pmm_alloc(physical,TABLE_PAGES);
    if(!base) return false;
    pool=(void *)(uintptr_t)base; tables=0; active=false;
    root=new_table();
    for(uint64_t off=0;off<boot->memory_map_size;off+=boot->descriptor_size) {
        const struct memory_descriptor *d=(void *)(uintptr_t)(boot->memory_map+off);
        if(d->type>=1 && d->type<=10 &&
           !map(d->physical,d->pages*PAGE_SIZE,AF|SH_INNER|PXN|UXN)) return false;
    }
    uint64_t device_flags=AF|(1ULL<<2)|PXN|UXN;
    uint64_t fb=boot->framebuffer&~(PAGE_SIZE-1);
    uint64_t fb_size=(boot->framebuffer_size+(boot->framebuffer-fb)+PAGE_SIZE-1)&~(PAGE_SIZE-1);
    if(!map(fb,fb_size,device_flags)) return false;
    if(gicd && !map(gicd&~(PAGE_SIZE-1),0x10000,device_flags)) return false;
    if(gicc && !map(gicc&~(PAGE_SIZE-1),0x2000,device_flags)) return false;
    if(!map((uintptr_t)__text_start,(uintptr_t)__text_end-(uintptr_t)__text_start,AF|SH_INNER|READONLY|UXN)) return false;
    if(!map((uintptr_t)__rodata_start,(uintptr_t)__rodata_end-(uintptr_t)__rodata_start,AF|SH_INNER|READONLY|PXN|UXN)) return false;
    if(!mmu_guard(0) || !mmu_guard((uintptr_t)__text_start+512*1024)) return false;
    clean_tables();
    uint64_t features; __asm__ volatile("mrs %0, id_aa64mmfr0_el1":"=r"(features));
    uint64_t ips=features&15;
    if(ips>5) ips=5; /* use at most 48-bit physical addresses, 4 KiB granule */
    uint64_t tcr=25ULL|(1ULL<<8)|(1ULL<<10)|(3ULL<<12)|(1ULL<<23)|(ips<<32);
    /* Attr0=normal WB/WA, Attr1=Device-nGnRE. EL1 only, TTBR1 disabled. */
    arch_mmu_switch((uintptr_t)root,tcr,0x04ff);
    active=true;
    return true;
}
