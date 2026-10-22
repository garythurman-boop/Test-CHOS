/* Compiler-required freestanding memory primitives; no host runtime is linked. */
#include <stddef.h>
void *memset(void *destination,int value,size_t length) {
    volatile unsigned char *d=destination;
    for(size_t i=0;i<length;i++) d[i]=(unsigned char)value;
    return destination;
}
void *memcpy(void *destination,const void *source,size_t length) {
    volatile unsigned char *d=destination;
    const volatile unsigned char *s=source;
    for(size_t i=0;i<length;i++) d[i]=s[i];
    return destination;
}
void *memmove(void *destination,const void *source,size_t length) {
    volatile unsigned char *d=destination;
    const volatile unsigned char *s=source;
    if((size_t)d<(size_t)s) for(size_t i=0;i<length;i++) d[i]=s[i];
    else for(size_t i=length;i;i--) d[i-1]=s[i-1];
    return destination;
}
