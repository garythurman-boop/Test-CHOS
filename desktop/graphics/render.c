#include "desktop.h"
#include <limits.h>
void gfx_clip(struct surface *s,int x,int y,int w,int h) {
    int64_t right=(int64_t)x+w,bottom=(int64_t)y+h;
    if(right<0) right=0;
    if(bottom<0) bottom=0;
    if(right>s->width) right=s->width;
    if(bottom>s->height) bottom=s->height;
    if(x<0) x=0;
    if(y<0) y=0;
    if(x>s->width) x=s->width;
    if(y>s->height) y=s->height;
    s->x=x;s->y=y;s->w=right>x?(int)(right-x):0;s->h=bottom>y?(int)(bottom-y):0;
}
void gfx_rect(struct surface *s,int x,int y,int w,int h,uint32_t color) {
    if(w<=0 || h<=0) return;
    int64_t right=(int64_t)x+w,bottom=(int64_t)y+h;
    if(x<s->x) x=s->x;
    if(y<s->y) y=s->y;
    if(right>(int64_t)s->x+s->w) right=s->x+s->w;
    if(bottom>(int64_t)s->y+s->h) bottom=s->y+s->h;
    for(int j=y;j<bottom;j++) for(int i=x;i<right;i++) s->pixels[(size_t)j*s->stride+i]=color;
}
extern uint8_t font_row(unsigned,unsigned);
void gfx_text(struct surface *s,int x,int y,const char *text,uint32_t color) {
    int64_t pen=x;
    for(unsigned n=0;text[n] && n<80;n++,pen+=12) {
        if(pen>INT_MAX-10) break;
        if(pen+10<s->x || pen>=s->x+s->w) continue;
        for(unsigned row=0;row<7;row++) {
            int64_t row_y=(int64_t)y+row*2;
            if(row_y<INT_MIN || row_y>INT_MAX-1)continue;
            uint8_t bits=font_row((unsigned char)text[n],row);
            for(unsigned col=0;col<5;col++) if(bits&(16>>col)) gfx_rect(s,(int)pen+(int)col*2,(int)row_y,2,2,color);
        }
    }
}
