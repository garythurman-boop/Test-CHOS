#ifndef CHOS_INPUT_H
#define CHOS_INPUT_H
#include "boot.h"
#include "memory.h"
struct input_event { uint16_t type,code; int32_t value; };
struct input_state { int32_t x,y; uint64_t keys,motion,clicks,dropped; char last; };
void input_reset(unsigned,unsigned);
void input_accept(struct input_event);
bool input_next(struct input_event *);
const struct input_state *input_get(void);
int input_initialize(struct pmm *,const struct chos_boot_info *);
void input_poll(void);
#endif
