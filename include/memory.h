#ifndef CHOS_MEMORY_H
#define CHOS_MEMORY_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define PAGE_SIZE 4096ULL
#define PMM_MAX_PAGES 262144u
#define PMM_MAX_RANGES 128u
struct memory_descriptor {
    uint32_t type,padding;
    uint64_t physical,virtual_address,pages,attributes;
};
struct pmm_range { uint64_t base; uint32_t pages,index; };
enum pmm_error { PMM_OK,PMM_MAP_SHAPE,PMM_DESCRIPTOR,PMM_ALIGNMENT,PMM_OVERFLOW,PMM_OVERLAP,PMM_RANGE_CAPACITY,PMM_EMPTY,PMM_ADDRESS };
struct pmm {
    struct pmm_range ranges[PMM_MAX_RANGES];
    uint64_t bits[PMM_MAX_PAGES/64];
    uint32_t range_count,total,used;
    uint32_t error,error_index,error_type;
    uint64_t error_address,error_pages;
};
bool pmm_init(struct pmm *,const void *map,size_t size,size_t stride);
uint64_t pmm_alloc(struct pmm *,size_t pages);
bool pmm_free(struct pmm *,uint64_t address,size_t pages);
struct heap { void *base; size_t size; };
bool heap_init(struct heap *,void *base,size_t size);
void *heap_alloc(struct heap *,size_t size);
bool heap_free(struct heap *,void *pointer);
#endif
