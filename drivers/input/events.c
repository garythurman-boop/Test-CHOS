#include "input.h"
static struct input_state state;
static struct input_event events[128];
static unsigned head,count,width,height;
void input_reset(unsigned w,unsigned h) {
    width=w; height=h; head=count=0;
    state=(struct input_state){.x=(int32_t)(w>80?w-80:0),.y=(int32_t)(h>60?h-60:0)};
}
void input_accept(struct input_event e) {
    if(e.type==0) return;
    /* Internal absolute coordinates use [0,65535]. Publish the existing relative
     * event ABI to native apps after scaling to the current framebuffer. */
    if(e.type==3 && e.code<=1){
        int64_t v=e.value;if(v<0)v=0;if(v>65535)v=65535;
        unsigned extent=e.code==0?width:height;
        int32_t target=(int32_t)(v*(extent?extent-1:0)/65535);
        e.type=2;e.value=target-(e.code==0?state.x:state.y);
        if(!e.value)return;
    }
    if(count==128) { head=(head+1)%128; count--; state.dropped++; }
    events[(head+count)%128]=e; count++;
    if(e.type==1 && e.code==272 && e.value==1) state.clicks++;
    else if(e.type==1 && e.code<256 && (e.value==1 || e.value==2)) {
        static const uint8_t codes[26]={30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};
        state.keys++;
        for(unsigned i=0;i<26;i++) if(e.code==codes[i]) state.last=(char)('A'+i);
        if(e.code==57) state.last=' ';
    } else if(e.type==2 && e.code<=1) {
        int64_t value=(e.code==0?state.x:state.y)+(int64_t)e.value;
        int64_t limit=(int64_t)(e.code==0?width:height)-1;
        if(limit<0) limit=0;
        if(value<0) value=0;
        if(value>limit) value=limit;
        if(e.code==0) state.x=(int32_t)value; else state.y=(int32_t)value;
        state.motion++;
    }
}
void input_accept_batch(const struct input_event *batch,unsigned n) {
    /* Independent Virtio queues have no shared timestamp. For one poll batch,
     * deliver pointer position/button changes before typing into that target. */
    for(unsigned i=0;i<n;i++)if((batch[i].type==2 || batch[i].type==3) || (batch[i].type==1 && batch[i].code>=256))input_accept(batch[i]);
    for(unsigned i=0;i<n;i++)if(!((batch[i].type==2 || batch[i].type==3) || (batch[i].type==1 && batch[i].code>=256)))input_accept(batch[i]);
}
bool input_next(struct input_event *e) {
    if(!count) return false;
    *e=events[head]; head=(head+1)%128; count--; return true;
}
const struct input_state *input_get(void) { return &state; }
