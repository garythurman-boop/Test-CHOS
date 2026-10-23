/* Bounded, append-only diagnostic export. No arbitrary memory or file access. */
#include "diagnostic.h"
static char data[DIAGNOSTIC_CAPACITY];
static unsigned length;
void diagnostic_reset(void){length=0;}
void diagnostic_append(const char *s){
    if(!s)return;
    for(unsigned i=0;i<512 && s[i] && length<sizeof(data);i++){
        unsigned char c=(unsigned char)s[i];
        data[length++]=(c=='\n' || (c>=32 && c<127))?(char)c:'?';
    }
}
unsigned diagnostic_size(void){return length;}
unsigned diagnostic_read(unsigned offset,void *out,unsigned capacity){
    if(!out || !capacity || capacity>4096 || offset>length)return 0;
    unsigned n=length-offset;if(n>capacity)n=capacity;
    for(unsigned i=0;i<n;i++)((char *)out)[i]=data[offset+i];
    return n;
}
