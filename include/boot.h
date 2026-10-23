#ifndef CHOS_BOOT_H
#define CHOS_BOOT_H
#include <stdint.h>
#include <stddef.h>
#include "hardware.h"
#define CHOS_BOOT_MAGIC UINT64_C(0x43484f53424f4f54)
struct chos_boot_info {
    uint64_t magic;
    uint64_t framebuffer, framebuffer_size;
    uint32_t width, height, stride, pixel_format;
    uint64_t memory_map, memory_map_size, descriptor_size;
    uint32_t descriptor_version, reserved;
    uint64_t rsdp;
    uint64_t clock_seconds,clock_counter;
    uint32_t clock_valid,clock_reserved;
    uint64_t kernel_source,kernel_size;
    uint32_t boot_medium,boot_contract;
    uint8_t esp_guid[16];
    uint32_t entry_el,entry_error;
    char firmware_vendor[64];
    uint32_t pci_count,pci_total;
    struct chos_pci_device pci[32];
    uint32_t firmware_revision,uefi_revision;
    uint64_t kernel_base,kernel_memory_bytes,stack_base,stack_bytes;
};
#define CHOS_BOOT_CONTRACT (((uint32_t)sizeof(struct chos_boot_info)<<16)|1u)
_Static_assert(sizeof(struct chos_boot_info)<65536,"boot contract size encoding");
_Static_assert(offsetof(struct chos_boot_info,entry_el)==144,"assembly boot entry offset");
_Static_assert(offsetof(struct chos_boot_info,entry_error)==148,"assembly boot error offset");
#endif
