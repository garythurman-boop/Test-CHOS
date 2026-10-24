/* Firmware-described serial only; firmware protocols are never used after exit. */
#include "diagnostic.h"
#include "platform.h"
#include "console.h"
#include "mmu.h"
#include "timer.h"
#include "storage.h"
#include "usb.h"
static const struct chos_boot_info *boot;
static uint64_t uart;
static bool serial_ready;
static void send(char c){
    if(!serial_ready)return;
    volatile uint32_t *reg=(void *)(uintptr_t)uart;
    /* Never let a stuck transmitter prevent diagnostics or boot. */
    for(unsigned i=0;i<4096;i++)if(!(reg[6]&(1u<<5))){reg[0]=(unsigned char)c;return;}
    serial_ready=false;
}
void diagnostic_log(const char *s){
    diagnostic_append(s);diagnostic_append("\n");
    if(s)for(unsigned i=0;i<512 && s[i];i++)send(s[i]);send('\r');send('\n');
}
void diagnostic_value(const char *label,uint64_t value,bool hex){
    char line[128];unsigned n=0;for(;n<75 && label[n];n++)line[n]=label[n];
    char digits[21];unsigned count=0,base=hex?16:10;
    do{digits[count++]="0123456789abcdef"[value%base];value/=base;}while(value);
    if(hex){line[n++]='0';line[n++]='x';}
    while(count)line[n++]=digits[--count];line[n]=0;diagnostic_log(line);
}
static const char *kind(const struct chos_pci_device *p){
    if(p->base_class==12 && p->subclass==3){
        switch(p->interface){case 0:return "USB UHCI unsupported";case 16:return "USB OHCI unsupported";case 32:return "USB EHCI unsupported";case 48:return "USB xHCI";default:return "USB unknown unsupported";}
    }
    if(p->base_class==1)return p->subclass==8 && p->interface==2?"Storage NVMe":"Storage unsupported";
    if(p->base_class==3)return "Display (uses firmware GOP)";
    if(p->vendor==0x1af4 && p->device==0x1052)return "Virtio input";
    return "Other PCI device";
}
void diagnostic_inventory(const struct chos_boot_info *b){
    diagnostic_log("CHOS diagnostic report v1; inventory read-only");
    diagnostic_log(b->firmware_vendor);
    diagnostic_value("Firmware revision: ",b->firmware_revision,true);
    diagnostic_value("UEFI revision: ",b->uefi_revision,true);
    diagnostic_value("Firmware entry EL: ",b->entry_el,false);
    uint64_t el;__asm__ volatile("mrs %0, CurrentEL":"=r"(el));
    diagnostic_value("Current kernel EL: ",el>>2,false);
    uint32_t mask;unsigned count;bool valid=platform_tables(b,&mask,&count);
    diagnostic_log(valid?"ACPI XSDT/checksums: PASS":"ACPI unavailable or invalid");
    diagnostic_value("ACPI table count: ",count,false);
    diagnostic_value("ACPI mask APIC/GTDT/MCFG/FADT/SPCR: ",mask,true);
    diagnostic_value("Framebuffer width: ",b->width,false);diagnostic_value("Framebuffer height: ",b->height,false);
    diagnostic_value("Framebuffer stride: ",b->stride,false);diagnostic_value("GOP pixel format: ",b->pixel_format,false);
    diagnostic_value("PCI retained: ",b->pci_count,false);diagnostic_value("PCI firmware handles: ",b->pci_total,false);
    for(unsigned i=0;i<b->pci_count && i<32;i++){
        const struct chos_pci_device *p=&b->pci[i];
        char line[]="PCI 0000:00:00.0 0000:0000 class 000000";
        const char *hex="0123456789abcdef";
        unsigned at[]={4,9,12,15,17,22,33,35,37},width[]={4,2,2,1,4,4,2,2,2};
        uint32_t v[]={p->segment,p->bus,p->slot,p->function,p->vendor,p->device,p->base_class,p->subclass,p->interface};
        for(unsigned j=0;j<9;j++)for(unsigned k=0;k<width[j];k++)line[at[j]+k]=hex[(v[j]>>((width[j]-k-1)*4))&15];
        diagnostic_log(line);diagnostic_log(kind(p));
    }
    diagnostic_log("Native serial: ACPI SPCR PL011 or unavailable");
}
void diagnostic_init(const struct chos_boot_info *b){
    boot=b;diagnostic_reset();uart=0;serial_ready=false;(void)platform_serial(b,&uart);
    diagnostic_inventory(b);
}
void diagnostic_map_serial(void){
    serial_ready=uart && mmu_device(uart,4096);
    char data[256];for(unsigned off=0;serial_ready && off<diagnostic_size();){unsigned n=diagnostic_read(off,data,sizeof(data));if(!n)break;off+=n;for(unsigned i=0;i<n;i++){if(data[i]=='\n')send('\r');send(data[i]);}}
}
unsigned diagnostic_screen(unsigned page){
    if(!boot)return 1;
    const unsigned panel=(boot->reserved&CHOS_INPUT_TEST_BOOT)?204:0;
    const unsigned rows=(boot->height-96-panel)/18,cols=(boot->width-32)/12;
    console_rect(0,0,boot->width,boot->height,0x102b50);
    console_text(16,16,"CHOS HARDWARE DIAGNOSTICS",0xffffff);
    console_text(16,42,"AUTOMATIC PAGES / NO INSTALLATION",0x9ed7ff);
    unsigned line=0,y=0,x=0,skip=page*rows;char block[4096];
    for(unsigned off=0;off<diagnostic_size();){
        unsigned n=diagnostic_read(off,block,sizeof(block));if(!n)break;off+=n;
        for(unsigned i=0;i<n;i++){
            char c=block[i];
            if(c=='\n' || x==cols){line++;x=0;if(c=='\n')continue;}
            if(line>=skip && line<skip+rows){char text[2]={c,0};y=78+(line-skip)*18;console_text(16+x*12,y,text,0xe0efff);}
            x++;
        }
    }
    console_number(boot->width-64,16,page+1);
    console_rect(0,boot->height-24,boot->width,24,0x244da8);
    console_text(16,boot->height-20,"REPORT IN SERIAL / READ-ONLY BUFFER",0xffffff);
    if(panel)diagnostic_input_panel();
    return line?(line+rows-1)/rows:1;
}

