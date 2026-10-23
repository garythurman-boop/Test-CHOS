#include "platform.h"
#include "storage.h"
#include "timer.h"
static struct chos_hardware snapshot;
void platform_record(const struct chos_boot_info *b,const struct platform *p,unsigned inputs,bool storage){
    uint64_t el;__asm__ volatile("mrs %0, CurrentEL":"=r"(el));
    snapshot=(struct chos_hardware){.version=1,.entry_el=b->entry_el,.kernel_el=(uint32_t)(el>>2),.gic_version=p->gic_version,.timer_irq=p->timer_irq,.input_devices=inputs,.sector_bytes=storage?nvme_sector_size():0,.storage=storage,.width=b->width,.height=b->height,.stride=b->stride,.pixel_format=b->pixel_format,.gic_distributor=p->gic_distributor,.gic_redistributor=timer_redistributor(),.nvme_bar=storage?nvme_bar():0,.pci_count=b->pci_count<=32?b->pci_count:0,.pci_total=b->pci_total};
    (void)platform_pcie(b,&snapshot.ecam);
    for(unsigned i=0;i<63 && b->firmware_vendor[i];i++)snapshot.firmware_vendor[i]=b->firmware_vendor[i];
    const char *serial=storage?nvme_serial():"";for(unsigned i=0;i<23 && serial[i];i++)snapshot.serial[i]=serial[i];
    for(unsigned i=0;i<snapshot.pci_count;i++)snapshot.pci[i]=b->pci[i];
}
const struct chos_hardware *platform_hardware(void){return &snapshot;}
