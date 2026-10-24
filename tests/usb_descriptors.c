#include "usb.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t config[512];static unsigned length;
static void begin(unsigned interfaces){memset(config,0,sizeof(config));length=9;uint8_t h[]={9,2,0,0,0,1,0,128,50};memcpy(config,h,9);config[4]=interfaces;}
static void add(const uint8_t *p,unsigned n){assert(length+n<=sizeof(config));memcpy(config+length,p,n);length+=n;config[2]=length;config[3]=length>>8;}
static void interface(unsigned number,unsigned alt,unsigned eps,unsigned cls,unsigned sub,unsigned protocol){uint8_t d[]={9,4,number,alt,eps,cls,sub,protocol,0};add(d,9);}
static void endpoint(unsigned address,unsigned type,unsigned packet,unsigned interval){uint8_t d[]={7,5,address,type,packet,packet>>8,interval};add(d,7);}
static void hid(void){uint8_t d[]={9,0x21,0x11,1,0,1,0x22,63,0};add(d,9);}
static unsigned select(struct usb_boot_interface *out){return usb_boot_select(config,length,1,out);}
static unsigned traced;static void trace(const char *s,uint64_t v,bool hex){(void)s;(void)v;(void)hex;traced++;}
void test_usb_descriptors(void){
 struct usb_boot_interface out={0};
 begin(2);interface(0,0,1,0xff,0,0);endpoint(0x83,2,64,0);
 interface(3,0,2,3,1,1);endpoint(1,3,8,10);endpoint(0x81,3,8,10);hid();
 assert(!select(&out) && out.interface==3 && out.protocol==1 && out.endpoint==1 && !out.additional);
 unsigned good=length;uint8_t copy[512];memcpy(copy,config,sizeof(copy));
 for(unsigned n=0;n<good;n++){assert(usb_boot_select(config,n,1,&out));usb_descriptor_trace(config,n,trace);}
 /* A second HID interface does not invalidate the first usable boot interface. */
 config[4]=3;interface(5,0,1,3,1,2);hid();endpoint(0x82,3,4,5);
 assert(!select(&out) && out.interface==3 && out.additional==1);
 /* Alt zero can be idle; choose a usable nondefault alternate, then prefer zero. */
 begin(1);interface(2,0,0,3,1,1);hid();interface(2,1,1,3,1,1);hid();endpoint(0x81,3,8,10);
 assert(!select(&out) && out.alternate==1 && out.interface==2);
 begin(1);interface(2,1,1,3,1,1);hid();endpoint(0x81,3,8,10);interface(2,0,1,3,1,1);hid();endpoint(0x81,3,8,10);
 assert(!select(&out) && !out.alternate);
 begin(1);interface(0,0,1,3,0,0);hid();endpoint(0x81,3,8,10);assert(select(&out)==USB_DESC_SUBCLASS);
 begin(1);interface(0,0,1,3,1,0);hid();endpoint(0x81,3,8,10);assert(select(&out)==USB_DESC_PROTOCOL);
 begin(1);interface(0,0,1,0xff,0,0);endpoint(0x81,2,64,0);assert(select(&out)==USB_DESC_NO_HID);
 begin(1);interface(0,0,1,3,1,1);hid();endpoint(1,3,8,10);assert(select(&out)==USB_DESC_NO_INPUT);
 begin(1);interface(0,0,2,3,1,1);hid();endpoint(0x81,3,8,10);endpoint(0x82,3,8,10);assert(select(&out)==USB_DESC_MULTIPLE_INPUT);
 begin(1);interface(0,0,1,3,1,1);hid();endpoint(0x81,3,65,10);assert(select(&out)==USB_DESC_PACKET);
 config[length-3]=8;config[length-2]=0;config[length-1]=17;
 assert(usb_boot_select(config,length,3,&out)==USB_DESC_INTERVAL);
 config[length-1]=0;assert(select(&out)==USB_DESC_INTERVAL);
 /* Malformed forms receive different codes, without changing prior output. */
 memcpy(config,copy,sizeof(copy));length=good;struct usb_boot_interface prior=out;
 config[length-9]=0;assert(select(&out)==USB_DESC_WALK);assert(!memcmp(&prior,&out,sizeof(out)));
 memcpy(config,copy,sizeof(copy));config[9]=8;assert(select(&out)==USB_DESC_INTERFACE);
 begin(1);endpoint(0x81,3,8,10);assert(select(&out)==USB_DESC_ORDER);
 begin(1);interface(0,0,1,3,1,1);hid();endpoint(0x81,3,8,10);config[length-7]=6;assert(select(&out)==USB_DESC_ENDPOINT);
 begin(1);interface(0,0,1,3,1,1);hid();endpoint(0,3,8,10);assert(select(&out)==USB_DESC_ADDRESS);
 begin(1);interface(0,0,2,3,1,1);hid();endpoint(0x81,3,8,10);endpoint(0x81,3,8,10);assert(select(&out)==USB_DESC_DUP_ENDPOINT);
 begin(1);interface(0,0,2,3,1,1);hid();endpoint(0x81,3,8,10);assert(select(&out)==USB_DESC_ENDPOINT_COUNT);
 begin(1);interface(0,0,1,3,1,1);hid();endpoint(0x81,3,8,10);interface(0,0,0,0xff,0,0);assert(select(&out)==USB_DESC_DUP_INTERFACE);
 begin(2);interface(0,0,1,3,1,1);hid();endpoint(0x81,3,8,10);assert(select(&out)==USB_DESC_INTERFACE_COUNT);
 begin(1);interface(0,0,1,3,1,1);endpoint(0x81,3,8,10);assert(select(&out)==USB_DESC_HID_MISSING);
 hid();config[length-4]=2;assert(select(&out)==USB_DESC_HID);
 /* Class-specific 0x21 under a non-HID interface is not a HID descriptor. */
 begin(2);interface(0,0,0,0xfe,1,1);uint8_t dfu[]={9,0x21,0,0,0,0,0,0,0};add(dfu,9);
 interface(1,0,1,3,1,1);hid();endpoint(0x81,3,8,10);assert(!select(&out));
 traced=0;usb_descriptor_trace(config,length,trace);assert(traced>0 && traced<32);
 puts("USB descriptor composite/extra OUT/non-HID/alternate selection, ordering, distinct malformed/unsupported codes and every-prefix trace bounds passed");
}
