#ifndef CHOS_MMU_H
#define CHOS_MMU_H
#include "memory.h"
#include "boot.h"
bool mmu_init(struct pmm *,const struct chos_boot_info *,uint64_t gicd,uint64_t gicc);
bool mmu_device(uint64_t address,uint64_t size);
bool mmu_guard(uint64_t address);
struct address_space;
bool mmu_restore(uint64_t);
uint64_t mmu_kernel_root(void);
void mmu_activate(uint64_t);
bool mmu_space_create(struct pmm *,struct address_space *);
void mmu_space_destroy(struct pmm *,struct address_space *);
bool mmu_user_map(struct pmm *,struct address_space *,uint64_t va,uint64_t pa,bool writable,bool executable);
bool mmu_user_unmap(struct address_space *,uint64_t va);
extern char __text_start[],__text_end[],__rodata_start[],__rodata_end[];
#endif
