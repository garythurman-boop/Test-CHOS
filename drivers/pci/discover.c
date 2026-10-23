#include "pci.h"
#include "platform.h"
#include "mmu.h"
#include "diagnostic.h"
static uint32_t read32(uint64_t a){return *(volatile uint32_t *)(uintptr_t)a;}
static bool candidate(const struct chos_boot_info *b,const struct chos_pci_device *hint,uint32_t class_code,struct pci_device *out){
    out->segment=hint->segment;out->bus=hint->bus;out->slot=hint->slot;out->function=hint->function;
    uint64_t address;
    if(!platform_pci_function(b,hint->segment,hint->bus,hint->slot,hint->function,&address)){out->error=1;return false;}
    if(!mmu_device(address,4096)){out->error=2;return false;}
    uint32_t id=read32(address),cls=read32(address+8)>>8;
    if(id!=((uint32_t)hint->vendor|((uint32_t)hint->device<<16)) || cls!=class_code){out->error=3;return false;}
    out->config=address;out->id=id;out->class_code=cls;out->error=0;return true;
}
bool pci_find_class(const struct chos_boot_info *b,uint32_t class_code,struct pci_device *out){
    *out=(struct pci_device){.error=4};
    if(b->pci_count>32){out->error=5;return false;}
    bool hinted=false;
    for(unsigned i=0;i<b->pci_count;i++){
        const struct chos_pci_device *d=&b->pci[i];
        if(((uint32_t)d->base_class<<16|((uint32_t)d->subclass<<8)|d->interface)!=class_code)continue;
        hinted=true;if(candidate(b,d,class_code,out))return true;
    }
    if(hinted)return false; /* Do not disguise an inaccessible known BDF as absent. */
    uint64_t ecam;if(!platform_pcie(b,&ecam)){out->error=1;return false;}
    if(!mmu_device(ecam,0x100000)){out->error=2;return false;}
    for(unsigned slot=0;slot<32;slot++)for(unsigned fn=0;fn<8;fn++){
        uint64_t address=ecam+(slot<<15)+(fn<<12);uint32_t id=read32(address);
        if((id&65535)!=65535 && (read32(address+8)>>8)==class_code){
            *out=(struct pci_device){.config=address,.id=id,.class_code=class_code,.slot=slot,.function=fn};return true;
        }
    }
    out->error=4;return false;
}
bool pci_enable(const struct pci_device *d,uint16_t bits){
    if(!d || !d->config || (bits&~6u))return false;
    volatile uint16_t *cmd=(void *)(uintptr_t)(d->config+4);*cmd|=bits;
    return (*cmd&bits)==bits;
}
void pci_report(const char *driver,const struct pci_device *d){
    diagnostic_log(driver);
    diagnostic_value("Driver PCI segment/bus/devfn: ",((uint64_t)d->segment<<16)|((uint64_t)d->bus<<8)|(d->slot<<3)|d->function,true);
    diagnostic_value("Driver PCI device/vendor: ",d->id,true);
    diagnostic_value("Driver PCI config: ",d->config,true);
    diagnostic_value("Driver PCI discovery result: ",d->error,false);
}
