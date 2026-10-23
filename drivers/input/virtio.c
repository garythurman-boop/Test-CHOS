/* Modern Virtio PCI input, split receive queue, polling (no MSI-X dependency).
 * Event protocol codes are the standardized virtio-input wire ABI. */
#include "input.h"
#include "platform.h"
#include "mmu.h"
#include "console.h"
#include "usb.h"
struct device { volatile uint8_t *common,*notify,*isr; uint64_t desc,avail,used,buffer; uint16_t available,last; };
static struct device devices[2];
static unsigned device_count;
static uint16_t r16(volatile uint8_t *p,unsigned n) { return *(volatile uint16_t *)(p+n); }
static uint32_t r32(volatile uint8_t *p,unsigned n) { return *(volatile uint32_t *)(p+n); }
static void w16(volatile uint8_t *p,unsigned n,uint16_t v) { *(volatile uint16_t *)(p+n)=v; }
static void w32(volatile uint8_t *p,unsigned n,uint32_t v) { *(volatile uint32_t *)(p+n)=v; }
static void w64(volatile uint8_t *p,unsigned n,uint64_t v) { w32(p,n,(uint32_t)v); w32(p,n+4,(uint32_t)(v>>32)); }
static void cache(uint64_t address,size_t size,bool invalidate) {
    uint64_t ctr; __asm__ volatile("mrs %0, ctr_el0":"=r"(ctr));
    uint64_t line=4ULL<<((ctr>>16)&15);
    for(uint64_t a=address&~(line-1);a<address+size;a+=line) {
        if(invalidate) __asm__ volatile("dc ivac, %0"::"r"(a):"memory");
        else __asm__ volatile("dc cvac, %0"::"r"(a):"memory");
    }
    __asm__ volatile("dsb sy":::"memory");
}
static uint64_t page(struct pmm *p) {
    uint64_t a=pmm_alloc(p,1); if(!a) return 0;
    for(unsigned i=0;i<4096;i++) ((uint8_t *)(uintptr_t)a)[i]=0;
    return a;
}
static bool setup(struct pmm *p,volatile uint8_t *pci,struct device *d) {
    uint8_t visited[64]={0}; unsigned cap=pci[0x34]&0xfc,steps=0;
    uint64_t notify_size=0; uint32_t multiplier=0;
    *d=(struct device){0};
    while(cap) {
        if(cap<0x40 || cap>0xfc || visited[cap/4] || ++steps>48) return false;
        visited[cap/4]=1;
        if(pci[cap]==9) {
            unsigned length=pci[cap+2],type=pci[cap+3],bar=pci[cap+4];
            if(length<16 || length>256-cap || bar>5) return false;
            if(type>=1 && type<=3) {
                uint32_t low=r32(pci,0x10+bar*4);
                if((low&1) || ((low&6)==4 && bar==5)) return false;
                uint64_t base=low&~UINT64_C(15);
                if((low&6)==4) base|=(uint64_t)r32(pci,0x14+bar*4)<<32;
                uint32_t offset=r32(pci,cap+8),size=r32(pci,cap+12);
                if(!base || !size || offset>0x1000000 || size>0x1000000-offset) return false;
                uint64_t address=base+offset,aligned=address&~UINT64_C(4095);
                if(!mmu_device(aligned,(address-aligned+size+4095)&~UINT64_C(4095))) return false;
                if(type==1) { if(size<56) return false; d->common=(void *)(uintptr_t)address; }
                if(type==2) { if(length<20 || size<2) return false; d->notify=(void *)(uintptr_t)address; notify_size=size; multiplier=r32(pci,cap+16); }
                if(type==3) d->isr=(void *)(uintptr_t)address;
            }
        }
        cap=pci[cap+1]&0xfc;
    }
    if(!d->common || !d->notify || !d->isr) return false;
    w16(pci,4,r16(pci,4)|6);
    d->common[20]=0;
    if(d->common[20]) return false;
    d->common[20]=3;
    w32(d->common,0,1); if(!(r32(d->common,4)&1)) return false;
    w32(d->common,8,0); w32(d->common,12,0);
    w32(d->common,8,1); w32(d->common,12,1); /* VERSION_1 */
    d->common[20]=11; if(!(d->common[20]&8)) return false;
    w16(d->common,22,0);
    if(r16(d->common,24)<32 || r16(d->common,28)) return false;
    uint64_t notify_offset=(uint64_t)r16(d->common,30)*multiplier;
    if(notify_offset>notify_size-2) return false;
    d->notify+=notify_offset;
    d->desc=page(p); d->avail=page(p); d->used=page(p); d->buffer=page(p);
    if(!d->desc || !d->avail || !d->used || !d->buffer) return false;
    for(unsigned i=0;i<32;i++) {
        uint8_t *entry=(void *)(uintptr_t)(d->desc+i*16);
        *(uint64_t *)entry=d->buffer+i*8; *(uint32_t *)(entry+8)=8; *(uint16_t *)(entry+12)=2;
        ((uint16_t *)(uintptr_t)d->avail)[2+i]=(uint16_t)i;
    }
    d->available=32; ((uint16_t *)(uintptr_t)d->avail)[1]=32;
    cache(d->desc,4096,false); cache(d->avail,4096,false); cache(d->used,4096,false); cache(d->buffer,4096,false);
    w16(d->common,24,32); w16(d->common,26,0xffff);
    w64(d->common,32,d->desc); w64(d->common,40,d->avail); w64(d->common,48,d->used);
    w16(d->common,28,1); __asm__ volatile("dsb sy":::"memory");
    d->common[20]=15; w16(d->notify,0,0); return true;
}
int input_initialize(struct pmm *p,const struct chos_boot_info *boot) {
    device_count=0; input_reset(boot->width,boot->height);
    uint64_t ecam; if(!platform_pcie(boot,&ecam) || !mmu_device(ecam,0x100000)) return usb_initialize(p,boot);
    for(unsigned slot=0;slot<32 && device_count<2;slot++) for(unsigned function=0;function<8 && device_count<2;function++) {
        volatile uint8_t *pci=(void *)(uintptr_t)(ecam+(slot<<15)+(function<<12));
        if(r16(pci,0)==0x1af4 && r16(pci,2)==0x1052) {
            if(!setup(p,pci,&devices[device_count])) return -1;
            device_count++;
        }
    }
    return (int)device_count+usb_initialize(p,boot);
}
void input_poll(void) {
    usb_poll();
    struct input_event batch[64];unsigned count=0;
    for(unsigned i=0;i<device_count;i++) {
        struct device *d=&devices[i];
        cache(d->used,4096,true);
        uint16_t end=((volatile uint16_t *)(uintptr_t)d->used)[1];
        if((uint16_t)(end-d->last)>32) panic("VIRTIO INPUT USED INDEX INVALID");
        while(d->last!=end) {
            const uint32_t *used=(void *)(uintptr_t)(d->used+4+(d->last%32)*8);
            uint32_t id=used[0]; if(id>=32 || used[1]!=8) panic("VIRTIO INPUT COMPLETION INVALID");
            uint64_t address=d->buffer+id*8; cache(address,8,true);
            const struct input_event *event=(void *)(uintptr_t)address;
            if(count==64)panic("INPUT BATCH CAPACITY");batch[count++]=*event;
            ((uint16_t *)(uintptr_t)d->avail)[2+d->available%32]=(uint16_t)id;
            d->available++; ((uint16_t *)(uintptr_t)d->avail)[1]=d->available;
            d->last++;
        }
        cache(d->avail,4096,false); w16(d->notify,0,0); (void)*d->isr;
    }
    input_accept_batch(batch,count);
}
