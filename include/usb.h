#ifndef CHOS_USB_H
#define CHOS_USB_H
#include "input.h"
/* Boot HID is a deliberately bounded subset, not a report-descriptor interpreter. */
struct usb_boot_interface {uint8_t configuration,interface,protocol,endpoint,interval;uint16_t packet;};
struct usb_hid_state {uint8_t modifiers,keys[6],buttons;};
bool usb_boot_configuration(const uint8_t *,unsigned,struct usb_boot_interface *);
bool usb_hid_report(struct usb_hid_state *,unsigned,const uint8_t *,unsigned,struct input_event *,unsigned *);
struct usb_trb {uint64_t parameter;uint32_t status,control;};
struct usb_ring {uint64_t address;unsigned index,cycle;};
_Static_assert(sizeof(struct usb_trb)==16,"xHCI TRB ABI");
bool usb_dma_span(uint64_t,unsigned,uint64_t,unsigned);
uint64_t usb_ring_push(struct usb_ring *,uint64_t,uint32_t,uint32_t);
bool usb_event_pop(uint64_t,unsigned *,unsigned *,struct usb_trb *);
/* Stable initialization refusal codes; zero means no controller refusal. */
enum xhci_reason {
    XHCI_BAR=101,XHCI_MAP=102,XHCI_DECODE=103,
    XHCI_CAPLEN=201,XHCI_DMA64=202,XHCI_SCRATCHPADS=203,
    XHCI_SLOTS=204,XHCI_PORTS=205,XHCI_DOORBELL=206,
    XHCI_RUNTIME=207,XHCI_OVERLAP=208,
    XHCI_EXT_BOUNDS=301,XHCI_LEGACY_OWNED=302,XHCI_EXT_LIMIT=303,
    XHCI_HALT=401,XHCI_RESET=501,XHCI_NOT_READY=502,XHCI_PAGESIZE=503,
    XHCI_ALLOC=601,XHCI_BUSMASTER=602,XHCI_RUN=701
};
struct xhci_layout {uint32_t operational,doorbell,runtime,slots,ports;};
unsigned xhci_layout_validate(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,struct xhci_layout *);
bool xhci_extended_span(const struct xhci_layout *,uint32_t,uint32_t);
const char *xhci_reason_name(unsigned);
struct usb_status {uint64_t base;uint32_t id,capabilities,hcs,ports,connected,stage,reason,enum_port,enum_stage,keyboards,mice,initialized,enumerated;};
const struct usb_status *usb_status(void);
int usb_initialize(struct pmm *,const struct chos_boot_info *);
void usb_poll(void);
#endif
