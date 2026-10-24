#ifndef CHOS_DISPLAY_MODE_H
#define CHOS_DISPLAY_MODE_H
#include <stdint.h>
#include <stdbool.h>
/* Hardware limits match the desktop allocation guard. Never fabricate a mode. */
static inline bool display_mode_supported(uint32_t w,uint32_t h,uint32_t stride,uint32_t format) {
    return w>=640 && h>=480 && w<=1920 && h<=1200 && stride>=w && stride<=8192 && format<=1;
}
static inline bool display_request(const char *s,unsigned n,uint32_t *w,uint32_t *h) {
    uint32_t values[2]={0};unsigned at=0;
    for(unsigned part=0;part<2;part++) {
        unsigned digits=0;
        while(at<n && s[at]>='0' && s[at]<='9') {if(++digits>4)return false;values[part]=values[part]*10+(unsigned)(s[at++]-'0');}
        if(!digits)return false;
        if(!part && (at==n || s[at++]!='x'))return false;
    }
    if(at<n && s[at]=='\n')at++;
    if(at!=n || !display_mode_supported(values[0],values[1],values[0],0))return false;
    *w=values[0];*h=values[1];return true;
}
/* Fixed, checksummed ESP preference. Trial is consumed before firmware mode
 * selection; an unconfirmed trial falls back on the next cold boot. */
struct display_preference {uint32_t width,height,state;};
static inline uint32_t display_u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static inline void display_put(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(i*8));}
static inline uint32_t display_crc(const uint8_t *p){uint32_t c=~0u;for(unsigned i=0;i<28;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^((0u-(c&1))&0xedb88320u);}return ~c;}
static inline bool display_encode(const struct display_preference *v,uint8_t p[32]){
    if(!v || !p || v->state>2 || !display_mode_supported(v->width,v->height,v->width,0))return false;
    for(unsigned i=0;i<32;i++)p[i]=0;
    const char magic[]="CHOSDSP1";for(unsigned i=0;i<8;i++)p[i]=(uint8_t)magic[i];
    display_put(p+8,v->width);display_put(p+12,v->height);display_put(p+16,v->state);display_put(p+28,display_crc(p));return true;
}
static inline bool display_decode(struct display_preference *v,const uint8_t *p,unsigned n){
    if(!v || !p || n!=32)return false;
    const char magic[]="CHOSDSP1";for(unsigned i=0;i<8;i++)if(p[i]!=(uint8_t)magic[i])return false;
    for(unsigned i=20;i<28;i++)if(p[i])return false;
    struct display_preference next={display_u32(p+8),display_u32(p+12),display_u32(p+16)};
    if(display_u32(p+28)!=display_crc(p) || next.state>2 || !display_mode_supported(next.width,next.height,next.width,0))return false;
    *v=next;return true;
}
#endif
