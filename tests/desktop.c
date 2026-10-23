#include "desktop.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static void click(struct wm *m,int x,int y){wm_mouse(m,x,y,1);wm_mouse(m,x,y,0);}
int main(void) {
    struct wm m;wm_init(&m,1024,768);
    int first=wm_create(&m,10,"one",500,300),second=wm_create(&m,20,"two",500,300);
    assert(first>0 && second>first && m.focus==(unsigned)second);
    assert(!wm_owned(&m,20,(unsigned)first));
    assert(wm_create(&m,10,"duplicate",500,300)<0);
    struct chos_event event;assert(!wm_event(&m,20,(unsigned)first,&event,true));
    assert(wm_event(&m,10,(unsigned)first,&event,false) && event.type==EVENT_RESIZE);
    assert(wm_event(&m,10,(unsigned)first,&event,true));
    assert(!wm_event(&m,10,(unsigned)first,&event,true));
    wm_focus(&m,(unsigned)first);
    struct wm_window *w=wm_owned(&m,10,(unsigned)first);
    int x=w->x,y=w->y;
    wm_mouse(&m,x+12,y+12,1);wm_mouse(&m,210,120,-1);wm_mouse(&m,210,120,0);
    assert(w->x==198 && w->y==108);
    wm_mouse(&m,w->x+w->w-4,w->y+w->h-4,1);wm_mouse(&m,850,620,-1);wm_mouse(&m,850,620,0);
    assert(w->w==656 && w->h==516);
    click(&m,w->x+w->w-40,w->y+12);assert(w->maximized && w->w==1024 && w->h==724);
    click(&m,w->x+w->w-40,w->y+12);assert(!w->maximized && w->w==656);
    click(&m,w->x+w->w-65,w->y+12);assert(w->minimized && m.focus==(unsigned)second);
    click(&m,120,744);assert(!w->minimized && m.focus==(unsigned)first);
    wm_key(&m,56,1);wm_key(&m,15,1);wm_key(&m,56,0);assert(m.focus==(unsigned)second);
    wm_focus(&m,(unsigned)first);
    wm_key(&m,29,1);wm_key(&m,31,1);wm_key(&m,29,0);
    bool found=false;while(wm_event(&m,10,(unsigned)first,&event,true))if(event.type==EVENT_KEY){assert(event.code==('s'|KEY_CTRL));found=true;}
    assert(found);
    for(unsigned i=0;i<WM_EVENTS*3;i++)wm_key(&m,30,1);
    assert(w->count==WM_EVENTS);
    click(&m,w->x+w->w-10,w->y+12);
    assert(wm_event(&m,10,(unsigned)first,&event,true) && event.type==EVENT_CLOSE);
    wm_remove(&m,10);assert(m.count==1 && !wm_owned(&m,10,(unsigned)first));
    int third=wm_create(&m,30,"new",300,200);assert(third>second && !wm_owned(&m,30,(unsigned)first));
    wm_mouse(&m,40,744,1);assert(!m.menu);wm_mouse(&m,200,680,0);assert(!m.menu);click(&m,40,744);assert(m.menu);
    wm_key(&m,3,1);assert(!m.menu && m.launch==2);
    wm_key(&m,125,1);wm_key(&m,28,1);assert(m.applications);
    wm_key(&m,108,1);wm_key(&m,28,1);assert(m.launch==2 && !m.menu);
    wm_key(&m,125,1);for(unsigned i=0;i<9;i++)wm_key(&m,108,1);wm_key(&m,28,1);
    assert(m.confirm==1);wm_key(&m,1,1);assert(!m.confirm && !m.power);
    wm_key(&m,125,1);for(unsigned i=0;i<10;i++)wm_key(&m,108,1);wm_key(&m,28,1);
    assert(m.confirm==2);wm_key(&m,21,1);assert(m.power==2 && !m.confirm);
    struct wm clients;wm_init(&clients,1024,768);int c=wm_create(&clients,40,"client",400,300);
    struct wm_window *cw=wm_owned(&clients,40,(unsigned)c);
    wm_mouse(&clients,80,100,-1);assert(wm_event(&clients,40,(unsigned)c,&event,true) && event.type==EVENT_RESIZE);
    for(int i=0;i<100;i++)wm_mouse(&clients,80+i,100,-1);
    assert(cw->count==1 && wm_event(&clients,40,(unsigned)c,&event,true) && event.type==EVENT_POINTER);
    wm_mouse(&clients,80,100,1);for(unsigned i=0;i<WM_EVENTS*3;i++)wm_key(&clients,30,1);
    wm_mouse(&clients,900,600,0);bool released=false;while(wm_event(&clients,40,(unsigned)c,&event,true))if(event.type==EVENT_RELEASE)released=true;
    assert(released && clients.client_capture<0);
    struct {uint32_t before,pixels[100],after;} buffer={.before=0x1234,.after=0x5678};
    struct surface surface={.pixels=buffer.pixels,.width=10,.height=10,.stride=10};
    gfx_clip(&surface,-5,-5,20,20);gfx_rect(&surface,INT_MIN,INT_MIN,INT_MAX,INT_MAX,1);
    gfx_rect(&surface,8,8,INT_MAX,INT_MAX,2);gfx_rect(&surface,-5,-5,8,8,3);
    assert(buffer.before==0x1234 && buffer.after==0x5678);
    assert(buffer.pixels[0]==3 && buffer.pixels[99]==2 && buffer.pixels[55]==0);
    gfx_clip(&surface,INT_MAX,INT_MAX,INT_MAX,INT_MAX);gfx_rect(&surface,0,0,10,10,9);
    assert(buffer.pixels[0]==3 && buffer.pixels[99]==2);
    puts("Desktop ownership, lifetime, focus, move/resize, controls, keys, queue saturation and clipping passed");
}
