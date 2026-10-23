#include "setup.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char *image;
static uint64_t blocks;
static int64_t fail=-1;
static struct gpt_workspace work;
static bool read_block(void *unused,uint64_t block,void *p){(void)unused;assert(block<blocks);if((int64_t)block==fail)return false;memcpy(p,image+block*4096,4096);return true;}
static bool write_block(void *unused,uint64_t block,const void *p){(void)unused;assert(block<blocks);memcpy(image+block*4096,p,4096);return true;}
static bool flush(void *unused){(void)unused;return true;}
static void w32(uint8_t *p,uint32_t value){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(i*8));}
static uint32_t crc(const uint8_t *p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^((0u-(c&1))&0xedb88320u);}return ~c;}
static void checksum(uint8_t *h){w32(h+16,0);w32(h+16,crc(h,92));}
int main(void){
    uint8_t guid[16]={1};struct install_plan plan;
    for(unsigned mode=0;mode<2;mode++){
        unsigned sector=mode?4096:512;uint64_t minimum=install_minimum_bytes(sector);
        assert(minimum==(mode?277ULL:69ULL)*1048576+20480);
        assert(!install_plan_make(minimum/4096-1,sector,guid,&plan));
        assert(install_plan_make(minimum/4096,sector,guid,&plan));
        blocks=minimum/4096;image=calloc((size_t)blocks,4096);assert(image);
        struct block_device d={0,blocks,read_block,write_block,flush},readonly=d;readonly.write=0;readonly.flush=0;
        struct chos_destination out;
        assert(setup_inventory(readonly,sector,&out,&work) && out.layout==0 && out.minimum_bytes==minimum && out.esp_bytes==(mode?272ULL:64ULL)*1048576);
        fail=0;assert(!setup_inventory(readonly,sector,&out,&work));fail=(int64_t)blocks-1;assert(!setup_inventory(readonly,sector,&out,&work));fail=-1;
        assert(install_gpt_write(d,sector,&plan,true,&work));
        assert(setup_inventory(readonly,sector,&out,&work) && out.layout==2 && out.partitions==2);
        assert(!strcmp(out.existing[0].name,"CHOS EFI") && !strcmp(out.existing[1].name,"CHOS System"));
        assert(out.existing[0].first_byte==1048576 && out.existing[0].bytes==out.esp_bytes && out.existing[1].bytes>=4*1048576);
        fail=2;assert(!setup_inventory(readonly,sector,&out,&work));fail=-1;
        image[2*4096+4]^=1;assert(setup_inventory(readonly,sector,&out,&work) && out.layout==3 && !out.partitions);image[2*4096+4]^=1;
        image[sector+16]^=1;assert(setup_inventory(readonly,sector,&out,&work) && out.layout==3);
        assert(install_gpt_write(d,sector,&plan,true,&work));
        uint8_t *h=image+sector;w32(h+80,UINT32_MAX);checksum(h);
        assert(setup_inventory(readonly,sector,&out,&work) && out.layout==3);
        assert(install_gpt_write(d,sector,&plan,true,&work));
        memset(h+72,0xff,8);checksum(h);
        assert(setup_inventory(readonly,sector,&out,&work) && out.layout==3);
        assert(install_gpt_write(d,sector,&plan,true,&work));
        /* A CRC-correct primary GPT with overlapping partitions is still invalid. */
        memcpy(image+2*4096+128+32,image+2*4096+32,8);
        w32(h+88,crc(image+2*4096,16384));checksum(h);
        assert(setup_inventory(readonly,sector,&out,&work) && out.layout==3 && !out.partitions);
        free(image);
    }
    assert(!install_minimum_bytes(1024));
    puts("Read-only inventory, GPT CRC/name/capacity bounds, read failures and exact 512/4096 layout capacity thresholds passed");
}
