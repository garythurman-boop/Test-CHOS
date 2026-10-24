#ifndef CHOS_DESKTOP_H
#define CHOS_DESKTOP_H
#include "memory.h"
#include "abi.h"
struct process;
struct exception_frame;
struct chos_boot_info;
#define WM_WINDOWS 8
#define WM_EVENTS 128
#define WM_COMMANDS 128
struct wm_window {
    uint64_t owner; uint32_t id; char title[48];
    int x,y,w,h,rx,ry,rw,rh; bool minimized,maximized,closing,fullscreen;
    struct chos_event events[WM_EVENTS]; unsigned head,count;
};
struct wm {
    struct wm_window windows[WM_WINDOWS]; unsigned order[WM_WINDOWS],count;
    uint32_t next_id,focus; int width,height,mouse_x,mouse_y,drag,dx,dy,client_capture,client_hover;
    unsigned desktop_selection,shell_press;bool button_down;
    int icon_x[7],icon_y[7],icon_drag,press_x,press_y,last_click_x,last_click_y;
    uint64_t now_ms,last_click_ms;
    unsigned last_click_icon,double_click_ms,cursor_size,context,context_item;
    int context_x,context_y;bool icon_moving,auto_arrange,preferences_dirty,display_requested;
    bool resize,menu,applications,alt,ctrl,shift,dirty,logout; unsigned launch,theme,confirm,power,selection;
};
void wm_init(struct wm *,int,int);
int wm_create(struct wm *,uint64_t,const char *,int,int);
bool wm_fullscreen(struct wm *,uint64_t,uint32_t);
struct wm_window *wm_exclusive(struct wm *);
void wm_refresh(struct wm *,uint64_t);
struct wm_window *wm_owned(struct wm *,uint64_t,uint32_t);
void wm_remove(struct wm *,uint64_t);
void wm_mouse(struct wm *,int,int,int);
void wm_context(struct wm *,int,int);
void wm_arrange(struct wm *);
bool wm_preferences_encode(const struct wm *,uint8_t [96]);
bool wm_preferences_decode(struct wm *,const uint8_t *,size_t);
unsigned wm_context_count(const struct wm *);
bool wm_context_enabled(const struct wm *,unsigned);
void wm_key(struct wm *,unsigned,int);
bool wm_event(struct wm *,uint64_t,uint32_t,struct chos_event *,bool consume);
void wm_focus(struct wm *,uint32_t);
int wm_task_width(const struct wm *);
unsigned wm_shell_hit(const struct wm *,int,int);
bool wm_menu_item(const struct wm *,unsigned,int *,int *,int *,int *);
struct surface { uint32_t *pixels; int width,height,stride; int x,y,w,h; };
void gfx_clip(struct surface *,int,int,int,int);
void gfx_rect(struct surface *,int,int,int,int,uint32_t);
void gfx_text(struct surface *,int,int,const char *,uint32_t);
void desktop_start(struct pmm *,struct chos_boot_info *);
void desktop_step(void);
int64_t desktop_syscall(struct process *,struct exception_frame *);
void desktop_process_exit(uint64_t);
int desktop_launch(unsigned);
bool desktop_active(void);
unsigned desktop_setup_displayed(void);
void desktop_setup_notify(uint64_t);
#endif
