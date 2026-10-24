#ifndef CHOS_GRAPHICS_H
#define CHOS_GRAPHICS_H
#include "desktop.h"
enum gfx_font { FONT_BODY, FONT_BOLD, FONT_TITLE, FONT_SMALL, FONT_MONO };
struct gfx_fonts { const unsigned char *data; size_t size; };
bool gfx_fonts_open(struct gfx_fonts *,const void *,size_t);
int gfx_font_width(const struct gfx_fonts *,unsigned,const char *);
void gfx_font_text(struct surface *,const struct gfx_fonts *,unsigned,int,int,const char *,uint32_t);
void gfx_ui_text(struct surface *,unsigned,int,int,const char *,uint32_t);
int gfx_ui_width(unsigned,const char *);
uint32_t gfx_blend(uint32_t,uint32_t,unsigned);
void gfx_round(struct surface *,int,int,int,int,int,uint32_t,uint32_t,unsigned);
void gfx_gradient(struct surface *,int,int,int,int,uint32_t,uint32_t);
void gfx_shadow(struct surface *,int,int,int,int,int);
void gfx_cursor(struct surface *,int,int,unsigned);
void gfx_blit(struct surface *,int,int,const uint32_t *,int,int,int);
#endif
