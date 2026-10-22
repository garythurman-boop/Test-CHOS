#ifndef CHOS_ELF_H
#define CHOS_ELF_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define USER_BASE UINT64_C(0x1000000000)
#define USER_IMAGE_LIMIT (USER_BASE+0x200000)
#define USER_HEAP_BASE (USER_BASE+0x400000)
#define USER_STACK_TOP (USER_BASE+0x40000000)
#define ELF_MAX_SEGMENTS 8
struct elf_segment { uint64_t address,offset,file_size,memory_size; uint32_t flags; };
struct elf_plan { uint64_t entry; unsigned count; struct elf_segment segments[ELF_MAX_SEGMENTS]; };
bool elf_validate(const void *,size_t,struct elf_plan *);
#endif
