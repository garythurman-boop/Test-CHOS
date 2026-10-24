#include "input.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    input_reset(800,600);
    input_accept((struct input_event){1,30,1}); assert(input_get()->last=='A' && input_get()->keys==1);
    input_accept((struct input_event){1,30,0}); assert(input_get()->keys==1);
    input_accept((struct input_event){2,0,17}); input_accept((struct input_event){2,1,9});
    assert(input_get()->x==737 && input_get()->y==549 && input_get()->motion==2);
    input_accept((struct input_event){1,272,1}); assert(input_get()->clicks==1);
    input_accept((struct input_event){2,0,INT32_MAX}); assert(input_get()->x==799);
    input_accept((struct input_event){2,0,INT32_MIN}); assert(input_get()->x==0);
    struct input_event e; unsigned n=0;
    while(input_next(&e)) n++;
    assert(n==7 && !input_next(&e));
    for(unsigned i=0;i<200;i++) input_accept((struct input_event){1,30,1});
    assert(input_get()->dropped==72);
    n=0; while(input_next(&e)) n++; assert(n==128);
    input_reset(0,0); input_accept((struct input_event){2,1,INT32_MAX}); assert(input_get()->y==0);
    input_reset(800,600);
    const struct input_event batch[]={{1,46,1},{2,0,-10},{1,272,1},{1,272,0},{1,46,0}};
    input_accept_batch(batch,5);
    assert(input_next(&e) && e.type==2 && e.value==-10);
    assert(input_next(&e) && e.code==272 && e.value==1);
    assert(input_next(&e) && e.code==272 && e.value==0);
    assert(input_next(&e) && e.code==46 && e.value==1);
    assert(input_next(&e) && e.code==46 && e.value==0 && !input_next(&e));
    input_reset(1024,768);
    input_accept((struct input_event){3,0,65535});input_accept((struct input_event){3,1,0});
    assert(input_get()->x==1023 && input_get()->y==0);
    assert(input_next(&e) && e.type==2 && e.code==0 && e.value==79);
    assert(input_next(&e) && e.type==2 && e.code==1 && e.value==-708);
    input_accept((struct input_event){3,0,32768});assert(input_get()->x==511);
    puts("PASS: input decoding, key releases, signed motion/clamping, buttons, FIFO ordering and bounded overflow");
}
