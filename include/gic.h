#ifndef CHOS_GIC_H
#define CHOS_GIC_H
#include "platform.h"
/* MMIO-only GICv3 core. Register access and deadline source are injected so
 * discovery, masking, routing and timeout paths can be tested on the host. */
struct gic_io {
    void *context;
    uint32_t (*read32)(void *,uint64_t);
    uint64_t (*read64)(void *,uint64_t);
    void (*write32)(void *,uint64_t,uint32_t);
    void (*write64)(void *,uint64_t,uint64_t);
    uint64_t (*counter)(void *);
    uint64_t frequency;
};
struct gic_v3 {struct gic_io io;uint64_t distributor,redistributor,affinity;unsigned interrupts;};
bool gic_v3_prepare(struct gic_v3 *,const struct platform *,struct gic_io);
bool gic_v3_enable(struct gic_v3 *,unsigned,bool);
#endif
