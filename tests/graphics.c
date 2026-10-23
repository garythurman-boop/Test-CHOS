#include "graphics.h"
#include "display_mode.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern const unsigned char royale_fonts[],royale_fonts_end[];
int main(void) {
    struct gfx_fonts fonts;size_t bytes=(size_t)(royale_fonts_end-royale_fonts);
    assert(gfx_fonts_open(&fonts,royale_fonts,bytes));
    assert(gfx_font_width(&fonts,FONT_MONO,"CHOS")==48);
    assert(gfx_font_width(&fonts,FONT_BODY,"iii")<gfx_font_width(&fonts,FONT_BODY,"WWW"));
    unsigned char *bad=malloc(bytes);assert(bad);
    memcpy(bad,royale_fonts,bytes);bad[0]=0;assert(!gfx_fonts_open(&fonts,bad,bytes));
    memcpy(bad,royale_fonts,bytes);memset(bad+36,255,4);assert(!gfx_fonts_open(&fonts,bad,bytes));
    assert(!gfx_fonts_open(&fonts,royale_fonts,103));
    memcpy(bad,royale_fonts,bytes);memset(bad+104,255,4);assert(!gfx_fonts_open(&fonts,bad,bytes));
    assert(gfx_fonts_open(&fonts,royale_fonts,bytes));free(bad);
    struct {uint32_t before,pixels[64*40],after;} buf={.before=0x76543210,.after=0x12345678};
    struct surface s={.pixels=buf.pixels,.width=64,.height=40,.stride=64};gfx_clip(&s,0,0,64,40);
    gfx_round(&s,2,2,30,20,8,0xffffff,0xffffff,255);
    assert(!buf.pixels[2*64+2] && buf.pixels[12*64+15]==0xffffff);
    bool antialias=false;for(unsigned i=0;i<64*40;i++)if(buf.pixels[i] && buf.pixels[i]!=0xffffff)antialias=true;
    assert(antialias);
    assert(gfx_blend(0,0xffffff,128)==0x808080 && gfx_blend(0x123456,0xffffff,0)==0x123456);
    gfx_rect(&s,0,0,64,40,0);gfx_font_text(&s,&fonts,FONT_BODY,1,1,"CHOS Royale",0xffffff);
    antialias=false;for(unsigned i=0;i<64*40;i++)if(buf.pixels[i] && buf.pixels[i]!=0xffffff)antialias=true;assert(antialias);
    gfx_clip(&s,8,8,12,12);gfx_round(&s,INT_MIN,INT_MIN,INT_MAX,INT_MAX,32,1,2,255);
    gfx_round(&s,INT_MAX,INT_MAX,20,20,32,1,2,255);gfx_shadow(&s,2,2,30,20,8);
    gfx_font_text(&s,&fonts,FONT_MONO,INT_MIN,INT_MAX,"unsafe?",0xff);
    uint32_t src[]={0x00ffffff,0x80ffffff,0xffffffff,0xff123456};
    gfx_clip(&s,0,0,64,40);gfx_rect(&s,0,0,64,40,0);gfx_blit(&s,0,0,src,2,2,2);
    assert(buf.pixels[0]==0 && buf.pixels[1]==0x808080 && buf.pixels[64]==0xffffff && buf.pixels[65]==0x123456);
    assert(buf.before==0x76543210 && buf.after==0x12345678);
    uint32_t reference[64*40]={0},optimized[64*40]={0};
    struct surface a={.pixels=reference,.width=64,.height=40,.stride=64},b={.pixels=optimized,.width=64,.height=40,.stride=64};gfx_clip(&a,0,0,64,40);gfx_clip(&b,0,0,64,40);
    for(int spread=8;spread>0;spread--)gfx_round(&a,8-spread,5-spread+4,40+spread*2,24+spread*2,6+spread,0x020b20,0x020b20,9);
    gfx_shadow(&b,8,5,40,24,6);gfx_round(&a,8,5,40,24,6,0xabcdef,0xabcdef,255);gfx_round(&b,8,5,40,24,6,0xabcdef,0xabcdef,255);
    assert(!memcmp(reference,optimized,sizeof(reference)));
    uint32_t w=0,h=0;assert(display_request("1024x768",8,&w,&h) && w==1024 && h==768);
    assert(display_request("800x600\n",8,&w,&h));
    assert(!display_request("10000x768",9,&w,&h) && !display_request("0x0",3,&w,&h));
    assert(!display_request("1024x768x",9,&w,&h) && !display_request("1024x",5,&w,&h));
    assert(!display_mode_supported(1921,1200,1921,0) && !display_mode_supported(1024,768,1024,2));
    puts("Royale font validation, grayscale coverage, proportional metrics, roundrects, blend, clipped blit and firmware mode policy passed");
}
