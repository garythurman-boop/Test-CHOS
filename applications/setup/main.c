/* Dedicated original setup presentation over the existing native installer.
 * Screens do not write disks: only the final process-bound ERASE request does. */
#include "app.h"
#include "nvme_format.h"
enum { WELCOME,LICENSE,DESTINATION,LAYOUT,CONFIGURATION,CONFIRMATION,RECOVERY };
static unsigned page,selection;
static struct chos_config chosen;
static bool loaded;
static struct chos_setup disk;
static struct chos_destination target;
static struct chos_confirm confirm={.computer="CHOS-PC"};
#define BLUE 0x102a83u
#define WHITE 0xf2f6ffu
static void go(unsigned next){page=next;status("");ui_modal_end(&app_gui);}
static void leave(void){
    if(api(SYS_SETUP,2,0,0)<0){status("The disk writer is active. Please wait.");return;}
    (void)api(SYS_EXIT,0,0,0);
}
static void line(int y,const char *s){ui_text(4,32,y,s,WHITE);}
static void amount(int x,int y,uint64_t n,uint32_t color){char s[21];number(s,n);ui_text(4,x,y,s,color);}
static void size(int x,int y,uint64_t bytes,uint32_t color){char s[32];if(ui_format_size(s,sizeof(s),bytes))ui_text(4,x,y,s,color);}
static void information(void){
    static const char *const reasons[]={"Read checks passed. Confirmation required.","No supported NVMe disk was detected.","Disabled: this is not an optical-media boot.","Disk too small for the displayed layout.","Disk read failed. No installation is allowed.","Disk identity unavailable. Installation blocked.","Namespace reports write protection."};
    line(112,"NVMe controller 0 / namespace 1");
    line(144,"Serial:");ui_text(4,152,144,disk.serial,WHITE);
    line(176,"Capacity:");size(224,176,disk.bytes,WHITE);
    line(208,"Logical block size:");size(320,208,disk.sector,WHITE);
    line(240,target.layout==2?"Existing primary GPT verified:":target.layout==1?"Existing MBR partition table detected:":target.layout==3?"Existing layout unrecognized or invalid:":"No partition table detected:");
    if(target.layout==2){
        unsigned limit=target.reason==SETUP_TARGET_SMALL?3:4;
        unsigned count=target.partitions<limit?target.partitions:limit;
        for(unsigned i=0;i<count;i++){
            char name[24];copy(name,target.existing[i].name,sizeof(name));
            ui_text(0,48,268+(int)i*22,name[0]?name:"Unnamed partition",WHITE);
            size(app_width-180,268+(int)i*22,target.existing[i].bytes,WHITE);
        }
    } else line(272,"Existing data will be replaced if confirmed.");
    if(target.reason==SETUP_TARGET_SMALL){ui_text(0,32,app_height-146,"Minimum required bytes:",WHITE);amount(224,app_height-146,target.minimum_bytes,WHITE);}
    ui_text(0,32,app_height-114,reasons[target.reason<=SETUP_TARGET_READONLY?target.reason:SETUP_TARGET_READ],WHITE);
    ui_text(3,32,app_height-88,"Only one NVMe controller/namespace is supported.",0xc3d5ff);
}
static void blue(void){
    begin(BLUE);ui_text(1,32,24,"CHOS ARM64 Setup",WHITE);box(32,56,app_width-64,1,0x9bb6ed);
    if(page==WELCOME){
        line(92,"Welcome to CHOS Setup.");
        box(28,154+(int)selection*52,app_width-56,38,0x244da8);
        line(164,"ENTER  Install CHOS");line(216,"L      Start CHOS Live Desktop");line(268,"R      Recovery status");
        line(332,"Setup uses the original CHOS kernel.");
        ui_text(0,32,app_height-90,"Your disk is unchanged until typed confirmation.",0xc3d5ff);
    } else if(page==LICENSE){
        line(92,"CHOS Setup Agreement");line(140,"CHOS is licensed under the MIT License.");
        line(176,"Keep its copyright and license notices.");line(212,"CHOS is supplied without warranty.");
        line(248,"Back up data before selecting a disk.");line(284,"Enter accepts this agreement and continues.");
        ui_text(0,32,app_height-96,"MIT and font license notices are installed on disk.",0xc3d5ff);
    } else if(page==DESTINATION){line(80,"Select installation destination");information();}
    else if(page==LAYOUT){
        line(92,"Partition layout for the selected disk");line(140,"ALL EXISTING PARTITIONS WILL BE REPLACED.");
        line(184,"GPT: primary and backup metadata");line(220,"EFI System Partition / FAT32:");size(440,220,target.esp_bytes,WHITE);
        line(256,"CHOS system partition: remaining disk");line(292,"CHFS currently uses the first 4 MiB.");
        line(328,"Minimum required bytes:");amount(344,328,target.minimum_bytes,WHITE);
        ui_text(0,32,app_height-96,"This layout is a preview. Nothing has been written.",0xc3d5ff);
    } else if(page==CONFIRMATION){
        line(92,"Confirm destruction of the selected disk");line(140,"WARNING: all data on this disk will be lost.");
        line(184,"NVMe serial:");ui_text(4,224,184,disk.serial,WHITE);
        line(220,"Capacity:");size(224,220,disk.bytes,WHITE);
        line(264,"Type ERASE to authorize only this disk.");
        ui_textfield(&app_gui,80,32,306,app_width-64,confirm.erase,sizeof(confirm.erase));
        ui_text(0,32,app_height-96,"Enter authorizes actual partitioning and writes.",0xc3d5ff);
    } else {
        line(92,"Recovery tools are not implemented yet.");line(148,"Setup cannot repair or recover a filesystem.");
        line(204,"Use Live Desktop for read-only diagnostics.");line(260,"Keep the ISO and back up the virtual disk.");line(316,"Enter returns. L starts the live desktop.");
    }
    if(disk.phase==SETUP_PREPARE && !disk.error){
        begin(BLUE);ui_text(1,32,24,"CHOS ARM64 Setup",WHITE);line(148,"Preparing the confirmed destination...");line(208,"Writing and verifying GPT partition tables.");line(268,"Do not power off the virtual machine.");
    }
    if(app_status[0])ui_text(0,32,app_height-65,app_status,0xffe7a5);
    box(0,app_height-36,app_width,36,0xc9d3e6);
    ui_text(3,20,app_height-25,disk.phase?"Disk operation active. Exit is disabled.":"ENTER Continue    ESC Back    F3 Live desktop",0x152a50);
}
static void graph_frame(const char *title,unsigned current){
    begin(0x14396d);
    /* Original restrained banded gradient; no per-frame allocation or assets. */
    for(int i=0;i<8;i++)box(0,72+i*(app_height-108)/8,app_width,(app_height-108)/8+1,0x214d86u+(unsigned)i*0x020202u);
    box(0,0,app_width,72,0x0b2855);box(0,70,app_width,2,0x73b2df);
    rounded(24,17,38,38,0x3e9bc1);ui_text(2,36,21,"C",WHITE);
    ui_text(2,78,18,"CHOS Royale",WHITE);ui_text(3,80,46,"ARM64 installation environment",0xbadcf6);
    int side=app_width>=800?228:174;
    box(side,72,1,app_height-108,0x6085b2);
    static const char *const stages[]={"Prepare disk","Format partitions","Copy system files","Install boot files","Configure CHOS","Verify installation"};
    for(unsigned i=0;i<6;i++){
        int y=138+(int)i*38;rounded(24,y+2,12,12,i+1<current?0x77d5b0:(i+1==current?0xffd785:0x6c87aa));
        ui_text(3,46,y,stages[i],i+1==current?WHITE:0xc2d9ef);
    }
    ui_text(3,24,app_height-100,"Independent ARM64 OS",0xbadcf6);
    ui_text(2,side+28,100,title,WHITE);
    box(0,app_height-36,app_width,36,0x0b2855);ui_text(3,24,app_height-25,"CHOS Setup  /  Original kernel and native services",0xc5dff3);
}
static void graphical(void){
    int x=(app_width>=800?228:174)+28,w=app_width-x-28;
    graph_frame(page==CONFIGURATION && !disk.phase?"Personalize CHOS":disk.error?"Installation could not finish":disk.phase==SETUP_COMPLETE?"CHOS is ready":"Installing CHOS",disk.phase);
    if(disk.error){
        ui_text(0,x,158,"The destination may be incomplete.",WHITE);
        static const char *const errors[]={"","Disk identity or write protection changed.","GPT write, flush or read-back failed.","EFI partition or payload verification failed.","System volume or configuration write failed.","Final partition or system file check failed.","Installation log write/read-back failed."};
        ui_text(0,x,198,disk.error>=64?nvme_check_message(disk.error-64):errors[disk.error<=6?disk.error:5],0xffd9a0);
        ui_text(0,x,242,"Keep the ISO. Power off and back up the disk.",WHITE);
        ui_text(0,x,274,"Retry only a confirmed disposable destination.",WHITE);
        ui_text(0,x,306,"No rollback or filesystem repair is available.",WHITE);
        ui_button(&app_gui,90,KEY_F3,x,app_height-80,180,30,"Live Desktop",true);return;
    }
    if(disk.phase==SETUP_COMPLETE){
        ui_text(0,x,154,"CHOS installation completed successfully.",WHITE);
        ui_text(0,x,196,"GPT, EFI boot files and system data verified.",WHITE);
        ui_text(0,x,232,"Remove the ISO before restarting the VM.",WHITE);
        ui_text(0,x,268,"The installed desktop uses persistent storage.",WHITE);
        ui_text(3,x,304,"Log: /system/install-log on the installed volume.",0xc5dff3);
        ui_text(3,x,336,"Accounts and authentication remain unfinished.",0xc5dff3);
        ui_progress(&app_gui,x,app_height-132,w,18,100);
        ui_button(&app_gui,91,'r',app_width-188,app_height-80,160,30,"Restart",true);return;
    }
    if(disk.phase){
        static const char *const operations[]={"","Creating and verifying primary/backup GPT.","Formatting FAT32 and CHFS partition metadata.","Copying CHOS/KERNEL.BIN from pristine payload.","Installing EFI/BOOT/BOOTAA64.EFI.","Writing CHFS settings, documents and licenses.","Reading back partitions and all required files."};
        ui_text(0,x,164,operations[disk.phase<=6?disk.phase:6],WHITE);
        ui_text(3,x,206,"Overall progress counts completed installation stages.",0xc5dff3);
        ui_progress(&app_gui,x,236,w,24,disk.progress);amount(x,276,disk.progress,WHITE);ui_text(0,x+48,278,"percent",WHITE);
        if(target.total_bytes){size(x,320,target.completed_bytes,WHITE);ui_text(0,x+132,322,"of",WHITE);size(x+164,320,target.total_bytes,WHITE);ui_text(3,x,352,"Actual file data copied. Verification follows.",0xc5dff3);}
        else ui_text(0,x,322,"Working on the operation shown above.",WHITE);
        ui_text(3,x,app_height-86,"Do not power off while the disk writer is active.",0xffdea2);return;
    }
    rounded(x-12,138,w+24,190,ROYALE_APP_BG);
    ui_text(0,x,148,"Computer name",ROYALE_APP_INK);ui_textfield(&app_gui,70,x,174,w,confirm.computer,sizeof(confirm.computer));
    ui_radio(&app_gui,71,'1',x,214,"Deep blue",chosen.theme==0,true);
    ui_radio(&app_gui,72,'2',x+200,214,"Harbor green",chosen.theme==1,true);
    ui_text(0,x,258,"Clock offset, minutes:",ROYALE_APP_INK);
    char offset[22];unsigned at=0;int value=chosen.utc_offset;if(value<0){offset[at++]='-';value=-value;}number(offset+at,(unsigned)value);ui_text(4,x+164,258,offset,ROYALE_APP_INK);
    ui_button(&app_gui,73,'[',x+260,252,54,28,"-1h",true);ui_button(&app_gui,74,']',x+322,252,54,28,"+1h",true);
    ui_checkbox(&app_gui,75,'s',x,294,"Show clock seconds",chosen.clock_seconds,true);
    ui_text(3,x,334,"US keyboard / firmware display mode / fixed clock offset.",0xc5dff3);
    ui_text(3,x,352,"Account and password setup is not implemented.",0xc5dff3);
    if(app_status[0])ui_text(0,x,app_height-110,app_status,0xffdea2);
    ui_button(&app_gui,76,701,x,app_height-80,104,30,"< Back",true);
    ui_button(&app_gui,77,13,app_width-188,app_height-80,160,30,"Review erase",true);
}
void setup_key(unsigned key){
    if(disk.phase || disk.error){
        if(disk.phase==SETUP_COMPLETE && key=='r')(void)api(SYS_POWER,1,0,0);
        if(disk.error && (key==KEY_F3 || key=='l'))leave();return;
    }
    if(key==KEY_F3 || (key=='l' && (page==WELCOME || page==RECOVERY))){leave();return;}
    if(key==27 || key==701){if(page==WELCOME)leave();else go(page==RECOVERY?WELCOME:page-1);return;}
    if(page==WELCOME){
        if(key==KEY_UP)selection=(selection+2)%3;
        if(key==KEY_DOWN)selection=(selection+1)%3;
        if(key=='r'){go(RECOVERY);return;}
        if(key==13){if(selection==1)leave();else go(selection==2?RECOVERY:LICENSE);}return;
    }
    if(page==RECOVERY){if(key==13)go(WELCOME);return;}
    if(page==CONFIGURATION){
        if(key=='1' || key=='2')chosen.theme=key-'1';
        if(key=='s')chosen.clock_seconds=!chosen.clock_seconds;
        if(key=='['){chosen.utc_offset-=60;if(chosen.utc_offset<-720)chosen.utc_offset=-720;}
        if(key==']'){chosen.utc_offset+=60;if(chosen.utc_offset>840)chosen.utc_offset=840;}
    }
    if(key!=13)return;
    if(page==LICENSE){go(DESTINATION);return;}
    if(page==DESTINATION){if(disk.allowed)go(LAYOUT);else status("Destination unavailable. No disk writes.");return;}
    if(page==LAYOUT){go(CONFIGURATION);ui_modal_edit(&app_gui,70,confirm.computer,sizeof(confirm.computer));return;}
    if(page==CONFIGURATION){
        copy(chosen.computer,confirm.computer,sizeof(chosen.computer));
        if(api(SYS_CONFIG,2,(uintptr_t)&chosen,0)<0){status("Invalid computer name or system setting.");return;}
        confirm.erase[0]=0;go(CONFIRMATION);ui_modal_edit(&app_gui,80,confirm.erase,sizeof(confirm.erase));return;
    }
    if(page==CONFIRMATION){
        int64_t result=api(SYS_SETUP,1,(uintptr_t)&confirm,disk.token);
        if(result<0)status("Confirmation refused. No disk writes.");
        else {ui_modal_end(&app_gui);status("");}
    }
}
void setup_widget(unsigned id){if(id==70)status("");}
unsigned setup_painted_phase(void){return disk.phase;}
void setup_paint(void){
    if(api(SYS_SETUP,0,(uintptr_t)&disk,0)<0 || api(SYS_SETUP,3,(uintptr_t)&target,0)<0){begin(BLUE);line(96,"CHOS Setup service unavailable.");return;}
    if(!loaded && api(SYS_CONFIG,0,(uintptr_t)&chosen,0)==0)loaded=true;
    if(page==CONFIGURATION || disk.error || disk.phase>=SETUP_FORMAT)graphical();else blue();
}
