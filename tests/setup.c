#include "setup.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void){
    struct chos_setup d={.medium=2,.allowed=1,.serial="CHOS-SETUP-1"};
    struct chos_confirm c={.erase="ERASE CHOS-SETUP-1",.computer="CHOS-PC"};
    assert(setup_confirmation_valid(&d,&c));
    d.medium=1;assert(!setup_confirmation_valid(&d,&c));d.medium=0;assert(!setup_confirmation_valid(&d,&c));d.medium=2;
    d.allowed=0;assert(!setup_confirmation_valid(&d,&c));d.allowed=1;
    c.erase[6]='X';assert(!setup_confirmation_valid(&d,&c));c.erase[6]='C';
    c.erase[18]=' ';assert(!setup_confirmation_valid(&d,&c));c.erase[18]=0;
    c.computer[0]=0;assert(!setup_confirmation_valid(&d,&c));strcpy(c.computer,"bad/name");assert(!setup_confirmation_valid(&d,&c));
    memset(c.computer,'A',32);assert(!setup_confirmation_valid(&d,&c));strcpy(c.computer,"CHOS-PC");
    memset(d.serial,'A',24);assert(!setup_confirmation_valid(&d,&c));d.serial[0]=0;assert(!setup_confirmation_valid(&d,&c));
    assert(!setup_confirmation_valid(0,&c) && !setup_confirmation_valid(&d,0));
    puts("Setup rejects self/unknown boot media, missing authority, wrong disk identity, malformed confirmation and invalid names");
}
