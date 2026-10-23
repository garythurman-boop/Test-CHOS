#include "graphics.h"
#include <limits.h>
/* All raster loops intersect the surface clip before touching pixels. Source
 * dimensions/stride are trusted kernel asset metadata, never user pointers. */
uint32_t gfx_blend(uint32_t dst,uint32_t src,unsigned a) {
    if(a>255)a=255;
    unsigned b=255-a;
    unsigned r=(((src>>16)&255)*a+((dst>>16)&255)*b+127)/255;
    unsigned g=(((src>>8)&255)*a+((dst>>8)&255)*b+127)/255;
    unsigned v=((src&255)*a+(dst&255)*b+127)/255;
    return (r<<16)|(g<<8)|v;
}
static bool bounds(struct surface *s,int x,int y,int w,int h,int *l,int *t,int *r,int *b) {
    if(w<=0 || h<=0)return false;
    int64_t right=(int64_t)x+w,bottom=(int64_t)y+h;
    *l=x>s->x?x:s->x;*t=y>s->y?y:s->y;
    *r=right<(int64_t)s->x+s->w?(int)right:s->x+s->w;
    *b=bottom<(int64_t)s->y+s->h?(int)bottom:s->y+s->h;
    return *r>*l && *b>*t;
}
void gfx_gradient(struct surface *s,int x,int y,int w,int h,uint32_t top,uint32_t bottom) {
    int l,t,r,b;if(!bounds(s,x,y,w,h,&l,&t,&r,&b))return;
    for(int j=t;j<b;j++) {
        unsigned a=h>1?(unsigned)(((int64_t)j-y)*255/(h-1)):0;
        uint32_t c=gfx_blend(top,bottom,a);
        for(int i=l;i<r;i++)s->pixels[(size_t)j*s->stride+i]=c;
    }
}
void gfx_round(struct surface *s,int x,int y,int w,int h,int radius,uint32_t top,uint32_t bottom,unsigned alpha) {
    int l,t,r,b;if(!bounds(s,x,y,w,h,&l,&t,&r,&b))return;
    if(radius<0)radius=0;
    if(radius>32)radius=32;
    if(radius>w/2)radius=w/2;
    if(radius>h/2)radius=h/2;
    for(int j=t;j<b;j++) {
        uint32_t color=gfx_blend(top,bottom,h>1?(unsigned)(((int64_t)j-y)*255/(h-1)):0);
        for(int i=l;i<r;i++) {
            int64_t dx=(int64_t)i-x,dy=(int64_t)j-y;
            unsigned coverage=16;
            if(radius && (dx<radius || dx>=w-radius) && (dy<radius || dy>=h-radius)) {
                int64_t cx=dx<radius?radius:w-radius,cy=dy<radius?radius:h-radius;
                coverage=0;
                for(int sy=0;sy<4;sy++)for(int sx=0;sx<4;sx++) {
                    int64_t a=dx*8+sx*2+1-cx*8,c=dy*8+sy*2+1-cy*8;
                    if(a*a+c*c<=(int64_t)radius*radius*64)coverage++;
                }
            }
            if(coverage) {
                uint32_t *p=&s->pixels[(size_t)j*s->stride+i];
                *p=gfx_blend(*p,color,(alpha>255?255:alpha)*coverage/16);
            }
        }
    }
}
void gfx_shadow(struct surface *s,int x,int y,int w,int h,int radius) {
    if(w<=0 || h<=0 || x<INT_MIN+8 || y<INT_MIN+8 || x>INT_MAX-12 || y>INT_MAX-12 || w>INT_MAX-16 || h>INT_MAX-16)return;
    for(int spread=8;spread>0;spread--)gfx_round(s,x-spread,y-spread+4,w+spread*2,h+spread*2,radius+spread,0x020b20,0x020b20,9);
}
void gfx_blit(struct surface *s,int x,int y,const uint32_t *src,int w,int h,int stride) {
    int l,t,r,b;if(!src || stride<w || !bounds(s,x,y,w,h,&l,&t,&r,&b))return;
    for(int j=t;j<b;j++)for(int i=l;i<r;i++) {
        uint32_t c=src[(size_t)((int64_t)j-y)*stride+(size_t)((int64_t)i-x)];
        uint32_t *p=&s->pixels[(size_t)j*s->stride+i];*p=gfx_blend(*p,c,c>>24);
    }
}
