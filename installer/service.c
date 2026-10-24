#include "setup.h"
#include "storage.h"
#include "process.h"
#include "vfs.h"
#include "console.h"
#include "desktop.h"
#include "timer.h"
#include "nvme_format.h"
#include "display_mode.h"
#include "diagnostic.h"
static struct block_device disk;
static struct chos_boot_info *boot;
static struct pmm *physical;
static struct install_plan plan;
static struct gpt_workspace workspace;
static struct chfs system_volume;
static struct chos_setup state;
static uint64_t owner;
static int worker=-1;
static bool persistent,graphical_pending;
#define STAGE_PATH "/system/setup-stage"
static const char stage_pending[]="CHOS SETUP PENDING 2\n";
static const char stage_ready[]="CHOS SETUP READY 1\n";
bool setup_pending(void){return graphical_pending;}

static unsigned volume_state;
static struct chos_config configuration,pending_config;
static struct chos_confirm confirmed;
static uint64_t first_block,partition_blocks;
static struct block_device system_device;
static struct chos_destination destination={.reason=SETUP_TARGET_MISSING};
static uint64_t live_request;
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
const char *setup_computer(void){return configuration.computer;}
const struct chos_config *setup_configuration(void){return &configuration;}
void setup_initialize(struct block_device *device,struct chos_boot_info *b,struct pmm *p){
    config_defaults(&configuration);config_defaults(&pending_config);
    boot=b;physical=p;state.medium=b->boot_medium;
    for(unsigned i=0;i<b->display.count;i++){
        diagnostic_value("GOP mode width: ",b->display.modes[i].width,false);
        diagnostic_value("GOP mode height: ",b->display.modes[i].height,false);
    }
    if(b->boot_medium==1)volume_state=VOLUME_NO_DEVICE;
    if(!device)return;
    disk=*device;state.bytes=disk.blocks*4096;state.sector=nvme_sector_size();copy(state.serial,nvme_serial(),sizeof(state.serial));
    destination.minimum_bytes=install_minimum_bytes(state.sector);
    destination.reason=b->boot_medium==2?SETUP_TARGET_READ:SETUP_TARGET_MEDIUM;
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
            const struct chfs_entry *entry=chfs_stat(&system_volume,CHOS_CONFIG_PATH);
            if(entry) {
                unsigned char wire[CHOS_CONFIG_BYTES];
                if(entry->kind!=1 || entry->size!=sizeof(wire) || !chfs_read(&system_volume,CHOS_CONFIG_PATH,0,wire,sizeof(wire),&size) || size!=sizeof(wire) || !config_decode(&configuration,wire,size))return;
            } else {
                /* M6B/M6C volumes are read without an automatic migration write. */
                char name[32]={0};
                if(!chfs_read(&system_volume,"/system/computer",0,name,31,&size) || !size || size>31)return;
                copy(configuration.computer,name,sizeof(configuration.computer));
                if(!config_valid(&configuration))return;
            }
            const struct chfs_entry *stage=chfs_stat(&system_volume,STAGE_PATH);
            if(!stage)return; /* Incomplete/legacy media require explicit recovery, never assume ready. */
            if(stage){
                char record[32];size_t got=0;
                if(!chfs_read(&system_volume,STAGE_PATH,0,record,sizeof(record),&got))return;
                bool pending=got==sizeof(stage_pending)-1,ready=got==sizeof(stage_ready)-1;
                for(size_t i=0;pending && i<got;i++)if(record[i]!=stage_pending[i])pending=false;
                for(size_t i=0;ready && i<got;i++)if(record[i]!=stage_ready[i])ready=false;
                if(!pending && !ready)return;
                graphical_pending=pending;
            }
            vfs_mount(&system_volume);persistent=true;volume_state=VOLUME_PERSISTENT;
        }
    }
    /* Optical boot is required: no path exists to erase the currently booted
     * NVMe ESP. The one currently supported target is explicitly inventoried. */
    uint64_t counter;__asm__ volatile("mrs %0, cntpct_el0":"=r"(counter));
    uint8_t guid[16];for(unsigned i=0;i<16;i++)guid[i]=(uint8_t)((counter>>(i%8)*8)^(uint64_t)(i*37+11));
    guid[7]=(guid[7]&15)|0x40;guid[8]=(guid[8]&63)|0x80;
    bool inventoried=setup_inventory(disk,state.sector,&destination,&workspace);
    destination.reason=SETUP_TARGET_OK;
    if(b->boot_medium!=2)destination.reason=SETUP_TARGET_MEDIUM;
    else if(!state.serial[0])destination.reason=SETUP_TARGET_IDENTITY;
    else if(!inventoried)destination.reason=SETUP_TARGET_READ;
    else if(!nvme_writable())destination.reason=SETUP_TARGET_READONLY;
    else if(!install_plan_make(disk.blocks,state.sector,guid,&plan))destination.reason=SETUP_TARGET_SMALL;
    else state.allowed=1;
}
static void report(void *unused,unsigned phase,uint64_t done,uint64_t total){
    uint64_t mask;__asm__ volatile("mrs %0, daif; msr daifset, #2":"=r"(mask)::"memory");
    bool changed=phase!=state.phase;
    (void)unused;destination.completed_bytes=done;destination.total_bytes=total;
    state.progress=(phase-1)*100/6+(total?(unsigned)(done*100/total/6):0);state.phase=phase;
    __asm__ volatile("msr daif, %0"::"r"(mask):"memory");
    if(changed){
        /* Synchronize a real stage change with its displayed native frame.
         * No cosmetic timer or minimum stage duration. A lost client cannot
         * stall disk work indefinitely: the wait is bounded to two seconds. */
        desktop_setup_notify(owner);uint64_t deadline=timer_ticks()+200;
        while(desktop_setup_displayed()!=phase && timer_ticks()<deadline){
            struct process *p=process_find(owner);if(!p || p->exited)break;
            thread_yield();
        }
    }
}
static bool verify_file(const char *path,const void *bytes,size_t size){
    const struct chfs_entry *entry=chfs_stat(&system_volume,path);
    if(!entry || entry->kind!=1 || entry->size!=size)return false;
    for(size_t at=0;at<size;){size_t n=size-at,got=0;if(n>4096)n=4096;
        if(!chfs_read(&system_volume,path,at,workspace.scratch,n,&got) || got!=n)return false;
        for(size_t i=0;i<n;i++)if(workspace.scratch[i]!=((const uint8_t *)bytes)[at+i])return false;at+=n;}
    return true;
}
static void install(void *unused){
    (void)unused;report(0,SETUP_PREPARE,0,0);
    if(!nvme_recheck()){state.error=64+nvme_check_error();return;}
    if(!nvme_writable()){state.error=64+NVME_CHECK_READONLY;return;}
    struct block_device target=disk;
    if(!install_gpt_write(target,state.sector,&plan,true,&workspace)){state.error=2;return;}
    report(0,SETUP_FORMAT,0,0);partition();
    if(!chfs_format(&system_volume,system_device,true)){state.error=4;return;}

    extern const uint8_t boot_efi[],boot_efi_end[];
    struct install_payload files={boot_efi,(const uint8_t *)(uintptr_t)boot->kernel_source,(size_t)(boot_efi_end-boot_efi),(size_t)boot->kernel_size};
    if(!install_fat32_observed(target,&plan,&files,true,&workspace,report,0)){state.error=3;return;}
    report(0,SETUP_CONFIGURE,0,0);
    static const char welcome[]="Welcome to installed CHOS.\nThis system volume persists across reboot.\n";
    extern const uint8_t license_text[],license_text_end[],font_license[],font_license_end[];
    unsigned length=0;while(confirmed.computer[length])length++;
    copy(pending_config.computer,confirmed.computer,sizeof(pending_config.computer));
    unsigned char config_wire[CHOS_CONFIG_BYTES];if(!config_encode(&pending_config,config_wire)){state.error=4;return;}
    if(!chfs_mkdir(&system_volume,"/system") || !chfs_mkdir(&system_volume,"/documents") || !chfs_write(&system_volume,CHOS_CONFIG_PATH,config_wire,sizeof(config_wire)) || !chfs_write(&system_volume,"/system/computer",confirmed.computer,length) || !chfs_write(&system_volume,"/documents/welcome.txt",welcome,sizeof(welcome)-1) || !chfs_write(&system_volume,"/system/license",license_text,(size_t)(license_text_end-license_text)) || !chfs_write(&system_volume,"/system/font-ofl",font_license,(size_t)(font_license_end-font_license))){state.error=4;return;}
    report(0,SETUP_VERIFY,0,0);
    if(!chfs_mount(&system_volume,system_device) || !install_gpt_verify(disk,state.sector,&plan,&workspace) || !install_fat32_verify(disk,&plan,&files,&workspace) || !verify_file(CHOS_CONFIG_PATH,config_wire,sizeof(config_wire)) || !verify_file("/system/computer",confirmed.computer,length) || !verify_file("/documents/welcome.txt",welcome,sizeof(welcome)-1) || !verify_file("/system/license",license_text,(size_t)(license_text_end-license_text)) || !verify_file("/system/font-ofl",font_license,(size_t)(font_license_end-font_license))){state.error=5;return;}
    static const char log[]="CHOS Setup log v1\nGPT primary/backup and partition arrays: verified\nFAT32 metadata and EFI/BOOT/BOOTAA64.EFI: verified\nCHOS/KERNEL.BIN: exact payload verified\nCHFS configuration, computer, welcome and licenses: verified\nBoot uses the ARM64 UEFI fallback path; no NVRAM entry required.\nUS keymap/fixed clock offset supported. Boot display trials available.\nAccounts/authentication, DST and runtime mode switching: unfinished\n";
    if(!chfs_write(&system_volume,"/system/install-log",log,sizeof(log)-1) || !verify_file("/system/install-log",log,sizeof(log)-1)){state.error=6;return;}
    /* Commit the continuation marker only after payload verification. Missing /
     * corrupt configuration never authorizes another destructive boot stage. */
    if(!chfs_write(&system_volume,STAGE_PATH,stage_pending,sizeof(stage_pending)-1) ||
       !verify_file(STAGE_PATH,stage_pending,sizeof(stage_pending)-1) || !disk.flush(disk.context)) {state.error=6;return;}
    destination.completed_bytes=destination.total_bytes=0;state.progress=100;state.phase=SETUP_COMPLETE;
}
int64_t setup_syscall(struct process *p,uint64_t op,uint64_t argument,uint64_t token){
    if(p->application!=7)return -13;
    if(op==4)return graphical_pending?1:0;
    if(op==5){
        if(!persistent || !graphical_pending || boot->boot_medium!=1 || state.phase)return -13;
        struct chos_config input;
        if(!user_copy(p,argument,&input,sizeof(input),false))return -14;
        if(!config_valid(&input))return -22;
        if(!nvme_recheck() || !nvme_writable())return -30;
        unsigned char wire[CHOS_CONFIG_BYTES];
        unsigned length=0;while(input.computer[length])length++;
        if(!chfs_write(&system_volume,"/system/computer",input.computer,length) || !verify_file("/system/computer",input.computer,length))return -5;
        if(!config_encode(&input,wire) || !config_save(&system_volume,&input,&configuration) ||
           !verify_file(CHOS_CONFIG_PATH,wire,sizeof(wire)) ||
           !chfs_write(&system_volume,STAGE_PATH,stage_ready,sizeof(stage_ready)-1) ||
           !verify_file(STAGE_PATH,stage_ready,sizeof(stage_ready)-1) || !disk.flush(disk.context))return -5;
        graphical_pending=false;return 0;
    }
    if(op==2){
        if(worker>=0 || (state.phase && !state.error && state.phase!=SETUP_COMPLETE))return -16;
        live_request=p->pid;return 0;
    }
    if(op==3)return user_copy(p,argument,&destination,sizeof(destination),true)?0:-14;
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
    confirmed=input;state.phase=SETUP_PREPARE;
    worker=thread_create(physical,install,0,1);
    if(worker<0){state.phase=0;return -12;}
    state.allowed=0;return 0;
}
/* Apply active settings only after a successful CHFS commit. A final flush
 * error can leave the disk with the old or new complete record; never claim rollback. */
