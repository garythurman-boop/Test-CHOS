#include "platform.h"
int64_t platform_power(const struct chos_boot_info *boot,unsigned action) {
    bool hvc;if(action<1 || action>2)return -22;
    if(!platform_psci(boot,&hvc))return -38;
    register uint64_t x0 __asm__("x0")=action==1?0x84000009u:0x84000008u;
    register uint64_t x1 __asm__("x1")=0,x2 __asm__("x2")=0,x3 __asm__("x3")=0;
    if(hvc)__asm__ volatile("hvc #0":"+r"(x0),"+r"(x1),"+r"(x2),"+r"(x3)::"memory");
    else __asm__ volatile("smc #0":"+r"(x0),"+r"(x1),"+r"(x2),"+r"(x3)::"memory");
    return (int64_t)x0;
}
