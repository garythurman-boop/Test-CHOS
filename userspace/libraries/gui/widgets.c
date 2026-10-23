#include "gui.h"
#include "royale.h"
static unsigned length(const char *s,unsigned cap) {unsigned n=0;if(s)while(n<cap && s[n])n++;return n;}
static void draw(struct ui_context *g,unsigned op,int x,int y,int w,int h,const char *text,uint32_t color) {
    struct chos_draw d={.op=op,.x=x,.y=y,.w=w,.h=h,.color=color};
    if(text)for(unsigned i=0;i<79 && text[i];i++)d.text[i]=text[i];
    if(g->emit)g->emit(g->output,&d);
}
static void rect(struct ui_context *g,int x,int y,int w,int h,uint32_t c) {draw(g,DRAW_RECT,x,y,w,h,0,c);}
static void round_box(struct ui_context *g,int x,int y,int w,int h,uint32_t c) {draw(g,DRAW_ROUND,x,y,w,h,0,c);}
void ui_label(struct ui_context *g,unsigned font,int x,int y,const char *s,uint32_t c) {draw(g,DRAW_UI_TEXT,x,y,(int)font,0,s,c);}
static bool inside(const struct ui_control *c,int x,int y) {return c->enabled && x>=c->x && y>=c->y && (int64_t)x-c->x<c->w && (int64_t)y-c->y<c->h;}
static struct ui_control *by_id(struct ui_context *g,unsigned id) {for(unsigned i=0;i<g->count;i++)if(g->controls[i].id==id)return &g->controls[i];return 0;}
static struct ui_control *add(struct ui_context *g,unsigned id,unsigned key,unsigned kind,int x,int y,int w,int h,bool enabled) {
    if(!id || g->count==UI_CONTROLS || by_id(g,id) || x < -4096 || x>4096 || y < -4096 || y>4096 || w<1 || w>4096 || h<1 || h>4096)return 0;
    struct ui_control *c=&g->controls[g->count++];*c=(struct ui_control){.id=id,.key=key,.kind=kind,.x=x,.y=y,.w=w,.h=h,.enabled=enabled};return c;
}
void ui_begin(struct ui_context *g) {g->count=0;}
void ui_modal_edit(struct ui_context *g,unsigned id,char *value,unsigned capacity) {
    /* Bind keyboard input immediately, before the next paint. Otherwise queued
     * characters can reach a stale pre-dialog registry and split caret state. */
    ui_begin(g);g->active=0;g->down=false;g->hot=0;g->relayout=true;
    if(!value || !capacity || capacity>120)return;
    struct ui_control *c=add(g,id,0,UI_TEXTFIELD,-4096,-4096,1,1,true);
    if(c){c->buffer=value;c->capacity=capacity;g->focus=id;g->caret=length(value,capacity-1);}
}
void ui_modal_end(struct ui_context *g){ui_begin(g);g->focus=g->active=g->hot=0;g->down=false;g->relayout=true;}
void ui_end(struct ui_context *g) {
    if(!by_id(g,g->focus))g->focus=0;
    if(!by_id(g,g->active)){g->active=0;g->down=false;}
    struct ui_control *hot=0;for(unsigned i=0;i<g->count;i++)if(inside(&g->controls[i],g->mouse_x,g->mouse_y))hot=&g->controls[i];
    g->hot=hot?hot->id:0;
}
void ui_button(struct ui_context *g,unsigned id,unsigned key,int x,int y,int w,int h,const char *s,bool enabled) {
    if(!add(g,id,key,UI_BUTTON,x,y,w,h,enabled))return;
    bool pressed=g->active==id && g->hot==id;
    uint32_t c=!enabled?ROYALE_WIDGET_DISABLED:(pressed?ROYALE_WIDGET_PRESSED:(g->hot==id?ROYALE_WIDGET_HOVER:ROYALE_WIDGET_BOTTOM));
    round_box(g,x,y,w,h,g->focus==id?ROYALE_FOCUS:ROYALE_WIDGET_BORDER);round_box(g,x+1,y+1,w-2,h-2,c);
    if(enabled && !pressed)round_box(g,x+2,y+2,w-4,h/2-1,ROYALE_WIDGET_TOP);
    ui_label(g,0,x+12+(pressed?1:0),y+(h-18)/2+(pressed?1:0),s,enabled?ROYALE_APP_INK:0x7d8995);
    /* A simple inner line gives keyboard focus without spending one draw
     * command per dotted pixel in the bounded kernel command list. */
    if(g->focus==id)rect(g,x+6,y+h-5,w-12,1,ROYALE_FOCUS);
}
void ui_textfield(struct ui_context *g,unsigned id,int x,int y,int w,char *value,unsigned cap) {
    if(!value || !cap || cap>120 || w<20)return;
    struct ui_control *c=add(g,id,0,UI_TEXTFIELD,x,y,w,25,true);if(!c)return;c->buffer=value;c->capacity=cap;
    round_box(g,x,y,w,25,g->focus==id?ROYALE_FOCUS:ROYALE_WIDGET_BORDER);rect(g,x+1,y+1,w-2,23,0xffffff);
    unsigned n=length(value,cap-1),visible=(unsigned)(w-12)/12,start=0;
    if(g->focus==id && g->caret>n)g->caret=n;
    if(n>visible)start=n-visible;
    if(g->focus==id && g->caret<start)start=g->caret;
    if(g->focus==id && g->caret>start+visible)start=g->caret-visible;
    char text[80]={0};unsigned shown=n-start;if(shown>visible)shown=visible;if(shown>79)shown=79;
    for(unsigned i=0;i<shown;i++)text[i]=value[start+i];
    ui_label(g,4,x+4,y+5,text,0x163b65);
    if(g->focus==id && g->caret>=start && g->caret<=start+shown)rect(g,x+4+(int)(g->caret-start)*12,y+4,1,17,ROYALE_FOCUS);
}
static void toggle(struct ui_context *g,unsigned id,unsigned key,unsigned kind,int x,int y,const char *s,bool selected,bool enabled) {
    if(!add(g,id,key,kind,x,y,190,26,enabled))return;
    if(g->hot==id)round_box(g,x-3,y-1,190,26,ROYALE_WIDGET_HOVER);
    if(kind==UI_RADIO) {round_box(g,x,y+4,17,17,ROYALE_WIDGET_BORDER);round_box(g,x+2,y+6,13,13,0xffffff);if(selected)round_box(g,x+5,y+9,7,7,ROYALE_FOCUS);}
    else {round_box(g,x,y+4,17,17,ROYALE_WIDGET_BORDER);rect(g,x+2,y+6,13,13,enabled?0xffffff:ROYALE_WIDGET_DISABLED);if(selected)ui_label(g,1,x+4,y+3,"x",ROYALE_FOCUS);}
    ui_label(g,0,x+25,y+4,s,enabled?ROYALE_APP_INK:0x7d8995);
    if(g->focus==id)rect(g,x+25,y+23,160,1,ROYALE_FOCUS);
}
void ui_checkbox(struct ui_context *g,unsigned id,unsigned key,int x,int y,const char *s,bool selected,bool enabled) {toggle(g,id,key,UI_CHECKBOX,x,y,s,selected,enabled);}
void ui_radio(struct ui_context *g,unsigned id,unsigned key,int x,int y,const char *s,bool selected,bool enabled) {toggle(g,id,key,UI_RADIO,x,y,s,selected,enabled);}
void ui_scrollbar(struct ui_context *g,unsigned id,int x,int y,int h,unsigned value,unsigned total,unsigned page) {
    if(h<54 || !page)return;
    struct ui_control *c=add(g,id,0,UI_SCROLLBAR,x,y,18,h,total>page);if(!c)return;
    unsigned maximum=total>page?total-page:0;if(value>maximum)value=maximum;c->value=value;c->total=total;c->page=page;
    round_box(g,x,y,18,h,0xdce6f0);round_box(g,x+1,y+1,16,17,ROYALE_WIDGET_BOTTOM);round_box(g,x+1,y+h-18,16,17,ROYALE_WIDGET_BOTTOM);
    ui_label(g,3,x+5,y+1,"^",ROYALE_APP_INK);ui_label(g,3,x+5,y+h-17,"v",ROYALE_APP_INK);
    int track=h-36,thumb=total>page?(int)((uint64_t)page*(unsigned)track/total):track;
    if(thumb<18)thumb=18;
    if(thumb>track)thumb=track;
    int offset=maximum?(int)((uint64_t)value*(unsigned)(track-thumb)/maximum):0;
    round_box(g,x+2,y+18+offset,14,thumb,g->active==id?ROYALE_WIDGET_PRESSED:ROYALE_WIDGET_BORDER);
}
void ui_list_row(struct ui_context *g,unsigned id,int x,int y,int w,const char *s,bool selected,uint32_t color) {
    if(!add(g,id,0,UI_LISTROW,x,y,w,22,true))return;
    if(selected || g->hot==id)rect(g,x,y,w,22,selected?ROYALE_ROW_SELECT:0xe4eef8);
    ui_label(g,4,x+12,y+3,s,color);
}
void ui_menu_panel(struct ui_context *g,int x,int y,int w,int h){if(w<4 || h<4)return;round_box(g,x,y,w,h,ROYALE_WIDGET_BORDER);rect(g,x+2,y+2,w-4,h-4,0xf5f9fd);}
void ui_menu_item(struct ui_context *g,unsigned id,unsigned key,int x,int y,int w,const char *s,bool enabled) {
    if(!add(g,id,key,UI_MENU,x,y,w,27,enabled))return;
    if(g->hot==id || g->focus==id)round_box(g,x,y,w,27,g->active==id?ROYALE_WIDGET_PRESSED:ROYALE_WIDGET_HOVER);
    ui_label(g,0,x+10,y+5,s,enabled?ROYALE_APP_INK:0x7d8995);
}
void ui_tab(struct ui_context *g,unsigned id,unsigned key,int x,int y,int w,const char *s,bool selected) {
    if(!add(g,id,key,UI_TAB,x,y,w,28,true))return;
    round_box(g,x,y,w,28,selected?0xf8fbff:(g->hot==id?ROYALE_WIDGET_HOVER:ROYALE_WIDGET_BOTTOM));
    ui_label(g,selected?1:0,x+12,y+5,s,ROYALE_APP_INK);if(selected)rect(g,x+4,y+26,w-8,2,ROYALE_FOCUS);
}
void ui_toolbar(struct ui_context *g,int x,int y,int w,int h) {rect(g,x,y,w,h,0xd5e5f2);rect(g,x,y+h-1,w,1,0xb7cddd);}
void ui_dialog(struct ui_context *g,int x,int y,int w,int h,const char *title) {round_box(g,x,y,w,h,ROYALE_DIALOG_BG);ui_label(g,1,x+8,y+8,title,0x163b65);}
void ui_progress(struct ui_context *g,int x,int y,int w,int h,unsigned value) {
    if(w<4 || h<4)return;
    if(value>100)value=100;
    round_box(g,x,y,w,h,ROYALE_WIDGET_BORDER);rect(g,x+1,y+1,w-2,h-2,0xe0ebf5);
    int filled=(int)((uint64_t)(unsigned)(w-4)*value/100);if(filled)round_box(g,x+2,y+2,filled,h-4,0x398cbb);
}
static unsigned scroll_value(const struct ui_control *c,int y,bool drag) {
    unsigned maximum=c->total>c->page?c->total-c->page:0;
    if(!drag && y<c->y+18)return c->value?c->value-1:0;
    if(!drag && y>=c->y+c->h-18)return c->value<maximum?c->value+1:maximum;
    int track=c->h-36,thumb=c->total?(int)((uint64_t)c->page*(unsigned)track/c->total):track;
    if(thumb<18)thumb=18;
    if(thumb>=track)return 0;
    int offset=y-c->y-18-thumb/2;if(offset<0)offset=0;if(offset>track-thumb)offset=track-thumb;
    return (unsigned)((uint64_t)(unsigned)offset*maximum/(unsigned)(track-thumb));
}
struct ui_result ui_event(struct ui_context *g,const struct chos_event *e) {
    struct ui_result r={0};
    if(e->type==EVENT_RESIZE){g->active=0;g->down=false;return r;}
    if(e->type==EVENT_POINTER || e->type==EVENT_CLICK || e->type==EVENT_RELEASE) {
        unsigned old=g->hot;g->mouse_x=e->x;g->mouse_y=e->y;g->hot=0;
        for(unsigned i=0;i<g->count;i++)if(inside(&g->controls[i],e->x,e->y))g->hot=g->controls[i].id;
        r.repaint=old!=g->hot;
        if(e->type==EVENT_POINTER) {
            struct ui_control *c=by_id(g,g->active);
            if(c && c->kind==UI_SCROLLBAR && g->down){r=(struct ui_result){.consumed=true,.repaint=true,.id=c->id,.value=scroll_value(c,e->y,true)};}
            return r;
        }
        if(e->type==EVENT_CLICK) {
            struct ui_control *c=by_id(g,g->hot);g->active=g->hot;g->down=c!=0;
            if(c){g->focus=c->id;if(c->kind==UI_TEXTFIELD)g->caret=length(c->buffer,c->capacity-1);r.consumed=true;r.repaint=true;if(c->kind==UI_LISTROW){r.id=c->id;g->active=0;g->down=false;}}
            return r;
        }
        struct ui_control *c=by_id(g,g->active);g->active=0;g->down=false;
        if(c){r.consumed=true;r.repaint=true;if(g->hot==c->id){r.id=c->id;r.key=c->key;r.value=c->kind==UI_SCROLLBAR?scroll_value(c,e->y,false):c->value;}}
        return r;
    }
    if(e->type!=EVENT_KEY)return r;
    if(e->code==9 || e->code==(9|KEY_SHIFT)) {
        unsigned start=(e->code&KEY_SHIFT)?0:(g->count?g->count-1:0);
        for(unsigned i=0;i<g->count;i++)if(g->controls[i].id==g->focus)start=i;
        for(unsigned step=1;step<=g->count;step++) {
            unsigned at=(e->code&KEY_SHIFT)?(start+g->count-step)%g->count:(start+step)%g->count;
            if(g->controls[at].enabled){g->focus=g->controls[at].id;r.consumed=true;r.repaint=true;break;}
        }
        return r;
    }
    struct ui_control *c=by_id(g,g->focus);if(!c || !c->enabled)return r;
    if(c->kind==UI_TEXTFIELD) {
        char *s=c->buffer;unsigned n=length(s,c->capacity-1);if(g->caret>n)g->caret=n;
        unsigned key=e->code;r.consumed=true;r.repaint=true;r.id=c->id;
        if(key==('u'|KEY_CTRL)){s[0]=0;g->caret=0;}
        else if(key==KEY_LEFT){if(g->caret)g->caret--;}
        else if(key==KEY_RIGHT){if(g->caret<n)g->caret++;}
        else if(key==8){if(g->caret){for(unsigned i=g->caret-1;i<n;i++)s[i]=s[i+1];g->caret--;}}
        else if(key>=32 && key<127 && n+1<c->capacity){for(unsigned i=n+1;i>g->caret;i--)s[i]=s[i-1];s[g->caret++]=(char)key;}
        else {r.consumed=false;r.id=0;r.repaint=false;}
        return r;
    }
    if(c->kind==UI_MENU && (e->code==KEY_UP || e->code==KEY_DOWN)) {
        unsigned start=0;for(unsigned i=0;i<g->count;i++)if(g->controls[i].id==c->id)start=i;
        for(unsigned step=1;step<=g->count;step++) {
            unsigned at=e->code==KEY_UP?(start+g->count-step)%g->count:(start+step)%g->count;
            if(g->controls[at].enabled && g->controls[at].kind==UI_MENU){g->focus=g->controls[at].id;r.consumed=true;r.repaint=true;break;}
        }
        return r;
    }
    if(e->code==13 || e->code==' '){r=(struct ui_result){.consumed=true,.repaint=true,.id=c->id,.key=c->key,.value=c->value};}
    return r;
}
