#ifndef CHOS_ABI_H
#define CHOS_ABI_H
#include <stdint.h>
enum { SYS_ABI=0,SYS_WRITE=1,SYS_TICKS=2,SYS_EXIT=3,SYS_YIELD=4,SYS_ALLOC=5,SYS_FREE=6,SYS_SEND=7,SYS_RECV=8,SYS_PID=9,SYS_OPEN=10,SYS_FREAD=11,SYS_FWRITE=12,SYS_CLOSE=13,SYS_MKDIR=14,SYS_UNLINK=15,SYS_RENAME=16,SYS_SEEK=17,SYS_SIZE=18,SYS_LIST=19 };
struct chos_dirent { char path[60]; uint32_t size,kind; };
struct chos_message { uint64_t sender,value; };
#endif
