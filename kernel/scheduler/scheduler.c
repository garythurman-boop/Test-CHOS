#include "scheduler.h"
#include "mmu.h"
#include "console.h"
#define MAX_THREADS 8
struct thread { struct exception_frame *frame; unsigned priority; bool ready; };
static struct thread threads[MAX_THREADS];
static unsigned count,current;
static uint64_t switches;
static _Noreturn void thread_exit(void) {
    __asm__ volatile("svc #0x4348":::"memory");
    panic("TERMINATED THREAD RESUMED");
}
void scheduler_init(void) {
    for(unsigned i=0;i<MAX_THREADS;i++) threads[i]=(struct thread){0};
    count=1; current=0; switches=0;
    threads[0].priority=1; threads[0].ready=true;
}
int thread_create(struct pmm *p,void (*entry)(void *),void *argument,unsigned priority) {
    if(!entry || priority>31 || count==MAX_THREADS) return -1;
    uint64_t stack=pmm_alloc(p,3);
    if(!stack) return -1;
    if(!mmu_guard(stack)) { (void)pmm_free(p,stack,3); return -1; }
    struct exception_frame *f=(void *)(uintptr_t)(stack+3*PAGE_SIZE-sizeof(*f));
    for(unsigned i=0;i<sizeof(*f)/8;i++) ((uint64_t *)f)[i]=0;
    f->x[0]=(uintptr_t)argument; f->x[30]=(uintptr_t)thread_exit;
    f->elr=(uintptr_t)entry; f->spsr=0x345; /* EL1h, D/A/F masked, IRQ enabled */
    threads[count]=(struct thread){f,priority,true};
    return (int)count++;
}
bool thread_dead(unsigned id) { return id<count && !threads[id].ready; }
void thread_priority(unsigned priority) {
    uint64_t mask; __asm__ volatile("mrs %0, daif; msr daifset, #2":"=r"(mask)::"memory");
    if(priority<=31) threads[current].priority=priority;
    __asm__ volatile("msr daif, %0"::"r"(mask):"memory");
}
uint64_t scheduler_switches(void) { return switches; }
struct exception_frame *scheduler_tick(struct exception_frame *f) {
    threads[current].frame=f;
    unsigned best=MAX_THREADS;
    for(unsigned n=1;n<=count;n++) {
        unsigned i=(current+n)%count;
        if(threads[i].ready && (best==MAX_THREADS || threads[i].priority>threads[best].priority)) best=i;
    }
    if(best==MAX_THREADS) panic("NO RUNNABLE THREAD");
    if(best!=current) switches++;
    current=best;
    return threads[current].frame;
}
struct exception_frame *scheduler_trap(struct exception_frame *f) {
    if((f->esr&0xffff)!=0x4348 || !current) panic("INVALID THREAD EXIT");
    threads[current].ready=false;
    return scheduler_tick(f);
}
