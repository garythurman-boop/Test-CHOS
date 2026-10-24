#include "desktop.h"
#include "graphics.h"
#include "royale.h"
#include "boot.h"
#include "process.h"
#include "timer.h"
#include "input.h"
#include "console.h"
#include "platform.h"
#include "setup.h"
static struct wm model;
static struct pmm *physical;
static struct chos_boot_info *boot;
static struct surface screen;
static bool active;
static bool setup_entry,preferences_failed;
static void rect(int,int,int,int,uint32_t);
static uint32_t *wallpaper;
static unsigned wallpaper_theme=99;
static int damage_x,damage_y,damage_w,damage_h;
static void clip(int x,int y,int w,int h) {
    int right=x+w,bottom=y+h;
    if(x<damage_x)x=damage_x;if(y<damage_y)y=damage_y;
    if(right>damage_x+damage_w)right=damage_x+damage_w;
    if(bottom>damage_y+damage_h)bottom=damage_y+damage_h;
    gfx_clip(&screen,x,y,right>x?right-x:0,bottom>y?bottom-y:0);
}
static void ui(unsigned font,int x,int y,const char *s,uint32_t c) {gfx_ui_text(&screen,font,x,y,s,c);}
static void round_box(int x,int y,int w,int h,int radius,uint32_t top,uint32_t bottom) {gfx_round(&screen,x,y,w,h,radius,top,bottom,255);}
static bool hit(int x,int y,int w,int h) {return model.mouse_x>=x && model.mouse_y>=y && model.mouse_x<x+w && model.mouse_y<y+h;}
static void make_wallpaper(void) {
    /* Original analytical artwork: a luminous sky with flowing cyan ribbons.
     * Cached once per palette, no image codec or per-frame scene allocation. */
    for(int y=0;y<screen.height;y++)for(int x=0;x<screen.width;x++) {
        int u=x*1024/screen.width,v=y*768/screen.height;
        uint32_t c=gfx_blend(model.theme?0x123b52:0x102c67,model.theme?0x278e91:0x2377b4,(unsigned)(v*220/768));
        int light=255-(u*u/5500+(v-110)*(v-110)/1800);
        if(light>0)c=gfx_blend(c,0xb8edff,(unsigned)light*2/3);
        int curve=470-u/3+(u-500)*(u-500)/2400;
        int d=v-curve;if(d<0)d=-d;
        if(d<76)c=gfx_blend(c,model.theme?0x67d8c7:0x67c9ef,(unsigned)(76-d)*100/76);
        int second=v-(650-u/5+(u-500)*(u-500)/3600);if(second<0)second=-second;
        if(second<20)c=gfx_blend(c,0x8cddfa,(unsigned)(20-second)*25/20);
        wallpaper[(size_t)y*screen.width+x]=c;
    }
    wallpaper_theme=model.theme;
}
/* Small original vector icons share a 32-pixel canvas and palette. */
static void icon(int x,int y,unsigned app) {
    if(app==1) {
        round_box(x+2,y+5,17,10,3,0xffdf89,0xd39635);
        round_box(x,y+10,32,21,3,0xffe9ac,0xe5ae46);rect(x+3,y+12,26,1,0xfff5d3);
    } else if(app==2) {
        round_box(x+4,y+1,24,30,3,0xffffff,0xd4e8f5);
        for(int i=0;i<4;i++)rect(x+9,y+9+i*5,14,1,0x6d9ebc);
        round_box(x+23,y+8,4,20,1,0xf4bd64,0xc57d2b);
    } else if(app==3) {
        round_box(x,y+3,32,25,4,0x9dd6f0,0x4c88bd);round_box(x+2,y+5,28,21,2,0x112f4c,0x071d34);
        ui(FONT_BOLD,x+5,y+6,">",0xb0f1de);rect(x+15,y+20,8,2,0xb0f1de);
    } else if(app==4 || app==5) {
        round_box(x+1,y+3,30,25,4,0xe7f5fd,0x77b4d9);
        for(int i=0;i<3;i++){rect(x+6+i*8,y+8,2,15,0x386b97);round_box(x+4+i*8,y+10+(i%2)*7,6,4,1,0xffffff,0x649fc8);}
    } else if(app==6) {
        round_box(x+1,y+1,30,30,15,0xc6f4ff,0x328bc9);ui(FONT_TITLE,x+13,y+3,"i",0xffffff);
    } else {
        round_box(x+1,y+3,30,25,4,0xbfe6f7,0x498bc0);round_box(x+10,y+10,12,12,6,0xffffff,0xd3ecfa);rect(x+15,y+7,2,18,0xffffff);rect(x+7,y+15,18,2,0xffffff);
    }
}
struct commands { struct chos_draw *front,*back; uint64_t memory; unsigned count,pending,phase; };
static struct commands drawings[WM_WINDOWS];
static unsigned displayed_phase;
unsigned desktop_setup_displayed(void){return displayed_phase;}
void desktop_setup_notify(uint64_t pid){
    uint64_t mask;__asm__ volatile("mrs %0, daif; msr daifset, #2":"=r"(mask)::"memory");
    wm_refresh(&model,pid);scheduler_wake(pid);
    __asm__ volatile("msr daif, %0"::"r"(mask):"memory");
}
static uint64_t last_second;
static struct chos_render metrics;
static void text(int x,int y,const char *s,uint32_t c) { gfx_text(&screen,x,y,s,c); }
static void rect(int x,int y,int w,int h,uint32_t c) { gfx_rect(&screen,x,y,w,h,c); }
static void decimal(char *s,uint64_t n) {
    char temp[21];unsigned i=0,j=0;do { temp[i++]=(char)('0'+n%10);n/=10; } while(n);
    while(i) s[j++]=temp[--i];s[j]=0;
}
bool desktop_active(void) { return active; }
int desktop_launch(unsigned app) {
    if(!active || app<1 || app>7) return -22;
    if(wm_exclusive(&model))return -16;
    extern const unsigned char desktop_elf[],desktop_elf_end[];
    int pid=process_create(desktop_elf,(size_t)(desktop_elf_end-desktop_elf),app,0);
    if(pid>0)process_find((uint64_t)pid)->application=app;
    return pid;
}
void desktop_process_exit(uint64_t pid) {
    if(!active) return;
    for(unsigned i=0;i<WM_WINDOWS;i++) if(model.windows[i].owner==pid) {
        if(drawings[i].memory && !pmm_free(physical,drawings[i].memory,8)) panic("WINDOW FREE FAILED");
        drawings[i]=(struct commands){0};
    }
    wm_remove(&model,pid);
}
int64_t desktop_syscall(struct process *p,struct exception_frame *f) {
    if(!active) return -19;
    uint64_t a=f->x[0],b=f->x[1];
    unsigned op=(unsigned)f->x[8];
    if(op==SYS_WINDOW) {
        struct chos_window spec;
        if(!user_copy(p,a,&spec,sizeof(spec),false)) return -14;
        spec.title[47]=0;
        uint64_t memory=pmm_alloc(physical,8);
        if(!memory) return -12;
        int id=wm_create(&model,p->pid,spec.title,spec.width,spec.height);
        if(id<0) { (void)pmm_free(physical,memory,8);return id; }
        struct wm_window *w=wm_owned(&model,p->pid,(uint32_t)id);
        if(p->application==7 && !wm_fullscreen(&model,p->pid,(uint32_t)id)){
            wm_remove(&model,p->pid);(void)pmm_free(physical,memory,8);return -16;
        }
        struct commands *c=&drawings[w-model.windows];
        *c=(struct commands){.front=(void *)(uintptr_t)memory,.back=(void *)(uintptr_t)(memory+4*PAGE_SIZE),.memory=memory};
        return id;
    }
    if(op==SYS_INFO) {
        struct chos_info info={.ticks=timer_ticks(),.total_pages=physical->total,.used_pages=physical->used,.switches=scheduler_switches(),.width=(uint32_t)screen.width,.height=(uint32_t)screen.height,.theme=model.theme};
        for(unsigned i=0;i<PROCESS_MAX;i++) { struct process *q=process_at(i);if(q && !q->exited) info.processes++; }
        info.persistent=setup_persistent();info.reserved=setup_volume_state();
        const char *name=setup_computer();for(unsigned i=0;i<31 && name[i];i++)info.computer[i]=name[i];
        return user_copy(p,a,&info,sizeof(info),true)?0:-14;
    }
    if(op==SYS_DISPLAY){if(a==3){if(p->application!=4)return -13;bool requested=model.display_requested;model.display_requested=false;return requested?1:0;}return setup_display(p,a,b);}
    if(op==SYS_CONFIG){int64_t r=setup_configure(p,a,b,f->x[2]);if(!r && a==1){model.theme=setup_configuration()->theme;model.dirty=true;}return r;}
    if(op==SYS_RENDER)return user_copy(p,a,&metrics,sizeof(metrics),true)?0:-14;
    if(op==SYS_POWER)return a>2?-22:platform_power(boot,(unsigned)a);
    if(op==SYS_TASK) {
        if(a>=PROCESS_MAX)return -22;
        struct process *q=process_at((unsigned)a);if(!q || q->exited)return -2;
        struct chos_task task={.pid=q->pid,.ticks=scheduler_samples(q->pid),.application=q->application,.state=1};
        return user_copy(p,b,&task,sizeof(task),true)?0:-14;
    }
    if(op==SYS_LAUNCH) return a>7?-22:desktop_launch((unsigned)a);
    if(op==SYS_THEME) { if(a>1) return -22;model.theme=(unsigned)a;model.dirty=true;return 0; }
    if(a>0x7fffffff) return -22;
    struct wm_window *w=wm_owned(&model,p->pid,(uint32_t)a);
    if(!w) return -9;
    struct commands *c=&drawings[w-model.windows];
    if(op==SYS_EVENT) {
        struct chos_event e;
        if(!user_validate(p,b,sizeof(e),true)) return -14;
        if(!wm_event(&model,p->pid,w->id,&e,false)) return -11;
        if(!user_copy(p,b,&e,sizeof(e),true)) return -14;
        (void)wm_event(&model,p->pid,w->id,&e,true); return 0;
    }
    if(op==SYS_PRESENT) {
        if(p->application==7 && b>SETUP_COMPLETE)return -22;
        struct chos_draw *swap=c->front;c->front=c->back;c->back=swap;
        c->count=c->pending;c->pending=0;if(p->application==7)c->phase=(unsigned)b;model.dirty=true;return 0;
    }
    if(op==SYS_DRAW) {
        struct chos_draw command;
        if(!user_copy(p,b,&command,sizeof(command),false)) return -14;
        if(command.op>DRAW_UI_TEXT || command.x < -4096 || command.x>4096 || command.y < -4096 || command.y>4096 || command.w<0 || command.w>4096 || command.h<0 || command.h>4096) return -22;
        if(command.op==DRAW_UI_TEXT && command.w>FONT_MONO)return -22;
        command.text[79]=0;
        if(command.op==DRAW_CLEAR) c->pending=0;
        if(c->pending==WM_COMMANDS) return -12;
        c->back[c->pending++]=command;return 0;
    }
    return -38;
}
static void window_draw(unsigned slot) {
    struct wm_window *w=&model.windows[slot];
    if(w->minimized || w->closing) return;
    int cx=w->fullscreen?0:w->x+4,cy=w->fullscreen?0:w->y+32;
    int cw=w->fullscreen?w->w:w->w-8,ch=w->fullscreen?w->h:w->h-36;
    if(w->fullscreen)goto contents;
    bool focus=model.focus==w->id;
    gfx_shadow(&screen,w->x,w->y,w->w,w->h,8);
    round_box(w->x,w->y,w->w,w->h,8,focus?0x5c9bdf:0x85a5c9,ROYALE_BORDER);
    round_box(w->x+1,w->y+1,w->w-2,31,7,focus?0x6ea6eb:0x9bb5d3,focus?0x174b9b:0x587ba6);
    gfx_gradient(&screen,w->x+4,w->y+15,w->w-8,15,focus?0x286bc2:0x7498be,focus?0x17468f:0x486c94);
    clip(w->x+8,w->y+4,w->w-96,22);
    ui(FONT_BOLD,w->x+12,w->y+8,w->title,0x102c60);ui(FONT_BOLD,w->x+12,w->y+7,w->title,0xffffff);
    clip(0,0,screen.width,screen.height);
    for(int b=0;b<3;b++) {
        int x=w->x+w->w-78+b*26,y=w->y+5;bool hover=hit(x,y,22,21);
        round_box(x,y,22,21,4,b==2?0xd77b74:0x92c4f2,b==2?0xa43237:0x2e6eb8);
        round_box(x+1,y+1,20,9,3,(model.shell_press==400+slot*3+(unsigned)b && hover)?0x6b94c5:(hover?0xe4f5ff:(b==2?0xf2b2a2:0xb0d8fa)),b==2?0xd46b60:0x6ba8e3);
        if(b==0)rect(x+6,y+14,10,2,0xffffff);
        if(b==1){
            if(w->maximized){rect(x+8,y+5,9,9,0xffffff);rect(x+9,y+8,7,5,0x437fbd);rect(x+5,y+8,9,9,0xffffff);rect(x+6,y+11,7,5,0x437fbd);}
            else {rect(x+6,y+6,10,9,0xffffff);rect(x+7,y+9,8,5,0x437fbd);}
        }
        if(b==2)for(int k=0;k<9;k++){rect(x+7+k,y+6+k,2,2,0xffffff);rect(x+15-k,y+6+k,2,2,0xffffff);}
    }
contents:
    clip(cx,cy,cw,ch);
    rect(cx,cy,cw,ch,ROYALE_PAPER);
    struct commands *c=&drawings[slot];
    for(unsigned i=0;i<c->count;i++) {
        struct chos_draw *d=&c->front[i];
        if(d->op==DRAW_CLEAR) rect(cx,cy,cw,ch,d->color);
        else if(d->op==DRAW_RECT) rect(cx+d->x,cy+d->y,d->w,d->h,d->color);
        else if(d->op==DRAW_ROUND)round_box(cx+d->x,cy+d->y,d->w,d->h,5,d->color,d->color);
        else if(d->op==DRAW_UI_TEXT)ui((unsigned)d->w,cx+d->x,cy+d->y,d->text,d->color);
        else text(cx+d->x,cy+d->y,d->text,d->color);
    }
    clip(0,0,screen.width,screen.height);
    if(!w->fullscreen)for(int i=0;i<3;i++) rect(w->x+w->w-5-i*4,w->y+w->h-3,2,2,0xffffff);
}
static void render(void) {
    uint64_t started;__asm__ volatile("mrs %0, cntpct_el0":"=r"(started));
    clip(0,0,screen.width,screen.height);
    struct wm_window *exclusive=wm_exclusive(&model);
    if(exclusive){window_draw((unsigned)(exclusive-model.windows));goto output;}
    if(setup_entry){rect(0,0,screen.width,screen.height,0x102a83);goto output;}
    if(!wallpaper){
        unsigned pages=(unsigned)(((uint64_t)screen.width*screen.height*4+4095)/4096);
        uint64_t art=pmm_alloc(physical,pages);if(!art)panic("WALLPAPER CACHE FAILED");wallpaper=(void *)(uintptr_t)art;
    }
    if(wallpaper_theme!=model.theme)make_wallpaper();
    for(int y=screen.y;y<screen.y+screen.h;y++)for(int x=screen.x;x<screen.x+screen.w;x++)screen.pixels[(size_t)y*screen.width+x]=wallpaper[(size_t)y*screen.width+x];
    if(!setup_persistent()) {
        ui(FONT_TITLE,screen.width-230,34,"CHOS Royale",0xc4e8fa);
        ui(FONT_SMALL,screen.width-230,62,"Live session - RAM volume",0xb0d7ef);
    }
    static const char names[7][16]={"Explorer","Notepad","Terminal","Control Panel","Task Manager","About CHOS","CHOS Setup"};
    for(unsigned i=0;i<7;i++) {
        int y=model.icon_y[i]+4;
        if(model.desktop_selection==i+1)gfx_round(&screen,model.icon_x[i],y-4,100,58,6,0x91c8f0,0x3375b8,170);
        icon(model.icon_x[i]+29,y,i+1);
        int x=model.icon_x[i]+45-gfx_ui_width(FONT_SMALL,names[i])/2;
        ui(FONT_SMALL,x+1,y+37,names[i],0x153759);ui(FONT_SMALL,x,y+36,names[i],0xf3fbff);
    }
    if(setup_volume_state()>VOLUME_PERSISTENT) {
        round_box(128,screen.height-92,screen.width-300,34,5,0xffe8b3,0xf0c97d);
        ui(FONT_BODY,140,screen.height-84,"System volume unavailable. Temporary RAM files only.",0x613b0d);
    }
    for(unsigned i=0;i<model.count;i++) window_draw(model.order[i]);
    int task_y=screen.height-44;
    gfx_gradient(&screen,0,task_y,screen.width,44,0x71a7e5,0x133e86);
    rect(0,task_y+1,screen.width,1,0xb0d9fc);gfx_gradient(&screen,0,task_y+9,screen.width,35,0x2c6bc0,0x174897);
    round_box(6,task_y+4,92,36,12,0x99d9d8,0x126d84);
    round_box(8,task_y+5,88,17,10,model.shell_press==1?0x4b9eba:(model.menu?0xc7f3f4:(hit(6,task_y+4,92,36)?0xb3e7ed:0x8cd3e1)),0x2987a9);
    ui(FONT_TITLE,28,task_y+8,"CHOS",0xffffff);
    unsigned n=0;int button_width=wm_task_width(&model);
    for(unsigned i=0;i<WM_WINDOWS;i++) if(model.windows[i].owner) {
        struct wm_window *w=&model.windows[i]; int x=108+(int)n++*button_width;
        if(x>=screen.width-160)break;
        bool focused=w->id==model.focus;bool hovered=hit(x,task_y+7,button_width-6,29);
        round_box(x,task_y+7,button_width-6,29,5,model.shell_press==300+i?0x255c9e:(focused?0x173e82:(hovered?0x9ac4ed:0x78abe6)),focused?0x397bc7:0x306bbb);
        clip(x+6,task_y+7,button_width-18,29);ui(FONT_SMALL,x+7,task_y+14,w->title,0xffffff);
        clip(0,0,screen.width,screen.height);
    }
    round_box(screen.width-155,task_y+4,150,36,5,0x4c98d6,0x2266a8);
    char timer[21];decimal(timer,timer_ticks()/100);
    if(boot->clock_valid) {
        uint64_t now,freq;__asm__ volatile("mrs %0, cntpct_el0":"=r"(now));__asm__ volatile("mrs %0, cntfrq_el0":"=r"(freq));
        uint64_t seconds=config_clock((unsigned)((boot->clock_seconds+(now-boot->clock_counter)/freq)%86400),setup_configuration()->utc_offset);
        char clock[9]={(char)('0'+seconds/36000),(char)('0'+seconds/3600%10),':',(char)('0'+seconds/60%60/10),(char)('0'+seconds/60%10),0};
        if(setup_configuration()->clock_seconds){clock[5]=':';clock[6]=(char)('0'+seconds%60/10);clock[7]=(char)('0'+seconds%10);clock[8]=0;}
        ui(FONT_SMALL,screen.width-137,task_y+6,"System time",0xd9f1ff);ui(FONT_BOLD,screen.width-137,task_y+20,clock,0xffffff);
    } else { ui(FONT_SMALL,screen.width-137,task_y+6,"Uptime",0xd9f1ff);ui(FONT_BOLD,screen.width-137,task_y+20,timer,0xffffff); }
    rect(screen.width-14,task_y+14,6,6,0x8dd9b3);
    if(model.menu) {
        static const char items[12][18]={"All applications","Documents","Computer","Control Panel","Settings","Search","Run","Help","Log Off","Restart","Shut Down","x"};
        int top=task_y-400;
        gfx_shadow(&screen,8,top,420,400,9);
        round_box(8,top,420,400,9,0x8cb7e8,0x194c94);
        round_box(10,top+2,416,54,8,0xc0ddf8,0x3977bb);
        round_box(23,top+11,34,34,7,0x6cb3e8,0x205ba9);ui(FONT_TITLE,34,top+15,"C",0xffffff);
        ui(FONT_TITLE,68,top+10,"CHOS",0xffffff);
        ui(FONT_SMALL,69,top+34,setup_computer(),0xebf6ff);
        rect(10,top+56,207,300,0xf6f9fd);rect(217,top+56,209,300,0xddeafa);rect(217,top+56,1,300,0xabc6e7);
        unsigned count=model.applications?8:12;
        if(!model.applications)for(unsigned i=0;i<7;i++) {
            int y=top+64+(int)i*36;
            if(hit(18,y,194,34))round_box(18,y,194,34,4,model.shell_press==200+i?0x80b0dd:0xb6d6f0,0x69a0d5);
            icon(23,y,i+1);ui(FONT_BODY,64,y+8,names[i],ROYALE_INK);
        }
        for(unsigned i=0;i<count;i++) {
            int x,y,w,h;if(!wm_menu_item(&model,i,&x,&y,&w,&h))continue;
            bool selected=i==model.selection;
            if(selected)round_box(x,y,w,h,4,model.shell_press==100+i?0x3e79ba:0x6da8e5,0x326eb4);
            if(model.applications){if(i<7){icon(x+5,y,i+1);ui(FONT_BODY,x+46,y+8,names[i],selected?0xffffff:ROYALE_INK);}else ui(FONT_BOLD,x+10,y+8,"<  Back to Start",0xffffff);}
            else {ui(i==0?FONT_BOLD:FONT_BODY,x+10,y+8,items[i],selected?0xffffff:(i>=8?0xf1f7ff:ROYALE_INK));if(i==0)ui(FONT_BOLD,x+w-22,y+8,">",selected?0xffffff:ROYALE_INK);}
        }
    }
    if(model.confirm) {
        int x=(screen.width-540)/2,y=(screen.height-150)/2;
        rect(x,y,540,150,0xc9deee);rect(x,y,540,32,0x286caf);
        ui(FONT_BOLD,x+14,y+9,model.confirm==1?"Restart CHOS":(model.confirm==2?"Shut down CHOS":"End graphical session"),0xffffff);
        ui(FONT_BODY,x+14,y+52,"Unsaved edits will be lost.",0x183b64);
        ui(FONT_BODY,x+14,y+80,model.confirm==3?"Applications close. No account security.":"RAM files are lost on restart/shutdown.",0x183b64);
        ui(FONT_BODY,x+14,y+114,"Y confirms. N or Escape cancels.",0x183b64);
    }
    if(model.context){
        int x=model.context_x,y=model.context_y,h=(int)wm_context_count(&model)*28;
        gfx_shadow(&screen,x,y,220,h,5);round_box(x,y,220,h,5,0xf7fbff,0xdeebf8);
        static const char desktop_items[8][32]={"Arrange icons","Auto arrange","Refresh","Display properties","Personalize","Double-click speed","Cursor size","New folder (unavailable)"};
        static const char icon_items[5][32]={"Open","Properties","Rename (unavailable)","Delete (unavailable)","Cancel"};
        if(model.context==3){
            ui(FONT_BOLD,x+12,y+10,names[model.desktop_selection-1],ROYALE_INK);
            ui(FONT_SMALL,x+12,y+42,"Native ARM64 application",ROYALE_INK);
            ui(FONT_SMALL,x+12,y+68,"Embedded system shortcut",ROYALE_INK);
            ui(FONT_SMALL,x+12,y+94,"Enter or Escape closes",ROYALE_INK);
        } else for(unsigned i=0;i<wm_context_count(&model);i++){
            bool enabled=wm_context_enabled(&model,i);
            if(i==model.context_item && enabled)round_box(x+3,y+(int)i*28+2,214,24,3,0x90bde9,0x4584c4);
            ui(FONT_SMALL,x+10,y+(int)i*28+7,model.context==1?desktop_items[i]:icon_items[i],enabled?ROYALE_INK:0x7f8c9b);
            if(model.context==1 && i==1 && model.auto_arrange)ui(FONT_SMALL,x+198,y+(int)i*28+7,"*",ROYALE_INK);
            if(model.context==1 && (i==5 || i==6)){char n[21];decimal(n,i==5?model.double_click_ms:model.cursor_size);ui(FONT_SMALL,x+180,y+(int)i*28+7,n,ROYALE_INK);}
        }
    }
    if(preferences_failed)ui(FONT_SMALL,128,screen.height-112,"Desktop settings could not be saved. Retry after restart.",0xffe2b0);
output:;
    gfx_cursor(&screen,model.mouse_x,model.mouse_y,model.cursor_size);
    volatile uint32_t *fb=(void *)(uintptr_t)boot->framebuffer;
    for(int y=damage_y;y<damage_y+damage_h;y++) for(int x=damage_x;x<damage_x+damage_w;x++) {
        uint32_t rgb=screen.pixels[(size_t)y*screen.width+x];
        fb[(uint64_t)y*boot->stride+x]=boot->pixel_format==1?rgb:((rgb&255)<<16)|(rgb&0xff00)|((rgb>>16)&255);
    }
    __asm__ volatile("dsb sy":::"memory");model.dirty=false;
    if(exclusive)displayed_phase=drawings[exclusive-model.windows].phase;
    uint64_t now,freq;__asm__ volatile("mrs %0, cntpct_el0":"=r"(now));__asm__ volatile("mrs %0, cntfrq_el0":"=r"(freq));
    metrics.frames++;metrics.pixels+=(uint64_t)damage_w*damage_h;
    if(damage_w<screen.width || damage_h<screen.height)metrics.partial_frames++;
    metrics.last_us=(now-started)*1000000/freq;metrics.total_us+=metrics.last_us;
}
void desktop_start(struct pmm *p,struct chos_boot_info *b) {
    if(b->width>1920 || b->height>1200) panic("DESKTOP MODE TOO LARGE");
    physical=p;boot=b;
    unsigned pages=(unsigned)(((uint64_t)b->width*b->height*4+4095)/4096);
    uint64_t memory=pmm_alloc(p,pages);if(!memory) panic("DESKTOP BACKBUFFER FAILED");
    screen=(struct surface){.pixels=(void *)(uintptr_t)memory,.width=(int)b->width,.height=(int)b->height,.stride=(int)b->width};
    damage_x=damage_y=0;damage_w=screen.width;damage_h=screen.height;
    wm_init(&model,screen.width,screen.height);model.theme=setup_configuration()->theme;
    uint8_t preferences[96];if(setup_desktop_load(preferences))(void)wm_preferences_decode(&model,preferences,sizeof(preferences));
    model.mouse_x=input_get()->x;model.mouse_y=input_get()->y;
    struct input_event event;while(input_next(&event)) {} /* discard diagnostic input */
    extern bool ram_volume_start(struct pmm *);
    if(!setup_persistent() && !ram_volume_start(p))panic("RAM VOLUME START FAILED");
    active=true;
    setup_entry=b->boot_medium==2 || setup_pending();
    if(desktop_launch(setup_entry?7:6)<0) panic("INITIAL CLIENT LOAD FAILED");
    render();
}
void desktop_step(void) {
    setup_step();
    bool pointer_only=!model.dirty;
    int old_x=model.mouse_x,old_y=model.mouse_y;
    model.now_ms=timer_ticks()*10;
    input_poll();struct input_event event;
    while(input_next(&event)) {
        if(event.type!=2 || model.drag>=0 || model.icon_drag>=0 || model.context || model.menu || model.confirm)pointer_only=false;
        if(event.type==2) {
            int64_t x=model.mouse_x,y=model.mouse_y;
            if(event.code==0) x+=event.value;
            if(event.code==1) y+=event.value;
            if(x<0)x=0;if(y<0)y=0;if(x>=screen.width)x=screen.width-1;if(y>=screen.height)y=screen.height-1;
            wm_mouse(&model,(int)x,(int)y,-1);
        } else if(event.type==1 && event.code==272) wm_mouse(&model,model.mouse_x,model.mouse_y,event.value?1:0);
        else if(event.type==1 && event.code==273 && event.value)wm_context(&model,model.mouse_x,model.mouse_y);
        else if(event.type==1 && event.code<256) wm_key(&model,event.code,event.value);
    }
    if(model.preferences_dirty){
        model.preferences_dirty=false;
        if(setup_persistent()){
            uint8_t preferences[96];wm_preferences_encode(&model,preferences);
            uint64_t mask;__asm__ volatile("mrs %0, daif; msr daifset, #2":"=r"(mask)::"memory");
            preferences_failed=!setup_desktop_save(preferences);
            __asm__ volatile("msr daif, %0"::"r"(mask):"memory");
        }
        model.dirty=true;pointer_only=false;
    }
    if(model.power){(void)platform_power(boot,model.power);model.power=0;model.dirty=true;}
    if(model.logout) {
        for(unsigned i=0;i<PROCESS_MAX;i++) {
            struct process *p=process_at(i);
            if(p && !p->exited && scheduler_cancel(p->pid)){desktop_process_exit(p->pid);p->exited=true;p->status=0;}
        }
        model.logout=false;model.launch=6;pointer_only=false;
    }
    for(unsigned i=0;i<PROCESS_MAX;i++) {
        struct process *p=process_at(i);if(!p)continue;
        if(p->exited) { pointer_only=false;if(!process_reap(p->pid)) panic("DESKTOP REAP FAILED");continue; }
        for(unsigned j=0;j<WM_WINDOWS;j++) if(model.windows[j].owner==p->pid && model.windows[j].count) scheduler_wake(p->pid);
    }
    if(setup_take_live()) {
        setup_entry=false;pointer_only=false;model.dirty=true;model.theme=setup_configuration()->theme;
        if(!model.count)model.launch=6;
    }
    if(model.launch) { pointer_only=false; (void)desktop_launch(model.launch);model.launch=0; }
    uint64_t second=timer_ticks()/100;
    if(second!=last_second) { last_second=second;model.dirty=true;pointer_only=false; }
    if(model.dirty) {
        /* Hover changes are part of chrome. Plain pointer motion repaints only
         * the union of old/new cursor bounds; app presents remain full damage. */
        for(unsigned i=0;i<model.count;i++) {
            struct wm_window *w=&model.windows[model.order[i]];
            if((old_y>=w->y && old_y<w->y+32) || (model.mouse_y>=w->y && model.mouse_y<w->y+32))pointer_only=false;
        }
        if(old_y>=screen.height-44 || model.mouse_y>=screen.height-44)pointer_only=false;
        damage_x=damage_y=0;damage_w=screen.width;damage_h=screen.height;
        if(pointer_only) {
            damage_x=old_x<model.mouse_x?old_x:model.mouse_x;damage_y=old_y<model.mouse_y?old_y:model.mouse_y;
            damage_w=(old_x>model.mouse_x?old_x:model.mouse_x)-damage_x+(int)model.cursor_size+4;
            damage_h=(old_y>model.mouse_y?old_y:model.mouse_y)-damage_y+(int)model.cursor_size+4;
            if(damage_x+damage_w>screen.width)damage_w=screen.width-damage_x;
            if(damage_y+damage_h>screen.height)damage_h=screen.height-damage_y;
        }
        render();
    }
}
