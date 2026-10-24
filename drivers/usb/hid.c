/* Original USB boot-interface validation and HID-to-CHOS event translation. */
#include "usb.h"
#include "hid_report.h"
uint16_t usb_usage_code(unsigned usage){
    static const uint8_t letters[26]={30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};
    static const uint8_t rest[]={2,3,4,5,6,7,8,9,10,11,28,1,14,15,57,12,13,26,27,43,43,39,40,41,51,52,53,58,59,60,61,62,63,64,65,66,67,68,87,88,99,70,119,110,102,104,111,107,109,106,105,108,103};
    if(usage>=4 && usage<=29)return letters[usage-4];
    if(usage>=30 && usage<30+sizeof(rest))return rest[usage-30];
    return 0;
}
static bool contains(const uint8_t *keys,uint8_t key){for(unsigned i=0;i<6;i++)if(keys[i]==key)return true;return false;}
bool usb_hid_report(struct usb_hid_state *s,unsigned protocol,const uint8_t *p,unsigned n,struct input_event *events,unsigned *count){
    if(!s || !p || !events || !count || *count>32)return false;
    struct input_event temp[24];unsigned used=0;
    if(protocol==1){
        if(n!=8 || p[1])return false;
        for(unsigned i=2;i<8;i++)if(p[i]){
            if(p[i]<4 || p[i]>0xe7)return false;
            for(unsigned j=2;j<i;j++)if(p[i]==p[j])return false;
        }
        static const uint8_t mods[8]={29,42,56,125,97,54,100,126};
        for(unsigned i=0;i<8;i++)if((s->modifiers^p[0])&(1u<<i))temp[used++]=(struct input_event){1,mods[i],(p[0]>>i)&1};
        for(unsigned i=0;i<6;i++)if(s->keys[i] && !contains(p+2,s->keys[i]) && usb_usage_code(s->keys[i]))temp[used++]=(struct input_event){1,usb_usage_code(s->keys[i]),0};
        for(unsigned i=2;i<8;i++)if(p[i] && !contains(s->keys,p[i]) && usb_usage_code(p[i]))temp[used++]=(struct input_event){1,usb_usage_code(p[i]),1};
        if(used>32-*count)return false;
        s->modifiers=p[0];for(unsigned i=0;i<6;i++)s->keys[i]=p[i+2];
    }else if(protocol==2){
        if(n<3 || n>4 || (p[0]&~7u))return false;
        for(unsigned i=0;i<3;i++)if((s->buttons^p[0])&(1u<<i))temp[used++]=(struct input_event){1,(uint16_t)(272+i),(p[0]>>i)&1};
        if(p[1])temp[used++]=(struct input_event){2,0,(int8_t)p[1]};
        if(p[2])temp[used++]=(struct input_event){2,1,(int8_t)p[2]};
        if(n==4 && p[3])temp[used++]=(struct input_event){2,8,(int8_t)p[3]};
        if(used>32-*count)return false;
        s->buttons=p[0];
    }else return false;
    for(unsigned i=0;i<used;i++)events[(*count)++]=temp[i];
    return true;
}
