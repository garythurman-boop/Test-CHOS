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
#endif
