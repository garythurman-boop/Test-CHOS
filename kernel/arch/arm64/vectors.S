.section .text.vectors,"ax"
.balign 2048
.global vectors
vectors:
.macro slot n
    .balign 128
    sub sp, sp, #304
    str x0, [sp, #0]
    mov x0, #\n
    str x0, [sp, #288]
    b exception_common
.endm
slot 0
slot 1
slot 2
slot 3
slot 4
slot 5
slot 6
slot 7
slot 8
slot 9
slot 10
slot 11
slot 12
slot 13
slot 14
slot 15
.balign 128
exception_common:
    str x1, [sp, #8]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
    mrs x0, sp_el0
    str x0, [sp, #248]
    mrs x0, elr_el1
    str x0, [sp, #256]
    mrs x0, spsr_el1
    str x0, [sp, #264]
    mrs x0, esr_el1
    str x0, [sp, #272]
    mrs x0, far_el1
    str x0, [sp, #280]
    mov x0, sp
    bl exception_dispatch
    mov sp, x0
    ldr x0, [sp, #248]
    msr sp_el0, x0
    ldr x0, [sp, #256]
    msr elr_el1, x0
    ldr x0, [sp, #264]
    msr spsr_el1, x0
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]
    ldp x0, x1, [sp, #0]
    add sp, sp, #304
    eret
.global arch_break_test
arch_break_test:
    brk #0x4348
    ret
.global arch_read_test
arch_read_test:
    ldr x0, [x0]
    ret
.global arch_write_test
arch_write_test:
    str xzr, [x0]
    ret

.global arch_exec_test
arch_exec_test:
    stp x29, x30, [sp, #-16]!
    blr x0
.global arch_exec_resume
arch_exec_resume:
    ldp x29, x30, [sp], #16
    ret
