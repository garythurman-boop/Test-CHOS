#ifndef CHOS_SETUP_H
#define CHOS_SETUP_H
#include "install.h"
#include "boot.h"
#include "memory.h"
#include "abi.h"
struct process;
void setup_initialize(struct block_device *,struct chos_boot_info *,struct pmm *);
bool setup_persistent(void);
const char *setup_computer(void);
int64_t setup_syscall(struct process *,uint64_t,uint64_t,uint64_t);
void setup_step(void);
bool setup_confirmation_valid(const struct chos_setup *,const struct chos_confirm *);
#endif
