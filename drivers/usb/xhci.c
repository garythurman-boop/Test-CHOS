/* Original xHCI boot-HID bring-up. One controller, two direct-root devices.
 * Polled event ring: MSI/INTx routing and hubs remain explicitly unsupported. */
#include "usb.h"
#include "platform.h"
#include "mmu.h"
#include "diagnostic.h"
struct hid_device {unsigned slot,port,speed,dci;struct usb_ring control,input;uint64_t contexts,output,data,report,pending[8],reports;struct usb_boot_interface interface;struct usb_hid_state hid;bool ready,warned;};
static struct {uint64_t base,op,runtime,doorbell,dcbaa,events,erst;struct usb_ring commands;unsigned consumer,cycle,context_size,ports,slots,count;bool running;struct pmm *memory;struct hid_device device[2];} host;
static uint32_t r32(uint64_t a){return *(volatile uint32_t *)(uintptr_t)a;}
static void w32(uint64_t a,uint32_t v){*(volatile uint32_t *)(uintptr_t)a=v;}
static void w64(uint64_t a,uint64_t v){w32(a,(uint32_t)v);w32(a+4,(uint32_t)(v>>32));}
static uint64_t now(void){uint64_t n;__asm__ volatile("mrs %0, cntpct_el0":"=r"(n));return n;}
static uint64_t frequency(void){uint64_t n;__asm__ volatile("mrs %0, cntfrq_el0":"=r"(n));return n;}
static bool wait_bits(uint64_t a,uint32_t bits,uint32_t expected){uint64_t start=now();while((r32(a)&bits)!=expected)if(now()-start>frequency())return false;return true;}
static void cache(uint64_t address,bool invalidate){
    uint64_t ctr;__asm__ volatile("mrs %0, ctr_el0":"=r"(ctr));unsigned line=4u<<((ctr>>16)&15);
    for(uint64_t at=address;at<address+4096;at+=line){if(invalidate)__asm__ volatile("dc ivac, %0"::"r"(at):"memory");else __asm__ volatile("dc cvac, %0"::"r"(at):"memory");}
    __asm__ volatile("dsb sy":::"memory");
}
static uint64_t page(void){uint64_t a=pmm_alloc(host.memory,1);if(a){for(unsigned i=0;i<4096;i++)((uint8_t *)(uintptr_t)a)[i]=0;cache(a,false);}return a;}
static bool ring_init(struct usb_ring *r){r->address=page();r->index=0;r->cycle=1;return r->address!=0;}
static uint64_t enqueue(struct usb_ring *r,uint64_t parameter,uint32_t status,uint32_t control){
    uint64_t at=usb_ring_push(r,parameter,status,control);
    cache(r->address,false);return at;
}

static void ring_doorbell(unsigned slot,unsigned target){__asm__ volatile("dsb sy":::"memory");w32(host.doorbell+slot*4,target);}
static bool event(struct usb_trb *out){
    cache(host.events,true);
    if(!usb_event_pop(host.events,&host.consumer,&host.cycle,out))return false;
    w64(host.runtime+0x18,(host.events+host.consumer*16)|8);return true;
}

