#ifndef CHOS_PLATFORM_H
#define CHOS_PLATFORM_H
#include "boot.h"
#include <stdbool.h>
struct platform {
    uint64_t gic_distributor,gic_cpu;
    uint32_t timer_irq,timer_flags,gic_version;
};
bool platform_psci(const struct chos_boot_info *,bool *hvc);
int64_t platform_power(const struct chos_boot_info *,unsigned);
bool platform_pcie(const struct chos_boot_info *,uint64_t *base);
bool platform_discover(const struct chos_boot_info *,uint64_t mpidr,struct platform *);
#endif
