#include "kernel_image.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void w(uint8_t *p,uint64_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(i*8));}
int main(void){
    uint8_t h[64]={0};w(h,0x14000010,4);w(h+8,CHOS_IMAGE_MAGIC,8);w(h+16,1,4);w(h+20,64,4);w(h+24,1048640,8);w(h+32,5*1048576,8);w(h+40,64,8);
    struct kernel_image_layout out;assert(kernel_image_validate(h,64,1048640,&out) && out.memory_bytes==5*1048576);
    for(unsigned n=0;n<64;n++)assert(!kernel_image_validate(h,n,1048640,&out));
    for(unsigned i=0;i<64;i++){if(i>=32 && i<40)continue;uint8_t copy[64];memcpy(copy,h,64);copy[i]^=128;assert(!kernel_image_validate(copy,64,1048640,&out));}
    const uint64_t bad_memory[]={0,4096,5*1048576+1,CHOS_IMAGE_MAX_MEMORY+4096,UINT64_MAX};
    for(unsigned i=0;i<sizeof(bad_memory)/sizeof(bad_memory[0]);i++){uint8_t copy[64];memcpy(copy,h,64);w(copy+32,bad_memory[i],8);assert(!kernel_image_validate(copy,64,1048640,&out));}
    uint8_t largest[64];memcpy(largest,h,64);w(largest+32,CHOS_IMAGE_MAX_MEMORY,8);assert(kernel_image_validate(largest,64,1048640,&out));
    assert(!kernel_boot_regions(0x40000000,4096,0x50000000,8192));
    assert(!kernel_image_validate(h,64,1048640,0));
    assert(!kernel_image_validate(h,64,1048641,&out) && !kernel_image_validate(0,64,1048640,&out));
    assert(kernel_boot_regions(0x40000000,5*1048576,0x50000000,CHOS_BOOT_STACK_ALLOCATION));
    assert(!kernel_boot_regions(0x40000000,5*1048576,0x40001000,CHOS_BOOT_STACK_ALLOCATION));
    assert(!kernel_boot_regions(UINT64_MAX-4095,4096,0x50000000,CHOS_BOOT_STACK_ALLOCATION));
    assert(!kernel_boot_regions(0x40000001,4096,0x50000000,CHOS_BOOT_STACK_ALLOCATION));
    puts("Image v1 header/version/size/reserved/entry/truncation validation and disjoint aligned bounded image/stack regions passed");
}
