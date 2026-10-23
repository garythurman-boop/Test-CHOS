#include "desktop.h"
static bool inside(int x,int y,int a,int b,int w,int h) {
    return x>=a && y>=b && (int64_t)x-a<w && (int64_t)y-b<h;
}
static struct wm_window *by_id(struct wm *m,uint32_t id) {
    for(unsigned i=0;i<WM_WINDOWS;i++) if(m->windows[i].owner && m->windows[i].id==id) return &m->windows[i];
    return 0;
}
static void post(struct wm_window *w,struct chos_event e) {
    /* Resize is coalesced, key/click overflow discards the newest event. Close
     * always survives a full queue so a well-behaved client can terminate. */
    if(e.type==EVENT_CLOSE) { w->head=0; w->count=0; }
    if(e.type==EVENT_RESIZE && w->count) {
        unsigned i=(w->head+w->count-1)%WM_EVENTS;
        if(w->events[i].type==EVENT_RESIZE) { w->events[i]=e; return; }
    }
    if(w->count<WM_EVENTS) { w->events[(w->head+w->count)%WM_EVENTS]=e; w->count++; }
}
void wm_init(struct wm *m,int w,int h) {
    *m=(struct wm){.width=w,.height=h,.next_id=1,.drag=-1,.dirty=true};
}
struct wm_window *wm_owned(struct wm *m,uint64_t owner,uint32_t id) {
    struct wm_window *w=by_id(m,id); return w && w->owner==owner?w:0;
}
void wm_focus(struct wm *m,uint32_t id) {
    struct wm_window *w=by_id(m,id); if(!w || w->closing) return;
    unsigned slot=(unsigned)(w-m->windows),pos=0;
    while(pos<m->count && m->order[pos]!=slot) pos++;
    if(pos==m->count) return;
    for(unsigned i=pos;i+1<m->count;i++) m->order[i]=m->order[i+1];
    m->order[m->count-1]=slot; w->minimized=false; m->focus=id; m->dirty=true;
}
static void next_focus(struct wm *m) {
    m->focus=0;
    for(unsigned i=m->count;i;i--) {
        struct wm_window *w=&m->windows[m->order[i-1]];
        if(!w->minimized && !w->closing) { m->focus=w->id; break; }
    }
    m->dirty=true;
}
int wm_create(struct wm *m,uint64_t owner,const char *title,int width,int height) {
    if(!owner || m->count==WM_WINDOWS || !m->next_id || m->next_id>0x7fffffff || width<260 || height<160) return -22;
    for(unsigned i=0;i<WM_WINDOWS;i++) if(m->windows[i].owner==owner) return -22;
    unsigned i=0; while(i<WM_WINDOWS && m->windows[i].owner) i++;
    struct wm_window *w=&m->windows[i];
    int limit_w=m->width-20,limit_h=m->height-64;
    if(width>limit_w) width=limit_w;
    if(height>limit_h) height=limit_h;
    *w=(struct wm_window){.owner=owner,.id=m->next_id++,.x=28+(int)m->count*24,.y=24+(int)m->count*22,.w=width,.h=height};
    if(w->x+w->w>m->width) w->x=m->width-w->w;
    if(w->y+w->h>m->height-44) w->y=m->height-44-w->h;
    for(unsigned j=0;j<47 && title[j];j++) w->title[j]=title[j];
    m->order[m->count++]=i; wm_focus(m,w->id);
    post(w,(struct chos_event){.type=EVENT_RESIZE,.width=w->w-8,.height=w->h-36});
    return (int)w->id;
}
void wm_remove(struct wm *m,uint64_t owner) {
    for(unsigned i=0;i<m->count;) {
        unsigned slot=m->order[i];
        if(m->windows[slot].owner!=owner) { i++; continue; }
        if(m->drag==(int)slot) m->drag=-1;
        m->windows[slot]=(struct wm_window){0};
        for(unsigned j=i;j+1<m->count;j++) m->order[j]=m->order[j+1];
        m->count--;
    }
    next_focus(m);
}
static void maximize(struct wm *m,struct wm_window *w) {
    if(w->maximized) { w->x=w->rx;w->y=w->ry;w->w=w->rw;w->h=w->rh; }
    else { w->rx=w->x;w->ry=w->y;w->rw=w->w;w->rh=w->h; w->x=0;w->y=0;w->w=m->width;w->h=m->height-44; }
    w->maximized=!w->maximized; m->dirty=true;
    post(w,(struct chos_event){.type=EVENT_RESIZE,.width=w->w-8,.height=w->h-36});
}
static void menu_activate(struct wm *m,unsigned item) {
    if(m->applications) {if(item<7)m->launch=item+1;m->applications=false;m->menu=false;return;}
    if(item==0){m->applications=true;m->selection=0;return;}
    static const unsigned apps[12]={0,1,1,4,4,1,3,6,0,0,0,0};
    if(item<12)m->launch=apps[item];
    if(item==8)m->confirm=3;
    if(item==9)m->confirm=1;
    if(item==10)m->confirm=2;
    m->menu=false;
}
int wm_task_width(const struct wm *m) {
    int width=m->count?(m->width-280)/(int)m->count:110;
    return width>110?110:width;
}
/* Shared menu geometry keeps keyboard selection, pointer hit testing and
 * rendering on the same layout. Main entries retain their documented order. */
