/* Minimal UEFI 2.x ABI declarations used by CHOS, written from the specification.
 * AArch64 uses its native calling convention, including for firmware calls. */
#ifndef CHOS_EFI_H
#define CHOS_EFI_H
#include <stdint.h>
#include <stddef.h>
typedef uint64_t status;
typedef uint16_t char16;
typedef void *handle;
#define EFI_ERROR(x) (((x) >> 63) != 0)
#define EFI_BUFFER_TOO_SMALL (UINT64_C(0x8000000000000005))
#define EFI_INVALID_PARAMETER (UINT64_C(0x8000000000000002))
struct guid { uint32_t a; uint16_t b,c; uint8_t d[8]; };
struct table_header { uint64_t signature; uint32_t revision,size,crc,reserved; };
struct text_output {
    void *reset;
    status (*output)(struct text_output *, const char16 *);
};
struct boot_services {
    struct table_header header;
    void *raise_tpl, *restore_tpl;
    status (*allocate_pages)(uint32_t,uint32_t,uint64_t,uint64_t *);
    void *free_pages;
    status (*get_memory_map)(uint64_t *,void *,uint64_t *,uint64_t *,uint32_t *);
    status (*allocate_pool)(uint32_t,uint64_t,void **);
    status (*free_pool)(void *);
    void *create_event,*set_timer,*wait_for_event,*signal_event,*close_event,*check_event;
    void *install_protocol,*reinstall_protocol,*uninstall_protocol;
    status (*handle_protocol)(handle,struct guid *,void **);
    void *reserved,*register_protocol_notify,*locate_handle,*locate_device_path;
    void *install_configuration_table,*load_image,*start_image,*exit,*unload_image;
    status (*exit_boot_services)(handle,uint64_t);
    void *get_next_monotonic_count,*stall;
    status (*set_watchdog_timer)(uint64_t,uint64_t,uint64_t,char16 *);
    void *connect_controller,*disconnect_controller,*open_protocol,*close_protocol;
    void *open_protocol_information,*protocols_per_handle,*locate_handle_buffer;
    status (*locate_protocol)(struct guid *,void *,void **);
};
struct system_table {
    struct table_header header;
    char16 *vendor;
    uint32_t firmware_revision;
    handle console_in_handle;
    void *console_in;
    handle console_out_handle;
    struct text_output *console_out;
    handle stderr_handle;
    void *stderr_protocol,*runtime_services;
    struct boot_services *boot;
    uint64_t configuration_count;
    void *configuration_table;
};
struct loaded_image {
    uint32_t revision;
    handle parent;
    struct system_table *system;
    handle device;
    void *file_path,*reserved;
    uint32_t options_size;
    void *options,*image_base;
    uint64_t image_size;
    uint32_t code_type,data_type;
    void *unload;
};
struct file {
    uint64_t revision;
    status (*open)(struct file *,struct file **,const char16 *,uint64_t,uint64_t);
    status (*close)(struct file *);
    void *delete;
    status (*read)(struct file *,uint64_t *,void *);
    void *write,*get_position,*set_position;
    status (*get_info)(struct file *,struct guid *,uint64_t *,void *);
};
struct filesystem { uint64_t revision; status (*open_volume)(struct filesystem *,struct file **); };
struct gop_info {
    uint32_t version,width,height,pixel_format;
    uint32_t masks[4];
    uint32_t stride;
};
struct gop_mode {
    uint32_t max_mode,mode;
    struct gop_info *info;
    uint64_t info_size,framebuffer,framebuffer_size;
};
struct gop { void *query_mode,*set_mode,*blt; struct gop_mode *mode; };
_Static_assert(offsetof(struct boot_services,exit_boot_services)==24+26*8,"UEFI boot ABI");
_Static_assert(offsetof(struct system_table,boot)==96,"UEFI system ABI");
#endif
