#include "memory.h"
/* Metadata is external to managed pages. Only UEFI conventional RAM is admitted;
 * firmware/loader/runtime/ACPI/MMIO ranges can never be returned by allocation.
 * Single CPU: callers must mask IRQs once scheduling is active. */
static bool bit(const struct pmm *p,uint32_t i) { return (p->bits[i/64]>>(i%64))&1; }
static void set(struct pmm *p,uint32_t i,bool value) {
    uint64_t mask=1ULL<<(i%64);
    if(value) p->bits[i/64]|=mask; else p->bits[i/64]&=~mask;
}
bool pmm_init(struct pmm *p,const void *map,size_t size,size_t stride) {
    p->range_count=p->total=p->used=0;
    if(!map || stride<sizeof(struct memory_descriptor) || stride%8 ||
       !size || size%stride || (uintptr_t)map%8) return false;
    size_t count=size/stride;
    if(count>4096) return false;
    for(size_t i=0;i<PMM_MAX_PAGES/64;i++) p->bits[i]=0;
    for(size_t i=0;i<count;i++) {
        const struct memory_descriptor *d=(const void *)((const uint8_t *)map+i*stride);
        if(!d->pages || d->physical%PAGE_SIZE || d->pages>(UINT64_MAX-d->physical)/PAGE_SIZE)
            goto invalid;
        uint64_t end=d->physical+d->pages*PAGE_SIZE;
        for(size_t j=0;j<i;j++) {
            const struct memory_descriptor *a=(const void *)((const uint8_t *)map+j*stride);
            if(d->physical<a->physical+a->pages*PAGE_SIZE && a->physical<end) goto invalid;
        }
        if(d->type!=7) continue;
        uint64_t base=d->physical,n=d->pages;
        if(!base) { base+=PAGE_SIZE; n--; } /* zero is the failure sentinel */
        /* Cap metadata, not physical addresses. Unmanaged excess remains reserved. */
        if(n>PMM_MAX_PAGES-p->total) n=PMM_MAX_PAGES-p->total;
        if(!n) continue;
        if(p->range_count==PMM_MAX_RANGES) goto invalid;
        p->ranges[p->range_count++]=(struct pmm_range){base,(uint32_t)n,p->total};
        p->total+=(uint32_t)n;
    }
    return p->total!=0;
invalid:
    p->range_count=p->total=p->used=0;
    return false;
}
uint64_t pmm_alloc(struct pmm *p,size_t pages) {
    if(!pages || pages>p->total-p->used) return 0;
    for(uint32_t r=0;r<p->range_count;r++) {
        const struct pmm_range *range=&p->ranges[r];
        size_t run=0;
        for(uint32_t i=0;i<range->pages;i++) {
            run=bit(p,range->index+i)?0:run+1;
            if(run==pages) {
                uint32_t first=i+1-(uint32_t)pages;
                for(size_t j=0;j<pages;j++) set(p,range->index+first+(uint32_t)j,true);
                p->used+=(uint32_t)pages;
                return range->base+(uint64_t)first*PAGE_SIZE;
            }
        }
    }
    return 0;
}
bool pmm_free(struct pmm *p,uint64_t address,size_t pages) {
    if(!pages || pages>p->used || address%PAGE_SIZE) return false;
    for(uint32_t r=0;r<p->range_count;r++) {
        const struct pmm_range *range=&p->ranges[r];
        if(address<range->base || (address-range->base)/PAGE_SIZE>=range->pages) continue;
        uint32_t first=(uint32_t)((address-range->base)/PAGE_SIZE);
        if(pages>range->pages-first) return false;
        for(size_t j=0;j<pages;j++) if(!bit(p,range->index+first+(uint32_t)j)) return false;
        for(size_t j=0;j<pages;j++) set(p,range->index+first+(uint32_t)j,false);
        p->used-=(uint32_t)pages;
        return true;
    }
    return false;
}
