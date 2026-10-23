/* Minimal original FAT32 ESP writer: two mirrored FATs, four directories,
 * BOOTAA64.EFI and CHOS/KERNEL.BIN. This is not a runtime FAT filesystem driver. */
#include "install.h"
#include "abi.h"
struct layout {uint64_t base,bytes,data;uint32_t sector,sectors,fat,clusters,loader_first,loader_count,kernel_first,kernel_count;};
static void zero(uint8_t *p,size_t n){for(size_t i=0;i<n;i++)p[i]=0;}
static void copy(uint8_t *p,const uint8_t *q,size_t n){for(size_t i=0;i<n;i++)p[i]=q[i];}
static bool same(const uint8_t *p,const uint8_t *q,size_t n){for(size_t i=0;i<n;i++)if(p[i]!=q[i])return false;return true;}
static void w16(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void w32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static bool layout(struct block_device d,const struct install_plan *p,const struct install_payload *files,struct layout *l) {
    if(!p || !files || !files->loader || !files->kernel || !d.read || d.blocks!=p->blocks || !files->loader_size || !files->kernel_size || files->loader_size>512*1024 || files->kernel_size>512*1024)return false;
    struct install_plan expected;
    if(!install_plan_make(d.blocks,p->sector_bytes,p->disk_guid,&expected) || p->esp_first!=expected.esp_first || p->esp_last!=expected.esp_last || p->system_first!=expected.system_first || p->system_last!=expected.system_last)return false;
    uint32_t b=p->sector_bytes,total=(uint32_t)(p->esp_last-p->esp_first+1),scale=4096/b;
    uint32_t fat=(uint32_t)(((uint64_t)(total-32+2)*4+b-1)/b);
    fat=(fat+scale-1)/scale*scale;
    uint32_t clusters=total-32-2*fat;
    if(clusters<65525 || (uint64_t)fat*b/4<clusters+2)return false;
    uint32_t loader_count=(uint32_t)((files->loader_size+b-1)/b),kernel_count=(uint32_t)((files->kernel_size+b-1)/b);
    if(4+loader_count+kernel_count>clusters)return false;
    *l=(struct layout){.base=p->esp_first*b,.bytes=(uint64_t)total*b,.data=(uint64_t)(32+2*fat)*b,.sector=b,.sectors=total,.fat=fat,.clusters=clusters,.loader_first=6,.loader_count=loader_count,.kernel_first=6+loader_count,.kernel_count=kernel_count};
    return true;
}
static bool write_span(struct block_device d,const struct layout *l,uint64_t offset,const uint8_t *data,size_t n,uint8_t *scratch) {
    if(offset>l->bytes || n>l->bytes-offset)return false;
    uint64_t address=l->base+offset;
    while(n) {
        unsigned at=(unsigned)(address%4096);size_t count=4096-at;if(count>n)count=n;
        if(at || count!=4096){if(!d.read(d.context,address/4096,scratch))return false;}
        copy(scratch+at,data,count);
        if(!d.write(d.context,address/4096,scratch))return false;
        address+=count;data+=count;n-=count;
    }
    return true;
}
static bool verify_span(struct block_device d,const struct layout *l,uint64_t offset,const uint8_t *data,size_t n,uint8_t *scratch) {
    if(offset>l->bytes || n>l->bytes-offset)return false;
    uint64_t address=l->base+offset;
    while(n) {
        unsigned at=(unsigned)(address%4096);size_t count=4096-at;if(count>n)count=n;
        if(!d.read(d.context,address/4096,scratch) || !same(scratch+at,data,count))return false;
        address+=count;data+=count;n-=count;
    }
    return true;
}
static void boot_sector(uint8_t *s,const struct layout *l,const struct install_plan *p) {
    zero(s,4096);s[0]=0xeb;s[1]=0x58;s[2]=0x90;copy(s+3,(const uint8_t *)"CHOS    ",8);
    w16(s+11,(uint16_t)l->sector);s[13]=1;w16(s+14,32);s[16]=2;s[21]=0xf8;
    w16(s+24,63);w16(s+26,255);w32(s+28,(uint32_t)p->esp_first);w32(s+32,l->sectors);w32(s+36,l->fat);
    w32(s+44,2);w16(s+48,1);w16(s+50,6);s[64]=0x80;s[66]=0x29;copy(s+67,p->disk_guid,4);
    copy(s+71,(const uint8_t *)"CHOS EFI   ",11);copy(s+82,(const uint8_t *)"FAT32   ",8);s[510]=0x55;s[511]=0xaa;
}
static void fsinfo(uint8_t *s) {
    zero(s,4096);w32(s,0x41615252);w32(s+484,0x61417272);w32(s+488,0xffffffff);w32(s+492,0xffffffff);w32(s+508,0xaa550000);
}
static void fat_block(uint8_t *s,unsigned block,const struct layout *l) {
    zero(s,4096);
    for(unsigned i=0;i<1024;i++) {
        uint32_t cluster=block*1024+i,value=0;
        if(cluster==0)value=0x0ffffff8;
        else if(cluster==1 || (cluster>=2 && cluster<6))value=0x0fffffff;
        else if(cluster>=l->loader_first && cluster<l->loader_first+l->loader_count)value=cluster+1==l->loader_first+l->loader_count?0x0fffffff:cluster+1;
        else if(cluster>=l->kernel_first && cluster<l->kernel_first+l->kernel_count)value=cluster+1==l->kernel_first+l->kernel_count?0x0fffffff:cluster+1;
        w32(s+i*4,value);
    }
}
static void dirent(uint8_t *p,const char name[11],uint32_t cluster,uint32_t size,bool directory) {
    zero(p,32);copy(p,(const uint8_t *)name,11);p[11]=directory?0x10:0x20;
    const uint16_t date=(2026-1980)*512+32+1;w16(p+16,date);w16(p+18,date);w16(p+24,date);
    w16(p+20,(uint16_t)(cluster>>16));w16(p+26,(uint16_t)cluster);w32(p+28,size);
}
static void directory(uint8_t *s,unsigned cluster,const struct layout *l,const struct install_payload *files) {
    zero(s,4096);
    if(cluster==2){dirent(s,"EFI        ",3,0,true);dirent(s+32,"CHOS       ",5,0,true);return;}
    dirent(s,".          ",cluster,0,true);dirent(s+32,"..         ",cluster==4?3:0,0,true);
    if(cluster==3)dirent(s+64,"BOOT       ",4,0,true);
    if(cluster==4)dirent(s+64,"BOOTAA64EFI",l->loader_first,(uint32_t)files->loader_size,false);
    if(cluster==5)dirent(s+64,"KERNEL  BIN",l->kernel_first,(uint32_t)files->kernel_size,false);
}
static bool payload_write(struct block_device d,const struct layout *l,unsigned cluster,const uint8_t *file,size_t size,uint8_t *buffer,uint8_t *scratch,install_report report,void *context,unsigned phase) {
    uint64_t offset=l->data+(uint64_t)(cluster-2)*l->sector;
    if(report)report(context,phase,0,size);
    for(size_t at=0;at<size;at+=l->sector) {
        size_t n=size-at;if(n>l->sector)n=l->sector;zero(buffer,4096);copy(buffer,file+at,n);
        if(!write_span(d,l,offset+at,buffer,l->sector,scratch))return false;
        if(report)report(context,phase,at+n,size);
    }
    return true;
}
bool install_fat32_verify(struct block_device d,const struct install_plan *p,const struct install_payload *files,struct gpt_workspace *w) {
    struct layout l;if(!w || !layout(d,p,files,&l))return false;
    boot_sector(w->primary,&l,p);
    if(!verify_span(d,&l,0,w->primary,l.sector,w->scratch) || !verify_span(d,&l,(uint64_t)6*l.sector,w->primary,l.sector,w->scratch))return false;
    fsinfo(w->primary);
    if(!verify_span(d,&l,l.sector,w->primary,l.sector,w->scratch) || !verify_span(d,&l,(uint64_t)7*l.sector,w->primary,l.sector,w->scratch))return false;
    for(unsigned fat=0;fat<2;fat++)for(unsigned block=0;block<l.fat*l.sector/4096;block++) {
        fat_block(w->primary,block,&l);
        if(!verify_span(d,&l,(uint64_t)(32+fat*l.fat)*l.sector+(uint64_t)block*4096,w->primary,4096,w->scratch))return false;
    }
    for(unsigned i=2;i<6;i++){directory(w->primary,i,&l,files);if(!verify_span(d,&l,l.data+(uint64_t)(i-2)*l.sector,w->primary,l.sector,w->scratch))return false;}
    return verify_span(d,&l,l.data+(uint64_t)(l.loader_first-2)*l.sector,files->loader,files->loader_size,w->scratch) && verify_span(d,&l,l.data+(uint64_t)(l.kernel_first-2)*l.sector,files->kernel,files->kernel_size,w->scratch);
}
bool install_fat32_observed(struct block_device d,const struct install_plan *p,const struct install_payload *files,bool confirmed,struct gpt_workspace *w,install_report report,void *context) {
    struct layout l;if(!confirmed || !w || !d.write || !d.flush || !layout(d,p,files,&l))return false;
    if(report)report(context,SETUP_FORMAT,0,0);
    zero(w->primary,4096);
    for(unsigned i=0;i<32*l.sector/4096;i++)if(!write_span(d,&l,(uint64_t)i*4096,w->primary,4096,w->scratch))return false;
    boot_sector(w->primary,&l,p);
    if(!write_span(d,&l,0,w->primary,l.sector,w->scratch) || !write_span(d,&l,(uint64_t)6*l.sector,w->primary,l.sector,w->scratch))return false;
    fsinfo(w->primary);
    if(!write_span(d,&l,l.sector,w->primary,l.sector,w->scratch) || !write_span(d,&l,(uint64_t)7*l.sector,w->primary,l.sector,w->scratch))return false;
    for(unsigned fat=0;fat<2;fat++)for(unsigned block=0;block<l.fat*l.sector/4096;block++) {
        fat_block(w->primary,block,&l);
        if(!write_span(d,&l,(uint64_t)(32+fat*l.fat)*l.sector+(uint64_t)block*4096,w->primary,4096,w->scratch))return false;
    }
    for(unsigned i=2;i<6;i++){directory(w->primary,i,&l,files);if(!write_span(d,&l,l.data+(uint64_t)(i-2)*l.sector,w->primary,l.sector,w->scratch))return false;}
    if(!d.flush(d.context))return false;
    if(!payload_write(d,&l,l.kernel_first,files->kernel,files->kernel_size,w->primary,w->scratch,report,context,SETUP_COPY) || !d.flush(d.context))return false;
    if(!payload_write(d,&l,l.loader_first,files->loader,files->loader_size,w->primary,w->scratch,report,context,SETUP_BOOT) || !d.flush(d.context))return false;
    return install_fat32_verify(d,p,files,w);
}
bool install_fat32_write(struct block_device d,const struct install_plan *p,const struct install_payload *files,bool confirmed,struct gpt_workspace *w) {
    return install_fat32_observed(d,p,files,confirmed,w,0,0);
}
