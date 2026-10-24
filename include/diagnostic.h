#ifndef CHOS_DIAGNOSTIC_H
#define CHOS_DIAGNOSTIC_H
#include "boot.h"
#include <stdbool.h>
#define CHOS_DIAGNOSTIC_BOOT 2u
#define CHOS_INPUT_TEST_BOOT 4u
#define DIAGNOSTIC_CAPACITY 65536u
void diagnostic_reset(void);
void diagnostic_append(const char *);
unsigned diagnostic_read(unsigned,void *,unsigned);
unsigned diagnostic_size(void);
void diagnostic_init(const struct chos_boot_info *);
void diagnostic_map_serial(void);
void diagnostic_log(const char *);
void diagnostic_value(const char *,uint64_t,bool);
void diagnostic_inventory(const struct chos_boot_info *);
unsigned diagnostic_screen(unsigned);
struct input_event;
void diagnostic_input_event(const struct input_event *);
void diagnostic_input_panel(void);
void diagnostic_force_input(void);
#endif
