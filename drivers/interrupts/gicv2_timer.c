#include "timer.h"
#include "exception.h"
#include "scheduler.h"
#include "console.h"
static uint64_t distributor,cpu_interface,interval;
static uint32_t timer_irq;
static volatile uint64_t ticks;
static volatile uint32_t *reg(uint64_t base,unsigned offset) { return (void *)(uintptr_t)(base+offset); }
bool timer_init(const struct platform *p) {
    if(p->gic_version!=2 || p->timer_irq<16 || p->timer_irq>31) return false;
    distributor=p->gic_distributor; cpu_interface=p->gic_cpu; timer_irq=p->timer_irq;
    uint64_t freq; __asm__ volatile("mrs %0, cntfrq_el0":"=r"(freq));
    if(freq<100 || freq>UINT32_MAX) return false;
    interval=freq/100; ticks=0;
    *reg(distributor,0)=0;
    unsigned banks=(*reg(distributor,4)&31)+1;
    for(unsigned i=0;i<banks;i++) {
        *reg(distributor,0x180+i*4)=0xffffffff;
        *reg(distributor,0x280+i*4)=0xffffffff;
    }
    *reg(distributor,0x80)|=1u<<timer_irq; /* non-secure Group 1 */
    *(volatile uint8_t *)(uintptr_t)(distributor+0x400+timer_irq)=0x80;
    unsigned shift=(timer_irq%16)*2;
    uint32_t config=*reg(distributor,0xc04);
    config=(config&~(3u<<shift))|((p->timer_flags&1?2u:0u)<<shift);
    *reg(distributor,0xc04)=config;
    *reg(cpu_interface,4)=0xff; *reg(cpu_interface,8)=0;
    /* Enable both groups plus AckCtl in a unified/security-disabled view.
     * In a non-secure bank reserved bits are ignored and bit 0 enables Group 1. */
    *reg(cpu_interface,0)=7;
    *reg(distributor,0x100)=1u<<timer_irq;
    *reg(distributor,0)=3;
    __asm__ volatile("dsb sy; isb":::"memory");
    uint64_t now; __asm__ volatile("mrs %0, cntpct_el0":"=r"(now));
    __asm__ volatile("msr cntp_cval_el0, %0; msr cntp_ctl_el0, %1; isb"::"r"(now+interval),"r"(1ULL):"memory");
    return true;
}
uint64_t timer_ticks(void) { return ticks; }
struct exception_frame *irq_dispatch(struct exception_frame *frame) {
    uint32_t ack=*reg(cpu_interface,0x0c),id=ack&1023;
    if(id>=1020) return frame;
    if(id!=timer_irq) panic("UNEXPECTED HARDWARE IRQ");
    uint64_t now; __asm__ volatile("mrs %0, cntpct_el0":"=r"(now));
    __asm__ volatile("msr cntp_cval_el0, %0; isb"::"r"(now+interval):"memory");
    ticks++;
    __asm__ volatile("dsb sy":::"memory");
    *reg(cpu_interface,0x10)=ack;
    return scheduler_tick(frame);
}
