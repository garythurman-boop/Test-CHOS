#ifndef CHOS_PCI_H
#define CHOS_PCI_H
#include "boot.h"
#include <stdbool.h>
/* Firmware BDF hints are revalidated against live config, never trusted as DMA
 * resources. Fallback is the existing bus-zero scan for old/absent inventories. */
struct pci_device {uint64_t config;uint32_t id,class_code,error;uint16_t segment;uint8_t bus,slot,function;};
void pci_inventory_keep(struct chos_boot_info *,const struct chos_pci_device *);
bool pci_memory_bar(uint32_t low,uint32_t high,uint64_t span,uint64_t *base);
bool pci_find_class(const struct chos_boot_info *,uint32_t,struct pci_device *);
bool pci_enable(const struct pci_device *,uint16_t bits);
void pci_report(const char *,const struct pci_device *);
#endif
