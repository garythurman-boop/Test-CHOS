#include "abi.h"
static volatile uint64_t private_data;
static int64_t call(uint64_t n,uint64_t a,uint64_t b) {
    register uint64_t x0 __asm__("x0")=a;
    register uint64_t x1 __asm__("x1")=b;
    register uint64_t x8 __asm__("x8")=n;
    __asm__ volatile("svc #0":"+r"(x0):"r"(x1),"r"(x8):"memory","cc");
    return (int64_t)x0;
}
#define CHECK(x) do { if(!(x)) return 90; } while(0)
int user_main(uint64_t role,uint64_t peer) {
    if(role==7) { __asm__ volatile(".inst 0x4f00e400"); return 93; }
    if(role==3) { volatile uint64_t value=*(volatile uint64_t *)(uintptr_t)peer; return (int)value; }
    if(role==4) { *(volatile uint32_t *)(uintptr_t)0x1000000000ULL=0; return 91; }
    if(role==5) { void (*fn)(void)=(void *)(uintptr_t)(0x1040000000ULL-4096); fn(); return 92; }
    struct chos_message message;
    if(role==6) {
    CHECK(call(SYS_ABI,0,0)==1 && call(999,0,0)==-38);
    CHECK(call(SYS_WRITE,0,1)==-14 && call(SYS_WRITE,UINT64_MAX,8)==-14);
    CHECK(call(SYS_ALLOC,0,0)==-22 && call(SYS_ALLOC,UINT64_MAX,0)==-22);
    uint64_t pid=(uint64_t)call(SYS_PID,0,0);
    CHECK(pid);
    uint8_t *memory=(void *)(uintptr_t)call(SYS_ALLOC,2,0);
    CHECK((int64_t)(uintptr_t)memory>0);
    for(unsigned i=0;i<8192;i++) memory[i]=(uint8_t)(i^pid);
    call(SYS_YIELD,0,0);
    for(unsigned i=0;i<8192;i++) CHECK(memory[i]==(uint8_t)(i^pid));
    CHECK(call(SYS_FREE,(uintptr_t)memory,0)==0 && call(SYS_FREE,(uintptr_t)memory,0)==-22);
    CHECK(call(SYS_RECV,(uintptr_t)&message,0)==-11);
    for(unsigned i=0;i<8;i++) CHECK(call(SYS_SEND,pid,i)==0);
    CHECK(call(SYS_SEND,pid,99)==-11);
    CHECK(call(SYS_RECV,0x1000000000ULL,0)==-14); /* executable page is read-only */
    CHECK(call(SYS_RECV,UINT64_MAX-7,0)==-14);
    for(unsigned i=0;i<8;i++) {
        CHECK(call(SYS_RECV,(uintptr_t)&message,0)==0 && message.sender==pid && message.value==i);
    }
    CHECK(call(SYS_SEND,0,0)==-3);
    return 43;
    }
    private_data=role==1?0x1111:0x2222;
    if(role==2) CHECK(call(SYS_SEND,peer,0x7777)==0);
    unsigned rounds=role==1?2:1;
    for(unsigned round=0;round<rounds;round++) {
        uint64_t deadline=(uint64_t)call(SYS_TICKS,0,0)+100;
        int64_t received;
        do {
            received=call(SYS_RECV,(uintptr_t)&message,0);
            if(received==-11) call(SYS_YIELD,0,0);
            CHECK((uint64_t)call(SYS_TICKS,0,0)<deadline);
        } while(received==-11);
        uint64_t expected=role==2?0x1234:(round==0?0x7777:0xabcd);
        CHECK(received==0 && message.sender==peer && message.value==expected);
        CHECK(private_data==(role==1?0x1111:0x2222));
        if(role==1 && round==0) CHECK(call(SYS_SEND,peer,0x1234)==0);
        if(role==2) CHECK(call(SYS_SEND,peer,0xabcd)==0);
    }
    static const char text[]="EL0 ELF SYSCALLS AND IPC PASS";
    CHECK(call(SYS_WRITE,(uintptr_t)text,sizeof(text)-1)==sizeof(text)-1);
    return role==1?41:42;
}
