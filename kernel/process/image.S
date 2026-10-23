.section .rodata.user,"a"
.balign 8
.global init_elf, init_elf_end
init_elf:
.incbin "build/init.elf"
init_elf_end:

.balign 8
.global desktop_elf, desktop_elf_end
desktop_elf:
.incbin "build/desktop.elf"
desktop_elf_end:
