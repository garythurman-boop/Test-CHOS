#include "desktop.h"
#include "boot.h"
#include "process.h"
#include "timer.h"
#include "input.h"
#include "console.h"
static struct wm model;
static struct pmm *physical;
static struct chos_boot_info *boot;
static struct surface screen;
static bool active;
struct commands { struct chos_draw *front,*back; uint64_t memory; unsigned count,pending; };
static struct commands drawings[WM_WINDOWS];
static uint64_t last_second;
static void text(int x,int y,const char *s,uint32_t c) { gfx_text(&screen,x,y,s,c); }
static void rect(int x,int y,int w,int h,uint32_t c) { gfx_rect(&screen,x,y,w,h,c); }
static void decimal(char *s,uint64_t n) {
    char temp[21];unsigned i=0,j=0;do { temp[i++]=(char)('0'+n%10);n/=10; } while(n);
    while(i) s[j++]=temp[--i];s[j]=0;
}
bool desktop_active(void) { return active; }
int desktop_launch(unsigned app) {
    if(!active || app<1 || app>6) return -22;
    extern const unsigned char desktop_elf[],desktop_elf_end[];
    return process_create(desktop_elf,(size_t)(desktop_elf_end-desktop_elf),app,0);
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
        struct commands *c=&drawings[w-model.windows];
        *c=(struct commands){.front=(void *)(uintptr_t)memory,.back=(void *)(uintptr_t)(memory+4*PAGE_SIZE),.memory=memory};
        return id;
    }
    if(op==SYS_INFO) {
        struct chos_info info={.ticks=timer_ticks(),.total_pages=physical->total,.used_pages=physical->used,.switches=scheduler_switches(),.width=(uint32_t)screen.width,.height=(uint32_t)screen.height,.theme=model.theme};
        for(unsigned i=0;i<PROCESS_MAX;i++) { struct process *q=process_at(i);if(q && !q->exited) info.processes++; }
        return user_copy(p,a,&info,sizeof(info),true)?0:-14;
    }
    if(op==SYS_LAUNCH) return a>6?-22:desktop_launch((unsigned)a);
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
        struct chos_draw *swap=c->front;c->front=c->back;c->back=swap;
        c->count=c->pending;c->pending=0;model.dirty=true;return 0;
    }
    if(op==SYS_DRAW) {
        struct chos_draw command;
        if(!user_copy(p,b,&command,sizeof(command),false)) return -14;
        if(command.op>DRAW_TEXT || command.x < -4096 || command.x>4096 || command.y < -4096 || command.y>4096 || command.w<0 || command.w>4096 || command.h<0 || command.h>4096) return -22;
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
    bool focus=model.focus==w->id;
    rect(w->x+5,w->y+6,w->w,w->h,0x071323);
    rect(w->x,w->y,w->w,w->h,focus?0x428ce0:0x445d7d);
    for(int y=2;y<30;y++) rect(w->x+2,w->y+y,w->w-4,1,focus?(0x22549a+(uint32_t)(30-y)*0x010201):0x3a5270);
    gfx_clip(&screen,w->x+8,w->y+4,w->w-96,22);text(w->x+10,w->y+8,w->title,0xffffff);
    gfx_clip(&screen,0,0,screen.width,screen.height);
    for(int b=0;b<3;b++) { int x=w->x+w->w-78+b*26;rect(x,w->y+5,22,21,b==2?0xb74950:0x397fc5);text(x+5,w->y+8,b==0?"_":(b==1?"+":"x"),0xffffff); }
    gfx_clip(&screen,w->x+4,w->y+32,w->w-8,w->h-36);
    rect(w->x+4,w->y+32,w->w-8,w->h-36,0xe5eef8);
    struct commands *c=&drawings[slot];
    for(unsigned i=0;i<c->count;i++) {
        struct chos_draw *d=&c->front[i];
        if(d->op==DRAW_CLEAR) rect(w->x+4,w->y+32,w->w-8,w->h-36,d->color);
        else if(d->op==DRAW_RECT) rect(w->x+4+d->x,w->y+32+d->y,d->w,d->h,d->color);
        else text(w->x+4+d->x,w->y+32+d->y,d->text,d->color);
    }
    gfx_clip(&screen,0,0,screen.width,screen.height);
    for(int i=0;i<3;i++) rect(w->x+w->w-5-i*4,w->y+w->h-3,2,2,0xffffff);
}
static void render(void) {
    gfx_clip(&screen,0,0,screen.width,screen.height);
    uint32_t base=model.theme?0x102d31:0x102b52;
    for(int y=0;y<screen.height;y++) {
        unsigned shade=(unsigned)((screen.height-y)*30/screen.height);
        rect(0,y,screen.width,1,base+(shade<<16)+(shade<<8)+shade);
    }
    text(screen.width-248,34,"CHOS Desktop",0x80bee8);
    text(screen.width-248,56,"Original ARM64 system",0x80bee8);
    static const char names[6][16]={"Explorer","Notepad","Terminal","Control Panel","Task Manager","About CHOS"};
    for(unsigned i=0;i<6;i++) {
        int y=60+(int)i*66;rect(34,y,28,28,0x7bb6df);rect(38,y+4,20,20,0x245a9a);
        text(10,y+36,names[i],0xe3f2ff);
    }
    for(unsigned i=0;i<model.count;i++) window_draw(model.order[i]);
    int task_y=screen.height-44;
    for(int y=0;y<44;y++) rect(0,task_y+y,screen.width,1,0x163965+(uint32_t)(44-y)*0x010202);
    rect(7,task_y+5,91,34,0x5b9dcc);rect(10,task_y+7,85,29,0x23649b);text(28,task_y+14,"CHOS",0xffffff);
    unsigned n=0;
    for(unsigned i=0;i<WM_WINDOWS;i++) if(model.windows[i].owner) {
        struct wm_window *w=&model.windows[i]; int x=108+(int)n++*110;
        if(x>screen.width-200) break;
        rect(x,task_y+7,104,29,w->id==model.focus?0x6996c0:0x2c5685);
        gfx_clip(&screen,x+3,task_y+7,98,29);text(x+5,task_y+15,w->title,0xffffff);
        gfx_clip(&screen,0,0,screen.width,screen.height);
    }
    char timer[21];decimal(timer,timer_ticks()/100);
    if(boot->clock_valid) {
        uint64_t now,freq;__asm__ volatile("mrs %0, cntpct_el0":"=r"(now));__asm__ volatile("mrs %0, cntfrq_el0":"=r"(freq));
        uint64_t seconds=(boot->clock_seconds+(now-boot->clock_counter)/freq)%86400;
        char clock[6]={(char)('0'+seconds/36000),(char)('0'+seconds/3600%10),':',(char)('0'+seconds/60%60/10),(char)('0'+seconds/60%10),0};
        text(screen.width-142,task_y+8,"Clock",0xcce4ff);text(screen.width-142,task_y+25,clock,0xffffff);
    } else { text(screen.width-142,task_y+8,"Uptime",0xcce4ff);text(screen.width-142,task_y+25,timer,0xffffff); }
    rect(screen.width-14,task_y+14,6,6,0x8dd9b3);
    if(model.menu) {
        static const char items[12][18]={"Applications","Documents","Computer","Control Panel","Settings","Search","Run","Help","Log Off [later]","Restart [later]","Shut Down [later]","Close menu"};
        int top=task_y-340;rect(8,top,260,340,0xe2eefa);rect(8,top,260,34,0x286caf);text(24,top+9,"CHOS",0xffffff);
        for(unsigned i=0;i<12;i++) text(22,top+44+(int)i*25,items[i],0x183b64);
    }
    static const uint8_t arrow[10]={0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xe0,0xb0,0x18,0x18};
    for(unsigned y=0;y<10;y++) for(unsigned x=0;x<8;x++) if(arrow[y]&(0x80>>x)) rect(model.mouse_x+(int)x,model.mouse_y+(int)y,1,1,0xffffff);
    volatile uint32_t *fb=(void *)(uintptr_t)boot->framebuffer;
    for(int y=0;y<screen.height;y++) for(int x=0;x<screen.width;x++) {
        uint32_t rgb=screen.pixels[(size_t)y*screen.width+x];
        fb[(uint64_t)y*boot->stride+x]=boot->pixel_format==1?rgb:((rgb&255)<<16)|(rgb&0xff00)|((rgb>>16)&255);
    }
    __asm__ volatile("dsb sy":::"memory");model.dirty=false;
}
void desktop_start(struct pmm *p,struct chos_boot_info *b) {
    if(b->width>1920 || b->height>1200) panic("DESKTOP MODE TOO LARGE");
    physical=p;boot=b;
    unsigned pages=(unsigned)(((uint64_t)b->width*b->height*4+4095)/4096);
    uint64_t memory=pmm_alloc(p,pages);if(!memory) panic("DESKTOP BACKBUFFER FAILED");
    screen=(struct surface){.pixels=(void *)(uintptr_t)memory,.width=(int)b->width,.height=(int)b->height,.stride=(int)b->width};
    wm_init(&model,screen.width,screen.height);
    model.mouse_x=input_get()->x;model.mouse_y=input_get()->y;
    struct input_event event;while(input_next(&event)) {} /* discard diagnostic input */
    active=true;
    if(desktop_launch(6)<0) panic("DESKTOP CLIENT LOAD FAILED");
    render();
}
void desktop_step(void) {
    input_poll();struct input_event event;
    while(input_next(&event)) {
        if(event.type==2) {
            int64_t x=model.mouse_x,y=model.mouse_y;
            if(event.code==0) x+=event.value;
            if(event.code==1) y+=event.value;
            if(x<0)x=0;if(y<0)y=0;if(x>=screen.width)x=screen.width-1;if(y>=screen.height)y=screen.height-1;
            wm_mouse(&model,(int)x,(int)y,-1);
        } else if(event.type==1 && event.code==272) wm_mouse(&model,model.mouse_x,model.mouse_y,event.value?1:0);
        else if(event.type==1 && event.code<256) wm_key(&model,event.code,event.value);
    }
    if(model.launch) { (void)desktop_launch(model.launch);model.launch=0; }
    for(unsigned i=0;i<PROCESS_MAX;i++) {
        struct process *p=process_at(i);if(!p)continue;
        if(p->exited) { if(!process_reap(p->pid)) panic("DESKTOP REAP FAILED");continue; }
        for(unsigned j=0;j<WM_WINDOWS;j++) if(model.windows[j].owner==p->pid && model.windows[j].count) scheduler_wake(p->pid);
    }
    uint64_t second=timer_ticks()/100;
    if(second!=last_second) { last_second=second;model.dirty=true; }
    if(model.dirty) render();
}
