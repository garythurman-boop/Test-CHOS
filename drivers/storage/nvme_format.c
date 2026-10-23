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
