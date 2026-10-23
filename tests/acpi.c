#include "platform.h"
#include "memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static _Alignas(4096) unsigned char data[4096];
static void w32(unsigned char *p,uint32_t n) { for(unsigned i=0;i<4;i++) p[i]=(unsigned char)(n>>(i*8)); }
static void w64(unsigned char *p,uint64_t n) { for(unsigned i=0;i<8;i++) p[i]=(unsigned char)(n>>(i*8)); }
static void sum(unsigned char *p,unsigned len,unsigned field) {
    p[field]=0; unsigned char s=0;
    for(unsigned i=0;i<len;i++) s=(unsigned char)(s+p[i]);
    p[field]=(unsigned char)-s;
}
int main(void) {
    unsigned char *r=data,*x=data+128,*m=data+256,*g=data+512,*mcfg=data+768;
    memcpy(r,"RSD PTR ",8); r[15]=2; w32(r+20,36); w64(r+24,(uintptr_t)x);
    memcpy(x,"XSDT",4); w32(x+4,60); w64(x+36,(uintptr_t)m); w64(x+44,(uintptr_t)g); w64(x+52,(uintptr_t)mcfg);
    memcpy(m,"APIC",4); w32(m+4,148);
    unsigned char *cpu=m+44,*dist=m+124;
    cpu[0]=11; cpu[1]=80; w32(cpu+12,1); w64(cpu+32,0x8010000); w64(cpu+68,0x102);
    dist[0]=12; dist[1]=24; w64(dist+8,0x8000000); dist[20]=2;
    memcpy(g,"GTDT",4); w32(g+4,104); w32(g+56,30);
    memcpy(mcfg,"MCFG",4); w32(mcfg+4,60); w64(mcfg+44,0x4010000000ULL); mcfg[55]=255; sum(mcfg,60,9);
    sum(r,20,8); sum(r,36,32); sum(x,60,9); sum(m,148,9); sum(g,104,9);
    struct memory_descriptor d={.type=9,.physical=(uintptr_t)data,.pages=1};
    struct chos_boot_info b={.memory_map=(uintptr_t)&d,.memory_map_size=sizeof(d),.descriptor_size=sizeof(d),.rsdp=(uintptr_t)r};
    struct platform p;
    assert(platform_discover(&b,0x102,&p));
    uint64_t ecam=0;
    assert(platform_pcie(&b,&ecam) && ecam==0x4010000000ULL);
    mcfg[54]=1; sum(mcfg,60,9); assert(!platform_pcie(&b,&ecam));
    mcfg[54]=0; mcfg[52]=1; sum(mcfg,60,9); assert(!platform_pcie(&b,&ecam));
    mcfg[52]=0; sum(mcfg,60,9);
    assert(p.gic_distributor==0x8000000 && p.gic_cpu==0x8010000 && p.timer_irq==30);
    assert(!platform_discover(&b,0,&p));
    g[9]++; assert(!platform_discover(&b,0x102,&p)); g[9]--;
    cpu[1]=1; sum(m,148,9); assert(!platform_discover(&b,0x102,&p));
    cpu[1]=80; sum(m,148,9);
    w32(g+56,42); sum(g,104,9); assert(!platform_discover(&b,0x102,&p));
    w32(g+56,30); sum(g,104,9);
    w64(x+44,UINT64_MAX-15); sum(x,60,9); assert(!platform_discover(&b,0x102,&p));
    w64(x+44,(uintptr_t)g); w64(x+52,(uintptr_t)mcfg); sum(x,60,9);
    d.type=11; assert(!platform_discover(&b,0x102,&p)); d.type=9;
    w32(r+20,8192); sum(r,36,32); assert(!platform_discover(&b,0x102,&p));
    w32(r+20,36);sum(r,36,32);
    /* GICv3: region discovery and the current CPU's direct GICC base. */
    dist[20]=3;w64(cpu+32,0);w32(m+4,164);unsigned char *rd=m+148;
    rd[0]=14;rd[1]=16;w64(rd+4,0x90a0000);w32(rd+12,0x100000);sum(m,164,9);
    assert(platform_discover(&b,0x102,&p) && p.gic_version==3 && p.redistributor_count==1 && p.redistributors[0].base==0x90a0000);
    w32(rd+12,0x10000);sum(m,164,9);assert(!platform_discover(&b,0x102,&p));
    w32(rd+12,0x100000);w64(rd+4,(uintptr_t)data);sum(m,164,9);assert(!platform_discover(&b,0x102,&p));
    w64(rd+4,0x90a0000);sum(m,164,9);
    w64(cpu+60,0x90a0000);w32(m+4,148);sum(m,148,9);assert(platform_discover(&b,0x102,&p) && p.gic_redistributor==0x90a0000);
    w64(cpu+60,0x90a1234);sum(m,148,9);assert(!platform_discover(&b,0x102,&p));
    w64(cpu+60,0);sum(m,148,9);assert(!platform_discover(&b,0x102,&p));
    dist[20]=2;w64(cpu+32,0x8010000);sum(m,148,9);
    unsigned char *fadt=data+1024;memcpy(fadt,"FACP",4);w32(fadt+4,132);fadt[8]=5;fadt[129]=3;sum(fadt,132,9);
    w32(x+4,68);w64(x+60,(uintptr_t)fadt);sum(x,68,9);
    bool hvc=false;assert(platform_psci(&b,&hvc) && hvc);
    fadt[129]=1;sum(fadt,132,9);assert(platform_psci(&b,&hvc) && !hvc);
    fadt[129]=0;sum(fadt,132,9);assert(!platform_psci(&b,&hvc));
    fadt[129]=3;sum(fadt,132,9);fadt[9]++;assert(!platform_psci(&b,&hvc));
    sum(fadt,132,9);
    unsigned char *spcr=data+1280;memcpy(spcr,"SPCR",4);w32(spcr+4,80);spcr[36]=3;spcr[41]=32;spcr[43]=3;w64(spcr+44,0x9000000);sum(spcr,80,9);
    w32(x+4,76);w64(x+68,(uintptr_t)spcr);sum(x,76,9);
    uint32_t mask=0;unsigned tables=0;uint64_t uart=0;
    assert(platform_tables(&b,&mask,&tables) && mask==31 && tables==5);
    assert(platform_serial(&b,&uart) && uart==0x9000000);
    spcr[40]=1;sum(spcr,80,9);assert(!platform_serial(&b,&uart));
    spcr[40]=0;w64(spcr+44,(uintptr_t)data);sum(spcr,80,9);assert(!platform_serial(&b,&uart));
    w64(spcr+44,0x9000001);sum(spcr,80,9);assert(!platform_serial(&b,&uart));
    spcr[9]++;assert(!platform_tables(&b,&mask,&tables) && !platform_serial(&b,&uart));
    puts("PASS: ACPI discovery, checksums, MPIDR selection, malformed entries, IRQ ranges, out-of-map pointers, reserved/MMIO exclusion and oversized RSDP");
}
