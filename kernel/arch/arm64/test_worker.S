// A CPU-bound thread with no yield instruction. Its progress requires preemption.
// Callee-saved register canaries exercise real exception-context restoration.
.section .text,"ax"
.global arch_test_worker
arch_test_worker:
    mov x19, #0x1357
    mov x20, #0x2468
    mov x21, x0
1:
    mov x1, #0x1357
    cmp x19, x1
    b.ne 4f
    mov x1, #0x2468
    cmp x20, x1
    b.ne 4f
    ldr x1, [x21]
    add x1, x1, #1
    str x1, [x21]
    ldr x1, [x21, #8]
    cbz x1, 1b
    ret
4:
    brk #0xbad
    b 4b
