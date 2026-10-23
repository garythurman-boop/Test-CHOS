#ifndef CHOS_PROCESS_H
#define CHOS_PROCESS_H
#include "elf.h"
#include "scheduler.h"
#define PROCESS_MAX 8
#define PROCESS_REGIONS 16
struct address_space { uint64_t root,tables[16]; unsigned count; };
struct user_region { uint64_t va,physical; unsigned pages,flags,kind; };
struct message { uint64_t sender,value; };
struct file_handle { char path[60]; uint32_t offset,flags; bool used; };
struct process {
    uint64_t pid; int thread; bool exited; int64_t status;
    uint64_t fault_class,fault_address;
    struct address_space space;
    struct user_region regions[PROCESS_REGIONS];
    struct file_handle files[8];
    struct message mailbox[8]; unsigned head,messages;
};
void process_init(struct pmm *);
int process_create(const void *,size_t,uint64_t role,uint64_t peer);
struct process *process_find(uint64_t);
struct process *process_current(void);
struct process *process_at(unsigned);
bool process_reap(uint64_t);
bool user_validate(struct process *,uint64_t,size_t,bool writable);
bool user_copy(struct process *,uint64_t,void *,size_t,bool to_user);
uint64_t process_allocate(struct process *,unsigned pages);
bool process_free(struct process *,uint64_t);
struct exception_frame *process_exit(struct exception_frame *,int64_t,uint64_t,uint64_t);
struct exception_frame *syscall_dispatch(struct exception_frame *);
#endif
