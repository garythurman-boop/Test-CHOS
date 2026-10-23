#include "scheduler.h"
#include "mmu.h"
#include "console.h"
#include "timer.h"
#define MAX_THREADS 12
struct thread { struct exception_frame *frame; uint64_t stack,root,pid; unsigned priority,state; uint64_t deadline,samples; };
static struct thread threads[MAX_THREADS];
static unsigned current;
static uint64_t switches;
static _Noreturn void thread_exit(void) {
    __asm__ volatile("svc #0x4348":::"memory");
    panic("TERMINATED THREAD RESUMED");
}
void scheduler_init(void) {
    for(unsigned i=0;i<MAX_THREADS;i++) threads[i]=(struct thread){0};
    current=0; switches=0;
    threads[0].priority=1; threads[0].state=1; threads[0].root=mmu_kernel_root();
}
int thread_create(struct pmm *p,void (*entry)(void *),void *argument,unsigned priority) {
    if(!entry || priority>31) return -1;
    unsigned id=1;
    while(id<MAX_THREADS && threads[id].state) id++;
    if(id==MAX_THREADS) return -1;
    uint64_t stack=pmm_alloc(p,3);
    if(!stack) return -1;
    if(!mmu_guard(stack)) { (void)pmm_free(p,stack,3); return -1; }
    struct exception_frame *f=(void *)(uintptr_t)(stack+3*PAGE_SIZE-sizeof(*f));
    for(unsigned i=0;i<sizeof(*f)/8;i++) ((uint64_t *)f)[i]=0;
    f->x[0]=(uintptr_t)argument; f->x[30]=(uintptr_t)thread_exit;
    f->elr=(uintptr_t)entry; f->spsr=0x345;
    threads[id]=(struct thread){.frame=f,.stack=stack,.root=mmu_kernel_root(),.priority=priority,.state=1};
    return (int)id;
}
int thread_user(struct pmm *p,uint64_t entry,uint64_t stack,uint64_t root_address,uint64_t pid,uint64_t role,uint64_t peer) {
    int id=thread_create(p,(void (*)(void *))(uintptr_t)entry,0,1);
    if(id<0) return -1;
    struct thread *t=&threads[id];
    t->root=root_address; t->pid=pid;
    t->frame->spsr=0x340; t->frame->sp_el0=stack;
    t->frame->x[0]=role; t->frame->x[1]=peer; t->frame->x[30]=0;
    return id;
}
bool thread_dead(unsigned id) { return id<MAX_THREADS && threads[id].state==2; }
bool thread_reap(struct pmm *p,unsigned id) {
    if(!id || id>=MAX_THREADS || id==current || threads[id].state!=2) return false;
    if(!mmu_restore(threads[id].stack) || !pmm_free(p,threads[id].stack,3)) panic("THREAD REAP FAILED");
    threads[id]=(struct thread){0};
    return true;
}
uint64_t scheduler_pid(void) { return threads[current].pid; }
void thread_priority(unsigned priority) {
    uint64_t mask; __asm__ volatile("mrs %0, daif; msr daifset, #2":"=r"(mask)::"memory");
    if(priority<=31) threads[current].priority=priority;
    __asm__ volatile("msr daif, %0"::"r"(mask):"memory");
}
uint64_t scheduler_switches(void) { return switches; }
struct exception_frame *scheduler_tick(struct exception_frame *f) {
    for(unsigned i=1;i<MAX_THREADS;i++) if(threads[i].state==3 && timer_ticks()>=threads[i].deadline) threads[i].state=1;
    threads[current].frame=f;
    unsigned best=MAX_THREADS;
    for(unsigned n=1;n<=MAX_THREADS;n++) {
        unsigned i=(current+n)%MAX_THREADS;
        if(threads[i].state==1 && (best==MAX_THREADS || threads[i].priority>threads[best].priority)) best=i;
    }
    if(best==MAX_THREADS) panic("NO RUNNABLE THREAD");
    if(best!=current) switches++;
    current=best;
    mmu_activate(threads[current].root);
    return threads[current].frame;
}
struct exception_frame *scheduler_terminate(struct exception_frame *f) {
    if(!current) panic("BOOT THREAD EXIT");
    threads[current].state=2;
    return scheduler_tick(f);
}
struct exception_frame *scheduler_trap(struct exception_frame *f) {
    if(scheduler_pid()) panic("INVALID KERNEL THREAD SVC");
    if((f->esr&0xffff)==0x4349)return scheduler_tick(f);
    if((f->esr&0xffff)!=0x4348)panic("INVALID THREAD EXIT");
    return scheduler_terminate(f);
}
void thread_yield(void){__asm__ volatile("svc #0x4349":::"memory","cc");}

struct exception_frame *scheduler_wait(struct exception_frame *f,uint64_t deadline) {
    if(!current || !threads[current].pid) panic("INVALID EVENT WAIT");
    threads[current].state=3;threads[current].deadline=deadline;
    return scheduler_tick(f);
}
void scheduler_wake(uint64_t pid) {
    for(unsigned i=1;i<MAX_THREADS;i++) if(threads[i].pid==pid && threads[i].state==3) threads[i].state=1;
}

void scheduler_account(void) { threads[current].samples++; }
uint64_t scheduler_samples(uint64_t pid) {
    for(unsigned i=1;i<MAX_THREADS;i++)if(threads[i].pid==pid)return threads[i].samples;
    return 0;
}
bool scheduler_cancel(uint64_t pid) {
    for(unsigned i=1;i<MAX_THREADS;i++)if(i!=current && threads[i].pid==pid && threads[i].state){threads[i].state=2;return true;}
    return false;
}