/* Single volatile event only. Never append event codes/content to the report,
 * serial stream, files or persistent configuration. */
static uint64_t test_events;
static unsigned key_code,key_value,mouse_buttons;
void diagnostic_input_event(const struct input_event *e){
    test_events++;
    if(e->type==1 && e->code<256){key_code=e->code;key_value=(unsigned)e->value;}
    if(e->type==1 && e->code>=272 && e->code<=274){unsigned mask=1u<<(e->code-272);if(e->value)mouse_buttons|=mask;else mouse_buttons&=~mask;}
}
void diagnostic_input_panel(void){
    if(!boot || !(boot->reserved&CHOS_INPUT_TEST_BOOT))return;
    const struct usb_status *s=usb_status();const struct input_state *i=input_get();unsigned y=boot->height-184;
    console_rect(0,y-36,boot->width,196,0x163a64);
    console_text(16,y-32,"CONTROLLER INITIALIZED",0x9ed7ff);console_text(304,y-32,s->initialized?"YES":"NO",0xffffff);
    console_text(400,y-32,"PORTS",0x9ed7ff);console_number(496,y-32,s->ports);
    console_text(16,y-14,"ENUMERATED",0x9ed7ff);console_number(160,y-14,s->enumerated);
    console_text(232,y-14,"HID",0x9ed7ff);console_number(280,y-14,s->keyboards+s->mice);
    console_text(328,y-14,"CONNECTED",0x9ed7ff);console_number(460,y-14,s->connected);
    console_text(16,y+4,"CHOS USB HARDWARE TEST",0xffffff);
    char id[]="0000:0000";for(unsigned n=0;n<4;n++){id[n]="0123456789ABCDEF"[(s->id>>((3-n)*4))&15];id[n+5]="0123456789ABCDEF"[(s->id>>(16+(3-n)*4))&15];}
    console_text(352,y+4,"PCI",0x9ed7ff);console_text(412,y+4,id,0xffffff);
    console_text(16,y+28,"USB KEYBOARD",0x9ed7ff);console_text(184,y+28,s->keyboards?"READY":"NOT DETECTED",0xffffff);
    console_text(16,y+46,"USB MOUSE",0x9ed7ff);console_text(184,y+46,s->mice?"READY":"NOT DETECTED",0xffffff);
    console_text(16,y+64,"PATH",0x9ed7ff);console_text(184,y+64,s->initialized?"XHCI POLLED":"NO ACTIVE USB PATH",0xffffff);
    console_text(16,y+82,"STAGE / REASON",0x9ed7ff);console_number(232,y+82,s->stage);console_number(328,y+82,s->reason);
    console_text(16,y+100,"LAST KEY CODE / VALUE",0x9ed7ff);console_number(304,y+100,key_code);console_number(448,y+100,key_value);
    console_text(16,y+118,"POINTER X Y BUTTONS",0x9ed7ff);console_number(304,y+118,(unsigned)i->x);console_number(400,y+118,(unsigned)i->y);console_number(496,y+118,mouse_buttons);
    console_text(16,y+136,"LIVE EVENTS",0x9ed7ff);console_number(184,y+136,test_events);
    console_text(280,y+136,"ENUM",0x9ed7ff);console_number(352,y+136,s->enum_stage);console_number(400,y+136,s->enum_reason);console_number(472,y+136,s->enum_port);
    console_rect(0,boot->height-24,boot->width,24,0x244da8);
    console_text(16,boot->height-20,"NO KEY CONTENT LOGGED / NO DISK WRITES",0xffffff);
}
