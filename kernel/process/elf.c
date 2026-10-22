#include "elf.h"
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0]|p[1]<<8); }
static uint32_t u32(const uint8_t *p) { return u16(p)|((uint32_t)u16(p+2)<<16); }
static uint64_t u64(const uint8_t *p) { return u32(p)|((uint64_t)u32(p+4)<<32); }
bool elf_validate(const void *image,size_t length,struct elf_plan *out) {
    *out=(struct elf_plan){0};
    if(!image || length<64) return false;
    const uint8_t *p=image;
    if(p[0]!=0x7f || p[1]!='E' || p[2]!='L' || p[3]!='F' || p[4]!=2 || p[5]!=1 || p[6]!=1 ||
       u16(p+16)!=2 || u16(p+18)!=183 || u32(p+20)!=1 || u32(p+48)!=0 ||
       u16(p+52)!=64 || u16(p+54)!=56) return false;
    uint64_t ph=u64(p+32),entry=u64(p+24);
    unsigned count=u16(p+56);
    if(!count || count>8 || ph<64 || ph>length || count>(length-ph)/56) return false;
    bool executable_entry=false;
    for(unsigned i=0;i<count;i++) {
        const uint8_t *h=p+ph+i*56;
        uint32_t type=u32(h),flags=u32(h+4);
        if(type!=1) return false; /* Strict subset: PT_LOAD only; no interpreter/dynamic objects. */
        uint64_t off=u64(h+8),va=u64(h+16),files=u64(h+32),mem=u64(h+40),align=u64(h+48);
        if(!(flags&4) || flags&~7u || (flags&3)==3 || !mem || mem>65536 || files>mem ||
           off>length || files>length-off || va<USER_BASE || va>=USER_IMAGE_LIMIT ||
           mem>USER_IMAGE_LIMIT-va || va%4096 || off%4096 || align!=4096) return false;
        uint64_t end=(va+mem+4095)&~UINT64_C(4095);
        for(unsigned j=0;j<out->count;j++) {
            const struct elf_segment *s=&out->segments[j];
            uint64_t other_end=(s->address+s->memory_size+4095)&~UINT64_C(4095);
            if(va<other_end && s->address<end) return false;
        }
        out->segments[out->count++]=(struct elf_segment){va,off,files,mem,flags};
        if((flags&1) && entry>=va && entry<va+files) executable_entry=true;
    }
    if(!executable_entry || entry%4) return false;
    out->entry=entry;
    return true;
}
