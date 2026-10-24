/* Bounded original HID short-item compiler. No fixed VMware report layouts.
 * Input offsets include constant/unknown fields. Output/Feature have independent
 * streams and do not consume Input offsets. Unsupported constructs fail closed. */
#include "hid_report.h"
static uint32_t value(const uint8_t *p,unsigned n){uint32_t v=0;for(unsigned i=0;i<n;i++)v|=(uint32_t)p[i]<<(i*8);return v;}
static int32_t signed_value(uint32_t v,unsigned bytes){if(bytes && bytes<4 && (v&(1u<<(bytes*8-1))))v|=~((1u<<(bytes*8))-1);return (int32_t)v;}
struct globals {uint32_t page,size,count,id;int32_t min,max;};
static int report(struct hid_layout *l,unsigned id){
 for(unsigned i=0;i<l->reports;i++)if(l->ids[i]==id)return (int)i;
 if(l->reports==HID_REPORTS)return -1;
 unsigned i=l->reports++;l->ids[i]=(uint8_t)id;return (int)i;
}
unsigned hid_compile(const uint8_t *p,unsigned n,struct hid_layout *out){
 if(!p || !out || !n || n>1024)return 1201;
 struct hid_layout l={0};struct globals g={0},stack[4];unsigned sp=0,depth=0,usages[96],used=0,low=0,high=0;bool range=false;unsigned collections[8]={0},application=0;
 for(unsigned at=0;at<n;){
  unsigned prefix=p[at++];if(prefix==0xfe)return 1203;
  unsigned bytes=prefix&3;if(bytes==3)bytes=4;
  if(bytes>n-at)return 1201;
  uint32_t v=value(p+at,bytes);at+=bytes;unsigned type=(prefix>>2)&3,tag=prefix>>4;
  if(type==1){switch(tag){
   case 0:if(v>65535)return 1203;g.page=v;break;
   case 1:g.min=signed_value(v,bytes);break;
   case 2:if(g.min<0)g.max=signed_value(v,bytes);else {if(v>INT32_MAX)return 1205;g.max=(int32_t)v;}break;
   case 7:g.size=v;break;
   case 8:if(!v || v>255)return 1203;g.id=v;l.numbered=true;break;
   case 9:g.count=v;break;
   case 10:if(sp==4)return 1202;stack[sp++]=g;break;
   case 11:if(!sp)return 1201;g=stack[--sp];break;
   default:break; /* Physical ranges/units do not change Input bit layout. */
  }}else if(type==2){
   if(tag==0){if(used==96)return 1202;usages[used++]=v;}
   else if(tag==1){low=v;range=true;}
   else if(tag==2){high=v;if(!range || high<low || high-low>255)return 1205;}
   else return 1203; /* No delimiter sets: selection would be ambiguous. */
  }else if(type==0){
   if(tag==10){
    if(depth==8)return 1202;
    collections[depth++]=application;
    if(v==1){unsigned u=used?usages[0]:low;unsigned page=u>65535?u>>16:g.page;u&=65535;application=page==1?(u==6?1:((u==1 || u==2)?2:0)):0;}
   }else if(tag==12){if(!depth)return 1201;application=collections[--depth];}
   else if(tag==8){
    if(!g.size || g.size>32 || !g.count || g.count>512)return 1202;
    int ri=report(&l,g.id);if(ri<0)return 1202;
    uint64_t bits=(uint64_t)g.size*g.count;if(bits>512u-l.bits[ri])return 1202;
    unsigned start=l.bits[ri];l.bits[ri]+=(uint16_t)bits;
    if(!(v&1) && application){
     if(g.max<g.min)return 1205;
     if(!(v&2)){ /* Keyboard usage array, with explicit usage range. */
      if(g.page!=7 || application!=1){if(g.page==9)return 1203;used=0;range=false;low=high=0;continue;}
      if(g.size>16 || g.count>32 || !range || low!=0 || high>255)return 1203;
      if(l.count==HID_FIELDS)return 1202;
      l.field[l.count++]=(struct hid_field){start,7,0,(uint16_t)g.count,(uint8_t)g.size,(uint8_t)g.id,0,g.min,g.max};l.keyboard=true;
     }else for(unsigned i=0;i<g.count;i++){
      unsigned u=used?usages[i<used?i:used-1]:(range?(low+i<=high?low+i:high):0);
      unsigned page=u>65535?u>>16:g.page;u&=65535;
      bool key=page==7 && application==1 && u>=4 && u<=255;
      bool button=page==9 && application==2 && u>=1 && u<=3;
      bool axis=page==1 && application==2 && (u==0x30 || u==0x31 || u==0x38);
      if(!key && !button && !axis)continue;
      if(l.count==HID_FIELDS)return 1202;
      if(axis && (!(v&4) && (u==0x38 || g.max<=g.min)))return 1205;
      l.field[l.count++]=(struct hid_field){(uint16_t)(start+i*g.size),(uint16_t)page,(uint16_t)u,1,(uint8_t)g.size,(uint8_t)g.id,(uint8_t)(2|(v&4)),g.min,g.max};
      if(key)l.keyboard=true;else l.pointer=true;
      if(axis && !(v&4))l.absolute=true;
     }
    }
   }else if(tag!=9 && tag!=11)return 1203;
   used=0;range=false;low=high=0;
  }else return 1203;
 }
 if(depth || sp || !l.reports)return 1201;
 if(l.numbered)for(unsigned i=0;i<l.reports;i++)if(!l.ids[i])return 1203;
 for(unsigned i=0;i<l.reports;i++)if((l.bits[i]+7u)/8u+(l.numbered?1u:0u)>64)return 1202;
 if(!l.keyboard && !l.pointer)return 1204;
 *out=l;return 0;
}
static uint32_t bits(const uint8_t *p,unsigned at,unsigned n){uint32_t v=0;for(unsigned i=0;i<n;i++)if(p[(at+i)/8]&(1u<<((at+i)%8)))v|=1u<<i;return v;}
static int32_t field_value(const uint8_t *p,const struct hid_field *f,unsigned at){uint32_t v=bits(p,at,f->size);if(f->minimum<0 && f->size<32 && (v&(1u<<(f->size-1))))v|=~((1u<<f->size)-1);return (int32_t)v;}
static bool held(const uint8_t *k,unsigned u){return (k[u/8]>>(u%8))&1;}
static bool changes(const uint8_t *before,const uint8_t *after,unsigned old_buttons,unsigned new_buttons,struct input_event *out,unsigned *n){
 struct input_event tmp[32];unsigned count=0;
 for(unsigned pass=0;pass<2;pass++)for(unsigned u=4;u<256;u++)if(((u>=224 && u<=231)==(pass==0)) && held(before,u)!=held(after,u)){
  unsigned code=u>=224 && u<=231?(unsigned[]){29,42,56,125,97,54,100,126}[u-224]:usb_usage_code(u);
  if(!code)continue;
  if(count==32)return false;
  tmp[count++]=(struct input_event){1,(uint16_t)code,held(after,u)};
 }
 for(unsigned i=0;i<3;i++)if((old_buttons^new_buttons)&(1u<<i)){if(count==32)return false;tmp[count++]=(struct input_event){1,(uint16_t)(272+i),(new_buttons>>i)&1};}
 if(count>32-*n)return false;
 for(unsigned i=0;i<count;i++)out[(*n)++]=tmp[i];
 return true;
}
bool hid_decode(const struct hid_layout *l,struct hid_report_state *s,const uint8_t *p,unsigned n,struct input_event *out,unsigned *count){
 if(!l || !s || !p || !out || !count || *count>32 || !n || l->reports>HID_REPORTS || l->count>HID_FIELDS)return false;
 unsigned id=0;if(l->numbered){id=*p++;n--;}
 unsigned ri=0;while(ri<l->reports && l->ids[ri]!=id)ri++;
 if(ri==l->reports || n<(l->bits[ri]+7u)/8u || n>64)return false;
 uint8_t next[32]={0},before[32]={0},after[32]={0};unsigned buttons=0;
 struct input_event temp[32];unsigned used=0;
 for(unsigned i=0;i<l->count;i++){
  const struct hid_field *f=&l->field[i];if(f->id!=id)continue;
  for(unsigned j=0;j<f->count;j++){
   int32_t v=field_value(p,f,f->bit+j*f->size);
   if(f->page==7){
    unsigned u=(f->flags&2)?f->usage:(unsigned)v;
    if(!(f->flags&2) && u && u<4)return false; /* rollover/error: preserve state */
    if(!(f->flags&2) && u>255)return false;
    if((f->flags&2)?v!=0:u!=0)next[u/8]|=1u<<(u%8);
   }else if(f->page==9){if(v)buttons|=1u<<(f->usage-1);}
   else {
    if(v<f->minimum || v>f->maximum)return false;
    unsigned code=f->usage==0x30?0:f->usage==0x31?1:8;unsigned type=2;
    if(!(f->flags&4)){v=(int32_t)(((int64_t)v-f->minimum)*65535/((int64_t)f->maximum-f->minimum));type=3;}
    if(type==3 || v){if(used==32)return false;temp[used++]=(struct input_event){(uint16_t)type,(uint16_t)code,v};}
   }
  }
 }
 unsigned old_buttons=0,new_buttons=buttons;
 for(unsigned i=0;i<l->reports;i++){
  old_buttons|=s->buttons[i];if(i!=ri)new_buttons|=s->buttons[i];
  for(unsigned j=0;j<32;j++){before[j]|=s->keys[i][j];after[j]|=i==ri?next[j]:s->keys[i][j];}
 }
 unsigned held_count=0;for(unsigned u=0;u<256;u++)if(held(after,u))held_count++;if(held_count>24)return false;
 if(!changes(before,after,old_buttons,new_buttons,temp,&used) || used>32-*count)return false;
 for(unsigned i=0;i<used;i++)out[(*count)++]=temp[i];
 for(unsigned j=0;j<32;j++)s->keys[ri][j]=next[j];
 s->buttons[ri]=(uint8_t)buttons;return true;
}
bool hid_release(struct hid_report_state *s,struct input_event *out,unsigned *count){
 if(!s || !out || !count || *count>32)return false;
 uint8_t before[32]={0},empty[32]={0};unsigned buttons=0;
 for(unsigned i=0;i<HID_REPORTS;i++){buttons|=s->buttons[i];for(unsigned j=0;j<32;j++)before[j]|=s->keys[i][j];}
 if(!changes(before,empty,buttons,0,out,count))return false;
 *s=(struct hid_report_state){0};return true;
}
