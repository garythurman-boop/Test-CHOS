#include "nvme_format.h"
static uint32_t le32(const uint8_t *b){return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
bool nvme_capabilities(uint64_t cap,struct nvme_capabilities *out){
    if(!out || (cap&65535)<15 || !(cap&(1ULL<<37)) || ((cap>>48)&15) || ((cap>>52)&15)<((cap>>48)&15) || ((cap>>32)&15)>7)return false;
    unsigned timeout=((cap>>24)&255)*500;if(timeout<500)timeout=500;if(timeout>10000)timeout=10000;
    *out=(struct nvme_capabilities){.stride=4u<<((cap>>32)&15),.timeout_ms=timeout};return true;
}
bool nvme_namespace(const uint8_t *b,unsigned size,struct nvme_namespace *out){
    if(!b || !out || size<4096 || b[25]>15 || (b[26]&0x60) || (b[26]&15)>b[25] || (b[29]&7))return false;
    unsigned fmt=b[26]&15;uint32_t lbaf=le32(b+128+fmt*4);unsigned shift=(lbaf>>16)&255;
    uint64_t sectors=le32(b)|((uint64_t)le32(b+4)<<32),capacity=le32(b+8)|((uint64_t)le32(b+12)<<32);
    if((lbaf&65535) || (shift!=9 && shift!=12) || sectors<128 || !capacity || capacity>sectors || sectors>UINT64_MAX/(1u<<shift))return false;
    *out=(struct nvme_namespace){.sectors=sectors,.sector_bytes=1u<<shift,.read_only=(b[99]&1)!=0};return true;
}

bool nvme_serial_normalize(const uint8_t *raw,unsigned size,char out[21]) {
    if(!raw || !out || size!=20)return false;
    unsigned end=20;while(end && (raw[end-1]==' ' || !raw[end-1]))end--;
    if(!end)return false;
    for(unsigned i=0;i<end;i++)if(raw[i]<32 || raw[i]>126)return false;
    for(unsigned i=0;i<21;i++)out[i]=i<end?(char)raw[i]:0;
    return true;
}
unsigned nvme_identity_compare(const struct nvme_identity *a,const struct nvme_identity *b) {
    if(!a || !b || !a->serial[0] || !b->serial[0])return NVME_CHECK_SERIAL_INVALID;
    for(unsigned i=0;i<21;i++)if(a->serial[i]!=b->serial[i])return NVME_CHECK_SERIAL_CHANGED;
    for(unsigned i=0;i<24;i++)if(a->namespace_id[i]!=b->namespace_id[i])return NVME_CHECK_NAMESPACE_ID;
    if(a->ns.sectors!=b->ns.sectors)return NVME_CHECK_SECTORS;
    if(a->ns.sector_bytes!=b->ns.sector_bytes)return NVME_CHECK_SECTOR_SIZE;
    if(b->ns.read_only)return NVME_CHECK_READONLY;
    return NVME_CHECK_OK;
}
const char *nvme_check_message(unsigned code) {
    static const char *const text[]={"Identity and writable state verified.",
        "NVMe controller is no longer ready.","NVMe controller Identify failed.",
        "NVMe serial is empty or malformed.","NVMe serial identity changed.",
        "NVMe namespace Identify failed.","NVMe namespace format is invalid.",
        "NVMe namespace NGUID or EUI64 changed.","NVMe sector count changed.",
        "NVMe sector size changed.","NVMe namespace is write protected.",
        "Read-only diagnostic policy blocks writes."};
    return text[code<=NVME_CHECK_POLICY?code:NVME_CHECK_NOT_READY];
}
