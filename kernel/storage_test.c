/* Destructive acceptance testing is gated by BOTH an opt-in test boot volume
 * and the exact disposable NVMe serial. The distribution volume has no opt-in. */
#include "filesystem.h"
#include "console.h"
static struct chfs fs;
static uint8_t data[9000],readback[9000];
unsigned storage_test(struct block_device device,bool test_identity) {
    if(!test_identity) panic("STORAGE TEST DISK ID REJECTED");
    unsigned phase=0; uint64_t previous=0; size_t count=0;
    if(!chfs_mount(&fs,device)) {
        uint8_t *buffer=fs.scratch;
        for(unsigned b=0;b<2;b++) {
            if(!device.read(device.context,b,buffer)) panic("DISK HEADER READ FAILED");
            for(unsigned i=0;i<4096;i++) if(buffer[i]) panic("TEST DISK IS NOT BLANK");
        }
        if(!chfs_format(&fs,device,true) || !chfs_mkdir(&fs,"/documents")) panic("CHFS FORMAT FAILED");
        phase=1;
    } else {
        if(!chfs_read(&fs,"/state",0,&previous,sizeof(previous),&count) || count!=sizeof(previous) || (previous!=1 && previous!=2)) panic("CHFS REBOOT STATE FAILED");
        if(!chfs_read(&fs,"/user/renamed.txt",0,readback,8192,&count) || count!=8192) panic("USER FILE DID NOT PERSIST");
        for(unsigned i=0;i<8192;i++) if(readback[i]!=(uint8_t)(i^0x9b)) panic("USER FILE PERSISTENCE FAILED");
        uint8_t seed=previous==1?0x31:0x52;
        if(!chfs_read(&fs,"/documents/check.txt",0,readback,sizeof(readback),&count) || count!=sizeof(readback)) panic("PERSISTENT FILE READ FAILED");
        for(unsigned i=0;i<sizeof(readback);i++) if(readback[i]!=(uint8_t)(i^seed)) panic("REBOOT INTEGRITY FAILED");
        if(previous==2) return 3; /* third boot verifies the second commit without writing */
        if(!chfs_remove(&fs,"/documents/check.txt")) panic("PERSISTENT DELETE FAILED");
        phase=2;
    }
    uint8_t seed=phase==1?0x31:0x52;
    for(unsigned i=0;i<sizeof(data);i++) data[i]=(uint8_t)(i^seed);
    if(!chfs_write(&fs,"/documents/boot.txt",data,sizeof(data)) ||
       !chfs_rename(&fs,"/documents/boot.txt","/documents/check.txt")) panic("PERSISTENT WRITE FAILED");
    if(!chfs_read(&fs,"/documents/check.txt",0,readback,sizeof(readback),&count) || count!=sizeof(data)) panic("FILE READ FAILED");
    for(unsigned i=0;i<sizeof(data);i++) if(data[i]!=readback[i]) panic("FILE INTEGRITY FAILED");
    uint64_t state=phase;
    if(!chfs_write(&fs,"/state",&state,sizeof(state))) panic("PERSISTENT STATE WRITE FAILED");
    return phase;
}

struct chfs *storage_volume(void) { return &fs; }
