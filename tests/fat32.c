#include "install.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct cached {uint64_t block;uint8_t data[4096];};
struct disk {struct cached slots[512];unsigned count,writes,reads;int fail;bool fail_flush;uint64_t low,high;};
static struct gpt_workspace workspace;
static bool read_block(void *ctx,uint64_t block,void *out) {
    struct disk *d=ctx;assert(block>=d->low && block<=d->high);d->reads++;
    for(unsigned i=0;i<d->count;i++)if(d->slots[i].block==block){memcpy(out,d->slots[i].data,4096);return true;}
    memset(out,0xa5,4096);return true;
}
static bool write_block(void *ctx,uint64_t block,const void *in) {
    struct disk *d=ctx;assert(block>=d->low && block<=d->high);
    if((int)d->writes++==d->fail)return false;
    unsigned i=0;while(i<d->count && d->slots[i].block!=block)i++;
    if(i==d->count){assert(i<512);d->count++;d->slots[i].block=block;}
    memcpy(d->slots[i].data,in,4096);return true;
}
static bool flush(void *ctx){return !((struct disk *)ctx)->fail_flush;}
int main(void) {
    struct disk *d=calloc(1,sizeof(*d));assert(d);
    uint8_t loader[1025],kernel[5133],guid[16]={1};
    for(unsigned i=0;i<sizeof(loader);i++)loader[i]=(uint8_t)(i^0xa5);
    for(unsigned i=0;i<sizeof(kernel);i++)kernel[i]=(uint8_t)(i^0x3c);
    struct install_payload payload={loader,kernel,sizeof(loader),sizeof(kernel)};
    for(unsigned mode=0;mode<2;mode++) {
        uint32_t sector=mode?4096:512;struct install_plan plan;
        struct block_device device={.context=d,.blocks=mode?131072:32768,.read=read_block,.write=write_block,.flush=flush};
        assert(install_plan_make(device.blocks,sector,guid,&plan));
        memset(d,0,sizeof(*d));d->fail=-1;d->low=plan.esp_first*sector/4096;d->high=(plan.esp_last+1)*sector/4096-1;
        assert(!install_fat32_write(device,&plan,&payload,false,&workspace) && !d->reads && !d->writes);
        struct install_payload bad=payload;bad.kernel_size=512*1024+1;
        assert(!install_fat32_write(device,&plan,&bad,true,&workspace) && !d->writes);
        assert(install_fat32_write(device,&plan,&payload,true,&workspace));
        unsigned writes=d->writes;assert(install_fat32_verify(device,&plan,&payload,&workspace));
        struct block_device readonly=device;readonly.write=0;readonly.flush=0;
        assert(install_fat32_verify(readonly,&plan,&payload,&workspace));
        assert(!install_fat32_write(readonly,&plan,&payload,true,&workspace));
        d->slots[d->count-1].data[13]^=1;assert(!install_fat32_verify(device,&plan,&payload,&workspace));
        const int positions[]={0,1,5,32,64,(int)writes-1};
        for(unsigned i=0;i<sizeof(positions)/sizeof(positions[0]);i++) {
            uint64_t lo=d->low,hi=d->high;memset(d,0,sizeof(*d));d->low=lo;d->high=hi;d->fail=positions[i];
            assert(!install_fat32_write(device,&plan,&payload,true,&workspace));
        }
        d->count=d->writes=0;d->fail=-1;d->fail_flush=true;
        assert(!install_fat32_write(device,&plan,&payload,true,&workspace));
    }
    free(d);puts("FAT32 bounds, confirmation, 512/4096 sectors, mirrored metadata, payload integrity, read-only verification and write/flush errors passed");
}
