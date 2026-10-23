#ifndef CHOS_KERNEL_IMAGE_H
#define CHOS_KERNEL_IMAGE_H
#define CHOS_IMAGE_HEADER_BYTES 64
#define CHOS_IMAGE_MAX_FILE (16*1024*1024)
#define CHOS_IMAGE_MAX_MEMORY (64*1024*1024)
#define CHOS_BOOT_STACK_BYTES (64*1024)
#define CHOS_BOOT_STACK_ALLOCATION (CHOS_BOOT_STACK_BYTES+8192)
#ifndef __ASSEMBLER__
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CHOS_IMAGE_MAGIC UINT64_C(0x31474d49534f4843)
struct kernel_image_layout {uint64_t file_bytes,memory_bytes,entry_offset;};
bool kernel_image_validate(const void *,size_t,uint64_t,struct kernel_image_layout *);
bool kernel_boot_regions(uint64_t,uint64_t,uint64_t,uint64_t);
#endif
#endif
