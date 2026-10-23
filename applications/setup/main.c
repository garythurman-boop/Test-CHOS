#include "app.h"
static unsigned stage;
static struct chos_config chosen;
static bool config_loaded;
static struct chos_setup disk;
static struct chos_confirm confirm={.computer="CHOS-PC"};
void setup_key(unsigned key){
    if(disk.phase || disk.error){if(disk.phase==4 && key=='r')(void)api(SYS_POWER,1,0,0);return;}
    if(prompting){
        if(prompt_key(key)){
            if(stage==3){copy(confirm.computer,prompt_value,sizeof(confirm.computer));copy(chosen.computer,confirm.computer,sizeof(chosen.computer));stage=4;status("Review the disk and system settings before ERASE.");}
            else if(stage==4){copy(confirm.erase,prompt_value,sizeof(confirm.erase));int64_t result=api(SYS_SETUP,1,(uintptr_t)&confirm,disk.token);if(result<0)status("Confirmation refused. No disk writes.");}
        }
        if(!prompting && stage==3)stage=2;
        return;
    }
    if(key==701){if(stage==4){stage=3;prompt("Computer name (letters, digits, hyphen)",confirm.computer);}else if(stage)stage--;return;}
    if(stage==4) {
        if(key=='1' || key=='2')chosen.theme=key-'1';
        if(key=='s')chosen.clock_seconds=!chosen.clock_seconds;
        if(key=='['){chosen.utc_offset-=60;if(chosen.utc_offset<-720)chosen.utc_offset=-720;}
        if(key==']'){chosen.utc_offset+=60;if(chosen.utc_offset>840)chosen.utc_offset=840;}
    }
    if(key==27){(void)api(SYS_EXIT,0,0,0);return;}
    if(key==13){
        if(stage==0){stage=1;return;}
        if(stage==1){stage=2;return;}
        if(stage==2 && disk.allowed){stage=3;prompt("Computer name (letters, digits, hyphen)",confirm.computer);return;}
        if(stage==4) {
            if(api(SYS_CONFIG,2,(uintptr_t)&chosen,0)<0){status("Invalid system configuration. No disk writes.");return;}
            prompt("Type ERASE followed by the exact serial","");
        }
    }
}
void setup_paint(void){
    if(api(SYS_SETUP,0,(uintptr_t)&disk,0)<0){begin(0xe5eef8);label(20,"Setup service unavailable");return;}
    if(!config_loaded && api(SYS_CONFIG,0,(uintptr_t)&chosen,0)==0)config_loaded=true;
    begin(ROYALE_APP_BG);label(16,"Welcome to CHOS Setup");
    if(disk.error){label(60,"Installation failed. Disk may be incomplete.");label(90,"Keep the ISO. Reboot and retry this disk.");label(120,"Do not use this disk for valuable data.");char value[21];number(value,disk.error);label(150,"Error code:");text(170,150,value,0xa13030);return;}
    if(disk.phase){
        if(disk.phase==4){label(64,"CHOS installation completed successfully.");label(100,"Remove the ISO, then press R to restart.");label(134,"System files persist on the installed disk.");label(168,"Accounts/passwords are not implemented yet.");ui_button(&app_gui,40,'r',app_width-160,app_height-64,140,30,"Restart",true);status("Core installation complete. Fusion unverified.");return;}
        label(64,disk.phase==1?"Creating GPT partitions":(disk.phase==2?"Copying EFI boot and kernel files":"Configuring the CHOS system volume"));
        ui_progress(&app_gui,20,120,app_width-40,24,disk.progress);
        char value[21];number(value,disk.progress);text(24,157,value,0x204b72);status("Writing explicitly confirmed destination.");return;
    }
    if(stage==0){label(62,"Install CHOS to a selected NVMe virtual disk.");label(98,"Enter starts. Escape exits.");label(134,"Setup will erase the selected destination.");label(170,"Use a disposable virtual disk only.");}
    if(stage==1){label(62,"CHOS Setup Agreement 1");label(98,"CHOS is licensed under the MIT License.");label(134,"Keep its copyright and license notices.");label(170,"CHOS is supplied without warranty.");label(206,"Setup erases only a confirmed destination.");label(242,"Back up files before choosing a disk.");label(278,"Enter accepts this license and continues.");}
    if(stage==2){
        label(62,"Destination: NVMe controller 0, namespace 1");label(94,"Serial identity:");text(220,94,disk.serial,0x204b72);
        char size[21];number(size,disk.bytes/1048576);label(126,"Capacity MiB:");text(220,126,size,0x204b72);
        number(size,disk.sector);label(158,"Logical sector bytes:");text(280,158,size,0x204b72);
        label(198,disk.allowed?"GPT + EFI partition + CHOS system volume":"Installation disabled for this boot/device.");
        label(230,"Requires optical boot and adequate capacity.");label(266,"No disk has been modified. Enter selects.");
    }
    if(stage>=3){label(62,"WARNING: ALL DATA ON THIS DISK IS ERASED");label(102,disk.serial);char size[21];number(size,disk.bytes/1048576);label(138,"Disk capacity MiB:");text(250,138,size,0xa13030);label(174,"Computer name:");text(220,174,confirm.computer,0x204b72);
        ui_radio(&app_gui,31,'1',12,208,"Deep blue",chosen.theme==0,true);ui_radio(&app_gui,32,'2',220,208,"Harbor green",chosen.theme==1,true);
        label(248,"Clock offset, minutes:");char offset[21];unsigned at=0;int value=chosen.utc_offset;if(value<0){offset[at++]='-';value=-value;}number(offset+at,(unsigned)value);text(180,248,offset,ROYALE_APP_INK);
        ui_button(&app_gui,33,'[',300,242,70,28,"-1 h",true);ui_button(&app_gui,34,']',380,242,70,28,"+1 h",true);
        ui_checkbox(&app_gui,35,'s',12,280,"Show clock seconds",chosen.clock_seconds,true);
        label(316,"No accounts or authentication. Enter reviews ERASE.");}
    if(stage)ui_button(&app_gui,29,701,16,app_height-64,110,30,"< Back",true);
    ui_button(&app_gui,30,13,app_width-160,app_height-64,140,30,stage==0?"Begin setup":"Continue",stage!=2 || disk.allowed);
    if(!app_status[0])status("Enter continues. Escape exits. No silent erase.");
}
