#include "mmu.h"
#include "console.h"
#include "mmu_status.h"
#include "diagnostic.h"
#define VALID 1ULL
#define TABLE 3ULL
#define AF (1ULL<<10)
#define SH_INNER (3ULL<<8)
#define READONLY (1ULL<<7)
#define PXN (1ULL<<53)
#define UXN (1ULL<<54)
#define ADDRESS UINT64_C(0x0000fffffffff000)
#ifndef CHOS_MMU_TABLE_CAPACITY
#define CHOS_MMU_TABLE_CAPACITY 1024
#endif
#define TABLE_PAGES CHOS_MMU_TABLE_CAPACITY
_Static_assert(TABLE_PAGES>=8 && TABLE_PAGES<=1024,"bounded table metadata policy");
static unsigned tables;
static uint64_t *root;
static uint64_t table_pages[TABLE_PAGES];
static struct pmm *table_allocator;
static struct mmu_status status;
static uint64_t physical_limit,request_address,request_size;
const struct mmu_status *mmu_status_get(void) { return &status; }
static bool fail(unsigned reason,uint64_t address,uint64_t size) {
    if(!status.reason) { status.reason=reason;status.address=address;status.size=size;status.pages_requested=size/PAGE_SIZE; }
    status.tables=tables;
    if(table_allocator){status.physical_total=table_allocator->total;status.physical_used=table_allocator->used;status.physical_error=table_allocator->error;status.physical_ranges=table_allocator->range_count;}
    return false;
}
void mmu_memory_rejected(const struct chos_boot_info *b,struct pmm *p) {
    static const unsigned reasons[]={MMU_OK,MMU_DESCRIPTOR,MMU_DESCRIPTOR,MMU_ALIGNMENT,MMU_OVERFLOW,MMU_OVERLAP,MMU_CAPACITY,MMU_ALLOC,MMU_ADDRESS};
    status=(struct mmu_status){.stage=MMU_MEMORY_MAP,.reason=p->error<9?reasons[p->error]:MMU_DESCRIPTOR,
        .descriptor_index=p->error_index,.descriptor_type=p->error_type,.address=p->error_address,
        .size=p->error_pages>UINT64_MAX/PAGE_SIZE?UINT64_MAX:p->error_pages*PAGE_SIZE,.pages_requested=p->error_pages,
        .physical_error=p->error,.physical_ranges=p->range_count,.capacity=TABLE_PAGES,.descriptors=b->descriptor_size?b->memory_map_size/b->descriptor_size:0};
}
static bool active;
static void invalidate(void) {
#ifndef CHOS_MMU_TEST
    __asm__ volatile("dsb ish; tlbi vmalle1; dsb ish; isb":::"memory");
#endif
}
static const struct chos_boot_info *boot_memory;
static void clean_tables(void);
static uint64_t *new_table(void) {
    if(tables==TABLE_PAGES) {fail(MMU_CAPACITY,request_address,request_size);return 0;}
    uint64_t page=pmm_alloc(table_allocator,1);
    if(!page) {fail(MMU_ALLOC,request_address,request_size);return 0;}
    if(page%PAGE_SIZE || page>=physical_limit || PAGE_SIZE>physical_limit-page) {
        fail(MMU_ADDRESS,page,PAGE_SIZE);return 0;
    }
    table_pages[tables++]=page;status.tables=tables;
    uint64_t *t=(void *)(uintptr_t)page;
    for(unsigned i=0;i<512;i++) t[i]=0;
    return t;
}
static uint64_t *owned_table(uint64_t address) {
    for(unsigned i=0;i<tables;i++)if(table_pages[i]==address)return (void *)(uintptr_t)address;
    fail(MMU_TABLE_ADDRESS,address,PAGE_SIZE);return 0;
}
static uint64_t *descend(uint64_t *parent,unsigned index,unsigned shift) {
    uint64_t old=parent[index];
    if((old&3)==TABLE) return owned_table(old&ADDRESS);
    if(active && (old&VALID)) {fail(MMU_CONFLICT,old&ADDRESS,1ULL<<shift);return 0;} /* live block splitting requires an alias */
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
static bool managed_span(uint64_t address,uint64_t size) {
    for(unsigned i=0;i<table_allocator->range_count;i++) {
        const struct pmm_range *r=&table_allocator->ranges[i];
        if(address<r->base+(uint64_t)r->pages*PAGE_SIZE && r->base<address+size)return true;
    }
    return false;
}
static bool map(uint64_t address,uint64_t size,uint64_t flags) {
    request_address=address;request_size=size;
    if(address%PAGE_SIZE || size%PAGE_SIZE)return fail(MMU_ALIGNMENT,address,size);
    if(address>=physical_limit)return fail(MMU_ADDRESS,address,size);
    if(size>physical_limit-address)return fail(MMU_OVERFLOW,address,size);
    while(size) {
        uint64_t *l1=descend(root,(address>>39)&511,30);
    if(!l1) return false;
    uint64_t *l2=descend(l1,(address>>30)&511,21);
        if(!l2) return false;
        unsigned index=(address>>21)&511;
        /* Managed RAM stays page-granular: runtime thread guards never need live
         * block splitting. Elsewhere use aligned 2 MiB blocks with identical
         * attributes. Text/rodata/boot guards split offline before the switch. */
        if(!(address&((1ULL<<21)-1)) && size>=(1ULL<<21) &&
           !managed_span(address,1ULL<<21) && !(l2[index]&VALID)) {
            l2[index]=address|flags|VALID;
            address+=1ULL<<21; size-=1ULL<<21;
        } else {
            uint64_t *l3=descend(l2,index,12);
            if(!l3) return false;
            uint64_t *pte=&l3[(address>>12)&511],value=address|flags|TABLE;
            if(active && (*pte&VALID) && *pte!=value)return fail(MMU_CONFLICT,address,PAGE_SIZE);
            *pte=value;
            address+=PAGE_SIZE; size-=PAGE_SIZE;
        }
    }
    return true;
}
static void clean_tables(void) {
#ifndef CHOS_MMU_TEST
    uint64_t ctr; __asm__ volatile("mrs %0, ctr_el0":"=r"(ctr));
    uint64_t line=4ULL<<((ctr>>16)&15);
    for(unsigned i=0;i<tables;i++)
        for(uintptr_t p=table_pages[i];p<table_pages[i]+PAGE_SIZE;p+=line) __asm__ volatile("dc cvac, %0"::"r"(p):"memory");
    __asm__ volatile("dsb sy":::"memory");
#endif
}
bool mmu_guard(uint64_t address) {
    request_address=address;request_size=PAGE_SIZE;
    if(!root || address%PAGE_SIZE || address>=physical_limit) return fail(MMU_ALIGNMENT,address,PAGE_SIZE);
    uint64_t *l1=descend(root,(address>>39)&511,30);
    if(!l1) return false;
    uint64_t *l2=descend(l1,(address>>30)&511,21);
    if(!l2) return false;
    uint64_t *l3=descend(l2,(address>>21)&511,12);
    if(!l3) return false;
    l3[(address>>12)&511]=0;
    if(active) {
        clean_tables();
        invalidate();
    }
    return true;
}
extern void arch_mmu_switch(uint64_t,uint64_t,uint64_t);
static void stage(unsigned value) {
    status.stage=value;
    console_rect(32,288,900,24,0x071b36);
    console_text(32,288,"MMU STAGE",0x9ed7ff);console_number(164,288,value);
}
static bool covers(const struct chos_boot_info *b,uint64_t address,uint64_t size) {
    if(!size || address>=physical_limit || size>physical_limit-address)return false;
    uint64_t end=address+size;
    while(address<end) {
        uint64_t next=address;
        for(uint64_t off=0;off<b->memory_map_size;off+=b->descriptor_size) {
            const struct memory_descriptor *d=(void *)(uintptr_t)(b->memory_map+off);
            uint64_t de=d->physical+d->pages*PAGE_SIZE;
            if(d->type>=1 && d->type<=10 && d->physical<=address && de>next)next=de;
        }
        if(next==address)return false;
        address=next;
    }
    return true;
}
static bool valid_map(const struct chos_boot_info *b) {
    if(!b->memory_map || b->memory_map%8 || b->descriptor_size<sizeof(struct memory_descriptor) || b->descriptor_size%8 ||
       !b->memory_map_size || b->memory_map_size%b->descriptor_size || b->memory_map_size/b->descriptor_size>4096 ||
       b->memory_map_size>UINT64_MAX-b->memory_map)
        return fail(MMU_DESCRIPTOR,b->memory_map,b->memory_map_size);
    if(b->memory_map>=physical_limit || b->memory_map_size>physical_limit-b->memory_map)return fail(MMU_ADDRESS,b->memory_map,b->memory_map_size);
    status.descriptors=b->memory_map_size/b->descriptor_size;
    for(uint64_t i=0;i<status.descriptors;i++) {
        const struct memory_descriptor *d=(void *)(uintptr_t)(b->memory_map+i*b->descriptor_size);
        status.descriptor_index=i;status.descriptor_type=d->type;status.descriptor_attributes=d->attributes;
        if(d->physical%PAGE_SIZE)return fail(MMU_ALIGNMENT,d->physical,0);
        if(!d->pages)return fail(MMU_DESCRIPTOR,d->physical,0);
        if(d->physical>=physical_limit)return fail(MMU_ADDRESS,d->physical,0);
        if(d->pages>(physical_limit-d->physical)/PAGE_SIZE){fail(MMU_OVERFLOW,d->physical,UINT64_MAX);status.pages_requested=d->pages;return false;}
        uint64_t size=d->pages*PAGE_SIZE,end=d->physical+size;
        if(end>status.highest_end)status.highest_end=end;
        if(d->type>=1 && d->type<=10)status.ram_pages+=d->pages;
        for(uint64_t j=0;j<i;j++) {
            const struct memory_descriptor *a=(void *)(uintptr_t)(b->memory_map+j*b->descriptor_size);
            if(d->physical<a->physical+a->pages*PAGE_SIZE && a->physical<end)return fail(MMU_OVERLAP,d->physical,size);
        }
    }
    return true;
}
static bool live_overlap(const struct chos_boot_info *b,uint64_t address,uint64_t size) {
    const uint64_t bases[]={b->kernel_base,b->stack_base,(uintptr_t)b,b->memory_map,b->kernel_source};
    const uint64_t sizes[]={b->kernel_memory_bytes,b->stack_bytes,sizeof(*b),b->memory_map_size,b->kernel_size};
    for(unsigned i=0;i<5;i++)if(address<bases[i]+sizes[i] && bases[i]<address+size)return true;
    return managed_span(address,size);
}
bool mmu_init(struct pmm *physical,const struct chos_boot_info *boot,uint64_t gicd,uint64_t gicc) {
    status=(struct mmu_status){.capacity=TABLE_PAGES,.descriptor_index=UINT32_MAX};
    table_allocator=physical;boot_memory=boot;tables=0;root=0;active=false;
    stage(MMU_FEATURES);
#ifdef CHOS_MMU_TEST
    extern uint64_t mmu_test_features;
    status.features=mmu_test_features;
#else
    __asm__ volatile("mrs %0, id_aa64mmfr0_el1":"=r"(status.features));
    __asm__ volatile("mrs %0, tcr_el1":"=r"(status.old_tcr));
    __asm__ volatile("mrs %0, ttbr0_el1":"=r"(status.old_ttbr));
    __asm__ volatile("mrs %0, sctlr_el1":"=r"(status.old_sctlr));
    if(!(status.old_sctlr&1))return fail(MMU_FIRMWARE_STATE,0,0);
#endif
    unsigned ips=status.features&15,granule=(status.features>>28)&15;
    if(ips>6)return fail(MMU_WIDTH,ips,0);
    if(granule>1)return fail(MMU_GRANULE,granule,0);
    if(ips>5)ips=5;
    const unsigned widths[]={32,36,40,42,44,48};physical_limit=1ULL<<widths[ips];
    status.physical_total=physical->total;status.physical_used=physical->used;
    for(unsigned i=0;i<physical->range_count;i++) {
        const struct pmm_range *r=&physical->ranges[i];uint64_t run=0;
        for(uint32_t j=0;j<r->pages;j++){uint32_t bit=r->index+j;run=(physical->bits[bit/64]>>(bit%64)&1)?0:run+1;if(run>status.largest_free_run)status.largest_free_run=run;}
    }
    stage(MMU_MEMORY_MAP);if(!valid_map(boot))return false;
    /* Every address touched during/after switching must remain identity mapped. */
    const uint64_t needed_address[]={boot->kernel_base,boot->stack_base,(uintptr_t)boot,boot->memory_map,boot->kernel_source};
    const uint64_t needed_size[]={boot->kernel_memory_bytes,boot->stack_bytes,sizeof(*boot),boot->memory_map_size,boot->kernel_size};
    for(unsigned i=0;i<5;i++)if(!covers(boot,needed_address[i],needed_size[i]))return fail(MMU_COVERAGE,needed_address[i],needed_size[i]);
    stage(MMU_ROOT);request_address=0;request_size=PAGE_SIZE;root=new_table();if(!root)return false;
    stage(MMU_RAM);
    for(uint64_t i=0;i<status.descriptors;i++) {
        const struct memory_descriptor *d=(void *)(uintptr_t)(boot->memory_map+i*boot->descriptor_size);
        status.descriptor_index=i;status.descriptor_type=d->type;status.descriptor_attributes=d->attributes;
        if(d->type>=1 && d->type<=10 && !map(d->physical,d->pages*PAGE_SIZE,AF|SH_INNER|PXN|UXN))return false;
    }
    status.descriptor_index=UINT32_MAX;status.descriptor_type=UINT32_MAX;
    uint64_t device_flags=AF|(1ULL<<2)|PXN|UXN;
    stage(MMU_FRAMEBUFFER);
    uint64_t fb=boot->framebuffer&~(PAGE_SIZE-1),offset=boot->framebuffer-fb;
    if(!boot->framebuffer_size || fb>=physical_limit || offset>physical_limit-fb ||
       boot->framebuffer_size>physical_limit-fb-offset || boot->framebuffer_size+offset>UINT64_MAX-(PAGE_SIZE-1))
        return fail(MMU_OVERFLOW,boot->framebuffer,boot->framebuffer_size);
    uint64_t fb_size=(boot->framebuffer_size+offset+PAGE_SIZE-1)&~(PAGE_SIZE-1);
    if(live_overlap(boot,fb,fb_size))return fail(MMU_CONFLICT,fb,fb_size);
    uint64_t fb_flags=device_flags;
    if(covers(boot,fb,fb_size))fb_flags=AF|SH_INNER|PXN|UXN; /* RAM-backed GOP retains Normal WB. */
    else for(uint64_t off=0;off<boot->memory_map_size;off+=boot->descriptor_size) {
        const struct memory_descriptor *d=(void *)(uintptr_t)(boot->memory_map+off);
        if(d->type>=1 && d->type<=10 && fb<d->physical+d->pages*PAGE_SIZE && d->physical<fb+fb_size)
            return fail(MMU_CONFLICT,fb,fb_size); /* mixed RAM/MMIO is ambiguous */
    }
    if(!map(fb,fb_size,fb_flags))return false;
    stage(MMU_GIC);
    if(gicd && (gicd%PAGE_SIZE || (gicd<fb+fb_size && fb<gicd+0x10000) || live_overlap(boot,gicd,0x10000)))return fail(MMU_CONFLICT,gicd,0x10000);
    if(gicc && (gicc%PAGE_SIZE || (gicc<fb+fb_size && fb<gicc+0x2000) || live_overlap(boot,gicc,0x2000)))return fail(MMU_CONFLICT,gicc,0x2000);
    if(gicd && !map(gicd,0x10000,device_flags))return false;
    if(gicc && !map(gicc,0x2000,device_flags))return false;
    stage(MMU_TEXT);
    if(!map((uintptr_t)__text_start,(uintptr_t)__text_end-(uintptr_t)__text_start,AF|SH_INNER|READONLY|UXN))return false;
    stage(MMU_RODATA);
    if(!map((uintptr_t)__rodata_start,(uintptr_t)__rodata_end-(uintptr_t)__rodata_start,AF|SH_INNER|READONLY|PXN|UXN))return false;
    stage(MMU_GUARDS);
    if(!mmu_guard(0) || !mmu_guard(boot->stack_base) || !mmu_guard(boot->stack_base+boot->stack_bytes-PAGE_SIZE))return false;
    /* Allocated tables also need identity coverage; firmware map owns no pointers
     * to these pages. Never infer a table address from an unvalidated descriptor. */
    for(unsigned i=0;i<tables;i++)if(!covers(boot,table_pages[i],PAGE_SIZE))return fail(MMU_COVERAGE,table_pages[i],PAGE_SIZE);
#ifndef CHOS_MMU_TEST
    uint64_t sp;__asm__ volatile("mov %0, sp":"=r"(sp));
    if(sp<boot->stack_base+PAGE_SIZE || sp>=boot->stack_base+boot->stack_bytes-PAGE_SIZE)return fail(MMU_COVERAGE,sp,PAGE_SIZE);
#endif
    clean_tables();
    status.new_tcr=16ULL|(1ULL<<8)|(1ULL<<10)|(3ULL<<12)|(1ULL<<23)|((uint64_t)ips<<32);
    stage(MMU_TRANSITION);
    arch_mmu_switch((uintptr_t)root,status.new_tcr,0x04ff);
    active=true;status.stage=MMU_READY;status.tables=tables;status.physical_used=physical->used;
    diagnostic_value("MMU table pages: ",tables,false);diagnostic_value("MMU table capacity: ",TABLE_PAGES,false);
    diagnostic_value("MMU RAM descriptors: ",status.descriptors,false);diagnostic_value("MMU highest physical end: ",status.highest_end,true);
    return true;
}

#include "process.h"
static void sync_space(struct address_space *s) {
#ifndef CHOS_MMU_TEST
    for(unsigned i=0;i<s->count;i++) {
        uint64_t ctr; __asm__ volatile("mrs %0, ctr_el0":"=r"(ctr));
        uint64_t line=4ULL<<((ctr>>16)&15);
        for(uint64_t a=s->tables[i];a<s->tables[i]+PAGE_SIZE;a+=line)
            __asm__ volatile("dc cvac, %0"::"r"(a):"memory");
    }
    invalidate();
#else
    (void)s;
#endif
}
bool mmu_restore(uint64_t address) {
    uint64_t *l1=descend(root,(address>>39)&511,30);
    if(!l1) return false;
    uint64_t *l2=descend(l1,(address>>30)&511,21);
    if(!l2) return false;
    uint64_t *l3=descend(l2,(address>>21)&511,12);
    if(!l3 || address%PAGE_SIZE) return false;
    l3[(address>>12)&511]=address|AF|SH_INNER|PXN|UXN|TABLE;
    clean_tables(); invalidate();
    return true;
}
uint64_t mmu_kernel_root(void) { return (uintptr_t)root; }
void mmu_activate(uint64_t address) {
#ifndef CHOS_MMU_TEST
    __asm__ volatile("dsb ish; msr ttbr0_el1, %0; isb; tlbi vmalle1; dsb ish; isb"::"r"(address):"memory");
#else
    (void)address;
#endif
}
static uint64_t *space_table(struct pmm *p,struct address_space *s) {
    if(s->count==16) return 0;
    uint64_t page=pmm_alloc(p,1);
    if(!page) return 0;
    s->tables[s->count++]=page;
    uint64_t *t=(void *)(uintptr_t)page;
    for(unsigned i=0;i<512;i++) t[i]=0;
    return t;
}
bool mmu_space_create(struct pmm *p,struct address_space *s) {
    *s=(struct address_space){0};
    /* Reserve one 1 GiB root slot for private user mappings, absent in kernel. */
    if((root[0]&3)!=TABLE) return false;
    uint64_t *kernel_l1=(void *)(uintptr_t)(root[0]&ADDRESS);
    if(kernel_l1[USER_BASE>>30]) return false;
    uint64_t *r=space_table(p,s);
    if(!r) return false;
    for(unsigned i=0;i<512;i++) r[i]=root[i];
    uint64_t *private_l1=space_table(p,s);
    if(!private_l1) { mmu_space_destroy(p,s); return false; }
    for(unsigned i=0;i<512;i++) private_l1[i]=kernel_l1[i];
    r[0]=(uintptr_t)private_l1|TABLE;
    s->root=(uintptr_t)r;
    return true;
}
void mmu_space_destroy(struct pmm *p,struct address_space *s) {
    for(unsigned i=0;i<s->count;i++) if(!pmm_free(p,s->tables[i],1)) panic("SPACE TABLE FREE FAILED");
    *s=(struct address_space){0};
}
static uint64_t *user_pte(struct pmm *p,struct address_space *s,uint64_t va,bool create) {
    if(va<USER_BASE || va>=USER_STACK_TOP || va%PAGE_SIZE || !s->root) return 0;
    uint64_t *t=(void *)(uintptr_t)s->root;
    for(unsigned shift=39;shift>=21;shift-=9) {
        unsigned index=(va>>shift)&511;
        if(!(t[index]&VALID)) {
            if(!create) return 0;
            uint64_t *next=space_table(p,s);
            if(!next) return 0;
            t[index]=(uintptr_t)next|TABLE;
        }
        if((t[index]&3)!=TABLE) return 0;
        t=(void *)(uintptr_t)(t[index]&ADDRESS);
    }
    return &t[(va>>12)&511];
}
bool mmu_user_map(struct pmm *p,struct address_space *s,uint64_t va,uint64_t pa,bool writable,bool executable) {
    if(pa%PAGE_SIZE || (writable&&executable)) return false;
    uint64_t *pte=user_pte(p,s,va,true);
    if(!pte || (*pte&VALID)) return false;
    *pte=pa|AF|SH_INNER|PXN|(writable?(1ULL<<6):(3ULL<<6))|(executable?0:UXN)|TABLE;
    sync_space(s);
    return true;
}
bool mmu_user_unmap(struct address_space *s,uint64_t va) {
    uint64_t *pte=user_pte(0,s,va,false);
    if(!pte || !(*pte&VALID)) return false;
    *pte=0; sync_space(s); return true;
}

bool mmu_device(uint64_t address,uint64_t size) {
    status.stage=MMU_DEVICE;status.reason=0;
    if(address%PAGE_SIZE || !size || size%PAGE_SIZE || address>=physical_limit || size>physical_limit-address) return fail(MMU_ADDRESS,address,size);
    for(uint64_t off=0;off<boot_memory->memory_map_size;off+=boot_memory->descriptor_size) {
        const struct memory_descriptor *d=(void *)(uintptr_t)(boot_memory->memory_map+off);
        if(d->type>=1 && d->type<=10 && address<d->physical+d->pages*PAGE_SIZE && d->physical<address+size) return fail(MMU_CONFLICT,address,size);
    }
    if(!map(address,size,AF|(1ULL<<2)|PXN|UXN)) return false;
    clean_tables(); invalidate();
    return true;
}
