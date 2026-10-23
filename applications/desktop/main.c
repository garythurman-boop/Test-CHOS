#include "app.h"
int user_main(uint64_t role,uint64_t unused) {
    (void)unused;if(role<1 || role>7)return 1;app_role=(unsigned)role;
    static const char titles[7][48]={"Explorer","Notepad","Terminal","Control Panel","Task Manager","About CHOS","CHOS Setup"};
    struct chos_window spec={.width=role==6?600:720,.height=role==6?330:460};
    copy(spec.title,titles[role-1],sizeof(spec.title));
    if(api(SYS_WINDOW,0,0,0)!=-14)return 90;
    struct chos_window invalid={.width=1,.height=1};
    if(api(SYS_WINDOW,(uintptr_t)&invalid,0,0)!=-22)return 91;
    app_window=(int)api(SYS_WINDOW,(uintptr_t)&spec,0,0);if(app_window<0)return 2;
    if(api(SYS_DRAW,0,0,0)!=-9 || api(SYS_DRAW,(uint64_t)app_window,0,0)!=-14 || api(SYS_EVENT,(uint64_t)app_window,0,0)!=-14)return 92;
    struct chos_draw invalid_draw={.op=500};
    if(api(SYS_DRAW,(uint64_t)app_window,(uintptr_t)&invalid_draw,0)!=-22)return 93;
    if(role==7 && api(SYS_PRESENT,(uint64_t)app_window,SETUP_COMPLETE+1,0)!=-22)return 97;
    struct chos_hardware hardware;
    if(api(SYS_HARDWARE,0,0,0)!=-14 || api(SYS_HARDWARE,(uintptr_t)&hardware,0,0) || hardware.version!=1 || hardware.kernel_el!=1 || (hardware.gic_version!=2 && hardware.gic_version!=3) || hardware.pci_count>32)return 98;
    struct chos_info info;if(api(SYS_INFO,(uintptr_t)&info,0,0)==0)app_persistent=info.persistent!=0;
    if(role==1)explorer_init();if(role==2)notepad_init();if(role==3)terminal_init();
    gui_initialize();
    struct chos_config config;
    if(api(SYS_CONFIG,0,0,0)!=-14 || api(SYS_CONFIG,0,(uintptr_t)&config,0))return 94;
    if(role!=4 && role!=7){if(api(SYS_CONFIG,1,(uintptr_t)&config,0)!=-13)return 95;}
    else {config.version=99;if(api(SYS_CONFIG,1,(uintptr_t)&config,1)!=-22)return 96;}
    bool dirty=true,draining=false;unsigned drained=0;
    for(;;) {
        if(app_gui.relayout || drained>=32){app_gui.relayout=false;draining=false;drained=0;}
        if(dirty && !draining) {
            ui_begin(&app_gui);
            switch(role){case 1:explorer_paint();break;case 2:notepad_paint();break;case 3:terminal_paint();break;case 4:settings_paint();break;case 5:task_paint();break;case 6:about_paint();break;case 7:setup_paint();break;}
            present();dirty=false;
        }
        struct chos_event event;int64_t result=api(SYS_EVENT,(uint64_t)app_window,(uintptr_t)&event,draining?1:0);
        if(result==-11){if(draining){draining=false;drained=0;continue;}if(role==5 || role==7)dirty=true;continue;}
        if(result<0)return 3;
        draining=true;drained++;
        struct ui_result input=ui_event(&app_gui,&event);
        if(input.consumed) {
            dirty|=input.repaint;
            if(input.key){event.type=EVENT_KEY;event.code=input.key;}
            else {
                if(input.id && !prompting) {
                    if(role==1)explorer_widget(input.id,input.value);
                    if(role==7)setup_widget(input.id);
                }
                continue;
            }
        }
        if(event.type==EVENT_POINTER || event.type==EVENT_RELEASE){dirty|=input.repaint;continue;}
        if(event.type==EVENT_CLOSE){if(role!=2 || notepad_close())return 0;dirty=true;}
        if(event.type==EVENT_RESIZE){app_width=event.width;app_height=event.height;}
        if(event.type==EVENT_KEY) {
            switch(role){case 1:explorer_key(event.code);break;case 2:notepad_key(event.code);break;case 3:terminal_key(event.code);break;case 4:settings_key(event.code);break;case 5:task_key(event.code);break;case 6:about_key(event.code);break;case 7:setup_key(event.code);break;}
        }
        dirty=true;
    }
}
