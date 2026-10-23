/* xHCI 1.2 sections 5.3--5.5. Offsets describe register blocks, not pages.
 * Validate every block the bounded driver uses before any operational access.
 * Reserved low DBOFF/RTSOFF bits are masked as prescribed by the interface. */
#include "usb.h"
#define APERTURE 65536u
const char *xhci_reason_name(unsigned reason){
    switch(reason){
    case XHCI_BAR:return "xHCI reason: invalid memory BAR";
    case XHCI_MAP:return "xHCI reason: MMIO mapping rejected";
    case XHCI_DECODE:return "xHCI reason: PCI memory decode not enabled";
    case XHCI_CAPLEN:return "xHCI reason: invalid CAPLENGTH/aperture";
    case XHCI_DMA64:return "xHCI reason: 32-bit-only DMA unsupported";
    case XHCI_SCRATCHPADS:return "xHCI reason: scratchpad buffers unsupported";
    case XHCI_SLOTS:return "xHCI reason: zero device slots";
    case XHCI_PORTS:return "xHCI reason: zero or more than 32 ports";
    case XHCI_DOORBELL:return "xHCI reason: doorbell block outside aperture";
    case XHCI_RUNTIME:return "xHCI reason: runtime block outside aperture";
    case XHCI_OVERLAP:return "xHCI reason: register blocks overlap";
    case XHCI_EXT_BOUNDS:return "xHCI reason: extended capability overlaps/out of bounds";
    case XHCI_LEGACY_OWNED:return "xHCI reason: BIOS ownership handoff unsupported";
    case XHCI_EXT_LIMIT:return "xHCI reason: extended capability traversal limit";
    case XHCI_HALT:return "xHCI reason: halt timeout";
    case XHCI_RESET:return "xHCI reason: reset timeout";
    case XHCI_NOT_READY:return "xHCI reason: controller-not-ready timeout";
    case XHCI_PAGESIZE:return "xHCI reason: 4 KiB pages unsupported";
    case XHCI_ALLOC:return "xHCI reason: DMA page allocation failed";
    case XHCI_BUSMASTER:return "xHCI reason: PCI bus master not enabled";
    case XHCI_RUN:return "xHCI reason: run timeout";
    default:return "xHCI reason: unknown";
    }
}
static bool overlap(uint32_t a,uint32_t n,uint32_t b,uint32_t m){return a<b+m && b<a+n;}
static bool span(uint32_t a,uint32_t n){return a<APERTURE && n<=APERTURE-a;}
unsigned xhci_layout_validate(uint32_t cap,uint32_t hcs,uint32_t hcs2,uint32_t hcc,uint32_t db,uint32_t rt,uint32_t bytes,struct xhci_layout *out){
    struct xhci_layout l={cap&255,db&~3u,rt&~31u,hcs&255,hcs>>24};
    if(bytes!=APERTURE || l.operational<32 || (l.operational&3))return XHCI_CAPLEN;
    if(!(hcc&1))return XHCI_DMA64;
    if(hcs2&0xfbe00000)return XHCI_SCRATCHPADS;
    if(!l.slots)return XHCI_SLOTS;
    if(!l.ports || l.ports>32)return XHCI_PORTS;
    uint32_t op_bytes=0x400+l.ports*16,db_bytes=(l.slots+1)*4;
    if(!span(l.doorbell,db_bytes))return XHCI_DOORBELL;
    if(!span(l.runtime,0x40))return XHCI_RUNTIME;
    if(overlap(0,l.operational,l.doorbell,db_bytes) || overlap(0,l.operational,l.runtime,0x40) ||
       overlap(l.operational,op_bytes,l.doorbell,db_bytes) || overlap(l.operational,op_bytes,l.runtime,0x40) ||
       overlap(l.doorbell,db_bytes,l.runtime,0x40))return XHCI_OVERLAP;
    *out=l;return 0;
}
bool xhci_extended_span(const struct xhci_layout *l,uint32_t offset,uint32_t bytes){
    return !(offset&3) && bytes && span(offset,bytes) &&
        !overlap(offset,bytes,0,32) && !overlap(offset,bytes,l->operational,0x3c) &&
        !overlap(offset,bytes,l->operational+0x400,l->ports*16) &&
        !overlap(offset,bytes,l->doorbell,(l->slots+1)*4) && !overlap(offset,bytes,l->runtime,0x40);
}
