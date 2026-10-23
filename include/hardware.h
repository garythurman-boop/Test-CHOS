#ifndef CHOS_HARDWARE_H
#define CHOS_HARDWARE_H
#include <stdint.h>
struct chos_pci_device {uint16_t vendor,device;uint8_t base_class,subclass,interface,revision;uint16_t segment;uint8_t bus,slot,function,reserved[3];};
_Static_assert(sizeof(struct chos_pci_device)==16,"PCI snapshot ABI");
struct chos_hardware {
    uint32_t version,entry_el,kernel_el,gic_version,timer_irq,input_devices,sector_bytes,storage;
    uint32_t width,height,stride,pixel_format;
    uint64_t gic_distributor,gic_redistributor,nvme_bar,ecam;
    char firmware_vendor[64],serial[24];
    uint32_t pci_count,pci_total;
    struct chos_pci_device pci[32];
};
_Static_assert(sizeof(struct chos_hardware)==688,"hardware snapshot ABI");
#endif
