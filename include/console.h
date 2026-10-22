#ifndef CHOS_CONSOLE_H
#define CHOS_CONSOLE_H
#include <stdint.h>
struct chos_boot_info;
void console_init(struct chos_boot_info *);
void console_rect(unsigned,unsigned,unsigned,unsigned,uint32_t);
void console_text(unsigned,unsigned,const char *,uint32_t);
void console_number(unsigned,unsigned,uint64_t);
_Noreturn void panic(const char *);
#endif
