#ifndef CHOS_INSTALL_H
#define CHOS_INSTALL_H
#include "filesystem.h"
/* GPT uses the device's logical sector size. block_device transfers stay 4 KiB. */
struct install_plan {
    uint64_t blocks,esp_first,esp_last,system_first,system_last;
    uint32_t sector_bytes; uint8_t disk_guid[16];
};
struct gpt_workspace { uint8_t entries[16384],primary[4096],backup[4096],scratch[4096]; };
bool install_plan_make(uint64_t,uint32_t,const uint8_t[16],struct install_plan *);
uint64_t install_minimum_bytes(uint32_t);
bool install_gpt_write(struct block_device,uint32_t,const struct install_plan *,bool confirmed,struct gpt_workspace *);
bool install_gpt_verify(struct block_device,uint32_t,const struct install_plan *,struct gpt_workspace *);
struct display_preference;
struct install_payload { const uint8_t *loader,*kernel; size_t loader_size,kernel_size; };
bool install_fat32_write(struct block_device,const struct install_plan *,const struct install_payload *,bool confirmed,struct gpt_workspace *);
bool install_fat32_verify(struct block_device,const struct install_plan *,const struct install_payload *,struct gpt_workspace *);
bool install_gpt_probe(struct block_device,uint32_t,struct install_plan *,struct gpt_workspace *);
typedef void (*install_report)(void *,unsigned,uint64_t,uint64_t);
bool install_fat32_observed(struct block_device,const struct install_plan *,const struct install_payload *,bool,struct gpt_workspace *,install_report,void *);
bool install_display_save(struct block_device,const struct install_plan *,const struct install_payload *,const struct display_preference *,struct gpt_workspace *);
#endif
