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
    if((e.type==EVENT_RESIZE || e.type==EVENT_POINTER) && w->count) {
        unsigned i=(w->head+w->count-1)%WM_EVENTS;
        if(w->events[i].type==e.type) { w->events[i]=e; return; }
    }
    if(e.type==EVENT_RELEASE && w->count==WM_EVENTS){w->head=(w->head+1)%WM_EVENTS;w->count--;}
    if(w->count<WM_EVENTS) { w->events[(w->head+w->count)%WM_EVENTS]=e; w->count++; }
}
struct wm_window *wm_exclusive(struct wm *m) {
    for(unsigned n=m->count;n;n--){struct wm_window *w=&m->windows[m->order[n-1]];if(w->fullscreen && !w->closing)return w;}
    return 0;
}
void wm_refresh(struct wm *m,uint64_t owner){
    for(unsigned i=0;i<m->count;i++){struct wm_window *w=&m->windows[m->order[i]];if(w->owner==owner)post(w,(struct chos_event){.type=EVENT_REFRESH});}
}
bool wm_fullscreen(struct wm *m,uint64_t owner,uint32_t id) {
    struct wm_window *w=wm_owned(m,owner,id);if(!w || wm_exclusive(m))return false;
    w->fullscreen=true;w->x=w->y=0;w->w=m->width;w->h=m->height;
    m->menu=m->applications=false;m->confirm=m->shell_press=0;m->drag=m->client_capture=m->client_hover=-1;
    wm_focus(m,id);post(w,(struct chos_event){.type=EVENT_RESIZE,.width=w->w,.height=w->h});return true;
}
void wm_init(struct wm *m,int w,int h) {
    *m=(struct wm){.width=w,.height=h,.next_id=1,.drag=-1,.client_capture=-1,.client_hover=-1,.icon_drag=-1,.double_click_ms=500,.cursor_size=28,.dirty=true};
    wm_arrange(m);m->preferences_dirty=false;
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
        if(m->client_capture==(int)slot)m->client_capture=-1;
        if(m->client_hover==(int)slot)m->client_hover=-1;
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
    if(m->applications) {if(item==7){m->applications=false;m->selection=0;return;}if(item<7)m->launch=item+1;m->applications=false;m->menu=false;return;}
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
    if(m->applications) {if(item==7){*x=18;*y=top+362;return true;}if(item>=7)return false;*x=18;*y=top+64+(int)item*38;return true;}
    if(item==0){*x=18;*y=top+324;return true;}
    if(item>=1 && item<=7){*x=224;*y=top+64+(int)(item-1)*38;return true;}
    if(item>=8 && item<=10){*x=18+(int)(item-8)*134;*y=top+362;*w=126;return true;}
    if(item==11){*x=390;*y=top+12;*w=28;*h=26;return true;}
    return false;
}
/* Shell controls activate on release in the same target. Native client
 * controls use the separate per-window pointer/release capture below. */
