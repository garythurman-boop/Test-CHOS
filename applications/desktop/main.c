#include "abi.h"
#include <stddef.h>
static int64_t call(unsigned number,uint64_t a,uint64_t b) {
    register uint64_t x0 __asm__("x0")=a,x1 __asm__("x1")=b,x8 __asm__("x8")=number;
    __asm__ volatile("svc #0":"+r"(x0):"r"(x1),"r"(x8):"memory");return (int64_t)x0;
}
static int window;
static void draw(unsigned op,int x,int y,int w,int h,const char *text,uint32_t color) {
    struct chos_draw d={.op=op,.x=x,.y=y,.w=w,.h=h,.color=color};
    if(text) for(unsigned i=0;i<79 && text[i];i++)d.text[i]=text[i];
    (void)call(SYS_DRAW,(uint64_t)window,(uintptr_t)&d);
}
int user_main(uint64_t role,uint64_t unused) {
    (void)role;(void)unused;
    struct chos_window spec={.title="About CHOS",.width=600,.height=330};
    if(call(SYS_WINDOW,0,0)!=-14)return 90;
    struct chos_window invalid={.width=1,.height=1};
    if(call(SYS_WINDOW,(uintptr_t)&invalid,0)!=-22)return 91;
    window=(int)call(SYS_WINDOW,(uintptr_t)&spec,0);if(window<0)return 1;
    if(call(SYS_DRAW,0,0)!=-9 || call(SYS_DRAW,(uint64_t)window,0)!=-14 || call(SYS_EVENT,(uint64_t)window,0)!=-14)return 92;
    struct chos_draw invalid_draw={.op=500};
    if(call(SYS_DRAW,(uint64_t)window,(uintptr_t)&invalid_draw)!=-22)return 93;
    unsigned pressed=0;
    for(;;) {
        draw(DRAW_CLEAR,0,0,0,0,0,0xe5eef8);
        draw(DRAW_TEXT,20,22,0,0,"CHOS Desktop",0x15487c);
        draw(DRAW_TEXT,20,54,0,0,"Original ARM64 operating system",0x15487c);
        draw(DRAW_TEXT,20,86,0,0,"Native EL0 window client",0x15487c);
        draw(DRAW_TEXT,20,118,0,0,"Move, resize, minimize or maximize.",0x15487c);
        draw(DRAW_TEXT,20,150,0,0,"Press a key to change the swatch.",0x15487c);
        draw(DRAW_RECT,20,184,80,32,0,pressed?0x328c69:0x326bb0);
        if(call(SYS_PRESENT,(uint64_t)window,0))return 2;
        struct chos_event event;
        if(call(SYS_EVENT,(uint64_t)window,(uintptr_t)&event)==0) {
            if(event.type==EVENT_CLOSE)return 0;
            if(event.type==EVENT_KEY)pressed++;
        }
    }
}
