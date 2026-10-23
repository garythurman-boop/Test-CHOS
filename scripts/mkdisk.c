/* Build-time utility for disposable GPT/ESP boot images, not a guest installer.
 * Never opens an existing destination, device node or user-selected physical disk. */
#define _POSIX_C_SOURCE 200809L
#include "install.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
static bool read_block(void *ctx,uint64_t block,void *data) { FILE *f=ctx;return fseeko(f,(off_t)(block*4096),SEEK_SET)==0 && fread(data,1,4096,f)==4096; }
static bool write_block(void *ctx,uint64_t block,const void *data) { FILE *f=ctx;return fseeko(f,(off_t)(block*4096),SEEK_SET)==0 && fwrite(data,1,4096,f)==4096; }
static bool flush(void *ctx) { FILE *f=ctx;return fflush(f)==0 && fsync(fileno(f))==0; }
static uint8_t *load(const char *path,size_t *size) {
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseeko(f,0,SEEK_END) || ftello(f)<=0 || ftello(f)>512*1024){fclose(f);return 0;}
    *size=(size_t)ftello(f);rewind(f);uint8_t *p=malloc(*size);
    bool ok=p && fread(p,1,*size,f)==*size;fclose(f);if(!ok){free(p);return 0;}return p;
}
int main(int argc,char **argv) {
    if(argc!=5 || (strcmp(argv[4],"512") && strcmp(argv[4],"4096"))){fprintf(stderr,"Usage: mkdisk NEW.img BOOTAA64.EFI KERNEL.BIN 512|4096\n");return 1;}
    struct install_payload payload={0};payload.loader=load(argv[2],&payload.loader_size);payload.kernel=load(argv[3],&payload.kernel_size);
    if(!payload.loader || !payload.kernel){fprintf(stderr,"Missing, empty or oversized payload\n");return 1;}
    FILE *out=fopen(argv[1],"wx+b");if(!out){perror("Destination must be a new image file");return 1;}
    struct stat st;if(fstat(fileno(out),&st) || !S_ISREG(st.st_mode)){fclose(out);return 1;}
    uint32_t sector=(uint32_t)strtoul(argv[4],0,10);uint64_t blocks=sector==512?32768:131072;
    if(ftruncate(fileno(out),(off_t)(blocks*4096))){fclose(out);return 1;}
    const uint8_t guid[16]={0x43,0x48,0x4f,0x53,0xa1,0x64,0x45,0x41,0x82,0x10,0x20,0x26,0,0,0,1};
    struct install_plan plan;struct gpt_workspace *workspace=malloc(sizeof(*workspace));
    struct block_device disk={.context=out,.blocks=blocks,.read=read_block,.write=write_block,.flush=flush};
    bool ok=workspace && install_plan_make(blocks,sector,guid,&plan) && install_gpt_write(disk,sector,&plan,true,workspace) && install_fat32_write(disk,&plan,&payload,true,workspace) && install_gpt_verify(disk,sector,&plan,workspace);
    free(workspace);free((void *)payload.loader);free((void *)payload.kernel);if(fclose(out))ok=false;
    if(!ok){fprintf(stderr,"Image construction/verification failed; incomplete destination retained for diagnosis\n");return 1;}
    printf("Verified new GPT/FAT32 image: %s, %llu bytes, logical sector %u\n",argv[1],(unsigned long long)(blocks*4096),sector);return 0;
}
