#include "config.h"
#include "filesystem.h"
bool config_save(struct chfs *fs,const struct chos_config *next,struct chos_config *active) {
    unsigned char wire[CHOS_CONFIG_BYTES];struct chos_config canonical;
    if(!active || !config_encode(next,wire) || !config_decode(&canonical,wire,sizeof(wire)) || !chfs_write(fs,CHOS_CONFIG_PATH,wire,sizeof(wire)))return false;
    *active=canonical;return true;
}
