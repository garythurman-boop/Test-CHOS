#include "boot.h"
#include "console.h"
/* Original compact 5x7 uppercase diagnostic font, row-major, five low bits. */
static const uint8_t font[36][7]={
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14}
};
static struct chos_boot_info *boot;
static uint32_t color(uint32_t rgb) {
    return boot->pixel_format==1?rgb:((rgb&255)<<16)|(rgb&0xff00)|((rgb>>16)&255);
}
void console_rect(unsigned x,unsigned y,unsigned w,unsigned h,uint32_t rgb) {
    volatile uint32_t *fb=(void *)(uintptr_t)boot->framebuffer;
    uint32_t c=color(rgb);
    for (unsigned j=y;j<y+h && j<boot->height;j++)
        for (unsigned i=x;i<x+w && i<boot->width;i++) fb[(uint64_t)j*boot->stride+i]=c;
}
void console_text(unsigned x,unsigned y,const char *s,uint32_t rgb) {
    for (;*s;s++,x+=12) {
        int index=*s>='A'&&*s<='Z'?*s-'A':(*s>='0'&&*s<='9'?26+*s-'0':-1);
        if (index<0) continue;
        for (unsigned row=0;row<7;row++)
            for (unsigned col=0;col<5;col++)
                if (font[index][row]&(16>>col)) console_rect(x+col*2,y+row*2,2,2,rgb);
    }
}
void console_number(unsigned x,unsigned y,uint64_t n) {
    char buf[21]; unsigned i=20; buf[i]=0;
    do { buf[--i]=(char)('0'+n%10); n/=10; } while(n);
    console_text(x,y,buf+i,0xffffff);
}
void console_init(struct chos_boot_info *info) { boot=info; }
