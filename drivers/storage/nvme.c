/* Original polling NVMe 1.x NVM command subset. Queue/DMA ownership belongs to
 * CHOS after controller disable/reset; never calls firmware for I/O. */
#include "storage.h"
#include "platform.h"
#include "mmu.h"
#include "pci.h"
#include "diagnostic.h"
#include "nvme_format.h"
struct queue { uint64_t sq,cq; unsigned tail,head,phase,id; uint16_t cid; };
static const char *error;
static char serial_number[21];
static bool read_only,diagnostic_only;
static unsigned driver_stage,last_completion;
const char *nvme_error(void) { diagnostic_value("NVMe failure stage: ",driver_stage,false);diagnostic_value("NVMe last completion: ",last_completion,true);return error; }
static struct { uint64_t bar,buffer,frequency,sectors; unsigned sector_size,stride,timeout_ms; struct queue admin,io; bool ready; } controller;
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
    while(((*reg(0x1c)&1)!=0)!=ready) if(counter()-start>controller.frequency/1000*controller.timeout_ms) return false;
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
        last_completion=result;
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
    if(!controller.ready || (write && (read_only || diagnostic_only)) || block>=controller.sectors/(4096/controller.sector_size)) return false;
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
    return controller.ready && !diagnostic_only && command(&controller.io,cmd);
}
bool nvme_init(struct pmm *physical,const struct chos_boot_info *boot,struct block_device *device,bool *identity) {
    driver_stage=1;last_completion=0;diagnostic_only=(boot->reserved&CHOS_DIAGNOSTIC_BOOT)!=0;
    error="NVME PCI DISCOVERY FAILED";
    *identity=false;controller=(typeof(controller)){0};serial_number[0]=0;
    struct pci_device pci;
    if(!pci_find_class(boot,0x010802,&pci)) {
        pci_report("NVMe PCI discovery",&pci);if(pci.error==4)error="NVME PCI CONTROLLER NOT FOUND";return false;
    }
    pci_report("NVMe driver selected",&pci);
    driver_stage=2;error="NVME BAR MAP FAILED";
    const volatile uint32_t *config=(void *)(uintptr_t)pci.config;
    uint64_t bar;
    if(!pci_memory_bar(config[4],config[5],0x4000,&bar))return false;
    controller.bar=bar;diagnostic_value("NVMe MMIO base: ",bar,true);
    if(!mmu_device(bar,0x4000))return false;
    driver_stage=3;error="NVME PCI ENABLE FAILED";if(!pci_enable(&pci,6))return false;
    driver_stage=4;error="NVME CAPABILITIES UNSUPPORTED";
    uint64_t cap=*reg(0)|((uint64_t)*reg(4)<<32);struct nvme_capabilities capabilities;
    diagnostic_value("NVMe CAP: ",cap,true);diagnostic_value("NVMe version: ",*reg(8),true);
    if(!nvme_capabilities(cap,&capabilities))return false;
    controller.stride=capabilities.stride;controller.timeout_ms=capabilities.timeout_ms;
    diagnostic_value("NVMe ready timeout ms: ",controller.timeout_ms,false);
    __asm__ volatile("mrs %0, cntfrq_el0":"=r"(controller.frequency));
    driver_stage=5;error="NVME DISABLE FAILED";
    *reg(0x14)=0; if(!wait_ready(false)) return false;
    driver_stage=6;error="NVME QUEUE ALLOCATION FAILED";
    if(!queue_init(physical,&controller.admin,0) || !queue_init(physical,&controller.io,1)) return false;
    controller.buffer=pmm_alloc(physical,1); if(!controller.buffer) return false;
    for(unsigned i=0;i<4096;i++) ((uint8_t *)(uintptr_t)controller.buffer)[i]=0;
    *reg(0x24)=(15u<<16)|15;
    *reg(0x28)=(uint32_t)controller.admin.sq; *reg(0x2c)=(uint32_t)(controller.admin.sq>>32);
    *reg(0x30)=(uint32_t)controller.admin.cq; *reg(0x34)=(uint32_t)(controller.admin.cq>>32);
    __asm__ volatile("dsb sy":::"memory");
    driver_stage=7;error="NVME ENABLE OR IDENTIFY FAILED";
    *reg(0x14)=(6u<<16)|(4u<<20)|1;
    if(!wait_ready(true) || !identify(0,1)) return false;
    const uint8_t *buffer=(void *)(uintptr_t)controller.buffer;
    for(unsigned i=0;i<20;i++)serial_number[i]=(char)buffer[4+i];
    serial_number[20]=0;for(unsigned i=20;i && serial_number[i-1]==' ';i--)serial_number[i-1]=0;
    const char serial[]="CHOS-TEST-0001"; bool match=true;
    for(unsigned i=0;i<20;i++) if(buffer[4+i]!=(uint8_t)(i<sizeof(serial)-1?serial[i]:' ')) match=false;
    *identity=match;
    driver_stage=8;error="NVME NAMESPACE IDENTIFY FAILED";
    if(!identify(1,0)) return false;
    struct nvme_namespace ns;error="NVME SECTOR FORMAT UNSUPPORTED";
    if(!nvme_namespace(buffer,4096,&ns))return false;
    controller.sectors=ns.sectors;controller.sector_size=ns.sector_bytes;read_only=ns.read_only;
    diagnostic_value("NVMe namespace 1 sectors: ",controller.sectors,false);
    diagnostic_value("NVMe sector bytes: ",controller.sector_size,false);
    diagnostic_log(diagnostic_only?"NVMe diagnostic policy: READ ONLY":"NVMe normal policy: explicit installer confirmation");
    uint32_t cmd[16]={0}; cmd[0]=5; cmd[10]=(15u<<16)|1; cmd[11]=1; prp(cmd,controller.io.cq);
    driver_stage=9;error="NVME IO COMPLETION QUEUE FAILED";
    if(!command(&controller.admin,cmd)) return false;
    for(unsigned i=0;i<16;i++) cmd[i]=0;
    cmd[0]=1; cmd[10]=(15u<<16)|1; cmd[11]=(1u<<16)|1; prp(cmd,controller.io.sq);
    driver_stage=10;error="NVME IO SUBMISSION QUEUE FAILED";
    if(!command(&controller.admin,cmd)) return false;
    driver_stage=11;controller.ready=true;diagnostic_log("NVMe controller and namespace operational: PASS");
    *device=(struct block_device){0,controller.sectors/(4096/controller.sector_size),read_block,write_block,flush};
    return true;
}

uint64_t nvme_bar(void) { return controller.bar; }

unsigned nvme_sector_size(void){return controller.ready?controller.sector_size:0;}
const char *nvme_serial(void){return serial_number;}
bool nvme_writable(void){return controller.ready && !read_only && !diagnostic_only;}
bool nvme_recheck(void){
    if(!controller.ready || !identify(0,1))return false;
    const uint8_t *b=(void *)(uintptr_t)controller.buffer;
    unsigned n=0;while(serial_number[n])n++;
    for(unsigned i=0;i<20;i++)if(b[4+i]!=(uint8_t)(i<n?serial_number[i]:' '))return false;
    if(!identify(1,0))return false;
    struct nvme_namespace ns;if(!nvme_namespace(b,4096,&ns) || ns.sectors!=controller.sectors || ns.sector_bytes!=controller.sector_size)return false;
    read_only=ns.read_only;return true;
}

/* Automatic input-failure diagnostics use the same non-writable media policy. */
void nvme_diagnostic_guard(void){diagnostic_only=true;}
