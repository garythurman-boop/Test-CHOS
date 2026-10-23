#ifndef CHOS_BOOT_H
#define CHOS_BOOT_H
#include <stdint.h>
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
    uint32_t boot_medium,boot_reserved;
    uint8_t esp_guid[16];
};
#endif
