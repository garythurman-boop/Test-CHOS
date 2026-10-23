#include "vfs.h"
#include "process.h"
#include "abi.h"
static struct chfs *volume;
static uint8_t content[32768],input[4096];
void vfs_mount(struct chfs *f) { volume=f; }
static int path_copy(struct process *p,uint64_t address,char path[60]) {
    for(unsigned i=0;i<60;i++) {
        if(!user_copy(p,address+i,&path[i],1,false)) return -14;
        if(!path[i]) return 0;
    }
    return -36;
}
int64_t file_syscall(struct process *p,struct exception_frame *f) {
    if(!volume || !volume->mounted) return -19;
    unsigned number=(unsigned)f->x[8];
    uint64_t a=f->x[0],b=f->x[1],c=f->x[2];
    if(number==SYS_REPLACE) {
        char path[60];int error=path_copy(p,a,path);if(error)return error;
        if(c>4096)return -22;
        if(!user_copy(p,b,input,(size_t)c,false))return -14;
        return chfs_write(volume,path,input,(size_t)c)?0:-5;
    }
    if(number==SYS_OPEN || number==SYS_MKDIR || number==SYS_UNLINK || number==SYS_RENAME) {
        char path[60],other[60]; int error=path_copy(p,a,path);
        if(error) return error;
        if(number==SYS_MKDIR) return chfs_mkdir(volume,path)?0:-22;
        if(number==SYS_UNLINK) return chfs_remove(volume,path)?0:-22;
        if(number==SYS_RENAME) { error=path_copy(p,b,other); return error?error:(chfs_rename(volume,path,other)?0:-22); }
        if(!(b&3) || (b&~15ULL) || ((b&12) && !(b&2))) return -22;
        unsigned slot=0; while(slot<8 && p->files[slot].used) slot++;
        if(slot==8) return -24;
        const struct chfs_entry *e=chfs_stat(volume,path);
        if(!e && !(b&4)) return -2;
        if(e && e->kind!=1) return -22;
        if((!e || (b&8)) && !chfs_write(volume,path,0,0)) return -5;
        struct file_handle *h=&p->files[slot]; *h=(struct file_handle){.flags=(uint32_t)b,.used=true};
        for(unsigned i=0;i<60;i++) { h->path[i]=path[i]; if(!path[i]) break; }
        return (int64_t)slot+3;
    }
    if(number==SYS_LIST) {
        if(a>=CHFS_ENTRIES) return -22;
        const struct chfs_entry *e=&volume->super.entries[a];
        if(!e->kind) return -2;
        struct chos_dirent d={.size=e->size,.kind=e->kind};
        for(unsigned i=0;i<60;i++) d.path[i]=e->name[i];
        return user_copy(p,b,&d,sizeof(d),true)?0:-14;
    }
    if(a<3 || a>=11 || !p->files[a-3].used) return -9;
    struct file_handle *h=&p->files[a-3];
    if(number==SYS_CLOSE) { *h=(struct file_handle){0}; return 0; }
    const struct chfs_entry *e=chfs_stat(volume,h->path);
    if(!e) return -2; /* path handles intentionally do not retain unlinked files */
    if(number==SYS_SIZE) return e->size;
    if(number==SYS_SEEK) { if(b>32768) return -22; h->offset=(uint32_t)b; return (int64_t)b; }
    if(c>4096) return -22;
    if(number==SYS_FREAD) {
        if(!(h->flags&1)) return -9;
        if(!user_validate(p,b,(size_t)c,true)) return -14;
        if(h->offset>e->size) return 0;
        size_t n=0;
        if(!chfs_read(volume,h->path,h->offset,content,(size_t)c,&n)) return -5;
        if(!user_copy(p,b,content,n,true)) return -14;
        h->offset+=(uint32_t)n; return (int64_t)n;
    }
    if(number==SYS_FWRITE) {
        if(!(h->flags&2)) return -9;
        if(!user_copy(p,b,input,(size_t)c,false)) return -14;
        if(c>32768-h->offset) return -22;
        if(!c) return 0;
        size_t n=0;
        if(!chfs_read(volume,h->path,0,content,sizeof(content),&n)) return -5;
        for(size_t i=n;i<h->offset;i++) content[i]=0;
        for(size_t i=0;i<c;i++) content[h->offset+i]=input[i];
        size_t size=h->offset+c; if(size<n) size=n;
        if(!chfs_write(volume,h->path,content,size)) return -5;
        h->offset+=(uint32_t)c; return (int64_t)c;
    }
    return -38;
}
