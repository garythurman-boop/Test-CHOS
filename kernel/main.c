#include "boot.h"
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
static void rect(unsigned x,unsigned y,unsigned w,unsigned h,uint32_t rgb) {
    volatile uint32_t *fb=(void *)(uintptr_t)boot->framebuffer;
    uint32_t c=color(rgb);
    for (unsigned j=y;j<y+h && j<boot->height;j++)
        for (unsigned i=x;i<x+w && i<boot->width;i++) fb[(uint64_t)j*boot->stride+i]=c;
}
static void text(unsigned x,unsigned y,const char *s,uint32_t rgb) {
    for (;*s;s++,x+=12) {
        int index=*s>='A'&&*s<='Z'?*s-'A':(*s>='0'&&*s<='9'?26+*s-'0':-1);
        if (index<0) continue;
        for (unsigned row=0;row<7;row++)
            for (unsigned col=0;col<5;col++)
                if (font[index][row]&(16>>col)) rect(x+col*2,y+row*2,2,2,rgb);
    }
}
static void number(unsigned x,unsigned y,uint64_t n) {
    char buf[21]; unsigned i=20; buf[i]=0;
    do { buf[--i]=(char)('0'+n%10); n/=10; } while(n);
    text(x,y,buf+i,0xffffff);
}
void kernel_main(struct chos_boot_info *info) {
    if (!info || info->magic!=CHOS_BOOT_MAGIC) for (;;) __asm__ volatile("wfe");
    boot=info;
    rect(0,0,boot->width,boot->height,0x071b36);
    rect(0,0,boot->width,72,0x164a82);
    text(32,24,"CHOS",0xffffff);
    text(32,100,"ORIGINAL ARM64 KERNEL",0xffffff);
    text(32,132,"FIRMWARE BOOT SERVICES EXITED",0x8edbff);
    uint64_t el,freq;
    __asm__ volatile("mrs %0, CurrentEL":"=r"(el));
    __asm__ volatile("mrs %0, cntfrq_el0":"=r"(freq));
    text(32,172,"EXCEPTION LEVEL",0xffffff); number(260,172,el>>2);
    text(32,200,"TIMER FREQUENCY HZ",0xffffff); number(300,200,freq);
    /* UEFI descriptor prefix: type, padding, physical, virtual, pages, attributes.
     * Iterate with the firmware-provided descriptor stride, never sizeof(prefix). */
    if (boot->descriptor_size<40 || boot->descriptor_size>boot->memory_map_size ||
        boot->memory_map_size%boot->descriptor_size) {
        text(32,244,"INVALID MEMORY MAP",0xff7070);
        for (;;) __asm__ volatile("wfe");
    }
    uint64_t pages=0,count=boot->memory_map_size/boot->descriptor_size;
    for (uint64_t i=0;i<count;i++) {
        const uint8_t *d=(void *)(uintptr_t)(boot->memory_map+i*boot->descriptor_size);
        if (*(const uint32_t *)d==7) pages+=*(const uint64_t *)(d+24);
    }
    text(32,228,"CONVENTIONAL RAM MIB",0xffffff); number(300,228,pages/256);
    text(32,256,"MEMORY DESCRIPTORS",0xffffff); number(300,256,count);
    text(32,308,"MILESTONE 1 BOOT COMPLETE",0x8effb0);
    text(32,344,"NO DESKTOP OR INSTALLER YET",0xffffff);
    /* Test completion signature is painted only after boot-info/map validation. */
    static const uint32_t signature[8]={0x43c0de,0x6b45a1,0xf3ba20,0x18ad72,
                                        0xe14567,0x2998ef,0xcddc39,0x804020};
    for (unsigned i=0;i<8;i++) rect(32+i*16,400,16,16,signature[i]);
    __asm__ volatile("dsb sy":::"memory");
    for (;;) __asm__ volatile("wfe");
}
