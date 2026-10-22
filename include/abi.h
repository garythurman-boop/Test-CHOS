#ifndef CHOS_ABI_H
#define CHOS_ABI_H
#include <stdint.h>
enum { SYS_ABI=0,SYS_WRITE=1,SYS_TICKS=2,SYS_EXIT=3,SYS_YIELD=4,SYS_ALLOC=5,SYS_FREE=6,SYS_SEND=7,SYS_RECV=8,SYS_PID=9 };
struct chos_message { uint64_t sender,value; };
#endif
