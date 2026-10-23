#include "diagnostic.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void){
    char out[4096];diagnostic_reset();assert(!diagnostic_size());
    diagnostic_append("CHOS\n\001\377");assert(diagnostic_size()==7);
    assert(diagnostic_read(0,out,4096)==7 && !memcmp(out,"CHOS\n??",7));
    assert(diagnostic_read(4,out,2)==2 && !memcmp(out,"\n?",2));
    assert(!diagnostic_read(8,out,1) && !diagnostic_read(0,0,1) && !diagnostic_read(0,out,4097));
    char big[1024];memset(big,'A',1023);big[1023]=0;
    diagnostic_reset();diagnostic_append(big);assert(diagnostic_size()==512);
    for(unsigned i=0;i<100;i++)diagnostic_append(big);
    assert(diagnostic_size()==DIAGNOSTIC_CAPACITY && diagnostic_read(DIAGNOSTIC_CAPACITY,out,1)==0);
    diagnostic_reset();assert(!diagnostic_read(0,out,1));
    puts("Diagnostic append/export sanitation, line cap, offset/capacity bounds and saturation passed");
}
