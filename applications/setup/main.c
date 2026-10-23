#include "app.h"
static unsigned stage;
static struct chos_setup disk;
static struct chos_confirm confirm={.computer="CHOS-PC"};
void setup_key(unsigned key){
    if(disk.phase || disk.error){if(disk.phase==4 && key=='r')(void)api(SYS_POWER,1,0,0);return;}
    if(prompting){
        if(prompt_key(key)){
            if(stage==3){copy(confirm.computer,prompt_value,sizeof(confirm.computer));stage=4;}
            else if(stage==4){copy(confirm.erase,prompt_value,sizeof(confirm.erase));int64_t result=api(SYS_SETUP,1,(uintptr_t)&confirm,disk.token);if(result<0)status("Confirmation refused. No disk writes.");}
        }
        return;
    }
    if(key==27){(void)api(SYS_EXIT,0,0,0);return;}
    if(key==13){
        if(stage==0){stage=1;return;}
        if(stage==1){stage=2;return;}
        if(stage==2 && disk.allowed){stage=3;prompt("Computer name (letters, digits, hyphen)",confirm.computer);return;}
        if(stage==4)prompt("Type ERASE followed by the exact serial","");
    }
}
void setup_click(int x,int y){(void)x;if(y>=app_height-70)setup_key(13);}
void setup_paint(void){
    if(api(SYS_SETUP,0,(uintptr_t)&disk,0)<0){begin(0xe5eef8);label(20,"Setup service unavailable");return;}
    begin(0xe5eef8);label(16,"Welcome to CHOS Setup");
    if(disk.error){label(60,"Installation failed. Disk may be incomplete.");label(90,"Keep the ISO. Reboot and retry this disk.");label(120,"Do not use this disk for valuable data.");char value[21];number(value,disk.error);label(150,"Error code:");text(170,150,value,0xa13030);return;}
    if(disk.phase){
        if(disk.phase==4){label(64,"CHOS installation completed successfully.");label(100,"Remove the ISO, then press R to restart.");label(134,"System files persist on the installed disk.");label(168,"Accounts/passwords are not implemented yet.");status("Core installation complete. Fusion unverified.");return;}
        label(64,disk.phase==1?"Creating GPT partitions":(disk.phase==2?"Copying EFI boot and kernel files":"Configuring the CHOS system volume"));
        box(20,120,app_width-40,24,0xa6c0d7);box(20,120,(app_width-40)*(int)disk.progress/100,24,0x398cbb);
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
    if(stage>=3){label(62,"WARNING: ALL DATA ON THIS DISK IS ERASED");label(102,disk.serial);char size[21];number(size,disk.bytes/1048576);label(138,"Disk capacity MiB:");text(250,138,size,0xa13030);label(174,"Computer name:");text(220,174,confirm.computer,0x204b72);label(214,"Enter opens the final typed confirmation.");label(250,"Accounts, password and timezone setup later.");}
    status("Enter continues. Escape exits. No silent erase.");
}
