#include "graphics.h"
#include <limits.h>
static uint32_t u32(const unsigned char *p) { return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static unsigned u16(const unsigned char *p) {return (unsigned)p[0]|((unsigned)p[1]<<8);}
bool gfx_fonts_open(struct gfx_fonts *f,const void *data,size_t size) {
    *f=(struct gfx_fonts){0};const unsigned char *p=data;
    if(!p || size<24 || size>1024*1024)return false;
    const char magic[]="CHOSFNT1";
    for(unsigned i=0;i<8;i++)if(p[i]!=(unsigned char)magic[i])return false;
    if(u32(p+8)!=5 || u32(p+12)!=32 || u32(p+16)!=95 || u32(p+20)!=24 || size<104)return false;
    for(unsigned style=0;style<5;style++) {
        const unsigned char *s=p+24+style*16;uint32_t table=u32(s+12);
        if(u32(s)>32 || u32(s+4)>40 || !u32(s+8) || u32(s+8)>48 || table>size || size-table<95*12)return false;
        for(unsigned i=0;i<95;i++) {
            const unsigned char *g=p+table+i*12;uint32_t offset=u32(g);unsigned w=u16(g+4),h=u16(g+6);
            if(w>32 || h>40 || !u16(g+10) || u16(g+10)>40 || offset>size || w*h>size-offset || (int8_t)g[8]<-32 || (int8_t)g[9]<-40)return false;
        }
    }
    *f=(struct gfx_fonts){p,size};return true;
}
static const unsigned char *glyph(const struct gfx_fonts *f,unsigned style,unsigned c) {
    if(!f || !f->data || style>=5)return 0;
    if(c<32 || c>126)c='?';
    return f->data+u32(f->data+24+style*16+12)+(c-32)*12;
}
int gfx_font_width(const struct gfx_fonts *f,unsigned style,const char *text) {
    int width=0;for(unsigned i=0;text[i] && i<80;i++){const unsigned char *g=glyph(f,style,(unsigned char)text[i]);if(g)width+=(int)u16(g+10);}return width;
}
void gfx_font_text(struct surface *s,const struct gfx_fonts *f,unsigned style,int x,int y,const char *text,uint32_t color) {
    if(!f || !f->data || style>=5)return;
    int64_t pen=x;int baseline=(int)u32(f->data+24+style*16+4);
    for(unsigned n=0;text[n] && n<80;n++) {
        const unsigned char *g=glyph(f,style,(unsigned char)text[n]);
        int w=(int)u16(g+4),h=(int)u16(g+6);int64_t left=pen+(int8_t)g[8],top=(int64_t)y+baseline+(int8_t)g[9];
        const unsigned char *pixels=f->data+u32(g);
        for(int j=0;j<h;j++)if(top+j>=s->y && top+j<(int64_t)s->y+s->h)
            for(int i=0;i<w;i++)if(left+i>=s->x && left+i<(int64_t)s->x+s->w) {
                uint32_t *p=&s->pixels[(size_t)(top+j)*s->stride+(size_t)(left+i)];
                *p=gfx_blend(*p,color,pixels[j*w+i]);
            }
        pen+=u16(g+10);if(pen>INT_MAX)break;
    }
}
extern const unsigned char royale_fonts[],royale_fonts_end[];
static struct gfx_fonts cache;
static bool ready(void) {return cache.data || gfx_fonts_open(&cache,royale_fonts,(size_t)(royale_fonts_end-royale_fonts));}
void gfx_ui_text(struct surface *s,unsigned style,int x,int y,const char *text,uint32_t color) {if(ready())gfx_font_text(s,&cache,style,x,y,text,color);}
int gfx_ui_width(unsigned style,const char *text) {return ready()?gfx_font_width(&cache,style,text):0;}
