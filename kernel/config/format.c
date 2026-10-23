#include "config.h"
static void put(unsigned char *p,uint32_t n) {for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(i*8));}
static uint32_t get(const unsigned char *p) {uint32_t n=0;for(unsigned i=0;i<4;i++)n|=(uint32_t)p[i]<<(i*8);return n;}
static uint32_t checksum(const unsigned char *p) {
    uint32_t crc=~0u;
    for(unsigned i=0;i<CHOS_CONFIG_BYTES;i++) {
        crc^=i>=12 && i<16?0:p[i];
        for(unsigned j=0;j<8;j++)crc=(crc>>1)^((0u-(crc&1))&0xedb88320u);
    }
    return ~crc;
}
void config_defaults(struct chos_config *c) {*c=(struct chos_config){.version=1,.computer="CHOS-PC"};}
bool config_valid(const struct chos_config *c) {
    if(!c || c->version!=1 || c->theme>1 || c->utc_offset < -720 || c->utc_offset>840 || c->utc_offset%15 || c->clock_seconds>1 || !c->computer[0])return false;
    for(unsigned i=0;i<4;i++)if(c->reserved[i])return false;
    unsigned length=0;
    for(;length<32 && c->computer[length];length++) {
        unsigned char a=(unsigned char)c->computer[length];
        if(!((a>='A' && a<='Z') || (a>='a' && a<='z') || (a>='0' && a<='9') || a=='-'))return false;
    }
    return length<32;
}
bool config_encode(const struct chos_config *c,unsigned char out[CHOS_CONFIG_BYTES]) {
    if(!config_valid(c) || !out)return false;
    for(unsigned i=0;i<CHOS_CONFIG_BYTES;i++)out[i]=0;
    const char magic[]="CHOSCFG1";for(unsigned i=0;i<8;i++)out[i]=(unsigned char)magic[i];
    put(out+8,CHOS_CONFIG_BYTES);put(out+16,c->version);put(out+20,c->theme);put(out+24,(uint32_t)c->utc_offset);put(out+28,c->clock_seconds);
    for(unsigned i=0;i<31 && c->computer[i];i++)out[32+i]=(unsigned char)c->computer[i];
    put(out+12,checksum(out));return true;
}
bool config_decode(struct chos_config *out,const void *data,size_t size) {
    if(!out || !data || size!=CHOS_CONFIG_BYTES)return false;
    const unsigned char *p=data;const char magic[]="CHOSCFG1";
    for(unsigned i=0;i<8;i++)if(p[i]!=(unsigned char)magic[i])return false;
    if(get(p+8)!=CHOS_CONFIG_BYTES || get(p+12)!=checksum(p))return false;
    struct chos_config c={.version=get(p+16),.theme=get(p+20),.utc_offset=(int32_t)get(p+24),.clock_seconds=get(p+28)};
    for(unsigned i=0;i<32;i++)c.computer[i]=(char)p[32+i];
    for(unsigned i=0;i<4;i++)c.reserved[i]=get(p+64+i*4);
    if(!config_valid(&c))return false;
    *out=c;return true;
}
unsigned config_clock(unsigned seconds,int offset) {
    /* Offsets are minutes relative to firmware time; fixed offset, no DST rule. */
    int64_t value=(int64_t)(seconds%86400)+(int64_t)offset*60;
    value%=86400;if(value<0)value+=86400;return (unsigned)value;
}