int64_t setup_configure(struct process *p,uint64_t op,uint64_t argument,uint64_t save) {
    if(op==0)return user_copy(p,argument,&configuration,sizeof(configuration),true)?0:-14;
    if((op!=1 && op!=2) || save>1)return -22;
    if(p->application!=4 && p->application!=7)return -13;
    struct chos_config input;
    if(!user_copy(p,argument,&input,sizeof(input),false))return -14;
    if(!config_valid(&input))return -22;
    if(op==2) {
        if(p->application!=7 || state.phase || (!state.allowed && !(persistent && graphical_pending && boot->boot_medium==1)))return -13;
        pending_config=input;return 0;
    }
    if(state.phase && state.phase!=SETUP_COMPLETE)return -16;
    if(save) {
        if(!persistent)return -30;
        return config_save(&system_volume,&input,&configuration)?0:-5;
    }
    configuration=input;return 0;
}
void setup_step(void){if(worker>=0 && thread_dead((unsigned)worker)){if(!thread_reap(physical,(unsigned)worker))panic("SETUP WORKER REAP FAILED");worker=-1;}}

bool setup_take_live(void){
    if(!live_request)return false;
    struct process *p=process_find(live_request);if(p && !p->exited)return false;
    live_request=0;return true;
}

bool setup_desktop_load(uint8_t data[96]){
    size_t got=0;const struct chfs_entry *entry=chfs_stat(&system_volume,"/system/desktop");
    return persistent && entry && entry->size==96 && chfs_read(&system_volume,"/system/desktop",0,data,96,&got) && got==96;
}
bool setup_desktop_save(const uint8_t data[96]){
    return persistent && nvme_writable() && chfs_write(&system_volume,"/system/desktop",data,96) && verify_file("/system/desktop",data,96);
}

