.section .text.entry,"ax"
.global _start
_start:
    msr daifset, #15
    mov sp, x1
    mov x29, xzr
    mov x30, xzr
    bl kernel_main
1:  wfe
    b 1b
