#ifndef CHOS_ABI_H
#define CHOS_ABI_H
#include <stdint.h>
#include "hardware.h"
enum { SYS_ABI=0,SYS_WRITE=1,SYS_TICKS=2,SYS_EXIT=3,SYS_YIELD=4,SYS_ALLOC=5,SYS_FREE=6,SYS_SEND=7,SYS_RECV=8,SYS_PID=9,SYS_OPEN=10,SYS_FREAD=11,SYS_FWRITE=12,SYS_CLOSE=13,SYS_MKDIR=14,SYS_UNLINK=15,SYS_RENAME=16,SYS_SEEK=17,SYS_SIZE=18,SYS_LIST=19,SYS_WINDOW=20,SYS_DRAW=21,SYS_PRESENT=22,SYS_EVENT=23,SYS_INFO=24,SYS_LAUNCH=25,SYS_THEME=26,SYS_REPLACE=27,SYS_POWER=28,SYS_TASK=29,SYS_SETUP=30,SYS_RENDER=31,SYS_CONFIG=32,SYS_HARDWARE=33 };
struct chos_dirent { char path[60]; uint32_t size,kind; };
struct chos_message { uint64_t sender,value; };
enum { DRAW_CLEAR=0,DRAW_RECT=1,DRAW_TEXT=2,DRAW_ROUND=3,DRAW_UI_TEXT=4 };
enum { EVENT_KEY=1,EVENT_CLICK=2,EVENT_RESIZE=3,EVENT_CLOSE=4,EVENT_POINTER=5,EVENT_RELEASE=6,EVENT_REFRESH=7 };
enum { KEY_UP=256,KEY_DOWN=257,KEY_LEFT=258,KEY_RIGHT=259,KEY_CTRL=65536,KEY_SHIFT=131072,KEY_ALT=262144 };
#define KEY_F3 260
struct chos_window { char title[48]; int32_t width,height; };
struct chos_draw { uint32_t op; int32_t x,y,w,h; uint32_t color; char text[80]; };
struct chos_event { uint32_t type,code; int32_t x,y,width,height; };
struct chos_task { uint64_t pid,ticks; uint32_t application,state; };
struct chos_info { uint64_t ticks,total_pages,used_pages,switches; uint32_t processes,width,height,theme,persistent,reserved; char computer[32]; };
struct chos_config { uint32_t version,theme; int32_t utc_offset; uint32_t clock_seconds; char computer[32]; uint32_t reserved[4]; };
_Static_assert(sizeof(struct chos_config)==64,"configuration ABI");
struct chos_render { uint64_t frames,pixels,partial_frames,last_us,total_us; };
enum { VOLUME_LIVE=0,VOLUME_PERSISTENT=1,VOLUME_NO_DEVICE=2,VOLUME_BAD_GPT=3,VOLUME_IDENTITY=4,VOLUME_BAD_CHFS=5,VOLUME_BAD_CONFIG=6 };
/* chos_info.reserved reports VOLUME_* while preserving the 88-byte ABI. */
struct chos_setup { uint64_t bytes,token; uint32_t sector,medium,allowed,phase,progress,error; char serial[24]; };
struct chos_confirm { char erase[48],computer[32]; };
enum { SETUP_IDLE=0,SETUP_PREPARE=1,SETUP_FORMAT=2,SETUP_COPY=3,SETUP_BOOT=4,SETUP_CONFIGURE=5,SETUP_VERIFY=6,SETUP_COMPLETE=7 };
enum { SETUP_TARGET_OK=0,SETUP_TARGET_MISSING=1,SETUP_TARGET_MEDIUM=2,SETUP_TARGET_SMALL=3,SETUP_TARGET_READ=4,SETUP_TARGET_IDENTITY=5,SETUP_TARGET_READONLY=6 };
/* Additive SYS_SETUP inventory operation 3; original operation 0 stays 64 bytes. */
struct chos_partition { uint64_t first_byte,bytes; char name[36]; uint32_t reserved; };
struct chos_destination {
    uint64_t minimum_bytes,esp_bytes,system_bytes,completed_bytes,total_bytes;
    uint32_t reason,layout,partitions,reserved;
    struct chos_partition existing[4];
};
#endif
