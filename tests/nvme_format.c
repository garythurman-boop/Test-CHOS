#include "nvme_format.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put(uint8_t *p,unsigned off,uint32_t n){for(unsigned i=0;i<4;i++)p[off+i]=(uint8_t)(n>>(i*8));}
int main(void){
 struct nvme_capabilities c;uint64_t cap=(1ULL<<37)|15|(1ULL<<24);
 assert(nvme_capabilities(cap,&c) && c.stride==4 && c.timeout_ms==500);
 assert(nvme_capabilities(cap|(255ULL<<24)|(7ULL<<32),&c) && c.stride==512 && c.timeout_ms==10000);
 assert(!nvme_capabilities(14|(1ULL<<37),&c) && !nvme_capabilities(15,&c));
 assert(!nvme_capabilities(cap|(1ULL<<48),&c) && !nvme_capabilities(cap|(8ULL<<32),&c));
 uint8_t b[4096]={0};struct nvme_namespace n;put(b,0,4096);put(b,8,4096);put(b,128,9u<<16);
 assert(nvme_namespace(b,sizeof(b),&n) && n.sectors==4096 && n.sector_bytes==512 && !n.read_only);
 b[99]=1;put(b,128,12u<<16);assert(nvme_namespace(b,sizeof(b),&n) && n.read_only && n.sector_bytes==4096);
 assert(!nvme_namespace(b,4095,&n));b[26]=1;assert(!nvme_namespace(b,sizeof(b),&n));b[26]=0;
 b[29]=1;assert(!nvme_namespace(b,sizeof(b),&n));b[29]=0;put(b,128,12u<<16|1);assert(!nvme_namespace(b,sizeof(b),&n));
 put(b,128,10u<<16);assert(!nvme_namespace(b,sizeof(b),&n));put(b,128,12u<<16);put(b,8,8192);assert(!nvme_namespace(b,sizeof(b),&n));
 put(b,8,4096);put(b,0,0xffffffff);put(b,4,0xffffffff);assert(!nvme_namespace(b,sizeof(b),&n));
 puts("NVMe CAP queue/CSS/granule/stride/timeout and bounded namespace capacity/LBA-format/metadata/protection/overflow parsing passed");
}
