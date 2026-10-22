#include "exception.h"
#include "console.h"
#include "scheduler.h"
extern void arch_break_test(void);
extern uint64_t arch_read_test(uint64_t);
extern void arch_write_test(uint64_t);
extern void arch_exec_test(uint64_t);
extern char arch_exec_resume[];
static volatile uint64_t expected_pc,expected_far,expected_class,observed;
static volatile bool write_probe;
_Noreturn void panic(const char *message) {
    __asm__ volatile("msr daifset, #15":::"memory");
    console_rect(0,72,640,408,0x300808);
    console_text(32,100,"CHOS KERNEL PANIC",0xff8080);
    console_text(32,132,message,0xffffff);
    console_rect(32,400,128,16,0xff0000);
    __asm__ volatile("dsb sy":::"memory");
    for(;;) __asm__ volatile("wfe");
}
void exception_init(void) {
    uint64_t el;
    __asm__ volatile("mrs %0, CurrentEL":"=r"(el));
    if(el!=4) panic("EL1 REQUIRED AT THIS STAGE");
    __asm__ volatile("msr vbar_el1, %0; isb"::"r"(vectors):"memory");
}
static bool probe(uint64_t pc,uint64_t far,uint64_t ec) {
    write_probe=pc==(uintptr_t)arch_write_test;
    observed=0; expected_pc=pc; expected_far=far; expected_class=ec;
    __asm__ volatile("dsb sy; isb":::"memory");
    if(ec==0x3c) arch_break_test();
    else if(pc==(uint64_t)(uintptr_t)arch_read_test) (void)arch_read_test(far);
    else if(ec==0x21) arch_exec_test(far);
    else arch_write_test(far);
    expected_pc=0;
    return observed==1;
}
bool exception_test_break(void) { return probe((uintptr_t)arch_break_test,0,0x3c); }
bool exception_test_read(uint64_t address) { return probe((uintptr_t)arch_read_test,address,0x25); }
bool exception_test_write(uint64_t address) { return probe((uintptr_t)arch_write_test,address,0x25); }
bool exception_test_execute(uint64_t address) { return probe(address,address,0x21); }
struct exception_frame *exception_dispatch(struct exception_frame *f) {
    if(f->vector==5 || f->vector==9) return irq_dispatch(f);
    uint64_t ec=f->esr>>26;
    if(f->vector==4 && ec==0x15) return scheduler_trap(f);
    if(f->vector==4 && f->elr==expected_pc && expected_pc && ec==expected_class &&
       ((ec==0x3c && (f->esr&0xffff)==0x4348) ||
        ((ec==0x25 || ec==0x21) && f->far==expected_far &&
         ((ec==0x25 && !write_probe && !(f->esr&(1ULL<<6)) && (f->esr&0x3f)>=4 && (f->esr&0x3f)<=7) ||
          (ec==0x25 && write_probe && (f->esr&(1ULL<<6)) && (f->esr&0x3f)>=12 && (f->esr&0x3f)<=15) ||
          (ec==0x21 && (f->esr&0x3f)>=12 && (f->esr&0x3f)<=15))))) {
        /* Only exact, armed self-test instructions may recover from a fault. */
        observed=1; expected_pc=0;
        f->elr=ec==0x21?(uintptr_t)arch_exec_resume:f->elr+4; f->x[0]=0;
        return f;
    }
    console_rect(0,72,640,408,0x300808);
    console_text(32,100,"CHOS UNHANDLED EXCEPTION",0xff8080);
    console_text(32,140,"ESR",0xffffff); console_number(110,140,f->esr);
    console_text(32,168,"FAR",0xffffff); console_number(110,168,f->far);
    console_text(32,196,"ELR",0xffffff); console_number(110,196,f->elr);
    console_text(32,224,"VECTOR",0xffffff); console_number(110,224,f->vector);
    console_rect(32,400,128,16,0xff0000);
    __asm__ volatile("dsb sy":::"memory");
    for(;;) __asm__ volatile("wfe");
}
