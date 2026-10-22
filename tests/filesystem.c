#include "filesystem.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t disk[64][4096],saved[64][4096];
static int fail=-1;
static bool read_block(void *ctx,uint64_t b,void *out) { (void)ctx; if(b>=64) return false; memcpy(out,disk[b],4096); return true; }
static bool write_block(void *ctx,uint64_t b,const void *in) {
    (void)ctx; if(b>=64) return false;
    if(fail==0) { memcpy(disk[b],in,2048); return false; }
    if(fail>0) fail--;
    memcpy(disk[b],in,4096); return true;
}
static bool flush(void *ctx) { (void)ctx; if(fail==0) return false; if(fail>0) fail--; return true; }
static struct chfs f,mounted;
static const struct block_device device={0,64,read_block,write_block,flush};
int main(void) {
    assert(!chfs_mount(&f,device));
    assert(!chfs_format(&f,device,false));
    assert(chfs_format(&f,device,true));
    assert(!chfs_write(&f,"/missing/file","A",1));
    assert(!chfs_mkdir(&f,"/../bad") && !chfs_mkdir(&f,"/a//b"));
    assert(chfs_mkdir(&f,"/docs") && chfs_mkdir(&f,"/docs/nested"));
    uint8_t input[8193],output[8193];
    for(unsigned i=0;i<sizeof(input);i++) input[i]=(uint8_t)(i*31);
    assert(chfs_write(&f,"/docs/file",input,sizeof(input)));
    size_t n=0;
    assert(chfs_read(&f,"/docs/file",0,output,sizeof(output),&n) && n==sizeof(input) && !memcmp(input,output,n));
    assert(chfs_read(&f,"/docs/file",4090,output,50,&n) && n==50 && !memcmp(input+4090,output,n));
    assert(chfs_write(&f,"/empty",0,0) && chfs_read(&f,"/empty",0,output,10,&n) && n==0);
    assert(chfs_rename(&f,"/docs/file","/docs/nested/moved"));
    assert(!chfs_stat(&f,"/docs/file") && chfs_stat(&f,"/docs/nested/moved"));
    assert(!chfs_remove(&f,"/docs") && !chfs_rename(&f,"/docs","/new"));
    assert(chfs_mount(&mounted,device));
    assert(chfs_read(&mounted,"/docs/nested/moved",0,output,sizeof(output),&n) && !memcmp(input,output,n));
    const struct chfs_entry *e=chfs_stat(&mounted,"/docs/nested/moved");
    assert(e); disk[e->first][0]^=1;
    assert(!chfs_read(&mounted,"/docs/nested/moved",0,output,sizeof(output),&n) && n==0);
    disk[e->first][0]^=1;
    assert(chfs_remove(&mounted,"/docs/nested/moved") && chfs_remove(&mounted,"/docs/nested"));
    assert(chfs_remove(&mounted,"/docs"));
    assert(chfs_write(&mounted,"/atomic","OLD",3));
    memcpy(saved,disk,sizeof(disk));
    for(int failure=0;failure<=4;failure++) {
        memcpy(disk,saved,sizeof(disk)); fail=-1; assert(chfs_mount(&f,device));
        fail=failure; (void)chfs_write(&f,"/atomic","NEW VERSION",11);
        fail=-1; assert(chfs_mount(&mounted,device));
        assert(chfs_read(&mounted,"/atomic",0,output,sizeof(output),&n));
        assert((n==3 && !memcmp(output,"OLD",3)) || (n==11 && !memcmp(output,"NEW VERSION",11)));
    }
    memcpy(disk,saved,sizeof(disk)); assert(chfs_mount(&f,device));
    disk[f.active][0]^=1; assert(chfs_mount(&mounted,device)); /* fallback root */
    disk[0][0]^=2; disk[1][0]^=2; assert(!chfs_mount(&mounted,device));
    memset(disk,0,sizeof(disk)); assert(chfs_format(&f,device,true));
    char name[16]; unsigned count=0;
    while(count<40) { snprintf(name,sizeof(name),"/file%u",count); assert(chfs_write(&f,name,"X",1)); count++; }
    assert(!chfs_write(&f,"/overflow","X",1));
    puts("PASS: CHFS directories, create/read/write/rename/delete, cross-block reads, remount, data CRC, metadata fallback, torn-write/flush failure recovery and capacity limits");
}
