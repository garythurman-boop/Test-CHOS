/* ACPI 6.x RSDP/XSDT, MADT GICC/GICD and GTDT subset. Byte reads deliberately
 * avoid unaligned native loads in packed firmware structures. */
#include "platform.h"
#include "memory.h"
static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static uint64_t u64(const uint8_t *p) { return u32(p)|((uint64_t)u32(p+4)<<32); }
static bool same(const uint8_t *p,const char *s,unsigned n) {
    for(unsigned i=0;i<n;i++) if(p[i]!=(uint8_t)s[i]) return false;
    return true;
}
static bool memory(const struct chos_boot_info *b,uint64_t address,uint64_t length) {
    if(!address || !length || length>UINT64_MAX-address) return false;
    if(b->descriptor_size<40 || b->descriptor_size%8 || b->memory_map%8 ||
       !b->memory_map_size || b->memory_map_size%b->descriptor_size ||
       b->memory_map_size/b->descriptor_size>4096) return false;
    for(uint64_t off=0;off<b->memory_map_size;off+=b->descriptor_size) {
        const struct memory_descriptor *d=(void *)(uintptr_t)(b->memory_map+off);
        if(d->type<1 || d->type>10 || d->pages>(UINT64_MAX-d->physical)/PAGE_SIZE) continue;
        uint64_t end=d->physical+d->pages*PAGE_SIZE;
        if(address>=d->physical && address<end && length<=end-address) return true;
    }
    return false;
}
static bool checksum(const uint8_t *p,size_t len) {
    uint8_t sum=0;
    for(size_t i=0;i<len;i++) sum=(uint8_t)(sum+p[i]);
    return sum==0;
}
static const uint8_t *table(const struct chos_boot_info *b,uint64_t address) {
    if(!memory(b,address,36)) return 0;
    const uint8_t *p=(void *)(uintptr_t)address;
    uint32_t len=u32(p+4);
    if(len<36 || len>1024*1024 || !memory(b,address,len) || !checksum(p,len)) return 0;
    return p;
}
bool platform_discover(const struct chos_boot_info *b,uint64_t mpidr,struct platform *out) {
    *out=(struct platform){0};
    if(!memory(b,b->rsdp,36)) return false;
    const uint8_t *rsdp=(void *)(uintptr_t)b->rsdp;
    if(!same(rsdp,"RSD PTR ",8) || !checksum(rsdp,20) || rsdp[15]<2) return false;
    uint32_t rsdp_len=u32(rsdp+20);
    if(rsdp_len<36 || rsdp_len>4096 || !memory(b,b->rsdp,rsdp_len) || !checksum(rsdp,rsdp_len)) return false;
    const uint8_t *xsdt=table(b,u64(rsdp+24));
    if(!xsdt || !same(xsdt,"XSDT",4)) return false;
    uint32_t len=u32(xsdt+4);
    if((len-36)%8 || (len-36)/8>1024) return false;
    bool got_madt=false,got_timer=false,got_cpu=false;
    for(uint32_t i=36;i<len;i+=8) {
        const uint8_t *t=table(b,u64(xsdt+i));
        if(!t) return false;
        if(same(t,"APIC",4)) {
            if(got_madt) return false;
            got_madt=true;
            uint32_t size=u32(t+4);
            if(size<44) return false;
            for(uint32_t j=44;j<size;) {
                if(size-j<2 || t[j+1]<2 || t[j+1]>size-j) return false;
                const uint8_t *e=t+j;
                if(e[0]==11) {
                    if(e[1]<76) return false;
                    if((u32(e+12)&1) && (u64(e+68)&0xff00ffffffULL)==(mpidr&0xff00ffffffULL)) {
                        if(got_cpu) return false;
                        got_cpu=true; out->gic_cpu=u64(e+32);
                    }
                } else if(e[0]==12) {
                    if(e[1]<24 || out->gic_distributor) return false;
                    out->gic_distributor=u64(e+8); out->gic_version=e[20];
                }
                j+=e[1];
            }
        } else if(same(t,"GTDT",4)) {
            if(got_timer || u32(t+4)<80) return false;
            got_timer=true; out->timer_irq=u32(t+56); out->timer_flags=u32(t+60);
        }
    }
    return got_madt && got_timer && got_cpu && out->gic_version==2 &&
           out->gic_distributor && out->gic_cpu &&
           !(out->gic_distributor%PAGE_SIZE) && !(out->gic_cpu%PAGE_SIZE) &&
           out->gic_distributor<(1ULL<<39)-0x10000 && out->gic_cpu<(1ULL<<39)-0x2000 &&
           out->timer_irq>=16 && out->timer_irq<=31 && !(out->timer_flags&2);
}

bool platform_pcie(const struct chos_boot_info *b,uint64_t *base) {
    if(!memory(b,b->rsdp,36)) return false;
    const uint8_t *r=(void *)(uintptr_t)b->rsdp;
    uint32_t rsdp_len=u32(r+20);
    if(!same(r,"RSD PTR ",8) || r[15]<2 || !checksum(r,20) || rsdp_len<36 || rsdp_len>4096 ||
       !memory(b,b->rsdp,rsdp_len) || !checksum(r,rsdp_len)) return false;
    const uint8_t *x=table(b,u64(r+24));
    if(!x || !same(x,"XSDT",4)) return false;
    uint32_t length=u32(x+4);
    if((length-36)%8 || (length-36)/8>1024) return false;
    for(uint32_t i=36;i<length;i+=8) {
        const uint8_t *t=table(b,u64(x+i));
        if(!t || !same(t,"MCFG",4)) continue;
        uint32_t n=u32(t+4);
        if(n<60 || (n-44)%16) return false;
        for(uint32_t j=44;j<n;j+=16) {
            if(t[j+8] || t[j+9] || t[j+10] || t[j+11]<t[j+10]) continue;
            uint64_t address=u64(t+j);
            if(!address || address%0x100000 || address>=(1ULL<<39)-0x100000) return false;
            *base=address; return true;
        }
    }
    return false;
}

/* ACPI FADT ARM_BOOT_ARCH specifies PSCI compliance and its call conduit. */
bool platform_psci(const struct chos_boot_info *b,bool *hvc) {
    if(!memory(b,b->rsdp,36))return false;
    const uint8_t *r=(void *)(uintptr_t)b->rsdp;uint32_t n=u32(r+20);
    if(!same(r,"RSD PTR ",8) || r[15]<2 || !checksum(r,20) || n<36 || n>4096 || !memory(b,b->rsdp,n) || !checksum(r,n))return false;
    const uint8_t *x=table(b,u64(r+24));if(!x || !same(x,"XSDT",4))return false;
    n=u32(x+4);if((n-36)%8 || (n-36)/8>1024)return false;
    for(uint32_t i=36;i<n;i+=8) {
        const uint8_t *t=table(b,u64(x+i));if(!t || !same(t,"FACP",4))continue;
        if(u32(t+4)<132 || t[8]<5 || !(t[129]&1))return false;
        *hvc=(t[129]&2)!=0;return true;
    }
    return false;
}
