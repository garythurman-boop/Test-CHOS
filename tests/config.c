#include "config.h"
#include "filesystem.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char *storage;static bool fail;static unsigned fail_at,operations;
static bool operation(void){return ++operations!=fail_at && !fail;}
static bool rd(void *p,uint64_t b,void *out){(void)p;if(b>=128)return false;memcpy(out,storage+b*4096,4096);return true;}
static bool wr(void *p,uint64_t b,const void *in){(void)p;if(b>=128 || !operation())return false;memcpy(storage+b*4096,in,4096);return true;}
static bool flush(void *p){(void)p;return operation();}
int main(void) {
    struct chos_config a,b;config_defaults(&a);assert(config_valid(&a));
    unsigned char wire[CHOS_CONFIG_BYTES];assert(config_encode(&a,wire));
    assert(config_decode(&b,wire,sizeof(wire)) && !memcmp(&a,&b,sizeof(a)));
    for(unsigned i=0;i<sizeof(wire);i++){wire[i]^=1;assert(!config_decode(&b,wire,sizeof(wire)));wire[i]^=1;}
    assert(!config_decode(&b,wire,sizeof(wire)-1));a.utc_offset=841;assert(!config_valid(&a));a.utc_offset=-721;assert(!config_valid(&a));a.utc_offset=1;assert(!config_valid(&a));a.utc_offset=330;a.theme=1;a.clock_seconds=1;assert(config_valid(&a));
    a.reserved[0]=1;assert(!config_valid(&a));a.reserved[0]=0;a.computer[0]='/';assert(!config_valid(&a));memset(a.computer,'x',32);assert(!config_valid(&a));strcpy(a.computer,"CHOS-TEST");
    assert(config_clock(0,-60)==82800 && config_clock(86399,60)==3599 && config_clock(0,330)==19800);
    storage=calloc(128,4096);assert(storage);struct block_device disk={0,128,rd,wr,flush};struct chfs *fs=calloc(1,sizeof(*fs));assert(fs);
    assert(chfs_format(fs,disk,true) && chfs_mkdir(fs,"/system"));config_defaults(&b);assert(config_encode(&b,wire));assert(chfs_write(fs,CHOS_CONFIG_PATH,wire,sizeof(wire)));
    assert(config_encode(&a,wire));fail=true;assert(!chfs_write(fs,CHOS_CONFIG_PATH,wire,sizeof(wire)));fail=false;assert(chfs_mount(fs,disk));size_t n;
    assert(chfs_read(fs,CHOS_CONFIG_PATH,0,wire,sizeof(wire),&n) && config_decode(&b,wire,n) && b.theme==0);
    assert(config_encode(&a,wire) && chfs_write(fs,CHOS_CONFIG_PATH,wire,sizeof(wire)) && chfs_mount(fs,disk));
    assert(chfs_read(fs,CHOS_CONFIG_PATH,0,wire,sizeof(wire),&n) && config_decode(&b,wire,n) && a.theme==b.theme && a.utc_offset==b.utc_offset && a.clock_seconds==b.clock_seconds && !strcmp(a.computer,b.computer));
    unsigned char *snapshot=malloc(128*4096);assert(snapshot);config_defaults(&b);assert(config_encode(&b,wire) && chfs_write(fs,CHOS_CONFIG_PATH,wire,sizeof(wire)));memcpy(snapshot,storage,128*4096);
    for(unsigned cut=1;cut<=6;cut++) {
        memcpy(storage,snapshot,128*4096);assert(chfs_mount(fs,disk));struct chos_config active;config_defaults(&active);operations=0;fail_at=cut;
        bool saved=config_save(fs,&a,&active);fail_at=0;
        assert(saved?active.theme==1:active.theme==0);
        assert(chfs_mount(fs,disk) && chfs_read(fs,CHOS_CONFIG_PATH,0,wire,sizeof(wire),&n) && config_decode(&b,wire,n));
        assert((b.theme==0 && !strcmp(b.computer,"CHOS-PC")) || (b.theme==1 && !strcmp(b.computer,"CHOS-TEST")));
    }
    free(snapshot);free(fs);free(storage);puts("Configuration version/CRC/ranges/clock wrap and failed atomic save/reopen passed");
}
