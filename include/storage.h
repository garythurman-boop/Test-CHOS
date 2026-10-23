#ifndef CHOS_STORAGE_H
#define CHOS_STORAGE_H
#include "boot.h"
#include "memory.h"
#include "filesystem.h"
uint64_t nvme_bar(void);
unsigned nvme_sector_size(void);
const char *nvme_serial(void);
bool nvme_recheck(void);
bool nvme_writable(void);
const char *nvme_error(void);
bool nvme_init(struct pmm *,const struct chos_boot_info *,struct block_device *,bool *test_identity);
#endif
