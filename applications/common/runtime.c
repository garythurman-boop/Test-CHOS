#include "app.h"
unsigned app_role;
bool app_persistent;
const char *volume_status(void){return app_persistent?"System disk: persistent files":"RAM volume: files lost at shutdown";}
int app_window,app_width=592,app_height=384;
char app_status[80],prompt_value[120];
bool prompting;
static char prompt_title[80];
int64_t api(unsigned number,uint64_t a,uint64_t b,uint64_t c) {
    register uint64_t x0 __asm__("x0")=a,x1 __asm__("x1")=b,x2 __asm__("x2")=c,x8 __asm__("x8")=number;
    __asm__ volatile("svc #0":"+r"(x0):"r"(x1),"r"(x2),"r"(x8):"memory");return (int64_t)x0;
}
size_t length(const char *s) { size_t n=0;while(s[n])n++;return n; }
void copy(char *to,const char *from,size_t cap) { if(!cap)return;size_t i=0;while(i+1<cap && from[i]){to[i]=from[i];i++;}to[i]=0; }
bool equal(const char *a,const char *b) { while(*a && *a==*b){a++;b++;}return *a==*b; }
void number(char *s,uint64_t n) { char b[21];unsigned i=0,j=0;do{b[i++]=(char)('0'+n%10);n/=10;}while(n);while(i)s[j++]=b[--i];s[j]=0; }
static void draw(unsigned op,int x,int y,int w,int h,const char *s,uint32_t color) {
    struct chos_draw d={.op=op,.x=x,.y=y,.w=w,.h=h,.color=color};
    if(s)copy(d.text,s,sizeof(d.text));
    (void)api(SYS_DRAW,(uint64_t)app_window,(uintptr_t)&d,0);
}
void text(int x,int y,const char *s,uint32_t c) { draw(DRAW_TEXT,x,y,0,0,s,c); }
void rounded(int x,int y,int w,int h,uint32_t c) { draw(DRAW_ROUND,x,y,w,h,0,c); }
void ui_text(unsigned font,int x,int y,const char *s,uint32_t c) { draw(DRAW_UI_TEXT,x,y,(int)font,0,s,c); }
void box(int x,int y,int w,int h,uint32_t c) { draw(DRAW_RECT,x,y,w,h,0,c); }
void begin(uint32_t c) { draw(DRAW_CLEAR,0,0,0,0,0,c); }
void status(const char *s) { copy(app_status,s,sizeof(app_status)); }
void label(int y,const char *s) { ui_text(0,12,y,s,0x204b72); }
void prompt(const char *title,const char *initial) { copy(prompt_title,title,sizeof(prompt_title));copy(prompt_value,initial,sizeof(prompt_value));prompting=true; }
bool prompt_key(unsigned key) {
    if(key==('u'|KEY_CTRL)){prompt_value[0]=0;return false;}
    if(key==27) { prompting=false;status("Cancelled");return false; }
    if(key==13) { prompting=false;return true; }
    size_t n=length(prompt_value);
    if(key==8 && n)prompt_value[n-1]=0;
    else if(key>=32 && key<127 && n+1<sizeof(prompt_value)){prompt_value[n]=(char)key;prompt_value[n+1]=0;}
    return false;
}
void present(void) {
    if(prompting) {
        rounded(4,app_height-110,app_width-8,84,0xb9d5ee);
        ui_text(1,12,app_height-102,prompt_title,0x163b65);
        box(12,app_height-78,app_width-24,25,0xffffff);text(16,app_height-73,prompt_value,0x163b65);
        ui_text(3,12,app_height-46,"Enter accepts. Escape cancels.",0x163b65);
    }
    box(0,app_height-23,app_width,23,0xc1d7e9);ui_text(3,8,app_height-18,app_status,0x163b65);
    (void)api(SYS_PRESENT,(uint64_t)app_window,0,0);
}
int read_file(const char *path,char *buffer,unsigned cap) {
    int64_t fd=api(SYS_OPEN,(uintptr_t)path,1,0);if(fd<0)return (int)fd;
    int64_t n=api(SYS_SIZE,(uint64_t)fd,0,0),got=-22;
    if(n>=0 && (uint64_t)n<cap)got=api(SYS_FREAD,(uint64_t)fd,(uintptr_t)buffer,(uint64_t)n);
    (void)api(SYS_CLOSE,(uint64_t)fd,0,0);
    if(got>=0)buffer[got]=0;return (int)got;
}
int write_file(const char *path,const char *buffer,unsigned size) { return (int)api(SYS_REPLACE,(uintptr_t)path,(uintptr_t)buffer,size); }
