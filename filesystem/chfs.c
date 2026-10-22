/* CHFS v1: bounded original filesystem, little-endian on disk, dual checksummed
 * metadata roots and copy-on-write file extents. See docs/storage.md for limits. */
#include "filesystem.h"
#define MAGIC UINT64_C(0x3153464843534f43)
static void zero(void *p,size_t n) { uint8_t *d=p; for(size_t i=0;i<n;i++) d[i]=0; }
static void copy(void *d,const void *s,size_t n) { for(size_t i=0;i<n;i++) ((uint8_t *)d)[i]=((const uint8_t *)s)[i]; }
static uint32_t crc_byte(uint32_t crc,uint8_t b) {
    crc^=b;
    for(unsigned j=0;j<8;j++) crc=(crc>>1)^(0xedb88320u&(0u-(crc&1)));
    return crc;
}
uint32_t chfs_crc(const void *p,size_t n) {
    uint32_t crc=0xffffffff;
    for(size_t i=0;i<n;i++) crc=crc_byte(crc,((const uint8_t *)p)[i]);
    return ~crc;
}
static uint32_t super_crc(const struct chfs_super *s) {
    uint32_t crc=0xffffffff;
    for(unsigned i=0;i<CHFS_BLOCK;i++) crc=crc_byte(crc,i>=12&&i<16?0:((const uint8_t *)s)[i]);
    return ~crc;
}
static size_t length(const char *s) {
    if(!s) return CHFS_NAME;
    size_t n=0; while(n<CHFS_NAME && s[n]) n++; return n;
}
static bool equal(const char *a,const char *b) {
    for(unsigned i=0;i<CHFS_NAME;i++) { if(a[i]!=b[i]) return false; if(!a[i]) return true; }
    return false;
}
static bool path(const char *s) {
    size_t n=length(s);
    if(n<2 || n>=CHFS_NAME || s[0]!='/' || s[n-1]=='/') return false;
    unsigned component=1;
    for(unsigned i=1;i<=n;i++) {
        if(i<n && ((unsigned char)s[i]<33 || (unsigned char)s[i]>126)) return false;
        if(s[i]=='/' || i==n) {
            unsigned len=i-component;
            if(!len || (len==1 && s[component]=='.') || (len==2 && s[component]=='.' && s[component+1]=='.')) return false;
            component=i+1;
        }
    }
    return true;
}
static int find(const struct chfs_super *s,const char *name) {
    for(unsigned i=0;i<CHFS_ENTRIES;i++) if(s->entries[i].kind && equal(s->entries[i].name,name)) return (int)i;
    return -1;
}
static bool parent(const struct chfs_super *s,const char *name) {
    char p[CHFS_NAME]; size_t n=length(name),last=0;
    for(size_t i=1;i<n;i++) if(name[i]=='/') last=i;
    if(!last) return true;
    copy(p,name,last); p[last]=0;
    int id=find(s,p); return id>=0 && s->entries[id].kind==2;
}
static void bit(uint8_t *map,unsigned n,bool value) {
    if(value) map[n/8]|=(uint8_t)(1u<<(n%8)); else map[n/8]&=(uint8_t)~(1u<<(n%8));
}
static bool used(const uint8_t *map,unsigned n) { return (map[n/8]>>(n%8))&1; }
static bool valid(const struct chfs_super *s,uint64_t capacity) {
    if(s->magic!=MAGIC || s->version!=1 || s->crc!=super_crc(s) ||
       !s->generation || s->blocks<16 || s->blocks>1024 || s->blocks>capacity) return false;
    uint8_t expected[128]; zero(expected,sizeof(expected)); bit(expected,0,true); bit(expected,1,true);
    for(unsigned i=(unsigned)s->blocks;i<1024;i++) bit(expected,i,true);
    for(unsigned i=0;i<CHFS_ENTRIES;i++) {
        const struct chfs_entry *e=&s->entries[i];
        if(!e->kind) continue;
        if(e->kind>2 || !path(e->name) || !parent(s,e->name)) return false;
        for(unsigned j=0;j<i;j++) if(s->entries[j].kind && equal(e->name,s->entries[j].name)) return false;
        if(e->kind==2) { if(e->blocks || e->size || e->first || e->crc) return false; continue; }
        if(e->size>32768 || e->blocks!=(e->size+4095)/4096) return false;
        if(!e->blocks) { if(e->first || e->crc!=chfs_crc("",0)) return false; continue; }
        if(e->first<2 || e->first>=s->blocks || e->blocks>s->blocks-e->first) return false;
        for(unsigned j=0;j<e->blocks;j++) { unsigned b=e->first+j; if(used(expected,b)) return false; bit(expected,b,true); }
    }
    for(unsigned i=0;i<128;i++) if(expected[i]!=s->bitmap[i]) return false;
    return true;
}
bool chfs_mount(struct chfs *f,struct block_device device) {
    f->mounted=false; f->device=device;
    if(!device.read || !device.write || !device.flush || device.blocks<16) return false;
    bool a=device.read(device.context,0,&f->super) && valid(&f->super,device.blocks);
    bool b=device.read(device.context,1,&f->transaction) && valid(&f->transaction,device.blocks);
    if(!a && !b) return false;
    f->active=0;
    if(b && (!a || f->transaction.generation>f->super.generation)) { copy(&f->super,&f->transaction,sizeof(f->super)); f->active=1; }
    f->mounted=true; return true;
}
bool chfs_format(struct chfs *f,struct block_device device,bool confirmed) {
    f->mounted=false; f->device=device;
    if(!confirmed || !device.read || !device.write || !device.flush || device.blocks<16) return false;
    zero(&f->super,sizeof(f->super));
    f->super.magic=MAGIC; f->super.version=1; f->super.generation=1;
    f->super.blocks=device.blocks>1024?1024:device.blocks;
    bit(f->super.bitmap,0,true); bit(f->super.bitmap,1,true);
    for(unsigned i=(unsigned)f->super.blocks;i<1024;i++) bit(f->super.bitmap,i,true);
    f->super.crc=super_crc(&f->super);
    if(!device.write(device.context,0,&f->super) || !device.flush(device.context) ||
       !device.write(device.context,1,&f->super) || !device.flush(device.context)) return false;
    f->active=1; f->mounted=true; return true;
}
static bool commit(struct chfs *f) {
    if(f->super.generation==UINT64_MAX) return false;
    f->transaction.generation=f->super.generation+1;
    f->transaction.crc=super_crc(&f->transaction);
    unsigned next=1-f->active;
    if(!f->device.flush(f->device.context) || !f->device.write(f->device.context,next,&f->transaction) || !f->device.flush(f->device.context)) {
        f->mounted=false; return false; /* uncertain commit: remount before any further operation */
    }
    copy(&f->super,&f->transaction,sizeof(f->super)); f->active=next; return true;
}
static int prepare(struct chfs *f,const char *name) {
    if(!f->mounted || !path(name) || !parent(&f->super,name)) return -1;
    copy(&f->transaction,&f->super,sizeof(f->super));
    int id=find(&f->super,name);
    if(id>=0) return id;
    for(unsigned i=0;i<CHFS_ENTRIES;i++) if(!f->transaction.entries[i].kind) {
        copy(f->transaction.entries[i].name,name,length(name)+1); return (int)i;
    }
    return -1;
}
bool chfs_mkdir(struct chfs *f,const char *name) {
    int id=prepare(f,name);
    if(id<0 || f->transaction.entries[id].kind) return false;
    f->transaction.entries[id].kind=2; return commit(f);
}
bool chfs_write(struct chfs *f,const char *name,const void *data,size_t length_bytes) {
    if(length_bytes>32768 || (!data&&length_bytes)) return false;
    int id=prepare(f,name);
    if(id<0 || f->transaction.entries[id].kind==2) return false;
    struct chfs_entry *e=&f->transaction.entries[id];
    unsigned blocks=(unsigned)((length_bytes+4095)/4096),first=0,run=0;
    for(unsigned b=2;b<f->super.blocks && blocks;b++) {
        run=used(f->super.bitmap,b)?0:run+1;
        if(run==blocks) { first=b+1-blocks; break; }
    }
    if(blocks && !first) return false;
    for(unsigned i=0;i<blocks;i++) {
        zero(f->scratch,CHFS_BLOCK); size_t off=(size_t)i*4096,n=length_bytes-off;
        if(n>4096) n=4096;
        copy(f->scratch,(const uint8_t *)data+off,n);
        if(!f->device.write(f->device.context,first+i,f->scratch)) { f->mounted=false; return false; }
    }
    for(unsigned i=0;i<e->blocks;i++) bit(f->transaction.bitmap,e->first+i,false);
    for(unsigned i=0;i<blocks;i++) bit(f->transaction.bitmap,first+i,true);
    e->kind=1; e->first=first; e->blocks=blocks; e->size=(uint32_t)length_bytes; e->crc=chfs_crc(data,length_bytes);
    return commit(f);
}
const struct chfs_entry *chfs_stat(const struct chfs *f,const char *name) {
    if(!f->mounted || !path(name)) return 0;
    int id=find(&f->super,name); return id<0?0:&f->super.entries[id];
}
bool chfs_read(struct chfs *f,const char *name,size_t offset,void *buffer,size_t length_bytes,size_t *read_bytes) {
    *read_bytes=0; const struct chfs_entry *e=chfs_stat(f,name);
    if(!e || e->kind!=1 || offset>e->size || (!buffer&&length_bytes)) return false;
    if(length_bytes>e->size-offset) length_bytes=e->size-offset;
    uint32_t crc=0xffffffff;
    for(unsigned i=0;i<e->blocks;i++) {
        if(!f->device.read(f->device.context,e->first+i,f->scratch)) return false;
        size_t count=e->size-(size_t)i*4096; if(count>4096) count=4096;
        for(size_t j=0;j<count;j++) {
            crc=crc_byte(crc,f->scratch[j]); size_t pos=(size_t)i*4096+j;
            if(pos>=offset && pos-offset<length_bytes) ((uint8_t *)buffer)[pos-offset]=f->scratch[j];
        }
    }
    if(~crc!=e->crc) return false;
    *read_bytes=length_bytes; return true;
}
bool chfs_remove(struct chfs *f,const char *name) {
    int id=prepare(f,name);
    if(id<0 || !f->transaction.entries[id].kind) return false;
    struct chfs_entry *e=&f->transaction.entries[id];
    size_t n=length(name);
    if(e->kind==2) for(unsigned i=0;i<CHFS_ENTRIES;i++) {
        const char *child=f->super.entries[i].name; bool match=true;
        for(size_t j=0;j<n;j++) if(child[j]!=name[j]) { match=false; break; }
        if(f->super.entries[i].kind && match && child[n]=='/') return false;
    }
    for(unsigned i=0;i<e->blocks;i++) bit(f->transaction.bitmap,e->first+i,false);
    zero(e,sizeof(*e)); return commit(f);
}
bool chfs_rename(struct chfs *f,const char *from,const char *to) {
    if(!f->mounted || !path(to) || !parent(&f->super,to) || find(&f->super,to)>=0) return false;
    int id=prepare(f,from);
    if(id<0 || f->transaction.entries[id].kind!=1) return false; /* directory rename deferred */
    zero(f->transaction.entries[id].name,CHFS_NAME);
    copy(f->transaction.entries[id].name,to,length(to)+1); return commit(f);
}
