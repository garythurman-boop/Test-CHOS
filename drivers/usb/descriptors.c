/* Original bounded USB configuration walker. Select one boot interface per
 * physical device; other interfaces remain unclaimed. No VID/PID exceptions. */
#include "usb.h"
static unsigned le16(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
const char *usb_descriptor_reason_name(unsigned r){
    switch(r){
    case 0:return "USB descriptor selection: PASS";
    case USB_DESC_HEADER:return "USB malformed: configuration header/value";
    case USB_DESC_TOTAL:return "USB malformed: total length mismatch";
    case USB_DESC_WALK:return "USB malformed: descriptor length/truncation";
    case USB_DESC_INTERFACE:return "USB malformed: interface descriptor length";
    case USB_DESC_ENDPOINT:return "USB malformed: endpoint descriptor length";
    case USB_DESC_ORDER:return "USB malformed: descriptor outside interface";
    case USB_DESC_ADDRESS:return "USB malformed: endpoint address/reserved bits";
    case USB_DESC_DUP_ENDPOINT:return "USB malformed: duplicate endpoint address";
    case USB_DESC_ENDPOINT_COUNT:return "USB malformed: interface endpoint count";
    case USB_DESC_DUP_INTERFACE:return "USB malformed: duplicate interface/alternate";
    case USB_DESC_INTERFACE_COUNT:return "USB malformed: configuration interface count";
    case USB_DESC_HID:return "USB malformed: HID subordinate descriptor list";
    case USB_DESC_HID_MISSING:return "USB malformed: boot interface missing HID descriptor";
    case USB_DESC_LIMIT:return "USB unsupported: descriptor/interface budget";
    case USB_DESC_NO_HID:return "USB unsupported: no HID interface";
    case USB_DESC_SUBCLASS:return "USB unsupported: HID subclass is not boot";
    case USB_DESC_PROTOCOL:return "USB unsupported: HID boot protocol not keyboard/mouse";
    case USB_DESC_NO_INPUT:return "USB unsupported: no interrupt IN endpoint";
    case USB_DESC_MULTIPLE_INPUT:return "USB unsupported: multiple interrupt IN endpoints";
    case USB_DESC_PACKET:return "USB unsupported: interrupt packet size/transactions";
    case USB_DESC_INTERVAL:return "USB unsupported: interrupt polling interval";
    default:return "USB descriptor refusal: unknown";
    }
}
struct candidate {struct usb_boot_interface out;unsigned cls,sub,expected,seen,inputs,reason;uint32_t addresses;bool hid;};
static unsigned finish(struct candidate *c,unsigned speed){
    if(c->seen!=c->expected)return USB_DESC_ENDPOINT_COUNT;
    if(c->cls!=3)return USB_DESC_NO_HID;
    if(c->sub!=1)return USB_DESC_SUBCLASS;
    if(c->out.protocol!=1 && c->out.protocol!=2)return USB_DESC_PROTOCOL;
    if(!c->hid)return USB_DESC_HID_MISSING;
    if(!c->inputs)return USB_DESC_NO_INPUT;
    if(c->inputs>1)return USB_DESC_MULTIPLE_INPUT;
    if(c->reason)return c->reason;
    if(!c->out.interval || (speed==3 && c->out.interval>16))return USB_DESC_INTERVAL;
    return 0;
}
unsigned usb_boot_select(const uint8_t *p,unsigned n,unsigned speed,struct usb_boot_interface *out){
    if(!p || !out || n<9 || p[0]!=9 || p[1]!=2 || !p[5])return USB_DESC_HEADER;
    if(n>512)return USB_DESC_LIMIT;
    if(le16(p+2)!=n)return USB_DESC_TOTAL;
    struct candidate c={0};struct usb_boot_interface selected={0};
    bool active=false,found=false;unsigned fallback=USB_DESC_NO_HID,candidates=0,descriptors=0,interfaces=0,pair_count=0;
    uint16_t pairs[32];uint8_t numbers[32]={0};
    for(unsigned at=9;at<n;){
        if(n-at<2 || p[at]<2 || p[at]>n-at)return USB_DESC_WALK;
        if(++descriptors>64)return USB_DESC_LIMIT;
        const uint8_t *d=p+at;
        if(d[1]==4){
            if(d[0]<9)return USB_DESC_INTERFACE;
            if(active){
                unsigned r=finish(&c,speed);if(r && r<1100)return r;
                if(!r){candidates++;if(!found || (selected.alternate && !c.out.alternate)){selected=c.out;found=true;}}
                else if(c.cls==3)fallback=r;
            }
            if(pair_count==32)return USB_DESC_LIMIT;
            uint16_t pair=d[2]|(unsigned)d[3]<<8;
            for(unsigned i=0;i<pair_count;i++)if(pairs[i]==pair)return USB_DESC_DUP_INTERFACE;
            pairs[pair_count++]=pair;
            if(!(numbers[d[2]/8]&(1u<<(d[2]%8)))){numbers[d[2]/8]|=1u<<(d[2]%8);interfaces++;}
            c=(struct candidate){.out={.configuration=p[5],.interface=d[2],.alternate=d[3],.protocol=d[7]},.cls=d[5],.sub=d[6],.expected=d[4]};active=true;
        }else if(d[1]==5){
            if(d[0]<7)return USB_DESC_ENDPOINT;
            if(!active)return USB_DESC_ORDER;
            unsigned address=d[2],ep=address&15;
            if(!ep || (address&0x70))return USB_DESC_ADDRESS;
            unsigned bit=ep+((address&128)?16:0);
            if(c.addresses&(1u<<bit))return USB_DESC_DUP_ENDPOINT;
            c.addresses|=1u<<bit;c.seen++;
            /* Extra OUT/bulk endpoints do not disqualify a usable interrupt IN. */
            if((address&128) && (d[3]&3)==3){
                c.inputs++;c.out.endpoint=ep;c.out.interval=d[6];unsigned packet=le16(d+4);c.out.packet=(uint16_t)packet;
                if(packet<3 || packet>64 || (c.out.protocol==1 && packet<8))c.reason=USB_DESC_PACKET;
            }
        }else if(d[1]==0x21 && active && c.cls==3){
            if(!active)return USB_DESC_ORDER;
            if(d[0]<6 || d[0]!=(unsigned)6+3*d[5] || !d[5] || c.hid)return USB_DESC_HID;
            bool report=false;for(unsigned i=0;i<d[5];i++)if(d[6+3*i]==0x22 && le16(d+7+3*i))report=true;
            if(!report)return USB_DESC_HID;
            c.hid=true;
        }else if(d[1]==2)return USB_DESC_HEADER;
        at+=d[0];
    }
    if(active){unsigned r=finish(&c,speed);if(r && r<1100)return r;if(!r){candidates++;if(!found || (selected.alternate && !c.out.alternate)){selected=c.out;found=true;}}else if(c.cls==3)fallback=r;}
    if(interfaces!=p[4])return USB_DESC_INTERFACE_COUNT;
    if(!found)return fallback;
    selected.additional=(uint8_t)(candidates-1);*out=selected;return 0;
}
bool usb_boot_configuration(const uint8_t *p,unsigned n,struct usb_boot_interface *out){return usb_boot_select(p,n,1,out)==0;}
/* Trace only initialized bytes within validated descriptor spans. The caller
 * supplies a per-boot output budget; raw payloads and USB strings are excluded. */
void usb_descriptor_trace(const uint8_t *p,unsigned n,void (*emit)(const char *,uint64_t,bool)){
    if(!p || !emit || n<9 || n>512 || p[0]!=9 || p[1]!=2)return;
    emit("USB configuration total length: ",le16(p+2),false);emit("USB configuration value: ",p[5],false);emit("USB configuration interfaces: ",p[4],false);
    unsigned count=0,cls=0;for(unsigned at=9;at<n && count++<64;){
        if(n-at<2 || p[at]<2 || p[at]>n-at)return;
        const uint8_t *d=p+at;
        if(d[1]==4 && d[0]>=9){cls=d[5];
            emit("USB interface number: ",d[2],false);emit("USB interface alternate: ",d[3],false);emit("USB interface endpoints: ",d[4],false);
            emit("USB interface class/subclass/protocol: ",(unsigned)d[5]<<16|(unsigned)d[6]<<8|d[7],true);
        }else if(d[1]==5 && d[0]>=7){
            emit("USB endpoint address: ",d[2],true);emit("USB endpoint transfer type: ",d[3]&3,false);emit("USB endpoint max packet: ",le16(d+4),false);emit("USB endpoint interval: ",d[6],false);
        }else if(d[1]==0x21 && cls==3){emit("USB HID descriptor length: ",d[0],false);if(d[0]>=6)emit("USB HID subordinate descriptors: ",d[5],false);}
        at+=d[0];
    }
}
