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
struct usb_status {uint64_t base;uint32_t id,capabilities,hcs,ports,connected,stage,reason,enum_port,enum_stage,keyboards,mice;};
const struct usb_status *usb_status(void);
int usb_initialize(struct pmm *,const struct chos_boot_info *);
void usb_poll(void);
#endif
