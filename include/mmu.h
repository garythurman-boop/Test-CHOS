#ifndef CHOS_MMU_H
#define CHOS_MMU_H
#include "memory.h"
#include "boot.h"
bool mmu_init(struct pmm *,const struct chos_boot_info *,uint64_t gicd,uint64_t gicc);
bool mmu_guard(uint64_t address);
extern char __text_start[],__text_end[],__rodata_start[],__rodata_end[];
#endif
