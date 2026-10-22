#ifndef CHOS_FILESYSTEM_H
#define CHOS_FILESYSTEM_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define CHFS_BLOCK 4096u
#define CHFS_ENTRIES 40u
#define CHFS_NAME 60u
struct block_device {
    void *context; uint64_t blocks;
    bool (*read)(void *,uint64_t,void *);
    bool (*write)(void *,uint64_t,const void *);
    bool (*flush)(void *);
};
struct chfs_entry { char name[CHFS_NAME]; uint32_t first,blocks,size,kind,crc; };
struct chfs_super {
    uint64_t magic; uint32_t version,crc; uint64_t generation,blocks;
    uint8_t bitmap[128]; struct chfs_entry entries[CHFS_ENTRIES]; uint8_t padding[736];
};
_Static_assert(sizeof(struct chfs_super)==CHFS_BLOCK,"CHFS disk layout");
struct chfs { struct block_device device; struct chfs_super super,transaction; uint8_t scratch[CHFS_BLOCK]; unsigned active; bool mounted; };
uint32_t chfs_crc(const void *,size_t);
bool chfs_mount(struct chfs *,struct block_device);
bool chfs_format(struct chfs *,struct block_device,bool confirmed);
bool chfs_mkdir(struct chfs *,const char *);
bool chfs_write(struct chfs *,const char *,const void *,size_t);
bool chfs_read(struct chfs *,const char *,size_t,void *,size_t,size_t *);
bool chfs_remove(struct chfs *,const char *);
bool chfs_rename(struct chfs *,const char *,const char *);
const struct chfs_entry *chfs_stat(const struct chfs *,const char *);
#endif