bool wm_menu_item(const struct wm *m,unsigned item,int *x,int *y,int *w,int *h) {
    int top=m->height-44-400;*w=194;*h=32;
    if(m->applications) {if(item>=7)return false;*x=18;*y=top+64+(int)item*38;return true;}
    if(item==0){*x=18;*y=top+324;return true;}
    if(item>=1 && item<=7){*x=224;*y=top+64+(int)(item-1)*38;return true;}
    if(item>=8 && item<=10){*x=18+(int)(item-8)*134;*y=top+362;*w=126;return true;}
    if(item==11){*x=390;*y=top+12;*w=28;*h=26;return true;}
    return false;
}
void wm_mouse(struct wm *m,int x,int y,int button) {
    m->mouse_x=x; m->mouse_y=y; m->dirty=true;
    if(m->confirm)return;
    if(button==0) { m->drag=-1; return; }
    if(button<0) {
        if(m->menu) {
            unsigned count=m->applications?7:12;
            for(unsigned item=0;item<count;item++) {int a,b,w,h;if(wm_menu_item(m,item,&a,&b,&w,&h) && inside(x,y,a,b,w,h))m->selection=item;}
        }
        if(m->drag<0) return;
        struct wm_window *w=&m->windows[m->drag];
        if(m->resize) {
            int nw=x-w->x+m->dx,nh=y-w->y+m->dy;
            if(nw<260) nw=260;
            if(nh<160) nh=160;
            if(nw>m->width-w->x) nw=m->width-w->x;
            if(nh>m->height-44-w->y) nh=m->height-44-w->y;
            w->w=nw;w->h=nh;
            post(w,(struct chos_event){.type=EVENT_RESIZE,.width=nw-8,.height=nh-36});
        } else {
            int nx=x-m->dx,ny=y-m->dy;
            if(nx<0) nx=0;
            if(ny<0) ny=0;
            if(nx>m->width-w->w) nx=m->width-w->w;
            if(ny>m->height-44-w->h) ny=m->height-44-w->h;
            w->x=nx;w->y=ny;
        }
        return;
    }
    if(y>=m->height-44) {
        if(x<100) { m->menu=!m->menu;m->applications=false;m->selection=0;return; }
        if(x<108 || x>=m->width-160)return;
        unsigned n=(unsigned)(x-108)/(unsigned)wm_task_width(m);
        /* Taskbar order is stable by slot; stacking is independent. */
        for(unsigned i=0;i<WM_WINDOWS;i++) if(m->windows[i].owner) {
            if(!n--) { wm_focus(m,m->windows[i].id); break; }
        }
        return;
    }
    if(m->menu) {
        unsigned count=m->applications?7:12;
        for(unsigned item=0;item<count;item++) {
            int a,b,w,h;if(wm_menu_item(m,item,&a,&b,&w,&h) && inside(x,y,a,b,w,h)){menu_activate(m,item);return;}
        }
        int top=m->height-44-400;
        if(!m->applications && inside(x,y,18,top+64,194,7*36)) {m->launch=(unsigned)(y-top-64)/36+1;m->menu=false;return;}
        m->menu=false;return;
    }
    for(unsigned n=m->count;n;n--) {
        unsigned slot=m->order[n-1];struct wm_window *w=&m->windows[slot];
        if(w->minimized || w->closing || !inside(x,y,w->x,w->y,w->w,w->h)) continue;
        wm_focus(m,w->id);
        if(y<w->y+30) {
            if(x>=w->x+w->w-28) { post(w,(struct chos_event){.type=EVENT_CLOSE}); }
            else if(x>=w->x+w->w-54) maximize(m,w);
            else if(x>=w->x+w->w-80) { w->minimized=true;next_focus(m); }
            else if(!w->maximized) { m->drag=(int)slot;m->resize=false;m->dx=x-w->x;m->dy=y-w->y; }
        } else if(!w->maximized && x>=w->x+w->w-14 && y>=w->y+w->h-14) {
            m->drag=(int)slot;m->resize=true;m->dx=w->x+w->w-x;m->dy=w->y+w->h-y;
        } else post(w,(struct chos_event){.type=EVENT_CLICK,.x=x-w->x-4,.y=y-w->y-32});
        return;
    }
    if(x<112 && y>=60 && y<60+7*66) m->launch=(unsigned)(y-60)/66+1;
}
void wm_key(struct wm *m,unsigned code,int value) {
    if(code==56 || code==100) { m->alt=value!=0; return; }
    if(code==29 || code==97) { m->ctrl=value!=0; return; }
    if(code==42 || code==54) { m->shift=value!=0; return; }
    if(!value) return;
    if(m->confirm) {
        if(code==21){if(m->confirm==3)m->logout=true;else m->power=m->confirm;m->confirm=0;m->dirty=true;}
        if(code==49 || code==1){m->confirm=0;m->dirty=true;}
        return;
    }
    if(code==125) { m->menu=!m->menu;m->applications=false;m->selection=0;m->dirty=true;return; }
    if(m->menu && (code==103 || code==108 || code==28 || code==1)) {
        unsigned count=m->applications?7:12;
        if(code==103)m->selection=(m->selection+count-1)%count;
        if(code==108)m->selection=(m->selection+1)%count;
        if(code==28)menu_activate(m,m->selection);
        if(code==1){m->menu=false;m->applications=false;}
        m->dirty=true;return;
    }
    if(m->alt && code==15 && m->count) {
        for(unsigned i=0;i<m->count;i++) {
            struct wm_window *w=&m->windows[m->order[i]];
            if(!w->minimized && !w->closing) { wm_focus(m,w->id);break; }
        }
        return;
    }
    if(m->menu && code>=2 && code<=8) { m->launch=code-1;m->menu=false;m->dirty=true;return; }
    unsigned key=0;
    static const uint8_t codes[26]={30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};
    for(unsigned i=0;i<26;i++) if(code==codes[i]) key=(m->shift?'A':'a')+i;
    if(code>=2 && code<=11) { static const char normal[]="1234567890",shift[]="!@#$%^&*()"; key=(unsigned char)(m->shift?shift:normal)[code-2]; }
    switch(code) {
    case 57:key=' ';break;case 28:key=13;break;case 14:key=8;break;case 15:key=9;break;case 1:key=27;break;
    case 53:key=m->shift?'?':'/';break;case 52:key=m->shift?'>':'.';break;case 51:key=m->shift?'<':',';break;
    case 12:key=m->shift?'_':'-';break;case 13:key=m->shift?'+':'=';break;case 39:key=m->shift?':':';';break;
    case 26:key=m->shift?'{':'[';break;case 27:key=m->shift?'}':']';break;case 41:key=m->shift?'~':'`';break;
    case 40:key=m->shift?'"':'\'';break;case 43:key=m->shift?'|':'\\';break;
    case 103:key=KEY_UP;break;case 108:key=KEY_DOWN;break;case 105:key=KEY_LEFT;break;case 106:key=KEY_RIGHT;break;
    }
    struct wm_window *w=by_id(m,m->focus);
    if(w && !w->closing && key) post(w,(struct chos_event){.type=EVENT_KEY,.code=key|(m->ctrl?KEY_CTRL:0)});
}
bool wm_event(struct wm *m,uint64_t owner,uint32_t id,struct chos_event *e,bool consume) {
    struct wm_window *w=wm_owned(m,owner,id);
    if(!w || !w->count) return false;
    *e=w->events[w->head];
    if(consume) { w->head=(w->head+1)%WM_EVENTS;w->count--; }
    return true;
}
