#include "process.h"
#include "mmu.h"
#include "console.h"
static struct process processes[PROCESS_MAX];
static struct pmm *physical;
static uint64_t next_pid;
void process_init(struct pmm *p) {
    physical=p; next_pid=1;
    for(unsigned i=0;i<PROCESS_MAX;i++) processes[i]=(struct process){0};
}
struct process *process_find(uint64_t pid) {
    for(unsigned i=0;i<PROCESS_MAX;i++) if(pid && processes[i].pid==pid) return &processes[i];
    return 0;
}
struct process *process_current(void) { return process_find(scheduler_pid()); }
static struct user_region *region_add(struct process *p,uint64_t va,unsigned pages,unsigned flags,unsigned kind) {
    unsigned id=0;
    while(id<PROCESS_REGIONS && p->regions[id].pages) id++;
    if(id==PROCESS_REGIONS) return 0;
    uint64_t pa=pmm_alloc(physical,pages);
    if(!pa) return 0;
    for(uint64_t i=0;i<(uint64_t)pages*PAGE_SIZE;i++) ((uint8_t *)(uintptr_t)pa)[i]=0;
    struct user_region *r=&p->regions[id];
    *r=(struct user_region){va,pa,pages,flags,kind};
    for(unsigned i=0;i<pages;i++) {
        if(!mmu_user_map(physical,&p->space,va+i*PAGE_SIZE,pa+i*PAGE_SIZE,(flags&2)!=0,(flags&1)!=0)) {
            for(unsigned j=0;j<i;j++) (void)mmu_user_unmap(&p->space,va+j*PAGE_SIZE);
            if(!pmm_free(physical,pa,pages)) panic("REGION ROLLBACK FAILED");
            *r=(struct user_region){0}; return 0;
        }
    }
    return r;
}
static void release(struct process *p) {
    for(unsigned i=0;i<PROCESS_REGIONS;i++) {
        if(p->regions[i].pages && !pmm_free(physical,p->regions[i].physical,p->regions[i].pages)) panic("PROCESS PAGE FREE FAILED");
    }
    mmu_space_destroy(physical,&p->space);
    *p=(struct process){0};
}
int process_create(const void *image,size_t length,uint64_t role,uint64_t peer) {
    struct elf_plan plan;
    if(!elf_validate(image,length,&plan) || next_pid>0x7fffffff) return -1;
    unsigned slot=0;
    while(slot<PROCESS_MAX && processes[slot].pid) slot++;
    if(slot==PROCESS_MAX) return -1;
    struct process *p=&processes[slot];
    *p=(struct process){.pid=next_pid++,.thread=-1};
    if(!mmu_space_create(physical,&p->space)) { release(p); return -1; }
    for(unsigned i=0;i<plan.count;i++) {
        const struct elf_segment *s=&plan.segments[i];
        struct user_region *r=region_add(p,s->address,(unsigned)((s->memory_size+4095)/4096),s->flags,1);
        if(!r) { release(p); return -1; }
        for(uint64_t j=0;j<s->file_size;j++) ((uint8_t *)(uintptr_t)r->physical)[j]=((const uint8_t *)image)[s->offset+j];
        if(s->flags&1) {
            uint64_t ctr; __asm__ volatile("mrs %0, ctr_el0":"=r"(ctr));
            uint64_t line=4ULL<<((ctr>>16)&15);
            for(uint64_t a=r->physical;a<r->physical+r->pages*PAGE_SIZE;a+=line) __asm__ volatile("dc cvau, %0"::"r"(a):"memory");
            __asm__ volatile("dsb ish; ic iallu; dsb ish; isb":::"memory");
        }
    }
    if(!region_add(p,USER_STACK_TOP-2*PAGE_SIZE,2,6,2)) { release(p); return -1; }
    p->thread=thread_user(physical,plan.entry,USER_STACK_TOP,p->space.root,p->pid,role,peer);
    if(p->thread<0) { release(p); return -1; }
    return (int)p->pid;
}
bool process_reap(uint64_t pid) {
    struct process *p=process_find(pid);
    if(!p || !p->exited || scheduler_pid() || !thread_reap(physical,(unsigned)p->thread)) return false;
    release(p); return true;
}
bool user_validate(struct process *p,uint64_t address,size_t length,bool to_user) {
    if(length>4096 || address<USER_BASE || address>=USER_STACK_TOP || length>USER_STACK_TOP-address) return false;
    /* Validate the entire span before any copy, including across region boundaries. */
    for(size_t offset=0;offset<length;) {
        const struct user_region *found=0;
        for(unsigned i=0;i<PROCESS_REGIONS;i++) {
            const struct user_region *r=&p->regions[i];
            if(r->pages && address+offset>=r->va && address+offset-r->va<(uint64_t)r->pages*PAGE_SIZE && (!to_user || (r->flags&2))) { found=r; break; }
        }
        if(!found) return false;
        uint64_t n=found->pages*PAGE_SIZE-(address+offset-found->va);
        if(n>length-offset) n=length-offset;
        offset+=(size_t)n;
    }
    return true;
}
bool user_copy(struct process *p,uint64_t address,void *buffer,size_t length,bool to_user) {
    if(!user_validate(p,address,length,to_user)) return false;
    for(size_t i=0;i<length;i++) {
        for(unsigned j=0;j<PROCESS_REGIONS;j++) {
            struct user_region *r=&p->regions[j];
            if(r->pages && address+i>=r->va && address+i-r->va<r->pages*PAGE_SIZE) {
                uint8_t *physical_byte=(void *)(uintptr_t)(r->physical+address+i-r->va);
                if(to_user) *physical_byte=((uint8_t *)buffer)[i]; else ((uint8_t *)buffer)[i]=*physical_byte;
                break;
            }
        }
    }
    return true;
}
uint64_t process_allocate(struct process *p,unsigned pages) {
    if(!pages || pages>16) return 0;
    for(unsigned slot=0;slot<PROCESS_REGIONS;slot++) {
        uint64_t va=USER_HEAP_BASE+(uint64_t)slot*65536;
        bool busy=false;
        for(unsigned i=0;i<PROCESS_REGIONS;i++) if(p->regions[i].pages && p->regions[i].va==va) busy=true;
        if(!busy) { struct user_region *r=region_add(p,va,pages,6,3); return r?r->va:0; }
    }
    return 0;
}
bool process_free(struct process *p,uint64_t va) {
    for(unsigned i=0;i<PROCESS_REGIONS;i++) {
        struct user_region *r=&p->regions[i];
        if(r->pages && r->kind==3 && r->va==va) {
            for(unsigned j=0;j<r->pages;j++) if(!mmu_user_unmap(&p->space,va+j*PAGE_SIZE)) panic("USER UNMAP FAILED");
            if(!pmm_free(physical,r->physical,r->pages)) panic("USER FREE FAILED");
            *r=(struct user_region){0}; return true;
        }
    }
    return false;
}
struct exception_frame *process_exit(struct exception_frame *f,int64_t code,uint64_t ec,uint64_t far) {
    struct process *p=process_current();
    if(!p) panic("NO CURRENT USER PROCESS");
    p->exited=true; p->status=code; p->fault_class=ec; p->fault_address=far;
    return scheduler_terminate(f);
}
