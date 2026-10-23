/* Original GICv3 Group-1 non-secure, single boot-CPU implementation.
 * Addresses and affinity come from ACPI; no virtual machine address constants. */
#include "gic.h"
static uint32_t read32(struct gic_v3 *g,uint64_t a){return g->io.read32(g->io.context,a);}
static uint64_t read64(struct gic_v3 *g,uint64_t a){return g->io.read64(g->io.context,a);}
static void write32(struct gic_v3 *g,uint64_t a,uint32_t v){g->io.write32(g->io.context,a,v);}
static bool wait(struct gic_v3 *g,uint64_t address,uint32_t mask){
    uint64_t start=g->io.counter(g->io.context);
    while(read32(g,address)&mask)if(g->io.counter(g->io.context)-start>=g->io.frequency)return false;
    return true;
}
static uint64_t find(struct gic_v3 *g,uint64_t base,uint64_t size,uint32_t affinity){
    if(!base || base%65536 || size<0x20000 || size>64*1024*1024 || size%65536 || base>=(1ULL<<48)-size)return 0;
    for(uint64_t at=0;at<=size-0x20000;){
        uint64_t typer=read64(g,base+at+8);
        if((uint32_t)(typer>>32)==affinity){
            unsigned arch=(read32(g,base+at+0xffe8)>>4)&15;
            return arch==3 || arch==4?base+at:0;
        }
        if(typer&(1u<<4))break;
        at+=(typer&2)?0x40000:0x20000;
    }
    return 0;
}
bool gic_v3_enable(struct gic_v3 *g,unsigned irq,bool enabled){
    if(!g || irq<16 || irq>=g->interrupts || !g->redistributor)return false;
    uint64_t base=irq<32?g->redistributor+0x10000:g->distributor;
    write32(g,base+(enabled?0x100:0x180)+(irq/32)*4,1u<<(irq%32));
    return wait(g,irq<32?g->redistributor:g->distributor,irq<32?8u:0x80000000u);
}
bool gic_v3_prepare(struct gic_v3 *g,const struct platform *p,struct gic_io io){
    if(!g || !p || p->gic_version!=3 || !io.read32 || !io.read64 || !io.write32 || !io.write64 || !io.counter || !io.frequency || p->redistributor_count>8 || p->timer_irq<16 || p->timer_irq>31 || (p->timer_flags&2) || !p->gic_distributor || p->gic_distributor%65536 || p->gic_distributor>=(1ULL<<48)-65536)return false;
    *g=(struct gic_v3){.io=io,.distributor=p->gic_distributor};
    uint32_t affinity=(uint32_t)(p->mpidr&0xffffff)|((uint32_t)(p->mpidr>>8)&0xff000000);
    g->affinity=p->mpidr&0xff00ffffffULL;
    unsigned arch=(read32(g,p->gic_distributor+0xffe8)>>4)&15;
    if(arch!=3 && arch!=4)return false;
    if(p->gic_redistributor)g->redistributor=find(g,p->gic_redistributor,0x20000,affinity);
    else for(unsigned i=0;i<p->redistributor_count && !g->redistributor;i++)g->redistributor=find(g,p->redistributors[i].base,p->redistributors[i].size,affinity);
    if(!g->redistributor)return false;
    g->interrupts=((read32(g,g->distributor+4)&31)+1)*32;if(g->interrupts>1020)g->interrupts=1020;
    uint32_t control=read32(g,g->distributor)&~7u;
    write32(g,g->distributor,control);
    if(!wait(g,g->distributor,0x80000000))return false;
    for(unsigned i=1;i<(g->interrupts+31)/32;i++){
        write32(g,g->distributor+0x180+i*4,0xffffffff);
        write32(g,g->distributor+0x280+i*4,0xffffffff);
        write32(g,g->distributor+0x380+i*4,0xffffffff);
        write32(g,g->distributor+0x80+i*4,0xffffffff);
        write32(g,g->distributor+0xd00+i*4,0);
    }
    if(!wait(g,g->distributor,0x80000000))return false;
    write32(g,g->distributor,control|0x10); /* affinity routing, groups disabled */
    if(!wait(g,g->distributor,0x80000000))return false;
    for(unsigned irq=32;irq<g->interrupts;irq++)g->io.write64(io.context,g->distributor+0x6000+irq*8,g->affinity);
    uint64_t r=g->redistributor;
    write32(g,r+0x14,read32(g,r+0x14)&~2u);
    if(!wait(g,r+0x14,4))return false;
    uint64_t s=r+0x10000;
    write32(g,s+0x180,0xffffffff);write32(g,s+0x280,0xffffffff);write32(g,s+0x380,0xffffffff);
    if(!wait(g,r,8))return false;
    write32(g,s+0x80,0xffffffff);write32(g,s+0xd00,0);
    unsigned shift=(p->timer_irq%4)*8;uint64_t priority=s+0x400+(p->timer_irq&~3u);
    write32(g,priority,(read32(g,priority)&~(255u<<shift))|(0x80u<<shift));
    shift=(p->timer_irq%16)*2;
    write32(g,s+0xc04,(read32(g,s+0xc04)&~(3u<<shift))|((p->timer_flags&1?2u:0u)<<shift));
    if(!gic_v3_enable(g,p->timer_irq,true))return false;
    write32(g,g->distributor,control|0x13); /* both NS-view Group-1 aliases */
    return wait(g,g->distributor,0x80000000);
}
