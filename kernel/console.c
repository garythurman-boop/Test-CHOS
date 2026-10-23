#include "boot.h"
#include "console.h"
extern uint8_t font_row(unsigned,unsigned);
static struct chos_boot_info *boot;
static uint32_t color(uint32_t rgb) {
    return boot->pixel_format==1?rgb:((rgb&255)<<16)|(rgb&0xff00)|((rgb>>16)&255);
}
void console_rect(unsigned x,unsigned y,unsigned w,unsigned h,uint32_t rgb) {
    volatile uint32_t *fb=(void *)(uintptr_t)boot->framebuffer;
    uint32_t c=color(rgb);
    if(x>=boot->width || y>=boot->height) return;
    if(w>boot->width-x) w=boot->width-x;
    if(h>boot->height-y) h=boot->height-y;
    for (unsigned j=y;j<y+h;j++)
        for (unsigned i=x;i<x+w;i++) fb[(uint64_t)j*boot->stride+i]=c;
}
void console_text(unsigned x,unsigned y,const char *s,uint32_t rgb) {
    for (;*s;s++,x+=12) {
        for (unsigned row=0;row<7;row++)
            for (unsigned col=0;col<5;col++)
                if (font_row((unsigned char)*s,row)&(16>>col)) console_rect(x+col*2,y+row*2,2,2,rgb);
    }
}
void console_number(unsigned x,unsigned y,uint64_t n) {
    char buf[21]; unsigned i=20; buf[i]=0;
    do { buf[--i]=(char)('0'+n%10); n/=10; } while(n);
    console_text(x,y,buf+i,0xffffff);
}
void console_init(struct chos_boot_info *info) { boot=info; }
