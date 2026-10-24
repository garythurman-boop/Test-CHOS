#include "usb.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static _Alignas(4096) struct usb_trb page[256];
void test_usb_descriptors(void);
void test_hid_report(void);
int main(void){
    test_usb_descriptors();test_hid_report();
    struct xhci_layout l;
    /* Actual M7G Fusion register values transcribed from the supplied page 5. */
    assert(!xhci_layout_validate(0x120020,0x08001f7f,0x7f,0x38f081,0x820,0xa20,65536,&l));
    assert(l.slots==127 && l.ports==8 && l.doorbell+(l.slots+1)*4==l.runtime);
    assert(xhci_extended_span(&l,0xe0,4));
    /* Compact, disjoint blocks are legal; old M7F rejects both offsets. This
     * is a synthetic layout (one interrupter), not a VMware register capture. */
    assert(!xhci_layout_validate(0x1000020,0x08000120,0,1,0x800,0x600,65536,&l));
    assert(l.operational==32 && l.doorbell==0x800 && l.runtime==0x600);
    assert(xhci_extended_span(&l,0x900,8));
    assert(!xhci_extended_span(&l,0x800,4) && !xhci_extended_span(&l,0x620,4));
    assert(!xhci_extended_span(&l,0xfffc,8) && !xhci_extended_span(&l,0x901,4));
    assert(!xhci_layout_validate(0x1000040,0x08000140,0,1,0x2000,0x1000,65536,&l));
    assert(xhci_extended_span(&l,0x20,16)); /* QEMU extended caps before op */
    assert(xhci_layout_validate(31,0x08000120,0,1,0x800,0x600,65536,&l)==XHCI_CAPLEN);
    assert(xhci_layout_validate(32,0x08000120,0,0,0x800,0x600,65536,&l)==XHCI_DMA64);
    for(unsigned b=21;b<32;b++)if(b!=26)assert(xhci_layout_validate(32,0x08000120,1u<<b,1,0x800,0x600,65536,&l)==XHCI_SCRATCHPADS);
    assert(xhci_layout_validate(32,0x08000100,0,1,0x800,0x600,65536,&l)==XHCI_SLOTS);
    assert(xhci_layout_validate(32,0x00000120,0,1,0x800,0x600,65536,&l)==XHCI_PORTS);
    assert(xhci_layout_validate(32,0x21000120,0,1,0x800,0x600,65536,&l)==XHCI_PORTS);
    assert(xhci_layout_validate(32,0x08000120,0,1,0xfffc,0x600,65536,&l)==XHCI_DOORBELL);
    assert(xhci_layout_validate(32,0x08000120,0,1,0x800,0xffe0,65536,&l)==XHCI_RUNTIME);
    assert(xhci_layout_validate(32,0x08000120,0,1,0x800,0x800,65536,&l)==XHCI_OVERLAP);
    assert(xhci_layout_validate(32,0x08000120,0,1,0x400,0x600,65536,&l)==XHCI_OVERLAP);
    /* MaxSlots+1 doorbells, including doorbell zero, must fit completely. */
    assert(!xhci_layout_validate(32,0x080001ff,0,1,0xfc00,0x600,65536,&l));
    assert(xhci_layout_validate(32,0x080001ff,0,1,0xfc04,0x600,65536,&l)==XHCI_DOORBELL);
    puts("xHCI compact sub-page layout, exact block bounds/overlap, extended capabilities, stable refusal reasons passed");

    struct usb_ring ring={(uintptr_t)page,0,1};
    for(unsigned i=0;i<765;i++){unsigned index=ring.index,cycle=ring.cycle;uint64_t at=usb_ring_push(&ring,i,7,1u<<10);assert(at==(uintptr_t)&page[index]);assert(page[index].parameter==i && (page[index].control&1)==cycle);if(index==254)assert(page[255].parameter==(uintptr_t)page && (page[255].control&3)==(cycle|2));}
    ring.index=255;assert(!usb_ring_push(&ring,0,0,0));
    unsigned idx=0,cycle=1;struct usb_trb completion;memset(page,0,sizeof(page));assert(!usb_event_pop((uintptr_t)page,&idx,&cycle,&completion));
    for(unsigned i=0;i<256;i++)page[i]=(struct usb_trb){i,0,(32u<<10)|1};
    for(unsigned i=0;i<256;i++){assert(usb_event_pop((uintptr_t)page,&idx,&cycle,&completion));assert(completion.parameter==i);}
    assert(idx==0 && cycle==0 && !usb_event_pop((uintptr_t)page,&idx,&cycle,&completion));idx=256;assert(!usb_event_pop((uintptr_t)page,&idx,&cycle,&completion));
    assert(usb_dma_span(0x1000,4096,0x1ff0,16) && !usb_dma_span(0x1000,4096,0x1ff0,17) && !usb_dma_span(UINT64_MAX-10,4096,UINT64_MAX-5,1) && !usb_dma_span(0x1000,4096,0xfff,1));
    uint8_t cfg[]={9,2,34,0,1,1,0,128,50,9,4,0,0,1,3,1,1,0,9,33,17,1,0,1,34,63,0,7,5,129,3,8,0,10};
    struct usb_boot_interface i;assert(usb_boot_configuration(cfg,sizeof(cfg),&i));assert(i.protocol==1 && i.packet==8 && i.endpoint==1 && i.configuration==1);
    for(unsigned n=0;n<sizeof(cfg);n++)assert(!usb_boot_configuration(cfg,n,&i));
    uint8_t copy[sizeof(cfg)];memcpy(copy,cfg,sizeof(cfg));copy[9]=0;assert(!usb_boot_configuration(copy,sizeof(copy),&i));
    memcpy(copy,cfg,sizeof(cfg));copy[29]=1;assert(!usb_boot_configuration(copy,sizeof(copy),&i));
    memcpy(copy,cfg,sizeof(cfg));copy[31]=255;assert(!usb_boot_configuration(copy,sizeof(copy),&i));
    struct usb_hid_state s={0};struct input_event e[32];unsigned n=0;
    uint8_t key[8]={2,0,4};assert(usb_hid_report(&s,1,key,8,e,&n));assert(n==2 && e[0].code==42 && e[0].value==1 && e[1].code==30);
    assert(usb_hid_report(&s,1,key,8,e,&n) && n==2);
    uint8_t invalid[8]={0,0,1};assert(!usb_hid_report(&s,1,invalid,8,e,&n) && s.modifiers==2 && s.keys[0]==4);
    invalid[2]=4;invalid[3]=4;assert(!usb_hid_report(&s,1,invalid,8,e,&n));
    memset(key,0,8);assert(usb_hid_report(&s,1,key,8,e,&n));assert(n==4 && e[2].code==42 && !e[2].value && e[3].code==30 && !e[3].value);
    uint8_t mouse[]={1,255,127,255};assert(usb_hid_report(&s,2,mouse,4,e,&n));assert(n==8 && e[4].code==272 && e[5].value==-1 && e[6].value==127 && e[7].value==-1);
    n=32;mouse[0]=0;assert(!usb_hid_report(&s,2,mouse,4,e,&n) && s.buttons==1);
    assert(!usb_hid_report(&s,1,key,7,e,&n) && !usb_hid_report(&s,2,mouse,2,e,&n));
    puts("USB rings cycle/link/event wrap/DMA bounds; boot descriptor bounds/type/endpoint rejection, HID key/release/modifier/rollover/duplicates, signed mouse/buttons/wheel and event-capacity atomicity passed");
}
