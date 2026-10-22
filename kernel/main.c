#include "boot.h"
#include "console.h"
#include "memory.h"
#include "exception.h"
#include "mmu.h"
#include "platform.h"
#include "scheduler.h"
#include "timer.h"
#include "process.h"
#include "storage.h"
#include "vfs.h"
static struct pmm physical;
static struct heap heap;
static struct worker_state workers[3];
static volatile uint64_t high_ran;
static void high_thread(void *unused) { (void)unused; high_ran++; }
void kernel_main(struct chos_boot_info *info) {
    if (!info || info->magic!=CHOS_BOOT_MAGIC) for (;;) __asm__ volatile("wfe");
    struct chos_boot_info *boot=info;
    console_init(info);
    exception_init();
    console_rect(0,0,boot->width,boot->height,0x071b36);
    console_rect(0,0,boot->width,72,0x164a82);
    console_text(32,24,"CHOS",0xffffff);
    console_text(32,100,"ORIGINAL ARM64 KERNEL",0xffffff);
    console_text(32,132,"FIRMWARE BOOT SERVICES EXITED",0x8edbff);
    uint64_t el,freq;
    __asm__ volatile("mrs %0, CurrentEL":"=r"(el));
    __asm__ volatile("mrs %0, cntfrq_el0":"=r"(freq));
    console_text(32,172,"EXCEPTION LEVEL",0xffffff); console_number(260,172,el>>2);
    console_text(32,200,"TIMER FREQUENCY HZ",0xffffff); console_number(300,200,freq);
    /* UEFI descriptor prefix: type, padding, physical, virtual, pages, attributes.
     * Iterate with the firmware-provided descriptor stride, never sizeof(prefix). */
    if (boot->descriptor_size<40 || boot->descriptor_size>boot->memory_map_size ||
        boot->memory_map_size%boot->descriptor_size) {
        console_text(32,244,"INVALID MEMORY MAP",0xff7070);
        for (;;) __asm__ volatile("wfe");
    }
    uint64_t pages=0,count=boot->memory_map_size/boot->descriptor_size;
    for (uint64_t i=0;i<count;i++) {
        const uint8_t *d=(void *)(uintptr_t)(boot->memory_map+i*boot->descriptor_size);
        if (*(const uint32_t *)d==7) pages+=*(const uint64_t *)(d+24);
    }
    console_text(32,228,"CONVENTIONAL RAM MIB",0xffffff); console_number(300,228,pages/256);
    console_text(32,256,"MEMORY DESCRIPTORS",0xffffff); console_number(300,256,count);
    if(!exception_test_break()) panic("BREAKPOINT TEST FAILED");
    if(!pmm_init(&physical,(void *)(uintptr_t)info->memory_map,info->memory_map_size,info->descriptor_size)) panic("MEMORY MAP REJECTED");
    uint64_t page=pmm_alloc(&physical,1);
    if(!page) panic("PAGE ALLOCATION FAILED");
    volatile uint64_t *check=(void *)(uintptr_t)page;
    for(unsigned i=0;i<PAGE_SIZE/8;i++) check[i]=0x43484f5300000000ULL+i;
    for(unsigned i=0;i<PAGE_SIZE/8;i++) if(check[i]!=0x43484f5300000000ULL+i) panic("PAGE INTEGRITY FAILED");
    if(!pmm_free(&physical,page,1) || pmm_free(&physical,page,1)) panic("PAGE FREE FAILED");
    uint64_t arena=pmm_alloc(&physical,16);
    if(!arena || !heap_init(&heap,(void *)(uintptr_t)arena,16*PAGE_SIZE)) panic("HEAP INIT FAILED");
    void *a=heap_alloc(&heap,17),*b=heap_alloc(&heap,4096);
    if(!a || !b || !heap_free(&heap,a) || !heap_free(&heap,b)) panic("HEAP TEST FAILED");
    uint64_t mpidr; __asm__ volatile("mrs %0, mpidr_el1":"=r"(mpidr));
    struct platform platform;
    if(!platform_discover(boot,mpidr,&platform)) panic("ACPI GIC2 TIMER DISCOVERY FAILED");
    if(!mmu_init(&physical,boot,platform.gic_distributor,platform.gic_cpu)) panic("MMU INIT FAILED");
    if(!exception_test_read(0)) panic("NULL GUARD TEST FAILED");
    if(!exception_test_read((uintptr_t)__text_start+512*1024)) panic("STACK GUARD TEST FAILED");
    if(!exception_test_write((uintptr_t)__text_start)) panic("READONLY TEXT TEST FAILED");
    if(!exception_test_execute(arena)) panic("NX HEAP TEST FAILED");
    console_rect(0,160,boot->width,boot->height-160,0x071b36);
    console_text(32,172,"EXCEPTIONS AND MEMORY TESTS PASS",0x8effb0);
    console_text(32,200,"ACPI GIC2 TIMER DISCOVERED",0x8edbff);
    console_text(32,228,"PREEMPTION TEST RUNNING",0xffffff);
    scheduler_init();
    int a_id=thread_create(&physical,arch_test_worker,&workers[0],1);
    int b_id=thread_create(&physical,arch_test_worker,&workers[1],1);
    int low_id=thread_create(&physical,arch_test_worker,&workers[2],0);
    int high_id=thread_create(&physical,high_thread,0,2);
    if(a_id<0 || b_id<0 || low_id<0 || high_id<0) panic("THREAD CREATION FAILED");
    if(!timer_init(&platform)) panic("TIMER INIT FAILED");
    __asm__ volatile("msr daifclr, #2":::"memory");
    while(timer_ticks()<30) __asm__ volatile("wfi");
    if(!workers[0].progress || !workers[1].progress || workers[2].progress ||
       high_ran!=1 || !thread_dead((unsigned)high_id) || scheduler_switches()<10)
        panic("PREEMPTION OR PRIORITY FAILED");
    workers[0].stop=workers[1].stop=1;
    while(!thread_dead((unsigned)a_id) || !thread_dead((unsigned)b_id)) __asm__ volatile("wfi");
    thread_priority(0);
    uint64_t deadline=timer_ticks()+10;
    while(timer_ticks()<deadline) __asm__ volatile("wfi");
    if(!workers[2].progress) panic("LOW PRIORITY DID NOT RUN");
    workers[2].stop=1;
    while(!thread_dead((unsigned)low_id)) __asm__ volatile("wfi");
    thread_priority(1);
    __asm__ volatile("msr daifset, #2":::"memory");
    if(!thread_reap(&physical,(unsigned)a_id) || !thread_reap(&physical,(unsigned)b_id) ||
       !thread_reap(&physical,(unsigned)low_id) || !thread_reap(&physical,(unsigned)high_id)) panic("KERNEL THREAD REAP FAILED");
    uint32_t baseline=physical.used;
    process_init(&physical);
    extern const uint8_t init_elf[],init_elf_end[];
    size_t image_size=(size_t)(init_elf_end-init_elf);
    int solo=process_create(init_elf,image_size,6,0);
    if(solo!=1) panic("ELF PROCESS LOAD FAILED");
    __asm__ volatile("msr daifclr, #2":::"memory");
    deadline=timer_ticks()+200;
    while(!process_find(1)->exited) {
        if(timer_ticks()>deadline) panic("USER SYSCALL TIMEOUT");
        __asm__ volatile("wfi");
    }
    if(process_find(1)->status!=43) panic("USER SYSCALL TEST FAILED");
    __asm__ volatile("msr daifset, #2":::"memory");
    if(!process_reap(1) || physical.used!=baseline) panic("SOLO PROCESS REAP LEAK");
    int first=process_create(init_elf,image_size,1,3),second=process_create(init_elf,image_size,2,2);
    if(first!=2 || second!=3) panic("IPC PROCESS LOAD FAILED");
    __asm__ volatile("msr daifclr, #2":::"memory");
    deadline=timer_ticks()+200;
    while(!process_find(2)->exited || !process_find(3)->exited) {
        if(timer_ticks()>deadline) panic("USER IPC TIMEOUT");
        __asm__ volatile("wfi");
    }
    if(process_find(2)->status!=41 || process_find(3)->status!=42) panic("USER IPC TEST FAILED");
    __asm__ volatile("msr daifset, #2":::"memory");
    if(!process_reap(2) || !process_reap(3) || physical.used!=baseline) panic("PROCESS REAP LEAK");
    const uint64_t fault_roles[]={3,4,5,7};
    for(unsigned test=0;test<4;test++) {
        uint64_t role=fault_roles[test];
        int pid=process_create(init_elf,image_size,role,(uintptr_t)&physical);
        if(pid<0) panic("FAULT PROCESS LOAD FAILED");
        __asm__ volatile("msr daifclr, #2":::"memory");
        deadline=timer_ticks()+100;
        while(!process_find((uint64_t)pid)->exited) {
            if(timer_ticks()>deadline) panic("USER FAULT TIMEOUT");
            __asm__ volatile("wfi");
        }
        struct process *p=process_find((uint64_t)pid);
        uint64_t expected=role==7?7:(role==5?0x20:0x24);
        uint64_t expected_address=role==3?(uintptr_t)&physical:(role==4?USER_BASE:USER_STACK_TOP-4096);
        if(p->fault_class!=expected || (role!=7 && p->fault_address!=expected_address)) panic("USER ISOLATION FAILED");
        __asm__ volatile("msr daifset, #2":::"memory");
        if(!process_reap((uint64_t)pid) || physical.used!=baseline) panic("FAULT PROCESS LEAK");
    }
    __asm__ volatile("msr daifclr, #2":::"memory");
    struct block_device disk; bool test_identity=false;
    bool disk_ready=nvme_init(&physical,boot,&disk,&test_identity);
    unsigned storage_phase=0;
    if(boot->reserved&1) {
        if(!disk_ready) { console_text(32,520,"BAR ADDRESS",0xffffff); console_number(240,520,nvme_bar()); panic(nvme_error()); }
        extern unsigned storage_test(struct block_device,bool);
        __asm__ volatile("msr daifset, #2":::"memory");
        storage_phase=storage_test(disk,test_identity);
        if(storage_phase==1) {
            extern struct chfs *storage_volume(void);
            vfs_mount(storage_volume()); uint32_t file_baseline=physical.used;
            int pid=process_create(init_elf,image_size,8,0);
            if(pid<0) panic("NATIVE FILE TEST LOAD FAILED");
            __asm__ volatile("msr daifclr, #2":::"memory");
            deadline=timer_ticks()+500;
            while(!process_find((uint64_t)pid)->exited) {
                if(timer_ticks()>deadline) panic("NATIVE FILE TEST TIMEOUT");
                __asm__ volatile("wfi");
            }
            if(process_find((uint64_t)pid)->status!=44) panic("NATIVE FILE SYSCALL TEST FAILED");
            __asm__ volatile("msr daifset, #2":::"memory");
            if(!process_reap((uint64_t)pid) || physical.used!=file_baseline) panic("NATIVE FILE PROCESS LEAK");
        }
        __asm__ volatile("msr daifclr, #2":::"memory");
    }
    console_rect(0,150,boot->width,boot->height-150,0x071b36);
    console_text(32,172,"MEMORY PREEMPTION AND REAP PASS",0x8effb0);
    console_text(32,200,"EL0 ELF SYSCALLS AND IPC PASS",0x8effb0);
    console_text(32,228,"USER ISOLATION FAULT TESTS PASS",0x8effb0);
    console_text(32,256,"TIMER TICKS",0xffffff); console_number(240,256,timer_ticks());
    console_text(32,284,"CONTEXT SWITCHES",0xffffff); console_number(240,284,scheduler_switches());
    console_text(32,316,"CORE TESTS PASS STORAGE OPTIONAL",0x8effb0);
    console_text(32,352,"NO DESKTOP OR INSTALLER YET",0xffffff);
    uint32_t signature[8]={0x32bbcd,0x6b45a1,0xf3ba20,0x18ad72,
                                        0xe14567,0x2998ef,0xcddc39,0x804020};
    if(!storage_phase) console_text(32,380,disk_ready?"NVME DETECTED DISK WRITES DISABLED":"NVME NOT PRESENT OPTIONAL",0x8edbff);
    if(storage_phase) {
        signature[0]=storage_phase==1?0x33ccdd:(storage_phase==2?0x33ddee:0x33eeff);
        console_rect(0,300,boot->width,84,0x071b36);
        console_text(32,316,"NVME CHFS PERSISTENCE TEST PASS",0x8effb0);
        console_text(32,352,"STORAGE TEST PHASE",0xffffff); console_number(300,352,storage_phase);
    }
    for(unsigned i=0;i<8;i++) console_rect(32+i*16,400,16,16,signature[i]);
    console_text(32,436,"ALLOCATED PHYSICAL PAGES",0xffffff); console_number(360,436,physical.used);
    console_text(32,464,"MANAGED PHYSICAL PAGES",0xffffff); console_number(360,464,physical.total);
    __asm__ volatile("dsb sy":::"memory");
    for(;;) __asm__ volatile("wfi");
}
