/* Original polling NVMe 1.x NVM command subset. Queue/DMA ownership belongs to
 * CHOS after controller disable/reset; never calls firmware for I/O. */
#include "storage.h"
#include "platform.h"
#include "mmu.h"
struct queue { uint64_t sq,cq; unsigned tail,head,phase,id; uint16_t cid; };
static const char *error;
const char *nvme_error(void) { return error; }
static struct { uint64_t bar,buffer,frequency,sectors; unsigned sector_size,stride; struct queue admin,io; bool ready; } controller;
static volatile uint32_t *reg(unsigned n) { return (void *)(uintptr_t)(controller.bar+n); }
static uint64_t counter(void) { uint64_t n; __asm__ volatile("mrs %0, cntpct_el0":"=r"(n)); return n; }
static void cache(uint64_t address,size_t size,bool invalidate) {
    uint64_t ctr; __asm__ volatile("mrs %0, ctr_el0":"=r"(ctr));
    uint64_t line=4ULL<<((ctr>>16)&15);
    for(uint64_t a=address&~(line-1);a<address+size;a+=line) {
        if(invalidate) __asm__ volatile("dc ivac, %0"::"r"(a):"memory");
        else __asm__ volatile("dc cvac, %0"::"r"(a):"memory");
    }
    __asm__ volatile("dsb sy":::"memory");
}
static bool wait_ready(bool ready) {
    uint64_t start=counter();
    while(((*reg(0x1c)&1)!=0)!=ready) if(counter()-start>controller.frequency*2) return false;
    return (*reg(0x1c)&2)==0;
}
static bool queue_init(struct pmm *p,struct queue *q,unsigned id) {
    *q=(struct queue){.sq=pmm_alloc(p,1),.cq=pmm_alloc(p,1),.phase=1,.id=id};
    if(!q->sq || !q->cq) return false;
    for(unsigned i=0;i<4096;i++) { ((uint8_t *)(uintptr_t)q->sq)[i]=0; ((uint8_t *)(uintptr_t)q->cq)[i]=0; }
    cache(q->sq,4096,false); cache(q->cq,4096,false); return true;
}
static bool command(struct queue *q,uint32_t cmd[16]) {
    uint16_t cid=++q->cid;
    cmd[0]=(cmd[0]&0xffffu)|((uint32_t)cid<<16);
    uint32_t *target=(void *)(uintptr_t)(q->sq+q->tail*64);
    for(unsigned i=0;i<16;i++) target[i]=cmd[i];
    cache((uintptr_t)target,64,false);
    q->tail=(q->tail+1)%16;
    *reg(0x1000+(2*q->id)*controller.stride)=q->tail;
    uint64_t start=counter();
    volatile uint32_t *completion=(void *)(uintptr_t)(q->cq+q->head*16);
    for(;;) {
        cache((uintptr_t)completion,16,true);
        uint32_t result=completion[3];
        if(((result>>16)&1)==q->phase) {
            bool success=(uint16_t)result==cid && (result>>17)==0;
            q->head=(q->head+1)%16; if(!q->head) q->phase^=1;
            __asm__ volatile("dsb sy":::"memory");
            *reg(0x1000+(2*q->id+1)*controller.stride)=q->head;
            return success;
        }
        if(counter()-start>controller.frequency*2) { controller.ready=false; return false; }
    }
}
static void prp(uint32_t *cmd,uint64_t address) { cmd[6]=(uint32_t)address; cmd[7]=(uint32_t)(address>>32); }
static bool identify(unsigned ns,unsigned cns) {
    uint32_t cmd[16]={0}; cmd[0]=6; cmd[1]=ns; cmd[10]=cns; prp(cmd,controller.buffer);
    cache(controller.buffer,4096,false);
    if(!command(&controller.admin,cmd)) return false;
    cache(controller.buffer,4096,true); return true;
}
static bool transfer(void *unused,uint64_t block,void *data,bool write) {
    (void)unused;
    if(!controller.ready || block>=controller.sectors/(4096/controller.sector_size)) return false;
    uint8_t *buffer=(void *)(uintptr_t)controller.buffer;
    if(write) for(unsigned i=0;i<4096;i++) buffer[i]=((const uint8_t *)data)[i];
    cache(controller.buffer,4096,false);
    uint64_t lba=block*(4096/controller.sector_size);
    uint32_t cmd[16]={0}; cmd[0]=write?1:2; cmd[1]=1; prp(cmd,controller.buffer);
    cmd[10]=(uint32_t)lba; cmd[11]=(uint32_t)(lba>>32); cmd[12]=4096/controller.sector_size-1;
    if(!command(&controller.io,cmd)) return false;
    if(!write) { cache(controller.buffer,4096,true); for(unsigned i=0;i<4096;i++) ((uint8_t *)data)[i]=buffer[i]; }
    return true;
}
static bool read_block(void *c,uint64_t b,void *p) { return transfer(c,b,p,false); }
static bool write_block(void *c,uint64_t b,const void *p) { return transfer(c,b,(void *)p,true); }
static bool flush(void *unused) {
    (void)unused; uint32_t cmd[16]={0}; cmd[1]=1;
    return controller.ready && command(&controller.io,cmd);
}
bool nvme_init(struct pmm *physical,const struct chos_boot_info *boot,struct block_device *device,bool *identity) {
    error="PCI ACPI OR ECAM MAP FAILED";
    *identity=false; controller.ready=false;
    uint64_t ecam;
    if(!platform_pcie(boot,&ecam) || !mmu_device(ecam,0x100000)) return false;
    volatile uint32_t *pci=0;
    for(unsigned d=0;d<32 && !pci;d++) for(unsigned f=0;f<8;f++) {
        volatile uint32_t *candidate=(void *)(uintptr_t)(ecam+(d<<15)+(f<<12));
        if((candidate[0]&0xffff)!=0xffff && (candidate[2]>>8)==0x010802) { pci=candidate; break; }
    }
    error="NVME PCI CONTROLLER NOT FOUND";
    if(!pci || (pci[4]&1)) return false;
    uint64_t bar=pci[4]&~UINT64_C(15);
    if((pci[4]&6)==4) bar|=(uint64_t)pci[5]<<32;
    error="NVME BAR MAP FAILED";
    controller.bar=bar;
    if(!bar || !mmu_device(bar,0x4000)) return false;
    controller.bar=bar;
    *(volatile uint16_t *)(uintptr_t)((uintptr_t)pci+4)|=6;
    error="NVME CAPABILITIES UNSUPPORTED";
    uint64_t cap=*reg(0)|((uint64_t)*reg(4)<<32);
    if((cap&0xffff)<15 || !(cap&(1ULL<<37)) || ((cap>>48)&15) || ((cap>>32)&15)>7) return false;
    controller.stride=4u<<((cap>>32)&15);
    __asm__ volatile("mrs %0, cntfrq_el0":"=r"(controller.frequency));
    error="NVME DISABLE FAILED";
    *reg(0x14)=0; if(!wait_ready(false)) return false;
    error="NVME QUEUE ALLOCATION FAILED";
    if(!queue_init(physical,&controller.admin,0) || !queue_init(physical,&controller.io,1)) return false;
    controller.buffer=pmm_alloc(physical,1); if(!controller.buffer) return false;
    for(unsigned i=0;i<4096;i++) ((uint8_t *)(uintptr_t)controller.buffer)[i]=0;
    *reg(0x24)=(15u<<16)|15;
    *reg(0x28)=(uint32_t)controller.admin.sq; *reg(0x2c)=(uint32_t)(controller.admin.sq>>32);
    *reg(0x30)=(uint32_t)controller.admin.cq; *reg(0x34)=(uint32_t)(controller.admin.cq>>32);
    __asm__ volatile("dsb sy":::"memory");
    error="NVME ENABLE OR IDENTIFY FAILED";
    *reg(0x14)=(6u<<16)|(4u<<20)|1;
    if(!wait_ready(true) || !identify(0,1)) return false;
    const uint8_t *buffer=(void *)(uintptr_t)controller.buffer;
    const char serial[]="CHOS-TEST-0001"; bool match=true;
    for(unsigned i=0;i<20;i++) if(buffer[4+i]!=(uint8_t)(i<sizeof(serial)-1?serial[i]:' ')) match=false;
    *identity=match;
    error="NVME NAMESPACE IDENTIFY FAILED";
    if(!identify(1,0)) return false;
    controller.sectors=*(const uint64_t *)buffer;
    unsigned format=buffer[26]&15;
    uint32_t lbaf=*(const uint32_t *)(buffer+128+format*4);
    unsigned shift=(lbaf>>16)&255;
    error="NVME SECTOR FORMAT UNSUPPORTED";
    if((lbaf&0xffff) || (shift!=9 && shift!=12) || controller.sectors<128) return false;
    controller.sector_size=1u<<shift;
    uint32_t cmd[16]={0}; cmd[0]=5; cmd[10]=(15u<<16)|1; cmd[11]=1; prp(cmd,controller.io.cq);
    error="NVME IO COMPLETION QUEUE FAILED";
    if(!command(&controller.admin,cmd)) return false;
    for(unsigned i=0;i<16;i++) cmd[i]=0;
    cmd[0]=1; cmd[10]=(15u<<16)|1; cmd[11]=(1u<<16)|1; prp(cmd,controller.io.sq);
    error="NVME IO SUBMISSION QUEUE FAILED";
    if(!command(&controller.admin,cmd)) return false;
    controller.ready=true;
    *device=(struct block_device){0,controller.sectors/(4096/controller.sector_size),read_block,write_block,flush};
    return true;
}

uint64_t nvme_bar(void) { return controller.bar; }