unsigned wm_shell_hit(const struct wm *m,int x,int y) {
    if(m->confirm)return 0;
    if(y>=m->height-44) {
        if(x<100)return 1;
        if(x<108 || x>=m->width-160)return 0;
        unsigned n=(unsigned)(x-108)/(unsigned)wm_task_width(m);
        for(unsigned i=0;i<WM_WINDOWS;i++)if(m->windows[i].owner && !n--)return 300+i;
        return 0;
    }
    if(m->menu) {
        unsigned count=m->applications?8:12;
        for(unsigned i=0;i<count;i++){int a,b,w,h;if(wm_menu_item(m,i,&a,&b,&w,&h) && inside(x,y,a,b,w,h))return 100+i;}
        int top=m->height-444;
        if(!m->applications && inside(x,y,18,top+64,194,7*36))return 200+(unsigned)(y-top-64)/36;
        return 0;
    }
    for(unsigned n=m->count;n;n--) {
        unsigned slot=m->order[n-1];const struct wm_window *w=&m->windows[slot];
        if(w->minimized || w->closing || !inside(x,y,w->x,w->y,w->w,w->h))continue;
        if(y<w->y+30 && x>=w->x+w->w-80){unsigned b=x>=w->x+w->w-28?2:(x>=w->x+w->w-54?1:0);return 400+slot*3+b;}
        return 0;
    }
    for(unsigned i=0;i<7;i++)if(inside(x,y,m->icon_x[i],m->icon_y[i],100,58))return 500+i;
    return 0;
}
static void shell_activate(struct wm *m,unsigned id) {
    if(id==1){m->menu=!m->menu;m->applications=false;m->selection=0;return;}
    if(id>=100 && id<112){menu_activate(m,id-100);return;}
    if(id>=200 && id<207){m->launch=id-199;m->menu=false;return;}
    if(id>=300 && id<300+WM_WINDOWS){struct wm_window *w=&m->windows[id-300];if(w->owner)wm_focus(m,w->id);return;}
    if(id>=400 && id<400+WM_WINDOWS*3) {
        struct wm_window *w=&m->windows[(id-400)/3];if(!w->owner)return;wm_focus(m,w->id);
        unsigned b=(id-400)%3;
        if(b==2)post(w,(struct chos_event){.type=EVENT_CLOSE});
        if(b==1)maximize(m,w);
        if(!b){w->minimized=true;next_focus(m);}return;
    }
    if(id>=500 && id<507){
        unsigned selected=id-499;
        int dx=m->mouse_x-m->last_click_x,dy=m->mouse_y-m->last_click_y;
        if(m->last_click_icon==selected && m->now_ms-m->last_click_ms<=m->double_click_ms && dx>=-4 && dx<=4 && dy>=-4 && dy<=4){m->launch=selected;m->last_click_icon=0;}
        else {m->last_click_icon=selected;m->last_click_ms=m->now_ms;m->last_click_x=m->mouse_x;m->last_click_y=m->mouse_y;}
    }
}
void wm_mouse(struct wm *m,int x,int y,int button) {
    m->mouse_x=x; m->mouse_y=y; m->dirty=true;
    if(button>=0)m->button_down=button!=0;
    if(m->context){
        bool in=inside(x,y,m->context_x,m->context_y,220,(int)wm_context_count(m)*28);
        if(button==1 && !in){m->context=0;return;}
        if(in)m->context_item=(unsigned)(y-m->context_y)/28;
        if(button==1 && in)m->shell_press=600+m->context_item;
        if(button==0){unsigned pressed=m->shell_press;m->shell_press=0;if(in && pressed==600+m->context_item)wm_key(m,28,1);}
        return;
    }
    struct wm_window *exclusive=wm_exclusive(m);
    if(exclusive){
        post(exclusive,(struct chos_event){.type=button<0?EVENT_POINTER:(button?EVENT_CLICK:EVENT_RELEASE),.x=x,.y=y});return;
    }
    if(button==0) {
        if(m->icon_drag>=0){
            if(m->icon_moving){m->preferences_dirty=true;m->shell_press=0;m->last_click_icon=0;}
            m->icon_drag=-1;m->icon_moving=false;
        }
        unsigned pressed=m->shell_press;m->shell_press=0;
        if(pressed && pressed==wm_shell_hit(m,x,y))shell_activate(m,pressed);
        if(m->client_capture>=0) {
            struct wm_window *w=&m->windows[m->client_capture];
            if(w->owner)post(w,(struct chos_event){.type=EVENT_RELEASE,.x=x-w->x-4,.y=y-w->y-32});
            m->client_capture=-1;
        }
        m->drag=-1;return;
    }
    if(m->confirm)return;
    if(button<0) {
        if(m->icon_drag>=0){
            int dx=x-m->press_x,dy=y-m->press_y;
            if(dx>4 || dx< -4 || dy>4 || dy< -4)m->icon_moving=true;
            if(m->icon_moving && !m->auto_arrange){
                int nx=x-m->dx,ny=y-m->dy;
                if(nx<0)nx=0;
                if(ny<0)ny=0;
                if(nx>m->width-100)nx=m->width-100;
                if(ny>m->height-102)ny=m->height-102;
                m->icon_x[m->icon_drag]=nx;m->icon_y[m->icon_drag]=ny;
            }
            return;
        }
        if(m->menu) {
            unsigned count=m->applications?8:12;
            for(unsigned item=0;item<count;item++) {int a,b,w,h;if(wm_menu_item(m,item,&a,&b,&w,&h) && inside(x,y,a,b,w,h))m->selection=item;}
        }
        if(m->drag<0) {
            int target=m->client_capture;
            if(target<0 && !m->menu)for(unsigned n=m->count;n;n--) {
                unsigned slot=m->order[n-1];struct wm_window *w=&m->windows[slot];
                if(!w->minimized && !w->closing && inside(x,y,w->x,w->y,w->w,w->h)) {
                    if(inside(x,y,w->x+4,w->y+32,w->w-8,w->h-36))target=(int)slot;
                    break;
                }
            }
            if(target!=m->client_hover && m->client_hover>=0)post(&m->windows[m->client_hover],(struct chos_event){.type=EVENT_POINTER,.x=-4096,.y=-4096});
            m->client_hover=target;
            if(target>=0){struct wm_window *w=&m->windows[target];post(w,(struct chos_event){.type=EVENT_POINTER,.x=x-w->x-4,.y=y-w->y-32});}
            return;
        }
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
    m->shell_press=wm_shell_hit(m,x,y);
    if(m->shell_press>=500 && m->shell_press<507){
        unsigned i=m->shell_press-500;m->desktop_selection=i+1;m->focus=0;
        m->icon_drag=(int)i;m->press_x=x;m->press_y=y;m->dx=x-m->icon_x[i];m->dy=y-m->icon_y[i];return;
    }
    if(m->shell_press)return;
    if(y>=m->height-44)return;
    if(m->menu){m->menu=false;return;}
    for(unsigned n=m->count;n;n--) {
        unsigned slot=m->order[n-1];struct wm_window *w=&m->windows[slot];
        if(w->minimized || w->closing || !inside(x,y,w->x,w->y,w->w,w->h)) continue;
        wm_focus(m,w->id);
        if(y<w->y+30) {
            if(!w->maximized) { m->drag=(int)slot;m->resize=false;m->dx=x-w->x;m->dy=y-w->y; }
        } else if(!w->maximized && x>=w->x+w->w-14 && y>=w->y+w->h-14) {
            m->drag=(int)slot;m->resize=true;m->dx=w->x+w->w-x;m->dy=w->y+w->h-y;
        } else {m->client_capture=(int)slot;post(w,(struct chos_event){.type=EVENT_CLICK,.x=x-w->x-4,.y=y-w->y-32});}
        return;
    }
    m->desktop_selection=0;m->last_click_icon=0;m->focus=0;
}
void wm_key(struct wm *m,unsigned code,int value) {
    if(code==56 || code==100) { m->alt=value!=0; return; }
    if(code==29 || code==97) { m->ctrl=value!=0; return; }
    if(code==42 || code==54) { m->shift=value!=0; return; }
    if(!value) return;
    struct wm_window *exclusive=wm_exclusive(m);
    if(exclusive)goto client_key;
    if(m->context){
        if(code==1){m->context=0;m->dirty=true;return;}
        unsigned count=wm_context_count(m);
        if(code==103)m->context_item=(m->context_item+count-1)%count;
        if(code==108)m->context_item=(m->context_item+1)%count;
        if(code==28 && wm_context_enabled(m,m->context_item)){
            unsigned item=m->context_item,context=m->context;m->context=0;
            if(context==2){if(!item)m->launch=m->desktop_selection;else if(item==1)m->context=3;}
            else if(context==1){
                if(item==0)wm_arrange(m);
                if(item==1){m->auto_arrange=!m->auto_arrange;if(m->auto_arrange)wm_arrange(m);m->preferences_dirty=true;}
                if(item==3 || item==4)m->launch=4;
                if(item==3)m->display_requested=true;
                if(item==5){m->double_click_ms=m->double_click_ms==750?250:m->double_click_ms+250;m->preferences_dirty=true;}
                if(item==6){m->cursor_size=m->cursor_size==28?36:28;m->preferences_dirty=true;}
            }
        }
        m->dirty=true;return;
    }
    if(!m->focus && m->desktop_selection && code==28){m->launch=m->desktop_selection;m->dirty=true;return;}
    if(m->confirm) {
        if(code==21){if(m->confirm==3)m->logout=true;else m->power=m->confirm;m->confirm=0;m->dirty=true;}
        if(code==49 || code==1){m->confirm=0;m->dirty=true;}
        return;
    }
    if(code==125) { m->menu=!m->menu;m->applications=false;m->selection=0;m->dirty=true;return; }
    if(m->menu && (code==103 || code==108 || code==105 || code==106 || code==28 || code==1)) {
        unsigned count=m->applications?8:12;
        if(code==103)m->selection=(m->selection+count-1)%count;
        if(code==108)m->selection=(m->selection+1)%count;
        if(code==106 && !m->applications && m->selection==0){m->applications=true;m->selection=0;}
        if(code==105 && m->applications){m->applications=false;m->selection=0;}
        if(code==28)menu_activate(m,m->selection);
        if(code==1){if(m->applications){m->applications=false;m->selection=0;}else m->menu=false;}
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
client_key:;
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
    case 61:key=KEY_F3;break;
    }
    struct wm_window *w=exclusive?exclusive:by_id(m,m->focus);
    if(w && !w->closing && key) post(w,(struct chos_event){.type=EVENT_KEY,.code=key|(m->ctrl?KEY_CTRL:0)|(m->alt?KEY_ALT:0)|((key==9 && m->shift)?KEY_SHIFT:0)});
}
bool wm_event(struct wm *m,uint64_t owner,uint32_t id,struct chos_event *e,bool consume) {
    struct wm_window *w=wm_owned(m,owner,id);
    if(!w || !w->count) return false;
    *e=w->events[w->head];
    if(consume) { w->head=(w->head+1)%WM_EVENTS;w->count--; }
    return true;
}

void wm_arrange(struct wm *m){
    unsigned rows=(unsigned)(m->height-110)/66;if(!rows)rows=1;
    for(unsigned i=0;i<7;i++){m->icon_x[i]=5+(int)(i/rows)*112;m->icon_y[i]=56+(int)(i%rows)*66;}
    m->preferences_dirty=true;m->dirty=true;
}
unsigned wm_context_count(const struct wm *m){return m->context==1?8:m->context==2?5:5;}
bool wm_context_enabled(const struct wm *m,unsigned item){return m->context==1?item!=7:m->context==2?(item<2 || item==4):true;}
void wm_context(struct wm *m,int x,int y){
    if(wm_exclusive(m) || m->confirm)return;
    unsigned hit=wm_shell_hit(m,x,y);
    if(hit && (hit<500 || hit>=507))return;
    for(unsigned n=0;n<m->count;n++){struct wm_window *w=&m->windows[m->order[n]];if(!w->minimized && !w->closing && inside(x,y,w->x,w->y,w->w,w->h)){
        if(inside(x,y,w->x+4,w->y+32,w->w-8,w->h-36)){wm_focus(m,w->id);post(w,(struct chos_event){.type=EVENT_CONTEXT,.x=x-w->x-4,.y=y-w->y-32});}
        return;
    }}
    m->context=hit>=500?2:1;if(hit>=500)m->desktop_selection=hit-499;
    m->menu=false;m->context_item=0;m->shell_press=0;m->icon_drag=-1;m->last_click_icon=0;
    int height=(int)wm_context_count(m)*28;
    m->context_x=x>m->width-224?m->width-224:x;
    m->context_y=y>m->height-48-height?m->height-48-height:y;
    m->dirty=true;
}
static void pref_put(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static uint32_t pref_get(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t pref_crc(const uint8_t *p){uint32_t c=~0u;for(unsigned i=0;i<92;i++){c^=p[i];for(unsigned b=0;b<8;b++)c=(c>>1)^((0u-(c&1))&0xedb88320u);}return ~c;}
bool wm_preferences_encode(const struct wm *m,uint8_t out[96]){
    for(unsigned i=0;i<96;i++)out[i]=0;
    pref_put(out,0x38445343);pref_put(out+4,1);pref_put(out+8,m->double_click_ms);pref_put(out+12,m->cursor_size);pref_put(out+16,m->auto_arrange);
    for(unsigned i=0;i<7;i++){pref_put(out+24+i*8,(unsigned)m->icon_x[i]);pref_put(out+28+i*8,(unsigned)m->icon_y[i]);}
    pref_put(out+92,pref_crc(out));return true;
}
bool wm_preferences_decode(struct wm *m,const uint8_t *p,size_t n){
    if(!p || n!=96 || pref_get(p)!=0x38445343 || pref_get(p+4)!=1 || pref_get(p+92)!=pref_crc(p) || pref_get(p+16)>1)return false;
    unsigned delay=pref_get(p+8),cursor=pref_get(p+12);
    if((delay!=250 && delay!=500 && delay!=750) || (cursor!=28 && cursor!=36))return false;
    for(unsigned i=0;i<7;i++)if(pref_get(p+24+i*8)>1920 || pref_get(p+28+i*8)>1200)return false;
    m->double_click_ms=delay;m->cursor_size=cursor;m->auto_arrange=pref_get(p+16)!=0;
    for(unsigned i=0;i<7;i++){
        m->icon_x[i]=(int)pref_get(p+24+i*8);m->icon_y[i]=(int)pref_get(p+28+i*8);
        if(m->icon_x[i]>m->width-100)m->icon_x[i]=m->width-100;
        if(m->icon_y[i]>m->height-102)m->icon_y[i]=m->height-102;
    }
    if(m->auto_arrange)wm_arrange(m);
    m->preferences_dirty=false;return true;
}
