.section .text.entry,"ax"
.global _start
_start:
    bl user_main
    mov x8, #3
    svc #0
1:  b 1b
