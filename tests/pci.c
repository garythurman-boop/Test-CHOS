#include "pci.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static _Alignas(4096) unsigned char root[1048576],usb[4096],nvme[4096];
static bool mapping=true;
static void put(void *p,unsigned off,uint32_t n){memcpy((char *)p+off,&n,4);}
bool mmu_device(uint64_t a,uint64_t size){return mapping && a && (size==4096 || size==1048576);}
bool platform_pci_function(const struct chos_boot_info *b,unsigned segment,unsigned bus,unsigned slot,unsigned fn,uint64_t *out){(void)b;if(segment || slot || fn)return false;if(bus==10)*out=(uintptr_t)usb;else if(bus==18)*out=(uintptr_t)nvme;else return false;return true;}
bool platform_pcie(const struct chos_boot_info *b,uint64_t *out){(void)b;*out=(uintptr_t)root;return true;}
void diagnostic_log(const char *s){(void)s;}
void diagnostic_value(const char *s,uint64_t v,bool h){(void)s;(void)v;(void)h;}
int main(void){
 struct chos_boot_info b={0};struct pci_device d;uint64_t bar;
 b.pci[0]=(struct chos_pci_device){.vendor=0x15ad,.device=0x077a,.base_class=12,.subclass=3,.interface=48,.bus=10};
 b.pci[1]=(struct chos_pci_device){.vendor=0x15ad,.device=0x07f0,.base_class=1,.subclass=8,.interface=2,.bus=18};b.pci_count=2;
 put(usb,0,0x077a15ad);put(usb,8,0x0c033000);put(nvme,0,0x07f015ad);put(nvme,8,0x01080200);
 assert(pci_find_class(&b,0x0c0330,&d) && d.bus==10 && d.id==0x077a15ad && d.config==(uintptr_t)usb);
 assert(pci_enable(&d,6));assert(!pci_enable(&d,8));
 assert(pci_find_class(&b,0x010802,&d) && d.bus==18 && d.id==0x07f015ad);
 mapping=false;assert(!pci_find_class(&b,0x010802,&d) && d.error==2);mapping=true;
 put(nvme,8,0x01060100);assert(!pci_find_class(&b,0x010802,&d) && d.error==3);put(nvme,8,0x01080200);
 b.pci[1].bus=19;assert(!pci_find_class(&b,0x010802,&d) && d.error==1);b.pci[1].bus=18;
 b.pci_count=33;assert(!pci_find_class(&b,0x010802,&d) && d.error==5);
 b.pci_count=0;memset(root,255,sizeof(root));assert(!pci_find_class(&b,0x010802,&d) && d.error==4);
 put(root,0,0x00101b36);put(root,8,0x01080200);assert(pci_find_class(&b,0x010802,&d) && !d.bus);
 assert(pci_memory_bar(0x80000000,0,16384,&bar) && bar==0x80000000);
 assert(pci_memory_bar(0x100004,8,65536,&bar) && bar==0x800100000ULL);
 assert(!pci_memory_bar(0,0,4096,&bar) && !pci_memory_bar(0x1001,0,4096,&bar) && !pci_memory_bar(0x1002,0,4096,&bar));
 assert(!pci_memory_bar(0x1004,0x10000,4096,&bar) && !pci_memory_bar(0x1800,0,4096,&bar) && !pci_memory_bar(0x1000,0,1,&bar));
 b=(struct chos_boot_info){0};for(unsigned n=0;n<32;n++){struct chos_pci_device bridge={.base_class=6,.slot=n};pci_inventory_keep(&b,&bridge);}
 struct chos_pci_device endpoint={.vendor=0x15ad,.device=0x07f0,.bus=18,.base_class=1,.subclass=8,.interface=2};pci_inventory_keep(&b,&endpoint);pci_inventory_keep(&b,&endpoint);
 assert(b.pci_count==32);unsigned found=0;for(unsigned n=0;n<32;n++)found+=b.pci[n].device==0x07f0;assert(found==1);
 puts("PCI actual VMware IDs at bus 0a/12, live revalidation/resource rejection, bus-zero fallback, BAR bounds/types and prioritized bounded inventory passed");
}
