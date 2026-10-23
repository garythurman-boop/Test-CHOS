#ifndef CHOS_CONFIG_H
#define CHOS_CONFIG_H
#include "abi.h"
#include <stddef.h>
#include <stdbool.h>
#define CHOS_CONFIG_BYTES 80u
#define CHOS_CONFIG_PATH "/system/config"
void config_defaults(struct chos_config *);
bool config_valid(const struct chos_config *);
bool config_encode(const struct chos_config *,unsigned char [CHOS_CONFIG_BYTES]);
bool config_decode(struct chos_config *,const void *,size_t);
struct chfs;
bool config_save(struct chfs *,const struct chos_config *,struct chos_config *);
unsigned config_clock(unsigned,int);
#endif
