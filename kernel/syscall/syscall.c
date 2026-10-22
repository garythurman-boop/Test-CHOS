#include "process.h"
#include "abi.h"
#include "timer.h"
#include "console.h"
struct exception_frame *syscall_dispatch(struct exception_frame *f) {
    struct process *p=process_current();
    if(!p || (f->spsr&15)!=0) panic("INVALID USER SYSCALL CONTEXT");
    int64_t result=-38;
    switch(f->x[8]) {
    case SYS_ABI: result=1; break;
    case SYS_PID: result=(int64_t)p->pid; break;
    case SYS_TICKS: result=(int64_t)timer_ticks(); break;
    case SYS_EXIT: return process_exit(f,(int64_t)f->x[0],0,0);
    case SYS_YIELD: f->x[0]=0; return scheduler_tick(f);
    case SYS_WRITE: {
        char text[65];
        if(f->x[1]>64) { result=-22; break; }
        if(!user_copy(p,f->x[0],text,(size_t)f->x[1],false)) { result=-14; break; }
        text[f->x[1]]=0;
        console_rect(32,164,768,16,0x071b36);
        console_text(32,164,text,0x8edbff); result=(int64_t)f->x[1]; break;
    }
    case SYS_ALLOC:
        result=f->x[0]>16 || !f->x[0]?-22:(int64_t)process_allocate(p,(unsigned)f->x[0]);
        if(!result) result=-12;
        break;
    case SYS_FREE: result=process_free(p,f->x[0])?0:-22; break;
    case SYS_SEND: {
        struct process *target=process_find(f->x[0]);
        if(!target || target->exited) { result=-3; break; }
        if(target->messages==8) { result=-11; break; }
        target->mailbox[(target->head+target->messages)%8]=(struct message){p->pid,f->x[1]};
        target->messages++; result=0; break;
    }
    case SYS_RECV:
        if(!p->messages) { result=-11; break; }
        if(!user_copy(p,f->x[0],&p->mailbox[p->head],sizeof(struct message),true)) { result=-14; break; }
        p->head=(p->head+1)%8; p->messages--; result=0; break;
    }
    f->x[0]=(uint64_t)result;
    return f;
}
