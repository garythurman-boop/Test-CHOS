#include "boot.h"
#include "kernel_image.h"
#include "diagnostic.h"
#include "console.h"
#include "exception.h"
extern uint8_t growth_ro_start[],growth_ro_end[],growth_bss_start[],growth_bss_end[],growth_ro_probe[],growth_bss_probe[],__image_end[];
void kernel_image_test(const struct chos_boot_info *b){
    struct kernel_image_layout layout;
    if(!kernel_image_validate((void *)(uintptr_t)b->kernel_source,64,b->kernel_size,&layout) || layout.memory_bytes!=b->kernel_memory_bytes || (uintptr_t)__image_end-b->kernel_base!=b->kernel_memory_bytes)panic("KERNEL IMAGE CONTRACT FAILED");
    size_t ro=(size_t)(growth_ro_end-growth_ro_start),zero=(size_t)(growth_bss_end-growth_bss_start);
    if(ro){for(size_t i=0;i<ro;i++)if(growth_ro_start[i]!=0xa5)panic("LARGE FILE PAYLOAD CORRUPT");if(!exception_test_write((uintptr_t)growth_ro_probe))panic("LARGE RODATA PROTECTION FAILED");}
    if(zero){for(size_t i=0;i<zero;i++)if(growth_bss_start[i])panic("LARGE BSS NOT ZERO");growth_bss_start[zero-1]=0x5a;if(!exception_test_execute((uintptr_t)growth_bss_probe))panic("LARGE BSS NX FAILED");}
    diagnostic_value("Controlled file growth bytes: ",ro,false);diagnostic_value("Controlled BSS growth bytes: ",zero,false);
    diagnostic_log("Kernel image contract and growth checks: PASS");
}
