#include "app.h"
static struct chos_config settings;
static bool loaded;
static unsigned tab;
static char offset[8];
static void offset_text(void) {int value=settings.utc_offset;unsigned at=0;if(value<0){offset[at++]='-';value=-value;}number(offset+at,(unsigned)value);}
static bool offset_value(void) {
    unsigned i=0;bool negative=offset[0]=='-';if(negative)i++;
    if(!offset[i])return false;int n=0;
    for(;offset[i];i++){if(offset[i]<'0' || offset[i]>'9' || i>4)return false;n=n*10+offset[i]-'0';}
    settings.utc_offset=negative?-n:n;return true;
}
static void initialize(void) {if(!loaded && api(SYS_CONFIG,0,(uintptr_t)&settings,0)==0){loaded=true;offset_text();}}
static void apply(bool save) {
    if(!offset_value()){status("Enter a signed offset in minutes, such as -300 or 330");return;}
    int64_t r=api(SYS_CONFIG,1,(uintptr_t)&settings,save);
    status(r==0?(save?"Settings saved to the system disk":"Settings applied to this session"):(r==-22?"Invalid name or time offset (15-minute steps, -720..840)":"Settings save failed; previous settings retained"));
}
void settings_key(unsigned key) {
    initialize();
    if(key=='1' || key=='2'){settings.theme=key-'1';apply(false);}
    if(key=='s')settings.clock_seconds=!settings.clock_seconds;
    if(key==('s'|KEY_CTRL) || key==13)apply(app_persistent);
    if(key>=512 && key<=514){tab=key-512;app_gui.relayout=true;}
}
void settings_paint(void) {
    initialize();begin(ROYALE_APP_BG);label(16,"CHOS Control Panel");
    ui_tab(&app_gui,1,512,12,44,140,"Appearance",tab==0);ui_tab(&app_gui,2,513,158,44,140,"Clock",tab==1);ui_tab(&app_gui,3,514,304,44,140,"System",tab==2);
    if(tab==0) {
        rounded(16,82,app_width-32,42,ROYALE_WIDGET_BOTTOM);rounded(16,142,app_width-32,42,ROYALE_WIDGET_BOTTOM);rounded(app_width-104,88,70,30,0x1c4f82);rounded(app_width-104,148,70,30,0x23565a);
        ui_radio(&app_gui,10,'1',28,90,"1  Deep blue",settings.theme==0,true);ui_radio(&app_gui,11,'2',28,150,"2  Harbor green",settings.theme==1,true);
        label(212,"Select a theme to preview it. Save keeps it after reboot.");
    } else if(tab==1) {
        label(92,"Offset from firmware time, minutes:");ui_textfield(&app_gui,12,280,88,120,offset,sizeof(offset));
        ui_checkbox(&app_gui,13,'s',16,142,"Show clock seconds",settings.clock_seconds,true);
        label(198,"Use -720 to +840 in 15-minute steps (UTC offset).");label(226,"This is a fixed offset; daylight-saving rules are not applied.");
    } else {
        label(88,"Computer name:");ui_textfield(&app_gui,14,160,84,320,settings.computer,sizeof(settings.computer));
        struct chos_info info;if(api(SYS_INFO,(uintptr_t)&info,0,0)==0){char n[21];number(n,info.width);label(156,"Framebuffer width:");text(264,156,n,ROYALE_APP_INK);number(n,info.height);label(184,"Framebuffer height:");text(264,184,n,ROYALE_APP_INK);}
        label(224,"UEFI chooses resolution before the kernel starts.");label(252,"User accounts and authentication are not implemented.");
    }
    ui_button(&app_gui,20,'s'|KEY_CTRL,app_width-208,app_height-64,192,30,app_persistent?"Save settings":"Apply to session",true);
    if(!app_status[0])status(app_persistent?"Changes persist only after Save settings / Ctrl-S":"Live session: settings are lost at shutdown");
}
