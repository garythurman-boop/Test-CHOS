#include "memory.h"
struct block { size_t size; struct block *next; uint64_t magic,free; };
#define MAGIC UINT64_C(0x43484f5348454150)
_Static_assert(sizeof(struct block)==32,"heap header alignment");
bool heap_init(struct heap *h,void *base,size_t size) {
    if(!base || (uintptr_t)base%16 || size<sizeof(struct block)+16 || size%16) return false;
    h->base=base; h->size=size;
    struct block *b=base;
    *b=(struct block){size-sizeof(*b),0,MAGIC,1};
    return true;
}
void *heap_alloc(struct heap *h,size_t size) {
    if(!h->base || !size || size>SIZE_MAX-15) return 0;
    size=(size+15)&~(size_t)15;
    for(struct block *b=h->base;b;b=b->next) {
        if(b->magic!=MAGIC) return 0;
        if(!b->free || b->size<size) continue;
        if(b->size-size>=sizeof(*b)+16) {
            struct block *n=(void *)((uint8_t *)(b+1)+size);
            *n=(struct block){b->size-size-sizeof(*b),b->next,MAGIC,1};
            b->next=n; b->size=size;
        }
        b->free=0;
        return b+1;
    }
    return 0;
}
bool heap_free(struct heap *h,void *pointer) {
    if(!h->base || !pointer) return false;
    /* Find exact payload boundaries; never trust a header inferred from caller input. */
    struct block *found=0;
    for(struct block *b=h->base;b;b=b->next) {
        if(b->magic!=MAGIC) return false;
        if((void *)(b+1)==pointer) { found=b; break; }
    }
    if(!found || found->free) return false;
    found->free=1;
    for(struct block *b=h->base;b&&b->next;) {
        struct block *n=b->next;
        if(b->free&&n->free) { b->size+=sizeof(*n)+n->size; b->next=n->next; n->magic=0; }
        else b=n;
    }
    return true;
}
