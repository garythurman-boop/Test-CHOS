#include "abi.h"
#include <stdbool.h>
bool setup_confirmation_valid(const struct chos_setup *disk,const struct chos_confirm *input){
    if(!disk || !input || disk->medium!=2 || !disk->allowed || !disk->serial[0] || !input->computer[0])return false;
    unsigned serial=0;
    for(;serial<sizeof(disk->serial) && disk->serial[serial];serial++)if(disk->serial[serial]<32 || disk->serial[serial]>126)return false;
    if(serial==sizeof(disk->serial) || serial>20)return false;
    const char word[]="ERASE";
    for(unsigned i=0;i<sizeof(word);i++)if(input->erase[i]!=word[i])return false;
    unsigned name=0;
    for(;name<sizeof(input->computer) && input->computer[name];name++){
        char c=input->computer[name];if(!((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='-'))return false;
    }
    return name<sizeof(input->computer);
}
