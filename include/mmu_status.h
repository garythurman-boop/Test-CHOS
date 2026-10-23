#ifndef CHOS_MMU_STATUS_H
#define CHOS_MMU_STATUS_H
#include "memory.h"
/* Stable numeric codes for a photograph/transcription without serial output. */
enum mmu_stage { MMU_RESET,MMU_FEATURES,MMU_MEMORY_MAP,MMU_ROOT,MMU_RAM,
    MMU_FRAMEBUFFER,MMU_GIC,MMU_TEXT,MMU_RODATA,MMU_GUARDS,MMU_TRANSITION,MMU_READY,MMU_DEVICE };
enum mmu_reason { MMU_OK,MMU_ALLOC,MMU_CAPACITY,MMU_ADDRESS,MMU_WIDTH,
    MMU_OVERFLOW,MMU_DESCRIPTOR,MMU_OVERLAP,MMU_CONFLICT,MMU_ALIGNMENT,
    MMU_GRANULE,MMU_COVERAGE,MMU_TABLE_ADDRESS,MMU_FIRMWARE_STATE };
struct mmu_status {
    uint32_t stage,reason,descriptor_index,descriptor_type;
    uint64_t address,size,pages_requested;
    uint32_t tables,capacity,physical_total,physical_used,physical_error,physical_ranges;
    uint64_t descriptors,ram_pages,highest_end,largest_free_run;
    uint64_t descriptor_attributes,features,old_tcr,old_ttbr,old_sctlr,new_tcr;
};
const struct mmu_status *mmu_status_get(void);
void mmu_failure_screen(void);
struct chos_boot_info;
void mmu_memory_rejected(const struct chos_boot_info *,struct pmm *);
#endif
