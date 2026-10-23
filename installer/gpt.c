/* Original GPT encoder for CHOS installation. No disk discovery or implicit erase.
 * The caller must bind identity/confirmation in the eventual graphical installer.
 * This module additionally rejects unconfirmed writes and changed geometry.
 */
#include "install.h"
static uint32_t crc(const uint8_t *p,size_t n) {
    uint32_t value=~0u;
    for(size_t i=0;i<n;i++){value^=p[i];for(unsigned j=0;j<8;j++)value=(value>>1)^((0u-(value&1))&0xedb88320u);}
    return ~value;
}
static void zero(uint8_t *p,size_t n){for(size_t i=0;i<n;i++)p[i]=0;}
static void bytes(uint8_t *p,const uint8_t *q,size_t n){for(size_t i=0;i<n;i++)p[i]=q[i];}
static bool same(const uint8_t *a,const uint8_t *b,size_t n){for(size_t i=0;i<n;i++)if(a[i]!=b[i])return false;return true;}
static void w32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static void w64(uint8_t *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(uint8_t)(n>>(8*i));}
static bool nonzero(const uint8_t *p){for(unsigned i=0;i<16;i++)if(p[i])return true;return false;}
uint64_t install_minimum_bytes(uint32_t sector) {
    if(sector!=512 && sector!=4096)return 0;
    /* 1 MiB alignment + FAT32 ESP + 4 MiB CHFS + five 4 KiB backup blocks. */
    return (1ULL+(sector==512?64:272)+4)*1048576+5*4096;
}
bool install_plan_make(uint64_t blocks,uint32_t sector,const uint8_t guid[16],struct install_plan *p) {
    /* Bound byte arithmetic, GPT LBAs and the existing allocator/driver contract. */
    if(!p || !guid || !nonzero(guid) || (sector!=512 && sector!=4096) || blocks>UINT64_MAX/4096 || blocks<8 || blocks<install_minimum_bytes(sector)/4096)return false;
    uint64_t sectors=blocks*(4096/sector),align=1048576/sector;
    uint64_t esp_bytes=sector==512?64ULL*1048576:272ULL*1048576;
    uint64_t first=align,last=first+esp_bytes/sector-1,system=last+1;
    uint64_t backup_array=(blocks-5)*(4096/sector),last_usable=backup_array-1;
    if(system>last_usable || last_usable-system+1<4*1048576/sector || sectors<last_usable)return false;
    *p=(struct install_plan){.blocks=blocks,.sector_bytes=sector,.esp_first=first,.esp_last=last,.system_first=system,.system_last=last_usable};
    bytes(p->disk_guid,guid,16);return true;
}
static bool plan_valid(struct block_device d,uint32_t sector,const struct install_plan *p) {
    if(!p || !d.read || p->blocks!=d.blocks || p->sector_bytes!=sector)return false;
    struct install_plan expected;
    if(!install_plan_make(d.blocks,sector,p->disk_guid,&expected))return false;
    return p->esp_first==expected.esp_first && p->esp_last==expected.esp_last && p->system_first==expected.system_first && p->system_last==expected.system_last;
}
static void entry(uint8_t *p,const uint8_t type[16],const struct install_plan *plan,unsigned which) {
    bytes(p,type,16);bytes(p+16,plan->disk_guid,16);
    /* Distinct deterministic partition GUIDs, preserving the caller's UUID bits. */
    p[16]^=0xa5;p[31]^=(uint8_t)(which+1);
    if(!nonzero(p+16))p[16]=1;
    w64(p+32,which?plan->system_first:plan->esp_first);w64(p+40,which?plan->system_last:plan->esp_last);
    const char *name=which?"CHOS System":"CHOS EFI";
    for(unsigned i=0;name[i];i++)p[56+i*2]=(uint8_t)name[i];
}
static void header(uint8_t *h,const struct install_plan *p,bool backup,uint32_t entries_crc) {
    zero(h,4096);bytes(h,(const uint8_t *)"EFI PART",8);w32(h+8,0x10000);w32(h+12,92);
    uint64_t scale=4096/p->sector_bytes,last=p->blocks*scale-1;
    w64(h+24,backup?last:1);w64(h+32,backup?1:last);
    w64(h+40,2*scale+16384/p->sector_bytes);w64(h+48,(p->blocks-5)*scale-1);
    bytes(h+56,p->disk_guid,16);w64(h+72,backup?(p->blocks-5)*scale:2*scale);
    w32(h+80,128);w32(h+84,128);w32(h+88,entries_crc);w32(h+16,crc(h,92));
}
static void encode(const struct install_plan *p,struct gpt_workspace *w) {
    static const uint8_t esp[16]={0x28,0x73,0x2a,0xc1,0x1f,0xf8,0xd2,0x11,0xba,0x4b,0x00,0xa0,0xc9,0x3e,0xc9,0x3b};
    static const uint8_t system[16]={0x50,0x05,0xec,0xc4,0x1d,0x3c,0x65,0x4e,0x9a,0xbd,0x43,0x48,0x4f,0x53,0,1};
    zero(w->entries,sizeof(w->entries));entry(w->entries,esp,p,0);entry(w->entries+128,system,p,1);
    uint32_t sum=crc(w->entries,sizeof(w->entries));header(w->primary,p,false,sum);header(w->backup,p,true,sum);
}
static bool write_header(struct block_device d,const struct install_plan *p,bool backup,struct gpt_workspace *w) {
    uint64_t block=backup?d.blocks-1:(p->sector_bytes==4096?1:0);
    unsigned offset=backup?4096-p->sector_bytes:(p->sector_bytes==4096?0:p->sector_bytes);
    if(!d.read(d.context,block,w->scratch))return false;
    bytes(w->scratch+offset,backup?w->backup:w->primary,p->sector_bytes);
    return d.write(d.context,block,w->scratch) && d.flush(d.context);
}
bool install_gpt_write(struct block_device d,uint32_t sector,const struct install_plan *p,bool confirmed,struct gpt_workspace *w) {
    if(!confirmed || !w || !d.write || !d.flush || !plan_valid(d,sector,p))return false;
    encode(p,w);
    /* Backup first; report any uncertainty instead of claiming a rollback.
     * Existing partitions may be partly replaced on any failure after this point. */
    for(unsigned i=0;i<4;i++)if(!d.write(d.context,d.blocks-5+i,w->entries+i*4096))return false;
    if(!d.flush(d.context) || !write_header(d,p,true,w))return false;
    for(unsigned i=0;i<4;i++)if(!d.write(d.context,2+i,w->entries+i*4096))return false;
    if(!d.flush(d.context) || !write_header(d,p,false,w))return false;
    if(!d.read(d.context,0,w->scratch))return false;
    zero(w->scratch,512);
    w->scratch[446+4]=0xee;w->scratch[447]=0;w->scratch[448]=2;w->scratch[449]=0;
    w->scratch[451]=255;w->scratch[452]=255;w->scratch[453]=255;
    w32(w->scratch+454,1);
    uint64_t sectors=d.blocks*(4096/sector)-1;w32(w->scratch+458,sectors>UINT32_MAX?UINT32_MAX:(uint32_t)sectors);
    w->scratch[510]=0x55;w->scratch[511]=0xaa;
    return d.write(d.context,0,w->scratch) && d.flush(d.context) && install_gpt_verify(d,sector,p,w);
}
bool install_gpt_verify(struct block_device d,uint32_t sector,const struct install_plan *p,struct gpt_workspace *w) {
    if(!w || !plan_valid(d,sector,p))return false;
    encode(p,w);
    if(!d.read(d.context,0,w->scratch) || w->scratch[510]!=0x55 || w->scratch[511]!=0xaa || w->scratch[450]!=0xee)return false;
    /* A 4 KiB logical sector places the primary header in block 1. */
    uint64_t primary=sector==4096?1:0;unsigned offset=sector==4096?0:sector;
    if(!d.read(d.context,primary,w->scratch) || !same(w->scratch+offset,w->primary,sector))return false;
    if(!d.read(d.context,d.blocks-1,w->scratch) || !same(w->scratch+4096-sector,w->backup,sector))return false;
    for(unsigned i=0;i<4;i++) {
        if(!d.read(d.context,2+i,w->scratch) || !same(w->scratch,w->entries+i*4096,4096))return false;
        if(!d.read(d.context,d.blocks-5+i,w->scratch) || !same(w->scratch,w->entries+i*4096,4096))return false;
    }
    return true;
}

bool install_gpt_probe(struct block_device d,uint32_t sector,struct install_plan *p,struct gpt_workspace *w) {
    if(!p || !w || !d.read || (sector!=512 && sector!=4096))return false;
    if(!d.read(d.context,sector==4096?1:0,w->scratch))return false;
    uint8_t *h=w->scratch+(sector==4096?0:512);uint8_t guid[16];bytes(guid,h+56,16);
    if(!install_plan_make(d.blocks,sector,guid,p))return false;
    return install_gpt_verify(d,sector,p,w);
}
