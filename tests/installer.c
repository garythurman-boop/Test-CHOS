#include "install.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct sector {uint64_t index;uint8_t bytes[4096];};
struct fixture {struct sector sectors[20];unsigned count,writes,flushes,reads;int fail_write,fail_flush;};
static struct gpt_workspace workspace;
static bool read_block(void *ctx,uint64_t block,void *data) {
    struct fixture *f=ctx;f->reads++;
    for(unsigned i=0;i<f->count;i++)if(f->sectors[i].index==block){memcpy(data,f->sectors[i].bytes,4096);return true;}
    memset(data,0xa5,4096);return true;
}
static bool write_block(void *ctx,uint64_t block,const void *data) {
    struct fixture *f=ctx;
    if(f->fail_write>=0 && f->writes++==(unsigned)f->fail_write)return false;
    unsigned i=0;while(i<f->count && f->sectors[i].index!=block)i++;
    if(i==f->count){assert(i<20);f->count++;f->sectors[i].index=block;}
    memcpy(f->sectors[i].bytes,data,4096);return true;
}
static bool flush(void *ctx) {struct fixture *f=ctx;return f->fail_flush<0 || f->flushes++!=(unsigned)f->fail_flush;}
static void reset(struct fixture *f){memset(f,0,sizeof(*f));f->fail_write=f->fail_flush=-1;}
int main(int argc,char **argv) {
    assert(argc==2);struct fixture *f=calloc(1,sizeof(*f));assert(f);
    uint8_t guid[16]={1,2,3,4,5,6,0x47,0x48,0x89,10,11,12,13,14,15,16},empty[16]={0};
    struct install_plan p;
    assert(!install_plan_make(32768,512,empty,&p));
    assert(!install_plan_make(32768,1024,guid,&p));
    assert(!install_plan_make(UINT64_MAX,512,guid,&p));
    assert(!install_plan_make(1024,512,guid,&p));
    assert(!install_plan_make(32768,4096,guid,&p));
    for(unsigned case_id=0;case_id<2;case_id++) {
        uint32_t bytes=case_id?4096:512;uint64_t blocks=case_id?131072:32768;
        reset(f);struct block_device d={.context=f,.blocks=blocks,.read=read_block,.write=write_block,.flush=flush};
        assert(install_plan_make(blocks,bytes,guid,&p));
        assert(p.esp_first*bytes==1048576 && p.system_first>p.esp_last && p.system_last<blocks*(4096/bytes));
        assert(!install_gpt_write(d,bytes,&p,false,&workspace) && !f->writes && !f->reads && !f->count);
        struct install_plan invalid=p;invalid.blocks++;assert(!install_gpt_write(d,bytes,&invalid,true,&workspace) && !f->count);
        invalid=p;invalid.esp_last++;assert(!install_gpt_write(d,bytes,&invalid,true,&workspace) && !f->count);
        assert(!install_gpt_write(d,bytes==512?4096:512,&p,true,&workspace) && !f->count);
        assert(install_gpt_write(d,bytes,&p,true,&workspace));
        assert(install_gpt_verify(d,bytes,&p,&workspace));
        struct install_plan recovered;
        assert(install_gpt_probe(d,bytes,&recovered,&workspace));
        assert(recovered.blocks==p.blocks && recovered.esp_first==p.esp_first && recovered.system_first==p.system_first);
        struct block_device readonly=d;readonly.write=0;readonly.flush=0;
        assert(install_gpt_verify(readonly,bytes,&p,&workspace));
        assert(!install_gpt_write(readonly,bytes,&p,true,&workspace));
        for(unsigned i=0;i<f->count;i++) {
            uint64_t block=f->sectors[i].index;
            assert(block<=5 || block>=blocks-5); /* no partition payload was touched */
            if(block==0 && bytes==512)assert(f->sectors[i].bytes[1024]==0xa5);
        }
        char name[1024];assert(snprintf(name,sizeof(name),"%s-%u.img",argv[1],bytes)>0);
        FILE *out=fopen(name,"wx");assert(out);assert(fseek(out,(long)(blocks*4096-1),SEEK_SET)==0 && fputc(0,out)!=EOF);
        for(unsigned i=0;i<f->count;i++){assert(fseek(out,(long)(f->sectors[i].index*4096),SEEK_SET)==0);assert(fwrite(f->sectors[i].bytes,1,4096,out)==4096);}
        assert(fclose(out)==0);
        f->sectors[0].bytes[200]^=1;assert(!install_gpt_verify(d,bytes,&p,&workspace));assert(!install_gpt_probe(d,bytes,&recovered,&workspace));
        for(int failure=0;failure<11;failure++) {
            reset(f);f->fail_write=failure;
            assert(!install_gpt_write(d,bytes,&p,true,&workspace));
        }
        for(int failure=0;failure<5;failure++) {
            reset(f);f->fail_flush=failure;
            assert(!install_gpt_write(d,bytes,&p,true,&workspace));
        }
    }
    free(f);puts("GPT planning, unconfirmed/changed-geometry refusal, 512/4096 sectors, write/flush errors and read-back verification passed");
}
