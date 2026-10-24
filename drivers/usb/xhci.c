/* Original bounded xHCI host: USB2 hubs and boot/report HID functions.
 * Completion is polled; USB3, MSI/INTx and automatic reset recovery are unsupported. */
#include "usb.h"
#include "hid_report.h"
#include "platform.h"
#include "mmu.h"
#include "diagnostic.h"
#include "pci.h"
static struct usb_status report;
const struct usb_status *usb_status(void){return &report;}
#define USB_NODES 16
#define USB_CHANNELS 16
struct hid_device {unsigned slot,port,speed,received,route,parent,parent_port,depth,hub_ports,hub_protocol,tt_slot,tt_port,think,poll_port;struct usb_ring control;uint64_t contexts,output,data,absolute_tick;bool alive,hub;};
struct hid_channel {unsigned node,dci,protocol;struct usb_hid_interface interface;struct usb_ring input;uint64_t report,pending[8],reports;struct usb_hid_state boot;struct hid_layout layout;struct hid_report_state state;bool ready,warned,keyboard,pointer;};
static struct {uint64_t base,op,runtime,doorbell,dcbaa,events,erst,hub_next;struct usb_ring commands;unsigned consumer,cycle,context_size,ports,slots,count,channels,hub_node,hub_port;bool running;struct pmm *memory;struct hid_device device[USB_NODES];struct hid_channel channel[USB_CHANNELS];uint8_t held[288];} host;
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

static void deliver(struct input_event *batch,unsigned count){
    struct input_event filtered[32];unsigned n=0;
    for(unsigned i=0;i<count;i++){
        struct input_event e=batch[i];
        if(e.type==1 && e.code<288 && (e.value==0 || e.value==1)){
            unsigned before=host.held[e.code];
            if(e.value){if(before<255)host.held[e.code]++;if(before)continue;}
            else {if(before)host.held[e.code]--;if(!before || host.held[e.code])continue;}
        }
        filtered[n++]=e;
    }
    input_accept_batch(filtered,n);
}
static void submit_input(struct hid_channel *d,unsigned receive){
    d->pending[receive]=enqueue(&d->input,d->report+receive*512,d->interface.packet,(1u<<10)|(1u<<5)|(1u<<2));ring_doorbell(host.device[d->node].slot,d->dci);
}
static void disconnect(struct hid_channel *d){
    struct input_event batch[32];unsigned count=0;uint8_t empty[8]={0};
    bool ok=d->protocol?usb_hid_report(&d->boot,d->protocol,empty,d->protocol==1?8:3,batch,&count):hid_release(&d->state,batch,&count);
    if(ok)deliver(batch,count);
    diagnostic_value("USB releases on disconnect: ",count,false);
    d->ready=false;if(d->keyboard && report.keyboards)report.keyboards--;if(d->pointer && report.mice)report.mice--;
}
static void input_event(const struct usb_trb *e){
    unsigned slot=e->control>>24,endpoint=(e->control>>16)&31,completion=e->status>>24;
    for(unsigned i=0;i<host.channels;i++){
        struct hid_channel *d=&host.channel[i];if(!d->ready || host.device[d->node].slot!=slot || d->dci!=endpoint)continue;
        unsigned receive=0;while(receive<8 && d->pending[receive]!=e->parameter)receive++;if(receive==8)continue;
        uint64_t buffer=d->report+receive*512;unsigned residual=e->status&0xffffff;
        if((completion!=1 && completion!=13) || residual>d->interface.packet || !usb_dma_span(d->report,4096,buffer,d->interface.packet-residual)){disconnect(d);diagnostic_log("USB HID transfer failed; endpoint stopped");return;}
        cache(d->report,true);struct input_event batch[32];unsigned count=0;
        bool ok=d->protocol?usb_hid_report(&d->boot,d->protocol,(void *)(uintptr_t)buffer,d->interface.packet-residual,batch,&count):hid_decode(&d->layout,&d->state,(void *)(uintptr_t)buffer,d->interface.packet-residual,batch,&count);
        if(ok){
            struct hid_device *node=&host.device[d->node];
            for(unsigned j=0;j<count;j++)if(batch[j].type==3 && batch[j].code<=1)node->absolute_tick=now();
            /* A composite virtual pointer can publish both relative and absolute
             * streams. Prefer recently received absolute axes from that device;
             * do not double-apply its relative deltas or suppress other mice. */
            unsigned kept=0;bool absolute=node->absolute_tick && now()-node->absolute_tick<frequency()/4;
            for(unsigned j=0;j<count;j++)if(!(absolute && batch[j].type==2 && batch[j].code<=1))batch[kept++]=batch[j];
            deliver(batch,kept);
        }
        else if(!d->warned){diagnostic_log("USB HID malformed/unsupported report rejected");d->warned=true;}
        d->reports++;if(d->reports%255==0)diagnostic_value("USB completed receive reports: ",d->reports,false);
        submit_input(d,receive);return;
    }
}
static bool completion(unsigned type,uint64_t pointer,unsigned *slot){
    uint64_t start=now();struct usb_trb e;
    while(now()-start<frequency()){
        if(!event(&e))continue;
        if(((e.control>>10)&63)==type && (e.parameter&~15ULL)==pointer){report.last_completion=e.status>>24;if(slot)*slot=e.control>>24;return report.last_completion==1;}
        if(((e.control>>10)&63)==32)input_event(&e);
    }
    diagnostic_log("xHCI command/transfer timeout");return false;
}
static bool command(uint64_t parameter,uint32_t control,unsigned *slot){uint64_t at=enqueue(&host.commands,parameter,0,control);ring_doorbell(0,0);return completion(33,at,slot);}
/* A data-stage IOC supplies its actual residual length; a status-stage event
 * alone cannot establish how many initialized descriptor bytes were received. */
