.section .rodata.user,"a"
.balign 8
.global init_elf, init_elf_end
init_elf:
.incbin "build/init.elf"
init_elf_end:
