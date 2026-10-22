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
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }
    ASSERT(. < 512K, "kernel image and BSS exceed loader reservation")
    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
