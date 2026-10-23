/* Compile the production mapper/allocator on the host. Only privileged register,
 * cache/TLB operations and screen writes are replaced, never mapping decisions. */
#include "mmu.h"
#include "mmu_status.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
uint64_t mmu_test_features=5;
/* Linker-defined bounds for a host-only two-page image. */
__asm__(".section .bss\n.balign 4096\n.global __text_start\n__text_start: .skip 4096\n.global __text_end\n__text_end:\n.global __rodata_start\n__rodata_start: .skip 4096\n.global __rodata_end\n__rodata_end:\n.previous");
static struct pmm p;
static struct chos_boot_info b;
static _Alignas(4096) unsigned char arena[8*1024*1024];
static _Alignas(4096) unsigned char stack[18*4096];
static _Alignas(4096) struct memory_descriptor map[4096];
static unsigned count,switches;
void console_rect(unsigned a,unsigned c,unsigned d,unsigned e,uint32_t f){(void)a;(void)c;(void)d;(void)e;(void)f;}
void console_text(unsigned a,unsigned c,const char *d,uint32_t e){(void)a;(void)c;(void)d;(void)e;}
void console_number(unsigned a,unsigned c,uint64_t d){(void)a;(void)c;(void)d;}
void diagnostic_value(const char *a,uint64_t c,bool d){(void)a;(void)c;(void)d;}
_Noreturn void panic(const char *s){fprintf(stderr,"%s\n",s);abort();}
void arch_mmu_switch(uint64_t root,uint64_t tcr,uint64_t mair){assert(root && (root&4095)==0 && mair==0x04ff && (tcr&63)==16);switches++;}
static void add(unsigned type,uint64_t address,uint64_t pages){map[count++]=(struct memory_descriptor){.type=type,.physical=address,.pages=pages};}
static void covering(uint64_t address,uint64_t bytes){uint64_t base=address&~4095ULL;add(2,base,(address-base+bytes+4095)/4096);}
static void setup(void){
 memset(&b,0,sizeof(b));memset(map,0,sizeof(map));count=0;switches=0;mmu_test_features=5;
 b.kernel_base=(uintptr_t)__text_start;b.kernel_memory_bytes=8192;
 b.stack_base=(uintptr_t)stack;b.stack_bytes=sizeof(stack);
 b.kernel_source=b.kernel_base;b.kernel_size=8192;
 b.framebuffer=0x200000000ULL;b.framebuffer_size=16*1024*1024;
 covering(b.kernel_base,b.kernel_memory_bytes);covering(b.stack_base,b.stack_bytes);covering((uintptr_t)&b,sizeof(b));covering((uintptr_t)map,sizeof(map));
 add(7,(uintptr_t)arena,sizeof(arena)/4096);
 b.memory_map=(uintptr_t)map;b.descriptor_size=sizeof(map[0]);
}
static bool init(void){b.memory_map_size=count*sizeof(map[0]);assert(pmm_init(&p,map,b.memory_map_size,sizeof(map[0])));return mmu_init(&p,&b,0x8000000,0x8010000);}
static uint64_t entry(uint64_t va,unsigned *level){uint64_t *t=(void *)(uintptr_t)mmu_kernel_root();for(unsigned l=0;l<4;l++){uint64_t v=t[(va>>(39-l*9))&511];if((v&3)!=3 || l==3){*level=l;return v;}t=(void *)(uintptr_t)(v&0x0000fffffffff000ULL);}abort();}
static void failure(unsigned stage,unsigned reason){assert(!mmu_init(&p,&b,0x8000000,0x8010000));assert(!switches);const struct mmu_status *s=mmu_status_get();if(s->stage!=stage || s->reason!=reason)fprintf(stderr,"expected %u/%u got %u/%u address %llx size %llu\n",stage,reason,s->stage,s->reason,(unsigned long long)s->address,(unsigned long long)s->size);assert(s->stage==stage && s->reason==reason);}
int main(void){
 setup();add(2,0x40000000,2*1024*1024*1024ULL/4096);assert(init());assert(switches==1 && mmu_status_get()->tables<64);
 unsigned level;uint64_t e=entry(0x40200000,&level);assert(level==2 && (e&3)==1 && (e&(1ULL<<53)) && (e&(1ULL<<54)) && !(e&(1ULL<<2)));
 e=entry((uintptr_t)__text_start,&level);assert(level==3 && (e&(1ULL<<7)) && !(e&(1ULL<<53)) && (e&(1ULL<<54)));
 e=entry((uintptr_t)__rodata_start,&level);assert(level==3 && (e&(1ULL<<7)) && (e&(1ULL<<53)));
 assert(!entry((uintptr_t)stack,&level));assert(!entry((uintptr_t)stack+sizeof(stack)-4096,&level));assert(!entry(0,&level));
 assert(mmu_guard((uintptr_t)arena+4*4096));assert(!entry((uintptr_t)arena+4*4096,&level));assert(mmu_restore((uintptr_t)arena+4*4096));assert(entry((uintptr_t)arena+4*4096,&level)&1);
 /* Existing no-live-block-split rule must remain enforced. */
 assert(!mmu_guard(0x40201000));assert(mmu_status_get()->reason==MMU_CONFLICT);
 setup();assert(init());switches=0;*(uint64_t *)(uintptr_t)mmu_kernel_root()=0x12345000|3;assert(!mmu_guard(0));assert(mmu_status_get()->reason==MMU_TABLE_ADDRESS);
 setup();add(2,0x100000000ULL,8*1024*1024*1024ULL/4096);add(11,0x4000000000ULL,65536);assert(init());e=entry(0x180000000ULL,&level);assert(level==2 && (e&1));assert(!entry(0x4000000000ULL,&level));
 /* Many sparse descriptors, including high physical addresses and MMIO holes. */
 setup();for(unsigned i=0;i<400;i++)add(i%2?11:2,0x100000000ULL+(uint64_t)i*0x400000,1);assert(init());assert(mmu_status_get()->descriptors==405);
 /* Allocation succeeds with no free run longer than one page. */
 setup();b.memory_map_size=count*sizeof(map[0]);assert(pmm_init(&p,map,b.memory_map_size,sizeof(map[0]))); /* Use explicit alternating bitmap occupancy, independent of allocator order. */
 for(unsigned i=0;i<PMM_MAX_PAGES/64;i++){p.bits[i]=0xaaaaaaaaaaaaaaaaULL;}
 p.used=p.total/2;
 assert(!pmm_alloc(&p,2));assert(mmu_init(&p,&b,0x8000000,0x8010000));
 setup();b.memory_map_size=count*sizeof(map[0]);assert(pmm_init(&p,map,b.memory_map_size,sizeof(map[0])));p.used=p.total;failure(MMU_ROOT,MMU_ALLOC);
 setup();add(2,0x40000000,1);assert(init());map[count-1].physical=map[0].physical;switches=0;failure(MMU_MEMORY_MAP,MMU_OVERLAP);
 setup();assert(init());switches=0;map[0].pages=UINT64_MAX;failure(MMU_MEMORY_MAP,MMU_OVERFLOW);
 setup();assert(init());switches=0;map[0].physical|=1;failure(MMU_MEMORY_MAP,MMU_ALIGNMENT);
 setup();assert(init());switches=0;map[0].pages=0;failure(MMU_MEMORY_MAP,MMU_DESCRIPTOR);
 setup();assert(init());switches=0;b.memory_map_size--;failure(MMU_MEMORY_MAP,MMU_DESCRIPTOR);
 setup();assert(init());switches=0;mmu_test_features=0xfULL<<28|5;failure(MMU_FEATURES,MMU_GRANULE);
 setup();assert(init());switches=0;mmu_test_features=7;failure(MMU_FEATURES,MMU_WIDTH);
 setup();assert(init());switches=0;mmu_test_features=0;failure(MMU_MEMORY_MAP,MMU_ADDRESS);
 setup();add(2,0x500000000ULL,8192);b.framebuffer=0x500000000ULL;b.framebuffer_size=16*1024*1024;assert(init());e=entry(b.framebuffer,&level);assert(!(e&(1ULL<<2)) && (e&(1ULL<<53)));
 setup();add(2,0x500000000ULL,1);b.framebuffer=0x500000000ULL;b.framebuffer_size=8192;b.memory_map_size=count*sizeof(map[0]);assert(pmm_init(&p,map,b.memory_map_size,sizeof(map[0])));failure(MMU_FRAMEBUFFER,MMU_CONFLICT);
 setup();b.framebuffer=UINT64_MAX-4096;b.framebuffer_size=8192;b.memory_map_size=count*sizeof(map[0]);assert(pmm_init(&p,map,b.memory_map_size,sizeof(map[0])));failure(MMU_FRAMEBUFFER,MMU_OVERFLOW);
 setup();b.framebuffer=(uintptr_t)arena;b.memory_map_size=count*sizeof(map[0]);assert(pmm_init(&p,map,b.memory_map_size,sizeof(map[0])));failure(MMU_FRAMEBUFFER,MMU_CONFLICT);
 setup();b.kernel_source=0x12345000;b.memory_map_size=count*sizeof(map[0]);assert(pmm_init(&p,map,b.memory_map_size,sizeof(map[0])));failure(MMU_MEMORY_MAP,MMU_COVERAGE);
 setup();for(unsigned i=0;i<1100;i++)add(2,0x100000000ULL+(uint64_t)i*0x40000000,1);b.memory_map_size=count*sizeof(map[0]);assert(pmm_init(&p,map,b.memory_map_size,sizeof(map[0])));failure(MMU_RAM,MMU_CAPACITY);assert(mmu_status_get()->tables==1024);
 puts("Production MMU: 2/8 GiB block layout, high/sparse/many descriptors, fragmented page allocation, RO/NX/guards, bounds/overlap/coverage/granule/PA/capacity/refusal tests passed");
}