int64_t setup_display(struct process *p,uint64_t op,uint64_t argument){
    if(op==0)return user_copy(p,argument,&boot->display,sizeof(boot->display),true)?0:-14;
    if(p->application!=4 && p->application!=7)return -13;
    if((op!=1 && op!=2) || !persistent || boot->boot_medium!=1 || state.phase)return -30;
    struct display_preference preference={boot->width,boot->height,0};
    if(op==1){
        struct chos_display_mode requested;
        if(!user_copy(p,argument,&requested,sizeof(requested),false))return -14;
        bool found=false;
        for(unsigned i=0;i<boot->display.count;i++)if(requested.width==boot->display.modes[i].width && requested.height==boot->display.modes[i].height)found=true;
        if(!found)return -22;
        preference=(struct display_preference){requested.width,requested.height,1};
    } else if(!boot->display.trial)return -22;
    if(!nvme_recheck() || !nvme_writable())return -30;
    extern const uint8_t boot_efi[],boot_efi_end[];
    struct install_payload files={boot_efi,(const uint8_t *)(uintptr_t)boot->kernel_source,(size_t)(boot_efi_end-boot_efi),(size_t)boot->kernel_size};
    if(!install_display_save(disk,&plan,&files,&preference,&workspace))return -5;
    if(op==2)boot->display.trial=0;
    return 0;
}
