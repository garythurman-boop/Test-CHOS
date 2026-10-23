#ifndef CHOS_ABI_H
#define CHOS_ABI_H
#include <stdint.h>
enum { SYS_ABI=0,SYS_WRITE=1,SYS_TICKS=2,SYS_EXIT=3,SYS_YIELD=4,SYS_ALLOC=5,SYS_FREE=6,SYS_SEND=7,SYS_RECV=8,SYS_PID=9,SYS_OPEN=10,SYS_FREAD=11,SYS_FWRITE=12,SYS_CLOSE=13,SYS_MKDIR=14,SYS_UNLINK=15,SYS_RENAME=16,SYS_SEEK=17,SYS_SIZE=18,SYS_LIST=19,SYS_WINDOW=20,SYS_DRAW=21,SYS_PRESENT=22,SYS_EVENT=23,SYS_INFO=24,SYS_LAUNCH=25,SYS_THEME=26,SYS_REPLACE=27,SYS_POWER=28,SYS_TASK=29,SYS_SETUP=30 };
struct chos_dirent { char path[60]; uint32_t size,kind; };
struct chos_message { uint64_t sender,value; };
enum { DRAW_CLEAR=0,DRAW_RECT=1,DRAW_TEXT=2 };
enum { EVENT_KEY=1,EVENT_CLICK=2,EVENT_RESIZE=3,EVENT_CLOSE=4 };
enum { KEY_UP=256,KEY_DOWN=257,KEY_LEFT=258,KEY_RIGHT=259,KEY_CTRL=65536 };
struct chos_window { char title[48]; int32_t width,height; };
struct chos_draw { uint32_t op; int32_t x,y,w,h; uint32_t color; char text[80]; };
struct chos_event { uint32_t type,code; int32_t x,y,width,height; };
struct chos_task { uint64_t pid,ticks; uint32_t application,state; };
struct chos_info { uint64_t ticks,total_pages,used_pages,switches; uint32_t processes,width,height,theme,persistent,reserved; char computer[32]; };
struct chos_setup { uint64_t bytes,token; uint32_t sector,medium,allowed,phase,progress,error; char serial[24]; };
struct chos_confirm { char erase[48],computer[32]; };
#endif
