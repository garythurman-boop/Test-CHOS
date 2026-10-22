.section .text,"ax"
.global arch_mmu_switch
arch_mmu_switch:
    // Both old and new mappings are identity mappings for this trampoline/stack.
    // Caches stay enabled; RAM remains Normal WB with inner-shareable attributes.
    dsb sy
    mrs x3, sctlr_el1
    bic x4, x3, #1
    msr sctlr_el1, x4
    isb
    msr mair_el1, x2
    msr tcr_el1, x1
    msr ttbr0_el1, x0
    tlbi vmalle1
    dsb sy
    isb
    orr x3, x3, #1
    orr x3, x3, #(1 << 19) // WXN
    msr sctlr_el1, x3
    isb
    ret
