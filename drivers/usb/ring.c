/* Original fixed-page xHCI command/transfer/event rings, portable for validation.
 * address is a trusted allocated DMA page with an identity CPU mapping. */
#include "usb.h"
bool usb_dma_span(uint64_t base,unsigned capacity,uint64_t at,unsigned length){
    return base && capacity && base<=UINT64_MAX-capacity && at>=base && at-base<=capacity && length<=capacity-(at-base);
}
uint64_t usb_ring_push(struct usb_ring *r,uint64_t parameter,uint32_t status,uint32_t control){
    if(!r || !r->address || r->address%4096 || r->address>UINT64_MAX-4096 || r->index>=255 || r->cycle>1)return 0;
    uint64_t at=r->address+r->index*16;struct usb_trb *t=(void *)(uintptr_t)at;
    t->parameter=parameter;t->status=status;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    *(volatile uint32_t *)&t->control=(control&~1u)|r->cycle;
    if(++r->index==255){
        struct usb_trb *link=(void *)(uintptr_t)(r->address+255*16);link->parameter=r->address;link->status=0;
        __atomic_thread_fence(__ATOMIC_RELEASE);
        *(volatile uint32_t *)&link->control=(6u<<10)|2u|r->cycle;
        r->index=0;r->cycle^=1;
    }
    return at;
}
bool usb_event_pop(uint64_t address,unsigned *index,unsigned *cycle,struct usb_trb *out){
    if(!address || address%4096 || address>UINT64_MAX-4096 || !index || !cycle || !out || *index>=256 || *cycle>1)return false;
    const volatile struct usb_trb *t=(void *)(uintptr_t)(address+*index*16);
    if((t->control&1)!=*cycle)return false;
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    *out=*t;if(++*index==256){*index=0;*cycle^=1;}return true;
}
