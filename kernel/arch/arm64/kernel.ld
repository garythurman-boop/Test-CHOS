#include "kernel_image.h"
ENTRY(_start)
SECTIONS {
    . = 0;
    __text_start = .;
    .text : { KEEP(*(.text.entry)) *(.text*) }
    . = ALIGN(4096);
    __text_end = .;
    __rodata_start = .;
    .rodata : { *(.rodata*) }
    . = ALIGN(4096);
    __rodata_end = .;
    .got : { *(.got*) }
    ASSERT(SIZEOF(.got) == 0, "flat kernel must not contain absolute GOT entries")
    .data : { *(.data*) }
    __file_end = .;
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }
    __image_end = ALIGN(.,4096);
    ASSERT(__file_end <= CHOS_IMAGE_MAX_FILE, "kernel file exceeds image policy")
    ASSERT(__image_end <= CHOS_IMAGE_MAX_MEMORY, "kernel memory exceeds image policy")
    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
