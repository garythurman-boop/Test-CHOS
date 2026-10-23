.section .text.entry,"ax"
.global _start
_start:
    b .Lcode
    .word 0
    .quad 0x31474d49534f4843
    .word 1,64
    .quad __file_end,__image_end,64,0,0
.Lcode:
    msr daifset, #15
    msr spsel, #1
    mov sp, x1
    mov x29, xzr
    mov x30, xzr
    mrs x2, CurrentEL
    lsr x2, x2, #2
    str w2, [x0, #144] // boot.h statically checks these offsets
    cmp x2, #1
    b.eq 4f
    cmp x2, #2
    b.ne 9f
    // UEFI identity mappings stay enabled. Transfer the same physical tables
    // and cache attributes to EL1; no dirty firmware cache is discarded.
    mrs x2, hcr_el2
    tbnz x2, #34, 9f // VHE requires a different translation conversion
    mrs x2, sctlr_el2
    tbz x2, #0, 9f
    tbnz x2, #25, 9f
    mrs x3, tcr_el2
    tbnz x3, #32, 9f // LPA2 descriptor regime is not supported yet
    and x4, x3, #63
    cmp x4, #16
    b.lo 9f
    cmp x4, #39
    b.hi 9f
    ubfx x4, x3, #16, #3
    cmp x4, #5
    b.hi 9f
    and x5, x3, #0xffff
    orr x5, x5, x4, lsl #32 // EL2.PS -> EL1.IPS
    orr x5, x5, #(1 << 23)  // EPD1: TTBR1 is unused
    ubfx x4, x3, #20, #1
    orr x5, x5, x4, lsl #37 // EL2.TBI -> EL1.TBI0
    mov x3, #(1 << 31)
    msr hcr_el2, x3
    isb
    msr tcr_el1, x5
    mrs x3, mair_el2
    msr mair_el1, x3
    mrs x3, ttbr0_el2
    msr ttbr0_el1, x3
    msr ttbr1_el1, xzr
    mov x3, #0x100f
    movk x3, #8, lsl #16  // common M/A/C/SA/I/WXN bits
    and x2, x2, x3
    mov x3, #0x0800
    movk x3, #0x30d0, lsl #16 // EL1 architectural RES1 bits
    orr x2, x2, x3
    msr sctlr_el1, x2
    mov x2, #(1 << 31)
    msr hcr_el2, x2 // AArch64 EL1, no stage 2, no IRQ/WFI/sysreg traps
    mov x2, #3
    msr cnthctl_el2, x2 // allow EL1 physical counter and timer
    msr cntvoff_el2, xzr
    msr cntkctl_el1, xzr
    msr cnthp_ctl_el2, xzr
    mov x2, #0x33ff
    msr cptr_el2, x2
    msr mdcr_el2, xzr
    mrs x2, id_aa64pfr0_el1
    ubfx x2, x2, #24, #4
    cbz x2, 2f
    mrs x2, S3_4_C12_C9_5 // ICC_SRE_EL2
    orr x2, x2, #15
    msr S3_4_C12_C9_5, x2
    isb
    mrs x2, S3_4_C12_C9_5
    tbz x2, #0, 9f
2:
    msr sp_el1, x1
    adr x2, 4f
    msr elr_el2, x2
    mov x2, #0x3c5 // EL1h, DAIF masked
    msr spsr_el2, x2
    dsb sy
    tlbi vmalle1
    dsb sy
    isb
    eret
9:
    mov w2, #1
    str w2, [x0, #148]
4:
    bl kernel_main
1:  wfe
    b 1b
