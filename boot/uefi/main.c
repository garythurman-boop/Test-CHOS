#include "efi.h"
#include "boot.h"
static const struct guid acpi_guid={0x8868e871,0xe4f1,0x11d3,{0xbc,0x22,0,0x80,0xc7,0x3c,0x88,0x81}};
static struct guid loaded_guid={0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0,0xa0,0xc9,0x69,0x72,0x3b}};
static struct guid fs_guid={0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
static struct guid info_guid={0x09576e92,0x6d3f,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
static struct guid gop_guid={0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};
static status fail(struct system_table *st,const char16 *msg,status code) {
    st->console_out->output(st->console_out,msg);
    return EFI_ERROR(code)?code:EFI_INVALID_PARAMETER;
}
status efi_main(handle image,struct system_table *st) {
    struct boot_services *bs=st->boot;
    st->console_out->output(st->console_out,(char16 *)u"CHOS - ARM64 UEFI loader\r\n");
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
    if (EFI_ERROR(s)) return fail(st,(char16 *)u"CHOS: kernel metadata failed\r\n",s);
    uint64_t file_size=((uint64_t *)file_info)[1];
    if (!file_size || file_size>512*1024) return fail(st,(char16 *)u"CHOS: invalid kernel size\r\n",EFI_INVALID_PARAMETER);
    uint64_t base=0;
    s=bs->allocate_pages(0,1,256,&base); /* EfiLoaderCode: executable image, zeroed BSS and temporary stack */
    if (EFI_ERROR(s)) return fail(st,(char16 *)u"CHOS: kernel allocation failed\r\n",s);
    for (uint64_t i=0;i<1024*1024;i++) ((volatile uint8_t *)(uintptr_t)base)[i]=0;
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
    root->close(root);
    if (EFI_ERROR(s)||read_size!=file_size) return fail(st,(char16 *)u"CHOS: incomplete kernel read\r\n",EFI_INVALID_PARAMETER);
    struct chos_boot_info *bi=0;
    s=bs->allocate_pool(2,sizeof(*bi),(void **)&bi);
    if (EFI_ERROR(s)) return fail(st,(char16 *)u"CHOS: boot information allocation failed\r\n",s);
    for (uint64_t i=0;i<sizeof(*bi);i++) ((uint8_t *)bi)[i]=0;
    bi->magic=CHOS_BOOT_MAGIC; bi->reserved=test_flags;
    if(st->configuration_count>1024) return fail(st,(char16 *)u"CHOS: invalid configuration table count\r\n",EFI_INVALID_PARAMETER);
    const struct configuration_entry *entries=st->configuration_table;
    for(uint64_t i=0;i<st->configuration_count;i++) {
        unsigned equal=1;
        for(unsigned j=0;j<sizeof(struct guid);j++)
            if(((const uint8_t *)&entries[i].guid)[j]!=((const uint8_t *)&acpi_guid)[j]) equal=0;
        if(equal) bi->rsdp=(uint64_t)(uintptr_t)entries[i].table;
    }
    struct gop *gop=0;
    s=bs->locate_protocol(&gop_guid,0,(void **)&gop);
    if (EFI_ERROR(s)||!gop||!gop->mode||!gop->mode->info)
        return fail(st,(char16 *)u"CHOS: GOP framebuffer required\r\n",EFI_INVALID_PARAMETER);
    struct gop_info *gi=gop->mode->info;
    if (gi->pixel_format>1 || gi->width<640 || gi->height<480 || gi->stride<gi->width ||
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
            void (*entry)(struct chos_boot_info *,uint64_t)=(void *)(uintptr_t)base;
            entry(bi,base+1024*1024);
            for (;;) __asm__ volatile("wfe");
        }
        if (s!=EFI_INVALID_PARAMETER) break;
    }
    /* Do not invoke firmware after any ExitBootServices attempt. */
    for (;;) __asm__ volatile("wfe");
}
