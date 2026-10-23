/* A CHFS block device backed only by allocated RAM. No physical disk aliases. */
#include "filesystem.h"
#include "memory.h"
#include "vfs.h"
static struct chfs volume;
static bool read_block(void *context,uint64_t block,void *out) {
    if(block>=1024)return false;
    const uint8_t *source=(const uint8_t *)context+block*4096;
    for(unsigned i=0;i<4096;i++)((uint8_t *)out)[i]=source[i];return true;
}
static bool write_block(void *context,uint64_t block,const void *in) {
    if(block>=1024)return false;
    uint8_t *destination=(uint8_t *)context+block*4096;
    for(unsigned i=0;i<4096;i++)destination[i]=((const uint8_t *)in)[i];return true;
}
static bool flush(void *context) {(void)context;return true;}
bool ram_volume_start(struct pmm *physical) {
    uint64_t memory=pmm_alloc(physical,1024);if(!memory)return false;
    for(uint64_t i=0;i<1024*4096;i++)((uint8_t *)(uintptr_t)memory)[i]=0;
    struct block_device disk={.context=(void *)(uintptr_t)memory,.blocks=1024,.read=read_block,.write=write_block,.flush=flush};
    static const char welcome[]="Welcome to CHOS.\nThis is a volatile RAM volume.\nFiles are lost at shutdown or restart.\n";
    if(!chfs_format(&volume,disk,true) || !chfs_mkdir(&volume,"/documents") || !chfs_write(&volume,"/documents/welcome.txt",welcome,sizeof(welcome)-1))return false;
    vfs_mount(&volume);return true;
}
