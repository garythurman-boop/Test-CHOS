#include "graphics.h"
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
void gfx_text(struct surface *s,int x,int y,const char *text,uint32_t color) {
    gfx_ui_text(s,FONT_MONO,x,y,text,color);
}
