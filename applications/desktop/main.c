#include "app.h"
int user_main(uint64_t role,uint64_t unused) {
    (void)unused;if(role<1 || role>6)return 1;app_role=(unsigned)role;
    static const char titles[6][48]={"Explorer","Notepad","Terminal","Control Panel","Task Manager","About CHOS"};
    struct chos_window spec={.width=role==6?600:720,.height=role==6?330:460};
    copy(spec.title,titles[role-1],sizeof(spec.title));
    if(api(SYS_WINDOW,0,0,0)!=-14)return 90;
    struct chos_window invalid={.width=1,.height=1};
    if(api(SYS_WINDOW,(uintptr_t)&invalid,0,0)!=-22)return 91;
    app_window=(int)api(SYS_WINDOW,(uintptr_t)&spec,0,0);if(app_window<0)return 2;
    if(api(SYS_DRAW,0,0,0)!=-9 || api(SYS_DRAW,(uint64_t)app_window,0,0)!=-14 || api(SYS_EVENT,(uint64_t)app_window,0,0)!=-14)return 92;
    struct chos_draw invalid_draw={.op=500};
    if(api(SYS_DRAW,(uint64_t)app_window,(uintptr_t)&invalid_draw,0)!=-22)return 93;
    if(role==1)explorer_init();if(role==2)notepad_init();if(role==3)terminal_init();
    bool dirty=true;
    for(;;) {
        if(dirty) {
            switch(role){case 1:explorer_paint();break;case 2:notepad_paint();break;case 3:terminal_paint();break;case 4:settings_paint();break;case 5:task_paint();break;case 6:about_paint();break;}
            present();dirty=false;
        }
        struct chos_event event;int64_t result=api(SYS_EVENT,(uint64_t)app_window,(uintptr_t)&event,0);
        if(result==-11){if(role==5)dirty=true;continue;}
        if(result<0)return 3;
        if(event.type==EVENT_CLOSE){if(role!=2 || notepad_close())return 0;dirty=true;}
        if(event.type==EVENT_RESIZE){app_width=event.width;app_height=event.height;}
        if(event.type==EVENT_KEY) {
            switch(role){case 1:explorer_key(event.code);break;case 2:notepad_key(event.code);break;case 3:terminal_key(event.code);break;case 4:settings_key(event.code);break;case 5:task_key(event.code);break;case 6:about_key(event.code);break;}
        }
        if(event.type==EVENT_CLICK && !prompting) {
            if(role==1)explorer_click(event.x,event.y);if(role==2)notepad_click(event.x,event.y);if(role==4)settings_click(event.x,event.y);
        }
        dirty=true;
    }
}
