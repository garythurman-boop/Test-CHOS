#ifndef CHOS_EXCEPTION_H
#define CHOS_EXCEPTION_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
struct exception_frame {
    uint64_t x[31],sp_el0,elr,spsr,esr,far,vector,padding;
};
_Static_assert(sizeof(struct exception_frame)==304,"exception frame size");
_Static_assert(offsetof(struct exception_frame,elr)==256,"exception frame ABI");
void exception_init(void);
bool exception_test_break(void);
bool exception_test_read(uint64_t address);
bool exception_test_write(uint64_t address);
bool exception_test_execute(uint64_t address);
struct exception_frame *exception_dispatch(struct exception_frame *);
struct exception_frame *irq_dispatch(struct exception_frame *);
extern char vectors[];
#endif
