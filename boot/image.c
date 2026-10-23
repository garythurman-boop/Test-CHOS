/* Original CHOS flat-image v1 contract. Sizes are offsets, never host pointers. */
#include "kernel_image.h"
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t u64(const uint8_t *p){return u32(p)|(uint64_t)u32(p+4)<<32;}
bool kernel_image_validate(const void *bytes,size_t available,uint64_t actual,struct kernel_image_layout *out){
    if(!bytes || !out || available<64 || actual<64 || actual>CHOS_IMAGE_MAX_FILE)return false;
    const uint8_t *p=bytes;
    if(u32(p)!=0x14000010 || u32(p+4) || u64(p+8)!=CHOS_IMAGE_MAGIC || u32(p+16)!=1 || u32(p+20)!=64 || u64(p+48) || u64(p+56))return false;
    uint64_t file=u64(p+24),memory=u64(p+32),entry=u64(p+40);
    if(file!=actual || memory<file || !memory || memory>CHOS_IMAGE_MAX_MEMORY || memory%4096 || entry!=64 || entry+4>file)return false;
    *out=(struct kernel_image_layout){file,memory,entry};return true;
}
bool kernel_boot_regions(uint64_t base,uint64_t size,uint64_t stack,uint64_t stack_size){
    const uint64_t limit=1ULL<<48;
    if(!base || base%4096 || !size || size%4096 || size>CHOS_IMAGE_MAX_MEMORY || base>=limit || size>limit-base || !stack || stack%4096 || stack_size!=CHOS_BOOT_STACK_ALLOCATION || stack>=limit || stack_size>limit-stack)return false;
    return base+size<=stack || stack+stack_size<=base;
}
