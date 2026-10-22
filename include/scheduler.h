#ifndef CHOS_SCHEDULER_H
#define CHOS_SCHEDULER_H
#include "memory.h"
#include "exception.h"
void scheduler_init(void);
int thread_create(struct pmm *,void (*entry)(void *),void *argument,unsigned priority);
int thread_user(struct pmm *,uint64_t entry,uint64_t stack,uint64_t root,uint64_t pid,uint64_t role,uint64_t peer);
bool thread_reap(struct pmm *,unsigned id);
uint64_t scheduler_pid(void);
struct exception_frame *scheduler_terminate(struct exception_frame *);
bool thread_dead(unsigned id);
void thread_priority(unsigned priority);
uint64_t scheduler_switches(void);
struct exception_frame *scheduler_tick(struct exception_frame *);
struct exception_frame *scheduler_trap(struct exception_frame *);
struct worker_state { volatile uint64_t progress,stop; };
void arch_test_worker(void *);
#endif
