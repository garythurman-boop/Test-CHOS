#include "setup.h"
#include "storage.h"
#include "process.h"
#include "vfs.h"
#include "console.h"
static struct block_device disk;
static struct chos_boot_info *boot;
static struct pmm *physical;
static struct install_plan plan;
static struct gpt_workspace workspace;
static struct chfs system_volume;
static struct chos_setup state;
static uint64_t owner;
static int worker=-1;
static bool persistent;
static unsigned volume_state;
static char computer[32]="CHOS-PC";
static struct chos_confirm confirmed;
static uint64_t first_block,partition_blocks;
static struct block_device system_device;
static unsigned writes;
static void copy(char *a,const char *b,unsigned n){unsigned i=0;while(i+1<n && b[i]){a[i]=b[i];i++;}a[i]=0;}
static bool system_read(void *unused,uint64_t block,void *data){(void)unused;return block<partition_blocks && disk.read(disk.context,first_block+block,data);}
static bool system_write(void *unused,uint64_t block,const void *data){(void)unused;return block<partition_blocks && disk.write(disk.context,first_block+block,data);}
static bool system_flush(void *unused){(void)unused;return disk.flush(disk.context);}
static void partition(void){
    first_block=plan.system_first*plan.sector_bytes/4096;
    partition_blocks=(plan.system_last-plan.system_first+1)*plan.sector_bytes/4096;
    if(partition_blocks>1024)partition_blocks=1024;
    system_device=(struct block_device){0,partition_blocks,system_read,system_write,system_flush};
}
bool setup_persistent(void){return persistent;}
unsigned setup_volume_state(void){return volume_state;}
const char *setup_computer(void){return computer;}
void setup_initialize(struct block_device *device,struct chos_boot_info *b,struct pmm *p){
    boot=b;physical=p;state.medium=b->boot_medium;
    if(b->boot_medium==1)volume_state=VOLUME_NO_DEVICE;
    if(!device)return;
    disk=*device;state.bytes=disk.blocks*4096;state.sector=nvme_sector_size();copy(state.serial,nvme_serial(),sizeof(state.serial));
    if(b->boot_medium==1)volume_state=VOLUME_BAD_GPT;
    if(b->boot_medium==1 && install_gpt_probe(disk,state.sector,&plan,&workspace)){
        uint8_t guid[16];for(unsigned i=0;i<16;i++)guid[i]=plan.disk_guid[i];guid[0]^=0xa5;guid[15]^=1;
        volume_state=VOLUME_IDENTITY;
        bool match=true;for(unsigned i=0;i<16;i++)if(guid[i]!=b->esp_guid[i])match=false;
        if(match){
            partition();
            volume_state=VOLUME_BAD_CHFS;
            if(!chfs_mount(&system_volume,system_device))return;
            volume_state=VOLUME_BAD_CONFIG;
            size_t size=0;
            char name[32]={0};
            if(!chfs_read(&system_volume,"/system/computer",0,name,31,&size) || !size || size>31)return;
            for(unsigned i=0;i<size;i++)if(!((name[i]>='A' && name[i]<='Z') || (name[i]>='a' && name[i]<='z') || (name[i]>='0' && name[i]<='9') || name[i]=='-'))return;
            copy(computer,name,sizeof(computer));vfs_mount(&system_volume);persistent=true;volume_state=VOLUME_PERSISTENT;
        }
    }
    /* Optical boot is required: no path exists to erase the currently booted
     * NVMe ESP. The one currently supported target is explicitly inventoried. */
    uint64_t counter;__asm__ volatile("mrs %0, cntpct_el0":"=r"(counter));
    uint8_t guid[16];for(unsigned i=0;i<16;i++)guid[i]=(uint8_t)((counter>>(i%8)*8)^(uint64_t)(i*37+11));
    guid[7]=(guid[7]&15)|0x40;guid[8]=(guid[8]&63)|0x80;
    if(state.serial[0] && b->boot_medium==2 && install_plan_make(disk.blocks,state.sector,guid,&plan))state.allowed=1;
}
static bool progress_write(void *unused,uint64_t block,const void *data){
    (void)unused;bool ok=disk.write(disk.context,block,data);
    if(ok){writes++;unsigned value=writes/6;if(value>99)value=99;state.progress=value;}return ok;
}
static bool progress_read(void *unused,uint64_t block,void *data){(void)unused;return disk.read(disk.context,block,data);}
static bool progress_flush(void *unused){(void)unused;return disk.flush(disk.context);}
static void install(void *unused){
    (void)unused;state.phase=1;
    if(!nvme_recheck()){state.error=1;return;}
    struct block_device target={0,disk.blocks,progress_read,progress_write,progress_flush};
    if(!install_gpt_write(target,state.sector,&plan,true,&workspace)){state.error=2;return;}
    state.phase=2;
    extern const uint8_t boot_efi[],boot_efi_end[];
    struct install_payload files={boot_efi,(const uint8_t *)(uintptr_t)boot->kernel_source,(size_t)(boot_efi_end-boot_efi),(size_t)boot->kernel_size};
    if(!install_fat32_write(target,&plan,&files,true,&workspace)){state.error=3;return;}
    state.phase=3;partition();
    static const char welcome[]="Welcome to installed CHOS.\nThis system volume persists across reboot.\n";
    extern const uint8_t license_text[],license_text_end[],font_license[],font_license_end[];
    unsigned length=0;while(confirmed.computer[length])length++;
    if(!chfs_format(&system_volume,system_device,true) || !chfs_mkdir(&system_volume,"/system") || !chfs_mkdir(&system_volume,"/documents") || !chfs_write(&system_volume,"/system/computer",confirmed.computer,length) || !chfs_write(&system_volume,"/documents/welcome.txt",welcome,sizeof(welcome)-1) || !chfs_write(&system_volume,"/system/license",license_text,(size_t)(license_text_end-license_text)) || !chfs_write(&system_volume,"/system/font-ofl",font_license,(size_t)(font_license_end-font_license))){state.error=4;return;}
    if(!chfs_mount(&system_volume,system_device) || !install_gpt_verify(disk,state.sector,&plan,&workspace) || !install_fat32_verify(disk,&plan,&files,&workspace)){state.error=5;return;}
    state.progress=100;state.phase=4;
}
int64_t setup_syscall(struct process *p,uint64_t op,uint64_t argument,uint64_t token){
    if(p->application!=7)return -13;
    if(op==0){
        struct chos_setup out=state;
        if(!state.phase){owner=p->pid;state.token=out.token=(p->pid<<32)^state.bytes^0x43484f5353455455ULL;}
        return user_copy(p,argument,&out,sizeof(out),true)?0:-14;
    }
    if(op!=1 || !state.allowed || state.phase || owner!=p->pid || token!=state.token)return -22;
    struct chos_confirm input;
    if(!user_copy(p,argument,&input,sizeof(input),false))return -14;
    input.erase[47]=0;input.computer[31]=0;
    if(!setup_confirmation_valid(&state,&input))return -22;
    confirmed=input;state.phase=1;writes=0;
    worker=thread_create(physical,install,0,1);
    if(worker<0){state.phase=0;return -12;}
    state.allowed=0;return 0;
}
void setup_step(void){if(worker>=0 && thread_dead((unsigned)worker)){if(!thread_reap(physical,(unsigned)worker))panic("SETUP WORKER REAP FAILED");worker=-1;}}
