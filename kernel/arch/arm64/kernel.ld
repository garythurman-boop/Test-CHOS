ENTRY(_start)
SECTIONS {
    . = 0;
    .text : { KEEP(*(.text.entry)) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }
    ASSERT(. < 512K, "kernel image and BSS exceed loader reservation")
    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
