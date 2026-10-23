#include "efi.h"
#include "boot.h"
#include "display_mode.h"
#include <stdbool.h>
#include "kernel_image.h"
#include "pci.h"
static const struct guid acpi_guid={0x8868e871,0xe4f1,0x11d3,{0xbc,0x22,0,0x80,0xc7,0x3c,0x88,0x81}};
static struct guid path_guid={0x09576e91,0x6d3f,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
static struct guid loaded_guid={0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0,0xa0,0xc9,0x69,0x72,0x3b}};
static struct guid fs_guid={0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
static struct guid info_guid={0x09576e92,0x6d3f,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
static struct guid gop_guid={0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};
static status fail(struct system_table *st,const char16 *msg,status code) {
    st->console_out->output(st->console_out,msg);
    return EFI_ERROR(code)?code:EFI_INVALID_PARAMETER;
}
static void pci_inventory(struct system_table *st,struct chos_boot_info *b){
    struct guid protocol={0x4cf5b200,0x68b8,0x4ca5,{0x9e,0xec,0xb2,0x3e,0x3f,0x50,0x02,0x9a}};
    handle *handles=0;uint64_t count=0;
    if(!st->boot->locate_handle_buffer || EFI_ERROR(st->boot->locate_handle_buffer(2,&protocol,0,&count,&handles)))return;
    if(count<=256 && handles){
        b->pci_total=(uint32_t)count;
        for(unsigned i=0;i<count;i++){
            struct pci_io *p=0;uint64_t segment,bus,slot,function;uint32_t header[3];
            if(EFI_ERROR(st->boot->handle_protocol(handles[i],&protocol,(void **)&p)) || !p || !p->pci_read || !p->get_location || EFI_ERROR(p->get_location(p,&segment,&bus,&slot,&function)) || segment>65535 || bus>255 || slot>31 || function>7 || EFI_ERROR(p->pci_read(p,2,0,3,header)) || (header[0]&65535)==65535)continue;
            struct chos_pci_device record,*d=&record;
            *d=(struct chos_pci_device){.vendor=(uint16_t)header[0],.device=(uint16_t)(header[0]>>16),.base_class=(uint8_t)(header[2]>>24),.subclass=(uint8_t)(header[2]>>16),.interface=(uint8_t)(header[2]>>8),.revision=(uint8_t)header[2],.segment=(uint16_t)segment,.bus=(uint8_t)bus,.slot=(uint8_t)slot,.function=(uint8_t)function};
            pci_inventory_keep(b,d);
            char16 line[]=u"CHOS: PCI 0000:00:00.0 ID 0000:0000 CLASS 000000\r\n";
            const char16 hex[]=u"0123456789abcdef";
            const unsigned at[]={10,15,18,21,26,31,42};
            const unsigned widths[]={4,2,2,1,4,4,6};
            const uint32_t values[]={(uint32_t)segment,(uint32_t)bus,(uint32_t)slot,(uint32_t)function,d->vendor,d->device,header[2]>>8};
            for(unsigned j=0;j<7;j++)for(unsigned k=0;k<widths[j];k++)line[at[j]+k]=hex[(values[j]>>((widths[j]-k-1)*4))&15];
            st->console_out->output(st->console_out,line);
        }
    }
    if(handles)st->boot->free_pool(handles);
}
status efi_main(handle image,struct system_table *st) {
    struct boot_services *bs=st->boot;
    st->console_out->output(st->console_out,(char16 *)u"CHOS - ARM64 UEFI loader\r\n");
    uint64_t entry_el;__asm__ volatile("mrs %0, CurrentEL":"=r"(entry_el));
    if(entry_el!=4 && entry_el!=8)return fail(st,(char16 *)u"CHOS: firmware must enter at EL1 or EL2\r\n",EFI_INVALID_PARAMETER);
    st->console_out->output(st->console_out,entry_el==8?(char16 *)u"CHOS: firmware entry EL2\r\n":(char16 *)u"CHOS: firmware entry EL1\r\n");
    if(entry_el==8){
        uint64_t hcr,tcr,sctlr;
        __asm__ volatile("mrs %0, hcr_el2; mrs %1, tcr_el2; mrs %2, sctlr_el2":"=r"(hcr),"=r"(tcr),"=r"(sctlr));
        if((hcr&(1ULL<<34)) || !(sctlr&1) || (sctlr&(1ULL<<25)) || (tcr&(1ULL<<32)) || (tcr&63)<16 || (tcr&63)>39 || ((tcr>>16)&7)>5 || ((tcr>>14)&3)==3)
            return fail(st,(char16 *)u"CHOS: unsupported EL2 firmware translation regime\r\n",EFI_INVALID_PARAMETER);
    }
    bs->set_watchdog_timer(0,0,0,0);
    struct loaded_image *loaded=0;
    struct filesystem *fs=0;
    struct file *root=0,*kernel=0;
    status s=bs->handle_protocol(image,&loaded_guid,(void **)&loaded);
    if (EFI_ERROR(s)) return fail(st,(char16 *)u"CHOS: loaded-image protocol failed\r\n",s);
    s=bs->handle_protocol(loaded->device,&fs_guid,(void **)&fs);
    if (EFI_ERROR(s)) return fail(st,(char16 *)u"CHOS: boot filesystem unavailable\r\n",s);
    s=fs->open_volume(fs,&root);
    if (EFI_ERROR(s)) return fail(st,(char16 *)u"CHOS: opening boot volume failed\r\n",s);
    s=root->open(root,&kernel,(char16 *)u"\\CHOS\\KERNEL.BIN",1,0);
    if (EFI_ERROR(s)) return fail(st,(char16 *)u"CHOS: CHOS/KERNEL.BIN missing\r\n",s);
    _Alignas(8) uint8_t file_info[512];
    uint64_t info_size=sizeof(file_info);
    s=kernel->get_info(kernel,&info_guid,&info_size,file_info);
    if (EFI_ERROR(s) || info_size<16) return fail(st,(char16 *)u"CHOS: kernel metadata failed\r\n",s);
    uint64_t file_size=((uint64_t *)file_info)[1];
    uint8_t header[CHOS_IMAGE_HEADER_BYTES];uint64_t header_bytes=sizeof(header);
    if(file_size<sizeof(header) || file_size>CHOS_IMAGE_MAX_FILE || EFI_ERROR(kernel->read(kernel,&header_bytes,header)) || header_bytes!=sizeof(header))
        return fail(st,(char16 *)u"CHOS: invalid kernel image header\r\n",EFI_INVALID_PARAMETER);
    struct kernel_image_layout layout;
    if(!kernel_image_validate(header,sizeof(header),file_size,&layout) || !kernel->set_position || EFI_ERROR(kernel->set_position(kernel,0)))
        return fail(st,(char16 *)u"CHOS: invalid kernel image header\r\n",EFI_INVALID_PARAMETER);
    uint64_t base=0,stack_base=0;
    s=bs->allocate_pages(0,1,layout.memory_bytes/4096,&base);
    if(EFI_ERROR(s))return fail(st,(char16 *)u"CHOS: kernel allocation failed\r\n",s);
    s=bs->allocate_pages(0,2,CHOS_BOOT_STACK_ALLOCATION/4096,&stack_base);
    if(EFI_ERROR(s) || !kernel_boot_regions(base,layout.memory_bytes,stack_base,CHOS_BOOT_STACK_ALLOCATION))
        return fail(st,(char16 *)u"CHOS: bootstrap regions rejected\r\n",EFI_INVALID_PARAMETER);
    for(uint64_t i=0;i<layout.memory_bytes;i++)((volatile uint8_t *)(uintptr_t)base)[i]=0;
    for(uint64_t i=0;i<CHOS_BOOT_STACK_ALLOCATION;i++)((volatile uint8_t *)(uintptr_t)stack_base)[i]=0;
    uint64_t read_size=file_size;
    s=kernel->read(kernel,&read_size,(void *)(uintptr_t)base);
    kernel->close(kernel);
    uint32_t test_flags=0;
    struct file *config=0;
    if(!EFI_ERROR(root->open(root,&config,(char16 *)u"\\CHOS\\TEST.CFG",1,0))) {
        char content[32]; uint64_t bytes=sizeof(content);
        status config_status=config->read(config,&bytes,content); config->close(config);
        const char expected[]="CHOS STORAGE TEST V1";
        unsigned equal=!EFI_ERROR(config_status) && bytes==sizeof(expected)-1;
        for(unsigned i=0;equal && i<sizeof(expected)-1;i++) if(content[i]!=expected[i]) equal=0;
        if(equal) test_flags=1;
    }
    if(!EFI_ERROR(root->open(root,&config,(char16 *)u"\\CHOS\\DIAG.CFG",1,0))) {
        char content[32];uint64_t bytes=sizeof(content);
        status cs=config->read(config,&bytes,content);config->close(config);
        const char expected[]="CHOS DIAGNOSTICS V1";bool equal=!EFI_ERROR(cs) && bytes==sizeof(expected)-1;
        for(unsigned i=0;equal && i<sizeof(expected)-1;i++)if(content[i]!=expected[i])equal=false;
        if(equal && !test_flags)test_flags=2;
        const char input_test[]="CHOS INPUT TEST V1";equal=!EFI_ERROR(cs) && bytes==sizeof(input_test)-1;
        for(unsigned i=0;equal && i<sizeof(input_test)-1;i++)if(content[i]!=input_test[i])equal=false;
        if(equal && !test_flags)test_flags=6;
    }
    uint32_t requested_w=1024,requested_h=768;
    if(!EFI_ERROR(root->open(root,&config,(char16 *)u"\\CHOS\\DISPLAY.CFG",1,0))) {
        char content[16];uint64_t bytes=sizeof(content);
        status cs=config->read(config,&bytes,content);config->close(config);
        if(EFI_ERROR(cs) || !display_request(content,(unsigned)bytes,&requested_w,&requested_h))
            st->console_out->output(st->console_out,(char16 *)u"CHOS: invalid DISPLAY.CFG; using preferred mode\r\n");
    }
    root->close(root);
    if (EFI_ERROR(s)||read_size!=file_size) return fail(st,(char16 *)u"CHOS: incomplete kernel read\r\n",EFI_INVALID_PARAMETER);
    struct chos_boot_info *bi=0;
    s=bs->allocate_pool(2,sizeof(*bi),(void **)&bi);
    if (EFI_ERROR(s)) return fail(st,(char16 *)u"CHOS: boot information allocation failed\r\n",s);
    for (uint64_t i=0;i<sizeof(*bi);i++) ((uint8_t *)bi)[i]=0;
    bi->magic=CHOS_BOOT_MAGIC; bi->reserved=test_flags;bi->boot_contract=CHOS_BOOT_CONTRACT;
    bi->kernel_base=base;bi->kernel_memory_bytes=layout.memory_bytes;bi->stack_base=stack_base;bi->stack_bytes=CHOS_BOOT_STACK_ALLOCATION;
    bi->firmware_revision=st->firmware_revision;bi->uefi_revision=st->header.revision;
    if(st->vendor)for(unsigned i=0;i<63 && st->vendor[i];i++)bi->firmware_vendor[i]=st->vendor[i]>=32 && st->vendor[i]<127?(char)st->vendor[i]:'?';
    char16 firmware[80];unsigned fw=0;
    const char prefix[]="CHOS: firmware ";for(unsigned i=0;prefix[i];i++)firmware[fw++]=(char16)prefix[i];
    for(unsigned i=0;bi->firmware_vendor[i] && fw<76;i++)firmware[fw++]=(char16)bi->firmware_vendor[i];
    firmware[fw++]=13;firmware[fw++]=10;firmware[fw]=0;st->console_out->output(st->console_out,firmware);
    char16 revisions[]=u"CHOS: firmware revision 00000000 UEFI 00000000\r\n";
    for(unsigned i=0;i<8;i++){revisions[24+i]=(char16)"0123456789abcdef"[(bi->firmware_revision>>((7-i)*4))&15];revisions[38+i]=(char16)"0123456789abcdef"[(bi->uefi_revision>>((7-i)*4))&15];}
    st->console_out->output(st->console_out,revisions);
    pci_inventory(st,bi);
    void *pristine=0;
    s=bs->allocate_pool(2,file_size,&pristine);
    if(EFI_ERROR(s))return fail(st,(char16 *)u"CHOS: payload snapshot allocation failed\r\n",s);
    for(uint64_t i=0;i<file_size;i++)((uint8_t *)pristine)[i]=((const uint8_t *)(uintptr_t)base)[i];
    bi->kernel_source=(uintptr_t)pristine;bi->kernel_size=file_size;
    const uint8_t *path=0;
    if(!EFI_ERROR(bs->handle_protocol(loaded->device,&path_guid,(void **)&path)) && path) {
        for(unsigned at=0;at+4<=4096;) {
            unsigned length=(unsigned)path[at+2]|((unsigned)path[at+3]<<8);
            if(length<4 || length>4096-at || path[at]==0x7f)break;
            if(path[at]==4 && path[at+1]==2)bi->boot_medium=2;
            if(path[at]==4 && path[at+1]==1 && length>=42 && path[at+40]==2 && path[at+41]==2) {
                if(!bi->boot_medium)bi->boot_medium=1;
                for(unsigned j=0;j<16;j++)bi->esp_guid[j]=path[at+24+j];
            }
            at+=length;
        }
    }
    struct efi_time time;
    if(st->runtime_services && !EFI_ERROR(st->runtime_services->get_time(&time,0)) && time.hour<24 && time.minute<60 && time.second<60) {
        bi->clock_seconds=(uint64_t)time.hour*3600+(uint64_t)time.minute*60+time.second;
        __asm__ volatile("mrs %0, cntpct_el0":"=r"(bi->clock_counter));
        bi->clock_valid=1;
    }
    if(st->configuration_count>1024) return fail(st,(char16 *)u"CHOS: invalid configuration table count\r\n",EFI_INVALID_PARAMETER);
    const struct configuration_entry *entries=st->configuration_table;
    for(uint64_t i=0;i<st->configuration_count;i++) {
        unsigned equal=1;
        for(unsigned j=0;j<sizeof(struct guid);j++)
            if(((const uint8_t *)&entries[i].guid)[j]!=((const uint8_t *)&acpi_guid)[j]) equal=0;
        if(equal) bi->rsdp=(uint64_t)(uintptr_t)entries[i].table;
    }
    st->console_out->output(st->console_out,bi->rsdp?(char16 *)u"CHOS: ACPI 2.x RSDP present\r\n":(char16 *)u"CHOS: ACPI 2.x RSDP MISSING\r\n");
    struct gop *gop=0;
    s=bs->locate_protocol(&gop_guid,0,(void **)&gop);
    if (EFI_ERROR(s)||!gop||!gop->mode||!gop->mode->info)
        return fail(st,(char16 *)u"CHOS: GOP framebuffer required\r\n",EFI_INVALID_PARAMETER);
    /* QueryMode may allocate a firmware pool. Release every result before the
     * final memory-map snapshot. Only request a mode the firmware enumerated. */
    uint32_t candidate=UINT32_MAX;
    if(gop->query_mode && gop->set_mode && gop->mode->max_mode<=256) {
        for(uint32_t i=0;i<gop->mode->max_mode;i++) {
            struct gop_info *info=0;uint64_t bytes=0;
            status qs=gop->query_mode(gop,i,&bytes,&info);
            if(!EFI_ERROR(qs) && info && bytes>=sizeof(*info) &&
               display_mode_supported(info->width,info->height,info->stride,info->pixel_format) &&
               info->width==requested_w && info->height==requested_h)candidate=i;
            if(info)bs->free_pool(info);
        }
        if(candidate!=UINT32_MAX && candidate!=gop->mode->mode && EFI_ERROR(gop->set_mode(gop,candidate)))
            st->console_out->output(st->console_out,(char16 *)u"CHOS: mode switch failed; retaining firmware mode\r\n");
        if(candidate==UINT32_MAX)st->console_out->output(st->console_out,(char16 *)u"CHOS: requested mode unavailable; retaining firmware mode\r\n");
    }
    if(!gop->mode || !gop->mode->info)return fail(st,(char16 *)u"CHOS: missing active mode\r\n",EFI_INVALID_PARAMETER);
    struct gop_info *gi=gop->mode->info;
    if (!display_mode_supported(gi->width,gi->height,gi->stride,gi->pixel_format) ||
        (uint64_t)gi->stride*gi->height*4>gop->mode->framebuffer_size)
        return fail(st,(char16 *)u"CHOS: unsupported framebuffer mode\r\n",EFI_INVALID_PARAMETER);
    bi->framebuffer=gop->mode->framebuffer;
    bi->framebuffer_size=gop->mode->framebuffer_size;
    bi->width=gi->width; bi->height=gi->height;
    bi->stride=gi->stride; bi->pixel_format=gi->pixel_format;
    /* Synchronize newly loaded instructions before transferring control.
     * CTR_EL0 describes separate minimum data and instruction cache line sizes. */
    uint64_t ctr;
    __asm__ volatile("mrs %0, ctr_el0":"=r"(ctr));
    uint64_t dline=4ULL<<((ctr>>16)&15), iline=4ULL<<(ctr&15);
    for (uint64_t a=base&~(dline-1);a<base+file_size;a+=dline)
        __asm__ volatile("dc cvau, %0"::"r"(a):"memory");
    __asm__ volatile("dsb ish":::"memory");
    for (uint64_t a=base&~(iline-1);a<base+file_size;a+=iline)
        __asm__ volatile("ic ivau, %0"::"r"(a):"memory");
    __asm__ volatile("dsb ish; isb":::"memory");
    st->console_out->output(st->console_out,(char16 *)u"CHOS: kernel loaded; leaving firmware\r\n");
    uint64_t map_size=0,key=0,desc_size=0;
    uint32_t desc_version=0;
    s=bs->get_memory_map(&map_size,0,&key,&desc_size,&desc_version);
    if (s!=EFI_BUFFER_TOO_SMALL || !desc_size)
        return fail(st,(char16 *)u"CHOS: memory map sizing failed\r\n",s);
    uint64_t capacity=map_size+32*desc_size;
    void *map=0;
    s=bs->allocate_pool(2,capacity,&map);
    if (EFI_ERROR(s)) return fail(st,(char16 *)u"CHOS: memory map allocation failed\r\n",s);
    /* No allocations, console output, or other firmware calls between map and exit.
     * A failed exit may partially shut firmware down; only map/exit are retried. */
    for (unsigned attempt=0;attempt<3;attempt++) {
        map_size=capacity;
        s=bs->get_memory_map(&map_size,map,&key,&desc_size,&desc_version);
        if (EFI_ERROR(s)) break;
        bi->memory_map=(uint64_t)(uintptr_t)map; bi->memory_map_size=map_size;
        bi->descriptor_size=desc_size; bi->descriptor_version=desc_version;
        s=bs->exit_boot_services(image,key);
        if (!EFI_ERROR(s)) {
            __asm__ volatile("msr daifset, #15":::"memory");
            void (*entry)(struct chos_boot_info *,uint64_t)=(void *)(uintptr_t)(base+layout.entry_offset);
            entry(bi,stack_base+CHOS_BOOT_STACK_ALLOCATION-4096);
            for (;;) __asm__ volatile("wfe");
        }
        if (s!=EFI_INVALID_PARAMETER) break;
    }
    /* Do not invoke firmware after any ExitBootServices attempt. */
    for (;;) __asm__ volatile("wfe");
}
