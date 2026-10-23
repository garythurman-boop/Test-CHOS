#include "gic.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define DIST 0x200000ULL
#define REDIST 0x4400000ULL
static struct fixture {uint32_t d[65536/4],r[0x60000/4];unsigned routes,writes;uint64_t time;bool stuck_dist,stuck_wake,stuck_redist;} f;
static uint32_t *cell(uint64_t a){
    assert(!(a%4));
    if(a>=DIST && a<DIST+sizeof(f.d))return &f.d[(a-DIST)/4];
    assert(a>=REDIST && a<REDIST+sizeof(f.r));return &f.r[(a-REDIST)/4];
}
static uint32_t read32(void *c,uint64_t a){(void)c;
    if(a==DIST && f.stuck_dist)return *cell(a)|0x80000000;
    if(a==REDIST+0x40000 && f.stuck_redist)return 8;
    if(a==REDIST+0x40014 && f.stuck_wake)return 4;
    return *cell(a);
}
static uint64_t read64(void *c,uint64_t a){return read32(c,a)|((uint64_t)read32(c,a+4)<<32);}
static void write32(void *c,uint64_t a,uint32_t v){(void)c;f.writes++;*cell(a)=v;if(a==REDIST+0x40014 && !f.stuck_wake)*cell(a)&=~4u;}
static void write64(void *c,uint64_t a,uint64_t v){(void)c;assert(a>=DIST+0x6100 && a<DIST+0x6400 && !(a%8));assert(v==0x100000102ULL);f.routes++;*cell(a)=(uint32_t)v;*cell(a+4)=(uint32_t)(v>>32);}
static uint64_t counter(void *c){(void)c;return f.time++;}
static struct platform platform(void){
    memset(&f,0,sizeof(f));*cell(DIST+4)=3;*cell(DIST+0xffe8)=0x30;*cell(DIST)=0x40;
    *cell(REDIST+8)=2;*cell(REDIST+12)=0x9999; /* VLPIS: stride 256 KiB */
    *cell(REDIST+0x40008)=16;*cell(REDIST+0x4000c)=0x01000102;
    *cell(REDIST+0x4ffe8)=0x30;*cell(REDIST+0x40014)=6;
    *cell(REDIST+0x50c04)=0xffffffff;
    struct platform p={.gic_version=3,.gic_distributor=DIST,.mpidr=0x100000102ULL,.timer_irq=30,.redistributor_count=1};
    p.redistributors[0].base=REDIST;p.redistributors[0].size=0x60000;return p;
}
int main(void){
    struct gic_io io={0,read32,read64,write32,write64,counter,100};struct gic_v3 g;struct platform p=platform();
    assert(gic_v3_prepare(&g,&p,io));assert(g.redistributor==REDIST+0x40000 && g.interrupts==128 && f.routes==96);
    assert(*cell(DIST)==0x53 && *cell(REDIST+0x50080)==0xffffffff && *cell(REDIST+0x50100)==1u<<30);
    assert(!(*cell(REDIST+0x50c04)&(3u<<28)));assert(((*cell(REDIST+0x5041c)>>16)&255)==0x80);
    assert(gic_v3_enable(&g,30,false) && *cell(REDIST+0x50180)==1u<<30);
    assert(gic_v3_enable(&g,127,true) && *cell(DIST+0x10c)==1u<<31);
    unsigned writes=f.writes;assert(!gic_v3_enable(&g,15,true) && !gic_v3_enable(&g,128,true) && writes==f.writes);
    p=platform();p.mpidr=1;assert(!gic_v3_prepare(&g,&p,io) && !f.writes); /* absent CPU */
    p=platform();p.redistributors[0].size=0x10000;assert(!gic_v3_prepare(&g,&p,io));
    p=platform();p.gic_redistributor=REDIST+0x40000;assert(gic_v3_prepare(&g,&p,io));
    p=platform();p.timer_flags=1;assert(gic_v3_prepare(&g,&p,io) && (*cell(REDIST+0x50c04)&(3u<<28))==(2u<<28));
    p=platform();p.timer_flags=2;assert(!gic_v3_prepare(&g,&p,io) && !f.writes);
    p=platform();f.stuck_dist=true;assert(!gic_v3_prepare(&g,&p,io) && f.time>=100);
    p=platform();f.stuck_wake=true;assert(!gic_v3_prepare(&g,&p,io) && f.time>=100);
    p=platform();f.stuck_redist=true;assert(!gic_v3_prepare(&g,&p,io) && f.time>=100);
    p=platform();*cell(DIST+0xffe8)=0x20;assert(!gic_v3_prepare(&g,&p,io) && !f.writes);
    p=platform();*cell(REDIST+8)=16;assert(!gic_v3_prepare(&g,&p,io) && !f.writes); /* Last terminates scan */
    puts("GICv3 affinity/stride/Last, direct GICC frame, SPI routing, PPI masking/priority/trigger, invalid resources and RWP/wake timeouts passed");
}
