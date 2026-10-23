#include "pci.h"
bool pci_memory_bar(uint32_t low,uint32_t high,uint64_t span,uint64_t *base){
    if(!base || (low&1) || ((low&6)!=0 && (low&6)!=4) || !span || span%4096)return false;
    uint64_t address=low&~15ULL;if((low&6)==4)address|=(uint64_t)high<<32;
    if(!address || address%4096 || address>=(1ULL<<48) || span>(1ULL<<48)-address)return false;
    *base=address;return true;
}

static unsigned priority(const struct chos_pci_device *d){
    if((d->base_class==12 && d->subclass==3) || (d->base_class==1 && d->subclass==8 && d->interface==2) || (d->vendor==0x1af4 && d->device==0x1052))return 3;
    if(d->base_class==1 || d->base_class==3)return 2;
    return d->base_class==6?0:1;
}
void pci_inventory_keep(struct chos_boot_info *b,const struct chos_pci_device *d){
    if(!b || !d || b->pci_count>32)return;
    for(unsigned i=0;i<b->pci_count;i++)if(b->pci[i].segment==d->segment && b->pci[i].bus==d->bus && b->pci[i].slot==d->slot && b->pci[i].function==d->function)return;
    if(b->pci_count<32){b->pci[b->pci_count++]=*d;return;}
    unsigned lowest=31;
    for(unsigned i=0;i<32;i++)if(priority(&b->pci[i])<priority(&b->pci[lowest]))lowest=i;
    if(priority(d)>priority(&b->pci[lowest]))b->pci[lowest]=*d;
}
