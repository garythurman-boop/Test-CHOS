#include "setup.h"
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t u64(const uint8_t *p){return u32(p)|((uint64_t)u32(p+4)<<32);}
static uint32_t crc(const uint8_t *p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^((0u-(c&1))&0xedb88320u);}return ~c;}
/* Read-only inventory, including foreign GPT layouts. Only the primary GPT is
 * described here; the actual installer verifies both copies after writing. */
bool setup_inventory(struct block_device d,uint32_t sector,struct chos_destination *out,struct gpt_workspace *w){
    if(!out || !w || !d.read || d.blocks<8 || d.blocks>UINT64_MAX/4096 || (sector!=512 && sector!=4096))return false;
    *out=(struct chos_destination){.minimum_bytes=install_minimum_bytes(sector),.esp_bytes=(sector==512?64ULL:272ULL)*1048576,.system_bytes=4*1048576};
    if(!d.read(d.context,0,w->scratch) || !d.read(d.context,d.blocks-1,w->primary))return false;
    bool blank=true;for(unsigned i=0;i<4096;i++)if(w->scratch[i])blank=false;
    out->layout=blank?0:3;
    if(w->scratch[510]==0x55 && w->scratch[511]==0xaa)out->layout=1;
    if(!d.read(d.context,sector==4096?1:0,w->scratch))return false;
    uint8_t *h=w->scratch+(sector==4096?0:512);
    const char signature[]="EFI PART";for(unsigned i=0;i<8;i++)if(h[i]!=(uint8_t)signature[i])return true;
    out->layout=3;
    unsigned length=u32(h+12);uint64_t lbas=d.blocks*(4096/sector),table=u64(h+72);
    unsigned count=u32(h+80),entry_bytes=u32(h+84);uint32_t sum=u32(h+16),entries_crc=u32(h+88);
    uint64_t first_usable=u64(h+40),last_usable=u64(h+48);
    if(u32(h+8)!=0x10000 || length<92 || length>sector || u64(h+24)!=1 || u64(h+32)>=lbas || !count || count>128 || entry_bytes!=128 || table<2 || table>=lbas || (uint64_t)count*128>(lbas-table)*sector || first_usable<table+((uint64_t)count*128+sector-1)/sector || first_usable>last_usable || last_usable>=lbas-1)return true;
    for(unsigned i=0;i<4;i++)h[16+i]=0;
    if(crc(h,length)!=sum)return true;
    uint64_t address=table*sector;unsigned bytes=count*128,at=0;
    while(at<bytes){unsigned offset=(unsigned)(address%4096),n=4096-offset;if(n>bytes-at)n=bytes-at;
        if(!d.read(d.context,address/4096,w->scratch))return false;
        for(unsigned i=0;i<n;i++)w->entries[at+i]=w->scratch[offset+i];
        at+=n;address+=n;
    }
    if(crc(w->entries,bytes)!=entries_crc)return true;
    for(unsigned i=0;i<count;i++){
        const uint8_t *p=w->entries+i*128;bool used=false;for(unsigned j=0;j<16;j++)if(p[j])used=true;if(!used)continue;
        uint64_t first=u64(p+32),last=u64(p+40);if(first>last || first<first_usable || last>last_usable){out->partitions=0;return true;}
        for(unsigned j=0;j<i;j++){const uint8_t *q=w->entries+j*128;bool occupied=false;for(unsigned k=0;k<16;k++)if(q[k])occupied=true;
            if(occupied && first<=u64(q+40) && u64(q+32)<=last){out->partitions=0;return true;}}
        if(out->partitions<4){struct chos_partition *part=&out->existing[out->partitions];part->first_byte=first*sector;part->bytes=(last-first+1)*sector;
            for(unsigned j=0;j<35;j++){unsigned c=p[56+j*2]|(unsigned)p[57+j*2]<<8;if(!c)break;part->name[j]=c>=32 && c<127?(char)c:'?';}}
        out->partitions++;
    }
    out->layout=2;return true;
}
