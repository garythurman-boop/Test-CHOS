#include "elf.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void w64(unsigned char *p,uint64_t n) { for(unsigned i=0;i<8;i++) p[i]=(unsigned char)(n>>(8*i)); }
int main(int argc,char **argv) {
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb"); assert(f);
    assert(fseek(f,0,SEEK_END)==0); long n=ftell(f); assert(n>64);
    rewind(f); unsigned char *data=malloc((size_t)n),*bad=malloc((size_t)n);
    assert(data&&bad && fread(data,1,(size_t)n,f)==(size_t)n); fclose(f);
    struct elf_plan plan;
    assert(elf_validate(data,(size_t)n,&plan) && plan.count==2 && plan.entry==USER_BASE);
    assert(!elf_validate(data,63,&plan));
    #define MUTATE(statement) do { memcpy(bad,data,(size_t)n); statement; assert(!elf_validate(bad,(size_t)n,&plan)); } while(0)
    MUTATE(bad[5]=2);
    MUTATE(bad[18]=62);
    MUTATE(bad[16]=3);
    MUTATE(w64(bad+32,UINT64_MAX));
    MUTATE(w64(bad+24,USER_BASE+0x100000));
    MUTATE(bad[56]=9);
    /* Program header begins at the canonical 64-byte ELF header in lld output. */
    assert(data[32]==64);
    MUTATE(bad[64+4]=7);
    MUTATE(w64(bad+64+40,UINT64_MAX));
    MUTATE(w64(bad+64+32,(uint64_t)n+1));
    MUTATE(w64(bad+64+16,0x40000000));
    MUTATE(w64(bad+64+56+16,USER_BASE));
    MUTATE(bad[64]=2);
    for(unsigned i=0;i<2000;i++) {
        memcpy(bad,data,(size_t)n);
        size_t offset=(i*73u)%176u;
        bad[offset]^=(unsigned char)(1u<<(i%8));
        (void)elf_validate(bad,(size_t)n,&plan);
    }
    free(data); free(bad);
    puts("PASS: actual AArch64 ELF, truncated headers, architecture/type/endianness, overflow, W+X, entry bounds, overlapping segments and metadata mutation sweep");
}
