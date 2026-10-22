#include "memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct pmm p;
static _Alignas(16) unsigned char arena[65536];
int main(void) {
    struct memory_descriptor map[]={
        {.type=1,.physical=0x1000,.pages=4},
        {.type=7,.physical=0x10000,.pages=16},
        {.type=7,.physical=0x200000,.pages=32},
        {.type=11,.physical=0x9000000,.pages=1}};
    assert(pmm_init(&p,map,sizeof(map),sizeof(map[0])));
    assert(p.total==48 && p.used==0);
    assert(!pmm_alloc(&p,0) && !pmm_alloc(&p,SIZE_MAX));
    uint64_t a=pmm_alloc(&p,16),b=pmm_alloc(&p,32);
    assert(a==0x10000 && b==0x200000 && p.used==48);
    assert(!pmm_alloc(&p,1));
    assert(!pmm_free(&p,a+1,1) && !pmm_free(&p,0x1000,1));
    assert(!pmm_free(&p,a,17) && p.used==48);
    assert(pmm_free(&p,a,16) && !pmm_free(&p,a,16));
    assert(pmm_free(&p,b,32) && p.used==0);
    /* Holes must not be coalesced into physically contiguous allocations. */
    assert(!pmm_alloc(&p,33));
    uint64_t pages[48];
    for(unsigned i=0;i<48;i++) {
        pages[i]=pmm_alloc(&p,1); assert(pages[i]);
        for(unsigned j=0;j<i;j++) assert(pages[j]!=pages[i]);
    }
    for(unsigned i=0;i<48;i+=2) assert(pmm_free(&p,pages[i],1));
    assert(!pmm_alloc(&p,2));
    for(unsigned i=1;i<48;i+=2) assert(pmm_free(&p,pages[i],1));
    assert(p.used==0);
    map[2].physical=0x11000;
    assert(!pmm_init(&p,map,sizeof(map),sizeof(map[0])) && p.total==0);
    map[2].physical=0x200000; map[2].pages=UINT64_MAX;
    assert(!pmm_init(&p,map,sizeof(map),sizeof(map[0])));
    assert(!pmm_init(&p,map,sizeof(map)-1,sizeof(map[0])));
    map[2].pages=32;
    struct { struct memory_descriptor d; uint64_t extension[2]; } extended[4]={0};
    for(unsigned i=0;i<4;i++) extended[i].d=map[i];
    assert(pmm_init(&p,extended,sizeof(extended),sizeof(extended[0])) && p.total==48);
    struct memory_descriptor big={.type=7,.physical=0x100000000,.pages=PMM_MAX_PAGES+100};
    assert(pmm_init(&p,&big,sizeof(big),sizeof(big)) && p.total==PMM_MAX_PAGES);
    assert(pmm_alloc(&p,1)==0x100000000);
    struct heap h={0};
    assert(!heap_init(&h,arena+1,sizeof(arena)-1));
    assert(heap_init(&h,arena,sizeof(arena)));
    assert(!heap_alloc(&h,0) && !heap_alloc(&h,SIZE_MAX));
    void *x=heap_alloc(&h,17),*y=heap_alloc(&h,8192),*z=heap_alloc(&h,73);
    assert(x&&y&&z && (uintptr_t)x%16==0);
    memset(x,0x35,17); memset(y,0xa2,8192); memset(z,0xff,73);
    assert(!heap_free(&h,(unsigned char *)x+1) && !heap_free(&h,arena));
    for(unsigned i=0;i<8192;i++) assert(((unsigned char *)y)[i]==0xa2);
    assert(heap_free(&h,y) && !heap_free(&h,y));
    assert(heap_free(&h,x) && heap_free(&h,z));
    void *whole=heap_alloc(&h,sizeof(arena)-32);
    assert(whole && !heap_alloc(&h,1));
    assert(heap_free(&h,whole));
    puts("PASS: PMM reserved ranges, fragmentation, exhaustion, overflow, stride, >4GiB; heap alignment, integrity, invalid/double free and coalescing");
}
