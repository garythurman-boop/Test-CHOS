#include "timer.h"
#include "exception.h"
#include "scheduler.h"
#include "console.h"
#include "gic.h"
#include "mmu.h"
static uint64_t distributor,cpu_interface,interval;
static uint32_t timer_irq,version;
static struct gic_v3 gic3;
static uint32_t read32(void *c,uint64_t a){(void)c;return *(volatile uint32_t *)(uintptr_t)a;}
static uint64_t read64(void *c,uint64_t a){(void)c;return *(volatile uint64_t *)(uintptr_t)a;}
static void write32(void *c,uint64_t a,uint32_t v){(void)c;*(volatile uint32_t *)(uintptr_t)a=v;}
static void write64(void *c,uint64_t a,uint64_t v){(void)c;*(volatile uint64_t *)(uintptr_t)a=v;}
static uint64_t counter(void *c){(void)c;uint64_t value;__asm__ volatile("mrs %0, cntpct_el0":"=r"(value));return value;}
static bool initialize_v3(const struct platform *p,uint64_t freq){
    uint64_t features;__asm__ volatile("mrs %0, id_aa64pfr0_el1":"=r"(features));
    if(!((features>>24)&15))return false;
    uint64_t value;__asm__ volatile("mrs %0, S3_0_C12_C12_5":"=r"(value));
    value|=7;__asm__ volatile("msr S3_0_C12_C12_5, %0; isb"::"r"(value):"memory");
    __asm__ volatile("mrs %0, S3_0_C12_C12_5":"=r"(value));if(!(value&1))return false;
    __asm__ volatile("msr S3_0_C12_C12_7, xzr; isb":::"memory"); /* ICC_IGRPEN1 */
    if(p->gic_redistributor){if(!mmu_device(p->gic_redistributor,0x20000))return false;}
    else for(unsigned i=0;i<p->redistributor_count;i++)if(!mmu_device(p->redistributors[i].base,p->redistributors[i].size))return false;
    struct gic_io io={0,read32,read64,write32,write64,counter,freq};
    if(!gic_v3_prepare(&gic3,p,io))return false;
    __asm__ volatile("mrs %0, S3_0_C12_C12_4":"=r"(value));value&=~2ULL; /* ICC_CTLR combined EOI */
    __asm__ volatile("msr S3_0_C12_C12_4, %0; msr S3_0_C4_C6_0, %1; msr S3_0_C12_C12_3, xzr; msr S3_0_C12_C12_7, %2; dsb sy; isb"::"r"(value),"r"(255ULL),"r"(1ULL):"memory");
    return true;
}
static volatile uint64_t ticks;
static volatile uint32_t *reg(uint64_t base,unsigned offset) { return (void *)(uintptr_t)(base+offset); }
bool timer_init(const struct platform *p) {
    if((p->gic_version!=2 && p->gic_version!=3) || p->timer_irq<16 || p->timer_irq>31) return false;
    distributor=p->gic_distributor; cpu_interface=p->gic_cpu; timer_irq=p->timer_irq;
    uint64_t freq; __asm__ volatile("mrs %0, cntfrq_el0":"=r"(freq));
    if(freq<100 || freq>UINT32_MAX) return false;
    interval=freq/100; ticks=0;version=p->gic_version;
    __asm__ volatile("msr cntp_ctl_el0, xzr; isb":::"memory");
    if(version==3){if(!initialize_v3(p,freq))return false;goto enable_timer;}
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
enable_timer:;
    uint64_t now; __asm__ volatile("mrs %0, cntpct_el0":"=r"(now));
    __asm__ volatile("msr cntp_cval_el0, %0; msr cntp_ctl_el0, %1; isb"::"r"(now+interval),"r"(1ULL):"memory");
    return true;
}
uint64_t timer_redistributor(void){return version==3?gic3.redistributor:0;}
uint64_t timer_ticks(void) { return ticks; }
struct exception_frame *irq_dispatch(struct exception_frame *frame) {
    uint64_t ack;
    if(version==3)__asm__ volatile("mrs %0, S3_0_C12_C12_0":"=r"(ack));
    else ack=*reg(cpu_interface,0x0c);
    uint32_t id=(uint32_t)ack&(version==3?0xffffff:1023);
    if(id>=1020 && id<=1023)return frame;
    if(id!=timer_irq) panic("UNEXPECTED HARDWARE IRQ");
    uint64_t now; __asm__ volatile("mrs %0, cntpct_el0":"=r"(now));
    __asm__ volatile("msr cntp_cval_el0, %0; isb"::"r"(now+interval):"memory");
    ticks++;scheduler_account();
    __asm__ volatile("dsb sy":::"memory");
    if(version==3)__asm__ volatile("msr S3_0_C12_C12_1, %0; isb"::"r"(ack):"memory");
    else *reg(cpu_interface,0x10)=(uint32_t)ack;
    return scheduler_tick(frame);
}