static void submit_input(struct hid_device *d,unsigned receive){
    d->pending[receive]=enqueue(&d->input,d->report+receive*512,d->interface.packet,(1u<<10)|(1u<<5)|(1u<<2));ring_doorbell(d->slot,d->dci);
}
static void disconnect(struct hid_device *d){
    struct input_event batch[32];unsigned count=0;uint8_t empty[8]={0};
    if(usb_hid_report(&d->hid,d->interface.protocol,empty,d->interface.protocol==1?8:3,batch,&count))input_accept_batch(batch,count);
    diagnostic_value("USB releases on disconnect: ",count,false);
    d->ready=false;
}
static void input_event(const struct usb_trb *e){
    unsigned slot=e->control>>24,endpoint=(e->control>>16)&31,completion=e->status>>24;
    for(unsigned i=0;i<host.count;i++){
        struct hid_device *d=&host.device[i];if(!d->ready || d->slot!=slot || d->dci!=endpoint)continue;
        unsigned receive=0;while(receive<8 && d->pending[receive]!=e->parameter)receive++;if(receive==8)continue;
        uint64_t buffer=d->report+receive*512;
        unsigned residual=e->status&0xffffff;
        if((completion!=1 && completion!=13) || residual>d->interface.packet || !usb_dma_span(d->report,4096,buffer,d->interface.packet-residual)){disconnect(d);diagnostic_log("USB HID transfer failed; endpoint stopped");return;}
        cache(d->report,true);struct input_event batch[32];unsigned count=0;
        if(usb_hid_report(&d->hid,d->interface.protocol,(void *)(uintptr_t)buffer,d->interface.packet-residual,batch,&count))input_accept_batch(batch,count);
        else if(!d->warned){diagnostic_log("USB HID malformed/unsupported report rejected");d->warned=true;}
        d->reports++;if(d->reports%255==0)diagnostic_value("USB completed receive reports: ",d->reports,false);
        submit_input(d,receive);return;
    }
}
static bool completion(unsigned type,uint64_t pointer,unsigned *slot){
    uint64_t start=now();struct usb_trb e;
    while(now()-start<frequency()){
        if(!event(&e))continue;
        if(((e.control>>10)&63)==type && (e.parameter&~15ULL)==pointer){if(slot)*slot=e.control>>24;return (e.status>>24)==1;}
        if(((e.control>>10)&63)==32)input_event(&e);
    }
    diagnostic_log("xHCI command/transfer timeout");return false;
}
static bool command(uint64_t parameter,uint32_t control,unsigned *slot){uint64_t at=enqueue(&host.commands,parameter,0,control);ring_doorbell(0,0);return completion(33,at,slot);}
static bool control(struct hid_device *d,unsigned request,unsigned value,unsigned index,unsigned length,bool in){
    if(length>512 || (length && !in) || !usb_dma_span(d->data,4096,d->data,length))return false;
    uint64_t setup=(uint64_t)(request&255)|((uint64_t)(request>>8)<<8)|((uint64_t)value<<16)|((uint64_t)index<<32)|((uint64_t)length<<48);
    enqueue(&d->control,setup,8,(2u<<10)|(1u<<6)|((length?3u:0u)<<16));
    if(length)enqueue(&d->control,d->data,length,(3u<<10)|(1u<<16));
    uint64_t at=enqueue(&d->control,0,0,(4u<<10)|(1u<<5)|(!length?1u<<16:0));
    ring_doorbell(d->slot,1);if(!completion(32,at,0))return false;
    if(length)cache(d->data,true);return true;
}
static uint32_t *context(uint64_t base,unsigned index){return (void *)(uintptr_t)(base+(uint64_t)index*host.context_size);}
static bool enumerate(struct hid_device *d,unsigned port){
    uint64_t portsc=host.op+0x400+(port-1)*16;uint32_t status=r32(portsc);
    if(!(status&1))return false;
    w32(portsc,(status&0xe00)|16); /* preserve power/indicator, start USB2 reset */
    if(!wait_bits(portsc,16,0) || !wait_bits(portsc,2,2))return false;
    d->port=port;d->speed=(r32(portsc)>>10)&15;
    if(d->speed<1 || d->speed>3){diagnostic_log("USB root speed unsupported (USB2 only)");return false;}
    unsigned slot=0;if(!command(0,9u<<10,&slot) || !slot || slot>host.slots)return false;d->slot=slot;
    d->contexts=page();d->output=page();d->data=page();d->report=page();
    if(!d->contexts || !d->output || !d->data || !d->report || !ring_init(&d->control) || !ring_init(&d->input))return false;
    ((uint64_t *)(uintptr_t)host.dcbaa)[slot]=d->output;cache(host.dcbaa,false);
    uint32_t *input=context(d->contexts,0),*s=context(d->contexts,1),*ep=context(d->contexts,2);
    input[1]=3;s[0]=(1u<<27)|(d->speed<<20);s[1]=port<<16;
    unsigned packet=d->speed==3?64:8;
    ep[1]=(packet<<16)|(4u<<3)|6;*(uint64_t *)(ep+2)=d->control.address|1;ep[4]=8;cache(d->contexts,false);
    if(!command(d->contexts,(11u<<10)|(slot<<24),0))return false;
    if(!control(d,0x0680,0x100,0,8,true))return false;
    const uint8_t *data=(void *)(uintptr_t)d->data;
    if(data[0]!=18 || data[1]!=1 || (data[7]!=8 && data[7]!=16 && data[7]!=32 && data[7]!=64))return false;
    if(packet!=data[7]){
        input[1]=2;ep[1]=(ep[1]&65535)|((unsigned)data[7]<<16);cache(d->contexts,false);
        if(!command(d->contexts,(13u<<10)|(slot<<24),0))return false;
    }
    if(!control(d,0x0680,0x100,0,18,true) || !control(d,0x0680,0x200,0,9,true))return false;
    unsigned bytes=(unsigned)data[2]|(unsigned)data[3]<<8;
    if(bytes<9 || bytes>512 || !control(d,0x0680,0x200,0,bytes,true) || !usb_boot_configuration(data,bytes,&d->interface))return false;
    if(!control(d,0x0900,d->interface.configuration,0,0,false) || !control(d,0x0b21,0,d->interface.interface,0,false))return false;
    /* SET_IDLE is optional; keyboards still emit changed reports without it. */
    (void)control(d,0x0a21,0,d->interface.interface,0,false);
    d->dci=d->interface.endpoint*2+1;
    for(unsigned i=0;i<4096;i++)((uint8_t *)(uintptr_t)d->contexts)[i]=0;
    input=context(d->contexts,0);s=context(d->contexts,1);ep=context(d->contexts,d->dci+1);
    input[1]=1u|(1u<<d->dci);s[0]=(d->dci<<27)|(d->speed<<20);s[1]=port<<16;
    unsigned interval=0;if(d->speed==3)interval=d->interface.interval-1;
    else {unsigned n=d->interface.interval;while(n>1){interval++;n>>=1;}interval+=3;}
    if(interval>15)return false;
    ep[0]=interval<<16;ep[1]=(d->interface.packet<<16)|(7u<<3)|6;
    *(uint64_t *)(ep+2)=d->input.address|1;ep[4]=d->interface.packet|((uint32_t)d->interface.packet<<16);cache(d->contexts,false);
    if(!command(d->contexts,(12u<<10)|(slot<<24),0))return false;
    d->ready=true;for(unsigned receive=0;receive<8;receive++)submit_input(d,receive);
    diagnostic_log(d->interface.protocol==1?"USB boot keyboard initialized: PASS":"USB boot mouse initialized: PASS");return true;
}
int usb_initialize(struct pmm *memory,const struct chos_boot_info *boot){
    host=(typeof(host)){0};host.memory=memory;uint64_t ecam;unsigned stage=1;
    if(!platform_pcie(boot,&ecam) || !mmu_device(ecam,0x100000))return 0;
    uint64_t pci=0;
    for(unsigned slot=0;slot<32 && !pci;slot++)for(unsigned fn=0;fn<8;fn++){
        uint64_t at=ecam+(slot<<15)+(fn<<12);
        if((r32(at)&65535)!=65535 && (r32(at+8)>>8)==0x0c0330){pci=at;break;}
    }
    if(!pci)return 0;
    uint32_t low=r32(pci+16);if((low&1) || ((low&6)!=0 && (low&6)!=4))goto failure;
    host.base=low&~15ULL;if((low&6)==4)host.base|=(uint64_t)r32(pci+20)<<32;
    if(!host.base || host.base%4096 || !mmu_device(host.base,65536))goto failure;
    *(volatile uint16_t *)(uintptr_t)(pci+4)|=2;
    stage=2;unsigned cap=r32(host.base),hcs=r32(host.base+4),hcs2=r32(host.base+8),hcc=r32(host.base+16);
    diagnostic_value("xHCI capabilities: ",hcc,true);diagnostic_value("xHCI HCSPARAMS2: ",hcs2,true);
    if((cap&255)<32 || (cap&255)>128 || !(hcc&1) || (hcs2&0xfbe00000))goto failure; /* scratchpads not implemented */
    host.op=host.base+(cap&255);host.context_size=(hcc&4)?64:32;host.slots=hcs&255;host.ports=hcs>>24;
    unsigned db=r32(host.base+20)&~3u,rt=r32(host.base+24)&~31u;
    if(!host.slots || !host.ports || host.ports>32 || db>0xfc00 || rt>0xff80 || db<0x1000 || rt<0x1000)goto failure;
    host.doorbell=host.base+db;host.runtime=host.base+rt+0x20;
    stage=3;uint32_t ext=hcc>>16;for(unsigned steps=0;ext && steps<64;steps++){
        if(ext>16380)goto failure;uint32_t c=r32(host.base+ext*4);
        if((c&255)==1 && (c&(1u<<16))){diagnostic_log("xHCI firmware ownership handoff unsupported");goto failure;}
        unsigned next=(c>>8)&255;if(!next)break;ext+=next;if(steps==63)goto failure;
    }
    stage=4;w32(host.op,r32(host.op)&~1u);if(!wait_bits(host.op+4,1,1))goto failure;
    stage=5;w32(host.op,2);if(!wait_bits(host.op,2,0) || !wait_bits(host.op+4,1u<<11,0) || !(r32(host.op+8)&1))goto failure;
    stage=6;host.dcbaa=page();host.events=page();host.erst=page();if(!host.dcbaa || !host.events || !host.erst || !ring_init(&host.commands))goto failure;
    ((uint64_t *)(uintptr_t)host.erst)[0]=host.events;((uint64_t *)(uintptr_t)host.erst)[1]=256;cache(host.erst,false);
    *(volatile uint16_t *)(uintptr_t)(pci+4)|=4;
    host.cycle=1;w64(host.op+0x30,host.dcbaa);w64(host.op+0x18,host.commands.address|1);w32(host.op+0x38,host.slots);
    w32(host.runtime,0);w32(host.runtime+8,1);w64(host.runtime+0x10,host.erst);w64(host.runtime+0x18,host.events);
    stage=7;w32(host.op,1);if(!wait_bits(host.op+4,1,0))goto failure;
    host.running=true;diagnostic_log("xHCI reset and polled event ring: PASS");
    for(unsigned port=1;port<=host.ports && host.count<2;port++)if(r32(host.op+0x400+(port-1)*16)&1){
        if(enumerate(&host.device[host.count],port))host.count++;
        else diagnostic_log("USB device enumeration failed/unsupported");
    }
    return (int)host.count;
failure:
    /* Never reclaim DMA pages while hardware ownership is uncertain. */
    host.running=false;diagnostic_value("xHCI failure stage: ",stage,false);diagnostic_log("xHCI initialization failed/unsupported; DMA reserved");return 0;
}
void usb_poll(void){
    if(!host.running)return;
    for(unsigned i=0;i<host.count;i++)if(host.device[i].ready && !(r32(host.op+0x400+(host.device[i].port-1)*16)&1)){disconnect(&host.device[i]);diagnostic_log("USB root disconnect; held keys/buttons released");}
    struct usb_trb e;for(unsigned i=0;i<64 && event(&e);i++)if(((e.control>>10)&63)==32)input_event(&e);
}
