#include "hid_report.h"
#include "usb.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const uint8_t keyboard[]={0x05,1,0x09,6,0xa1,1,0x05,7,0x19,0xe0,0x29,0xe7,0x15,0,0x25,1,0x75,1,0x95,8,0x81,2,0x95,1,0x75,8,0x81,1,0x95,5,0x75,1,0x05,8,0x19,1,0x29,5,0x91,2,0x95,1,0x75,3,0x91,1,0x95,6,0x75,8,0x15,0,0x25,0x65,0x05,7,0x19,0,0x29,0x65,0x81,0,0xc0};
static const uint8_t absolute[]={0x05,1,0x09,2,0xa1,1,0x85,7,0x05,9,0x19,1,0x29,3,0x15,0,0x25,1,0x75,1,0x95,3,0x81,2,0x75,5,0x95,1,0x81,1,0x05,1,0x09,0x30,0x09,0x31,0x15,0,0x26,0xff,0x7f,0x75,16,0x95,2,0x81,2,0xc0};
static const uint8_t relative[]={0x05,1,0x09,2,0xa1,1,0x05,9,0x19,1,0x29,3,0x15,0,0x25,1,0x75,1,0x95,3,0x81,2,0x75,5,0x95,1,0x81,1,0x05,1,0x09,0x30,0x09,0x31,0x09,0x38,0x15,0x81,0x25,0x7f,0x75,8,0x95,3,0x81,6,0xc0};
void test_hid_report(void){
 unsigned ports,characteristics,delay,route;
 uint8_t hub[]={10,0x29,8,0,0,1,0,0,0,0xff};
 assert(usb_hub_info(hub,sizeof(hub),&ports,&characteristics,&delay) && ports==8 && delay==2);
 for(unsigned i=0;i<sizeof(hub);i++)assert(!usb_hub_info(hub,i,&ports,&characteristics,&delay));
 hub[2]=16;assert(!usb_hub_info(hub,sizeof(hub),&ports,&characteristics,&delay));
 assert(usb_hub_route(0,0,1,&route) && route==1);
 assert(usb_hub_route(0x4321,4,5,&route) && route==0x54321);
 assert(!usb_hub_route(0x54321,5,1,&route) && !usb_hub_route(0,0,16,&route));
 struct hid_layout l;struct hid_report_state s={0};struct input_event e[32];unsigned n=0;
 assert(!hid_compile(keyboard,sizeof(keyboard),&l) && l.keyboard && !l.pointer && l.bits[0]==64);
 uint8_t k[8]={2,0,4};assert(hid_decode(&l,&s,k,8,e,&n) && n==2);
 assert(e[0].code==42 && e[1].code==30);
 n=0;assert(hid_decode(&l,&s,k,8,e,&n) && !n);k[2]=1;assert(!hid_decode(&l,&s,k,8,e,&n));
 assert(hid_release(&s,e,&n) && n==2 && !e[0].value && !e[1].value);
 assert(!hid_decode(&l,&s,k,7,e,&n));
 assert(!hid_compile(absolute,sizeof(absolute),&l) && l.pointer && l.absolute && l.numbered);
 uint8_t a[]={7,1,0xff,0x7f,0,0};n=0;assert(hid_decode(&l,&s,a,sizeof(a),e,&n) && n==3);
 assert(e[0].type==3 && e[0].code==0 && e[0].value==65535 && e[1].type==3 && e[1].value==0 && e[2].code==272);
 a[0]=8;assert(!hid_decode(&l,&s,a,sizeof(a),e,&n));n=0;assert(hid_release(&s,e,&n) && n==1 && !e[0].value);
 assert(!hid_compile(relative,sizeof(relative),&l) && !l.absolute);
 uint8_t r[]={2,0xff,0x7f,0x81};n=0;assert(hid_decode(&l,&s,r,4,e,&n));assert(n==4 && e[0].value==-1 && e[1].value==127 && e[2].value==-127 && e[3].code==273);
 n=32;uint8_t zero[4]={0};assert(!hid_decode(&l,&s,zero,4,e,&n));n=0;assert(hid_release(&s,e,&n) && n==1);
 for(unsigned i=0;i<sizeof(keyboard);i++)assert(hid_compile(keyboard,i,&l));
 for(unsigned i=0;i<sizeof(absolute);i++)assert(hid_compile(absolute,i,&l));
 uint8_t bad[]={0xfe,0,0};assert(hid_compile(bad,sizeof(bad),&l)==1203);
 uint8_t pop[]={0xb4};assert(hid_compile(pop,sizeof(pop),&l)==1201);
 uint8_t huge[sizeof(absolute)];memcpy(huge,absolute,sizeof(huge));huge[sizeof(huge)-6]=255;assert(hid_compile(huge,sizeof(huge),&l));
 /* Report-ID streams retain independent held state in a composite layout. */
 uint8_t composite[sizeof(keyboard)+sizeof(absolute)+2];
 memcpy(composite,keyboard,6);composite[6]=0x85;composite[7]=1;
 memcpy(composite+8,keyboard+6,sizeof(keyboard)-6);memcpy(composite+sizeof(keyboard)+2,absolute,sizeof(absolute));
 assert(!hid_compile(composite,sizeof(composite),&l) && l.keyboard && l.pointer && l.reports==2);
 uint8_t numbered_key[9]={1,2,0,4};s=(struct hid_report_state){0};n=0;assert(hid_decode(&l,&s,numbered_key,9,e,&n) && n==2 && e[0].code==42);
 uint8_t numbered_mouse[]={7,0,0,0,0,0};n=0;assert(hid_decode(&l,&s,numbered_mouse,6,e,&n) && n==2 && e[0].type==3 && e[1].type==3);
 n=0;assert(hid_release(&s,e,&n) && n==2);
 /* Mutate static descriptors: any admitted layout must remain bounded for all
  * payload prefix sizes; malformed packets must preserve memory integrity. */
 uint8_t mutant[sizeof(keyboard)],payload[65]={0};
 for(unsigned at=0;at<sizeof(keyboard);at++)for(unsigned bit=0;bit<8;bit++){
  memcpy(mutant,keyboard,sizeof(mutant));mutant[at]^=1u<<bit;
  if(!hid_compile(mutant,sizeof(mutant),&l))for(unsigned len=0;len<=sizeof(payload);len++){
   s=(struct hid_report_state){0};n=0;(void)hid_decode(&l,&s,payload,len,e,&n);assert(n<=32);n=0;(void)hid_release(&s,e,&n);
  }
 }
 puts("HID report compiler: keyboard arrays/modifiers, report IDs, signed relative/wheel, scaled absolute axes/buttons, atomic rejection/release and truncated/unsupported items passed");
}