static bool control(struct hid_device *d,unsigned request,unsigned value,unsigned index,unsigned length,bool in){
    d->received=0;
    if(length>1024 || (length && !in) || !usb_dma_span(d->data,4096,d->data,length)){report.enum_reason=901;return false;}
    if(length){for(unsigned i=0;i<1024;i++)((uint8_t *)(uintptr_t)d->data)[i]=0;cache(d->data,false);}
    uint64_t setup=(uint64_t)(request&255)|((uint64_t)(request>>8)<<8)|((uint64_t)value<<16)|((uint64_t)index<<32)|((uint64_t)length<<48);
    enqueue(&d->control,setup,8,(2u<<10)|(1u<<6)|((length?3u:0u)<<16));
    uint64_t data=length?enqueue(&d->control,d->data,length,(3u<<10)|(1u<<16)|(1u<<5)|(1u<<2)):0;
    uint64_t status=enqueue(&d->control,0,0,(4u<<10)|(1u<<5)|(!length?1u<<16:0));
    ring_doorbell(d->slot,1);bool got_data=!length;uint64_t start=now();struct usb_trb e;
    while(now()-start<frequency()){
        if(!event(&e))continue;
        if(((e.control>>10)&63)!=32)continue;
        if((e.control>>24)!=d->slot || ((e.control>>16)&31)!=1){input_event(&e);continue;}
        unsigned code=e.status>>24;report.last_completion=code;
        if(e.parameter==data && length){
            unsigned residual=e.status&0xffffff;
            if((code!=1 && code!=13) || residual>length){report.enum_reason=902;return false;}
            d->received=length-residual;got_data=true;
        }else if(e.parameter==status){
            if(code!=1 || !got_data){report.enum_reason=903;return false;}
            if(length)cache(d->data,true);return true;
        }else if(code!=1 && code!=13){report.enum_reason=904;return false;}
    }
    report.enum_reason=905;diagnostic_log("USB control transfer timeout");return false;
}
static bool descriptor(struct hid_device *d,unsigned type,unsigned bytes){
    if(!control(d,0x0680,type,0,bytes,true))return false;
    if(d->received!=bytes){report.enum_reason=906;diagnostic_value("USB descriptor requested bytes: ",bytes,false);diagnostic_value("USB descriptor received bytes: ",d->received,false);return false;}return true;
}
static unsigned descriptor_fields;
static void descriptor_value(const char *label,uint64_t value,bool hex){
    if(descriptor_fields<160){descriptor_fields++;diagnostic_value(label,value,hex);}
    else if(descriptor_fields==160){descriptor_fields++;diagnostic_log("USB descriptor trace budget reached; later summaries only");}
}
static uint32_t *context(uint64_t base,unsigned index){return (void *)(uintptr_t)(base+(uint64_t)index*host.context_size);}
static void slot_context(struct hid_device *d,unsigned entries){
    uint32_t *s=context(d->contexts,1);
    s[0]=(entries<<27)|(d->speed<<20)|d->route|(d->hub?(1u<<26):0)|((d->hub && d->hub_protocol==2)?1u<<25:0);
    s[1]=(d->port<<16)|(d->hub_ports<<24);s[2]=d->tt_slot|(d->tt_port<<8)|(d->think<<16);
}
static void delay_ms(unsigned ms){uint64_t start=now(),ticks=frequency()*ms/1000;while(now()-start<ticks)__asm__ volatile("yield");}
static bool hub_status(struct hid_device *d,unsigned port,uint32_t *status){
    if(!control(d,0x00a3,0,port,4,true) || d->received!=4)return false;
    const uint8_t *p=(void *)(uintptr_t)d->data;*status=p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;return true;
}
static bool discover(unsigned,unsigned,unsigned,unsigned,unsigned);
static bool hub_initialize(struct hid_device *d){
    report.enum_stage=22;report.enum_reason=1301;
    if(!control(d,0x06a0,0x2900,0,7,true) || d->received!=7)return false;
    const uint8_t *p=(void *)(uintptr_t)d->data;unsigned length=p[0];
    diagnostic_value("USB hub descriptor bLength: ",length,false);diagnostic_value("USB hub descriptor port count: ",p[2],false);
    if(length<9 || length>64 || p[1]!=0x29 || !p[2] || p[2]>15){report.enum_reason=1302;return false;}
    if(!control(d,0x06a0,0x2900,0,length,true) || d->received!=length)return false;
    unsigned ports,characteristics,power;
    if(!usb_hub_info(p,d->received,&ports,&characteristics,&power)){report.enum_reason=1302;return false;}
    d->hub=true;d->hub_ports=ports;d->think=(characteristics>>5)&3;
    for(unsigned i=0;i<4096;i++)((uint8_t *)(uintptr_t)d->contexts)[i]=0;
    context(d->contexts,0)[1]=1;slot_context(d,1);cache(d->contexts,false);
    report.enum_reason=1303;if(!command(d->contexts,(13u<<10)|(d->slot<<24),0))return false;
    if((characteristics&3)!=2)for(unsigned port=1;port<=ports;port++)if(!control(d,0x0323,8,port,0,false))return false;
    delay_ms(power+100);diagnostic_value("USB hub slot: ",d->slot,false);diagnostic_value("USB hub ports: ",ports,false);diagnostic_log("USB hub initialized: PASS");
    unsigned index=(unsigned)(d-host.device);
    for(unsigned port=1;port<=ports;port++){
        uint32_t status;report.enum_reason=1304;if(!hub_status(d,port,&status))return false;
        diagnostic_value("USB hub downstream port: ",port,false);diagnostic_value("USB hub port status: ",status,true);
        if(!(status&1))continue;
        if(!control(d,0x0323,4,port,0,false))return false;
        uint64_t start=now();do{delay_ms(10);if(!hub_status(d,port,&status))return false;}while((status&16) && now()-start<frequency());
        if(!(status&1) || !(status&2) || (status&16)){diagnostic_log("USB hub child reset failed");continue;}
        for(unsigned bit=0;bit<5;bit++)if(status&(1u<<(16+bit)))(void)control(d,0x0123,16+bit,port,0,false);
        unsigned speed=(status&512)?2:(status&1024)?3:1;
        unsigned route;if(!usb_hub_route(d->route,d->depth,port,&route)){diagnostic_log("USB hub route depth/port limit");continue;}
        (void)discover(d->port,speed,index+1,port,route);
    }
    report.enum_stage=23;report.enum_reason=0;return true;
}
static bool hid_initialize(struct hid_device *d,const uint8_t *configuration,unsigned bytes){
    struct usb_hid_interface interfaces[USB_INTERFACES];unsigned count=0;
    report.enum_reason=usb_hid_interfaces(configuration,bytes,d->speed,interfaces,&count);if(report.enum_reason)return false;
    diagnostic_log("USB descriptor selection: PASS");
    unsigned config=configuration[5];report.enum_stage=19;
    diagnostic_value("USB SET_CONFIGURATION: ",config,false);if(!control(d,0x0900,config,0,0,false))return false;
    unsigned first=host.channels,entries=1;uint32_t flags=1;
    for(unsigned i=0;i<count;i++){
        if(host.channels==USB_CHANNELS){report.enum_reason=1310;break;}
        struct usb_hid_interface *f=&interfaces[i];
        if(f->alternate && !control(d,0x0b01,f->alternate,f->number,0,false))break;
        struct hid_channel *c=&host.channel[host.channels];*c=(struct hid_channel){.node=(unsigned)(d-host.device),.interface=*f,.dci=f->endpoint*2+1};
        diagnostic_value("USB selected interface: ",f->number,false);diagnostic_value("USB selected alternate: ",f->alternate,false);
        if(f->subclass==1 && (f->protocol==1 || f->protocol==2)){
            if(!control(d,0x0b21,0,f->number,0,false))break;
            c->protocol=f->protocol;c->keyboard=f->protocol==1;c->pointer=f->protocol==2;
        }else{
            report.enum_stage=24;
            diagnostic_value("USB report descriptor requested bytes: ",f->report_bytes,false);
            if(!control(d,0x0681,0x2200,f->number,f->report_bytes,true))break;
            diagnostic_value("USB report descriptor received bytes: ",d->received,false);
            if(d->received!=f->report_bytes){report.enum_reason=906;break;}
            const uint8_t *p=(void *)(uintptr_t)d->data;
            /* Static descriptor bytes only. Never dump live Input reports. */
            for(unsigned at=0;at<d->received;at+=4){uint64_t word=0;for(unsigned k=0;k<4 && at+k<d->received;k++)word|=(uint64_t)p[at+k]<<(8*k);diagnostic_value("USB report descriptor LE32: ",word,true);}
            report.enum_reason=hid_compile(p,d->received,&c->layout);
            if(!report.enum_reason)for(unsigned ri=0;ri<c->layout.reports;ri++)if((c->layout.bits[ri]+7u)/8u+(c->layout.numbered?1u:0u)>f->packet)report.enum_reason=1202;
            if(report.enum_reason){diagnostic_value("USB HID compiler refusal: ",report.enum_reason,false);continue;}
            c->keyboard=c->layout.keyboard;c->pointer=c->layout.pointer;
            diagnostic_value("USB HID compiled fields: ",c->layout.count,false);diagnostic_value("USB HID report IDs: ",c->layout.reports,false);
            diagnostic_log(c->layout.absolute?"USB HID absolute pointer layout: PASS":"USB HID report layout: PASS");
        }
        c->report=page();if(!c->report || !ring_init(&c->input)){report.enum_reason=813;break;}
        host.channels++;flags|=1u<<c->dci;if(c->dci>entries)entries=c->dci;
    }
    if(host.channels==first)return false;
    for(unsigned i=0;i<4096;i++)((uint8_t *)(uintptr_t)d->contexts)[i]=0;
    context(d->contexts,0)[1]=flags;slot_context(d,entries);
    for(unsigned i=first;i<host.channels;i++){
        struct hid_channel *c=&host.channel[i];uint32_t *ep=context(d->contexts,c->dci+1);
        unsigned interval=0;if(d->speed==3)interval=c->interface.interval-1;else{unsigned n=c->interface.interval;while(n>1){interval++;n>>=1;}interval+=3;}
        ep[0]=interval<<16;ep[1]=(c->interface.packet<<16)|(7u<<3)|6;*(uint64_t *)(ep+2)=c->input.address|1;ep[4]=c->interface.packet|((uint32_t)c->interface.packet<<16);
    }
    cache(d->contexts,false);report.enum_stage=20;report.enum_reason=820;
    if(!command(d->contexts,(12u<<10)|(d->slot<<24),0))return false;
    for(unsigned i=first;i<host.channels;i++){
        struct hid_channel *c=&host.channel[i];c->ready=true;
        if(c->keyboard)report.keyboards++;if(c->pointer)report.mice++;
        diagnostic_value("USB HID protocol: ",c->protocol,false);diagnostic_value("USB HID interface: ",c->interface.number,false);
        if(c->protocol)diagnostic_log(c->keyboard?"USB boot keyboard initialized: PASS":"USB boot mouse initialized: PASS");
        else diagnostic_log("USB report HID initialized: PASS");
        for(unsigned receive=0;receive<8;receive++)submit_input(c,receive);
    }
    report.enum_stage=21;report.enum_reason=0;return true;
}
static bool enumerate(struct hid_device *d){
    report.enum_port=d->port;report.enum_stage=11;report.enum_reason=811;report.last_completion=0;
    diagnostic_value("USB enumeration root port: ",d->port,false);diagnostic_value("USB route: ",d->route,true);diagnostic_value("USB parent hub slot: ",d->parent?host.device[d->parent-1].slot:0,false);
    if(d->speed<1 || d->speed>3)return false;
    report.enum_stage=12;report.enum_reason=812;
    unsigned slot=0;if(!command(0,9u<<10,&slot) || !slot || slot>host.slots)return false;d->slot=slot;
    report.enum_stage=13;report.enum_reason=813;
    d->contexts=page();d->output=page();d->data=page();if(!d->contexts || !d->output || !d->data || !ring_init(&d->control))return false;
    ((uint64_t *)(uintptr_t)host.dcbaa)[slot]=d->output;cache(host.dcbaa,false);
    context(d->contexts,0)[1]=3;slot_context(d,1);uint32_t *ep=context(d->contexts,2);unsigned packet=d->speed==3?64:8;
    ep[1]=(packet<<16)|(4u<<3)|6;*(uint64_t *)(ep+2)=d->control.address|1;ep[4]=8;cache(d->contexts,false);
    report.enum_stage=14;report.enum_reason=814;if(!command(d->contexts,(11u<<10)|(slot<<24),0))return false;
    report.enum_stage=15;report.enum_reason=815;diagnostic_value("USB speed after reset: ",d->speed,false);
    if(!descriptor(d,0x100,8))return false;const uint8_t *data=(void *)(uintptr_t)d->data;
    if(data[0]!=18 || data[1]!=1 || (data[7]!=8 && data[7]!=16 && data[7]!=32 && data[7]!=64))return false;
    report.enum_stage=16;report.enum_reason=816;
    if(packet!=data[7]){context(d->contexts,0)[1]=2;ep[1]=(ep[1]&65535)|((unsigned)data[7]<<16);cache(d->contexts,false);if(!command(d->contexts,(13u<<10)|(slot<<24),0))return false;}
    report.enum_stage=17;report.enum_reason=817;if(!descriptor(d,0x100,18))return false;
    if(data[0]!=18 || data[1]!=1 || !data[17]){report.enum_reason=907;return false;}
    unsigned cls=data[4];d->hub_protocol=data[6];
    descriptor_value("USB device VID: ",data[8]|(unsigned)data[9]<<8,true);descriptor_value("USB device PID: ",data[10]|(unsigned)data[11]<<8,true);
    descriptor_value("USB device class/subclass/protocol: ",(unsigned)data[4]<<16|(unsigned)data[5]<<8|data[6],true);descriptor_value("USB device configurations: ",data[17],false);
    if(!descriptor(d,0x200,9))return false;report.enumerated++;report.enum_stage=18;
    unsigned bytes=data[2]|(unsigned)data[3]<<8;
    if(data[0]!=9 || data[1]!=2){report.enum_reason=USB_DESC_HEADER;return false;}
    descriptor_value("USB advertised config total length: ",bytes,false);descriptor_value("USB advertised config value: ",data[5],false);descriptor_value("USB advertised interface count: ",data[4],false);
    if(bytes<9 || bytes>512){report.enum_reason=USB_DESC_LIMIT;return false;}
    if(!descriptor(d,0x200,bytes))return false;usb_descriptor_trace(data,bytes,descriptor_value);
    d->alive=true;
    if(cls==9){unsigned config=data[5];if(!control(d,0x0900,config,0,0,false))return false;return hub_initialize(d);}
    return hid_initialize(d,data,bytes);
}
static bool discover(unsigned root,unsigned speed,unsigned parent,unsigned port,unsigned route){
    if(host.count==USB_NODES){report.enum_reason=1310;return false;}
    unsigned index=host.count++;struct hid_device *d=&host.device[index];*d=(struct hid_device){.port=root,.speed=speed,.parent=parent,.parent_port=port,.route=route};
    if(parent){struct hid_device *p=&host.device[parent-1];d->depth=p->depth+1;
        if(speed!=3){if(p->speed==3){d->tt_slot=p->slot;d->tt_port=port;}else{d->tt_slot=p->tt_slot;d->tt_port=p->tt_port;}}
    }
    bool ok=enumerate(d);
    if(!ok){diagnostic_value("USB enumeration port: ",root,false);diagnostic_value("USB enumeration failure stage: ",report.enum_stage,false);diagnostic_value("USB enumeration refusal reason: ",report.enum_reason,false);if(report.enum_reason>=1000 && report.enum_reason<1200)diagnostic_log(usb_descriptor_reason_name(report.enum_reason));diagnostic_value("USB last completion code: ",report.last_completion,false);diagnostic_log("USB device enumeration failed/unsupported");}
    return ok;
}
/* Register snapshot only after the layout helper has validated all used ranges. */
static uint64_t r64(uint64_t a){uint32_t low=r32(a);return low|((uint64_t)r32(a+4)<<32);}
static void registers(void){
    diagnostic_value("xHCI USBCMD: ",r32(host.op),true);
    diagnostic_value("xHCI USBSTS: ",r32(host.op+4),true);
    diagnostic_value("xHCI PAGESIZE: ",r32(host.op+8),true);
    diagnostic_value("xHCI CRCR: ",r64(host.op+0x18),true);
    diagnostic_value("xHCI DCBAAP: ",r64(host.op+0x30),true);
    diagnostic_value("xHCI CONFIG: ",r32(host.op+0x38),true);
}
int usb_initialize(struct pmm *memory,const struct chos_boot_info *boot){
    descriptor_fields=0;host=(typeof(host)){0};host.memory=memory;report=(struct usb_status){.stage=1};unsigned stage=1,reason=XHCI_BAR;
    bool layout_valid=false;struct xhci_layout layout;
    struct pci_device pci;
    if(!pci_find_class(boot,0x0c0330,&pci)){report.reason=pci.error;pci_report("xHCI PCI discovery",&pci);return 0;}
    pci_report("xHCI driver selected",&pci);report.id=pci.id;
    uint32_t low=r32(pci.config+16),high=r32(pci.config+20);
    diagnostic_value("xHCI PCI command before: ",r32(pci.config+4)&65535,true);
    diagnostic_value("xHCI BAR0 low: ",low,true);diagnostic_value("xHCI BAR0 high: ",high,true);
    if(!pci_memory_bar(low,high,65536,&host.base))goto failure;
    report.base=host.base;diagnostic_value("xHCI MMIO base: ",host.base,true);
    reason=XHCI_MAP;if(!mmu_device(host.base,65536))goto failure;
    reason=XHCI_DECODE;if(!pci_enable(&pci,2))goto failure;
    diagnostic_value("xHCI PCI command decode: ",r32(pci.config+4)&65535,true);
    stage=2;unsigned cap=r32(host.base),hcs=r32(host.base+4),hcs2=r32(host.base+8),hcc=r32(host.base+16);
    unsigned db=r32(host.base+20),rt=r32(host.base+24);
    report.capabilities=hcc;report.hcs=hcs;report.ports=hcs>>24;
    diagnostic_value("xHCI CAPLENGTH/version: ",cap,true);
    diagnostic_value("xHCI HCSPARAMS1: ",hcs,true);diagnostic_value("xHCI HCSPARAMS2: ",hcs2,true);
    diagnostic_value("xHCI HCCPARAMS1: ",hcc,true);
    if((cap>>16)>=0x110)diagnostic_value("xHCI HCCPARAMS2: ",r32(host.base+28),true);
    diagnostic_value("xHCI DBOFF raw: ",db,true);diagnostic_value("xHCI RTSOFF raw: ",rt,true);
    diagnostic_value("xHCI scratchpad buffers: ",((hcs2>>27)&31)|((hcs2>>16)&0x3e0),false);
    diagnostic_value("xHCI root ports: ",report.ports,false);
    reason=xhci_layout_validate(cap,hcs,hcs2,hcc,db,rt,65536,&layout);if(reason)goto failure;
    layout_valid=true;host.op=host.base+layout.operational;host.context_size=(hcc&4)?64:32;host.slots=layout.slots;host.ports=layout.ports;
    host.doorbell=host.base+layout.doorbell;host.runtime=host.base+layout.runtime+0x20;
    registers();
    stage=3;uint32_t ext=hcc>>16;for(unsigned steps=0;ext;steps++){
        reason=XHCI_EXT_LIMIT;if(steps==64)goto failure;
        reason=XHCI_EXT_BOUNDS;if(!xhci_extended_span(&layout,ext*4,4))goto failure;
        uint32_t c=r32(host.base+ext*4);diagnostic_value("xHCI extended capability: ",c,true);
        if((c&255)==1){
            if(!xhci_extended_span(&layout,ext*4,8))goto failure;
            diagnostic_value("xHCI legacy ownership: ",c,true);
            diagnostic_value("xHCI legacy control: ",r32(host.base+ext*4+4),true);
            reason=XHCI_LEGACY_OWNED;if(c&(1u<<16))goto failure;
        }
        unsigned next=(c>>8)&255;if(!next)break;ext+=next;
    }
    stage=4;reason=XHCI_NOT_READY;if(!wait_bits(host.op+4,1u<<11,0))goto failure;
    reason=XHCI_HALT;w32(host.op,r32(host.op)&~1u);if(!wait_bits(host.op+4,1,1))goto failure;
    diagnostic_log("xHCI halt: PASS");
    stage=5;reason=XHCI_RESET;w32(host.op,2);if(!wait_bits(host.op,2,0))goto failure;
    reason=XHCI_NOT_READY;if(!wait_bits(host.op+4,1u<<11,0))goto failure;
    diagnostic_log("xHCI reset/CNR: PASS");registers();
    reason=XHCI_PAGESIZE;if(!(r32(host.op+8)&1))goto failure;
    stage=6;reason=XHCI_ALLOC;host.dcbaa=page();host.events=page();host.erst=page();
    if(!host.dcbaa || !host.events || !host.erst || !ring_init(&host.commands))goto failure;
    diagnostic_log("xHCI four aligned DMA pages allocated: PASS");
    ((uint64_t *)(uintptr_t)host.erst)[0]=host.events;((uint64_t *)(uintptr_t)host.erst)[1]=256;cache(host.erst,false);
    reason=XHCI_BUSMASTER;if(!pci_enable(&pci,4))goto failure;
    diagnostic_value("xHCI PCI command DMA: ",r32(pci.config+4)&65535,true);
    host.cycle=1;w64(host.op+0x30,host.dcbaa);w64(host.op+0x18,host.commands.address|1);w32(host.op+0x38,host.slots);
    w32(host.runtime,0);w32(host.runtime+8,1);w64(host.runtime+0x10,host.erst);w64(host.runtime+0x18,host.events);
    __asm__ volatile("dsb sy":::"memory");
    stage=7;reason=XHCI_RUN;w32(host.op,1);if(!wait_bits(host.op+4,1,0))goto failure;
    report.stage=8;report.initialized=1;host.running=true;registers();diagnostic_log("xHCI reset and polled event ring: PASS");
    for(unsigned port=1;port<=host.ports;port++){
        uint64_t address=host.op+0x400+(port-1)*16;uint32_t portsc=r32(address);
        diagnostic_value("xHCI root port number: ",port,false);diagnostic_value("xHCI PORTSC before enumeration: ",portsc,true);
        if(!(portsc&1))continue;report.connected++;
        w32(address,(portsc&0x200)|16);report.enum_stage=10;report.enum_reason=810;
        if(!wait_bits(address,16,0) || !wait_bits(address,2,2)){diagnostic_log("USB root reset failed");continue;}
        unsigned speed=(r32(address)>>10)&15;(void)discover(port,speed,0,0,0);
    }
    unsigned ready=0;for(unsigned i=0;i<host.channels;i++)if(host.channel[i].ready)ready++;
    diagnostic_value("USB connected root ports: ",report.connected,false);diagnostic_value("USB operational boot HID devices: ",ready,false);
    return (int)ready;
failure:
    /* No uncertain DMA ownership is reclaimed. A capability refusal allocates
     * no DMA; reserve language must not imply an allocator failure. */
    host.running=false;report.stage=stage;report.reason=reason;
    diagnostic_value("xHCI failure stage: ",stage,false);diagnostic_value("xHCI failure reason: ",reason,false);
    diagnostic_log(xhci_reason_name(reason));
    if(layout_valid)registers();
    diagnostic_log("xHCI initialization refused; see stage/reason/registers");return 0;
}
static void detach(unsigned index){
    host.device[index].alive=false;
    for(unsigned i=0;i<host.channels;i++)if(host.channel[i].ready && host.channel[i].node==index)disconnect(&host.channel[i]);
    for(unsigned i=index+1;i<host.count;i++)if(host.device[i].alive && host.device[i].parent==index+1)detach(i);
}
void usb_poll(void){
    if(!host.running)return;
    for(unsigned i=0;i<host.count;i++)if(host.device[i].alive && !(r32(host.op+0x400+(host.device[i].port-1)*16)&1)){detach(i);diagnostic_log("USB root disconnect; held keys/buttons released");}
    struct usb_trb e;for(unsigned i=0;i<64 && event(&e);i++)if(((e.control>>10)&63)==32)input_event(&e);
    if(now()<host.hub_next)return;host.hub_next=now()+frequency()/10;
    for(unsigned i=0;i<host.count;i++){
        unsigned index=host.hub_node++%host.count;struct hid_device *d=&host.device[index];if(!d->alive || !d->hub)continue;
        unsigned port=1+(d->poll_port++%d->hub_ports);uint32_t status;
        if(!hub_status(d,port,&status)){detach(index);diagnostic_log("USB hub polling failed; subtree stopped and held states released");break;}
        if(!(status&1))for(unsigned child=index+1;child<host.count;child++)if(host.device[child].alive && host.device[child].parent==index+1 && host.device[child].parent_port==port){detach(child);diagnostic_log("USB hub child disconnect; held states released");}
        break;
    }
}
