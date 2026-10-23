#include "app.h"
void task_key(unsigned key) {(void)key;}
void task_paint(void) {
    begin(0xe5eef8);struct chos_info info;
    if(api(SYS_INFO,(uintptr_t)&info,0,0)){label(12,"System information unavailable");return;}
    label(12,"CHOS Task Manager");ui_text(1,14,44,"PID",0x204b72);ui_text(1,146,44,"App",0x204b72);ui_text(1,290,44,"Scheduled ticks",0x204b72);
    for(unsigned i=0;i<8;i++) {
        struct chos_task task;if(api(SYS_TASK,i,(uintptr_t)&task,0))continue;
        char value[21];number(value,task.pid);text(14,72+(int)i*22,value,0x204b72);
        number(value,task.application);text(146,72+(int)i*22,value,0x204b72);
        number(value,task.ticks);text(290,72+(int)i*22,value,0x204b72);
    }
    char value[21];number(value,info.used_pages*4);label(264,"Allocated RAM KiB:");text(278,264,value,0x204b72);
    number(value,info.total_pages*4);label(292,"Managed RAM KiB:");text(278,292,value,0x204b72);
    number(value,info.switches);label(320,"Context switches:");text(278,320,value,0x204b72);
    struct chos_render render;
    if(api(SYS_RENDER,(uintptr_t)&render,0,0)==0){number(value,render.partial_frames);label(348,"Partial pointer repaints:");text(278,348,value,0x204b72);number(value,render.last_us);label(370,"Last paint, microseconds:");text(278,370,value,0x204b72);}
    status("Timer samples are not CPU percent. Refresh: 1 s.");
}
