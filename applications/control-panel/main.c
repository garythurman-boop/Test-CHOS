#include "app.h"
static struct chos_config settings;
static bool loaded;
static unsigned tab,display_selected;
static struct chos_display_info display;
static char offset[8];
static void append(char *to,const char *from,size_t cap){size_t n=length(to);if(n<cap)copy(to+n,from,cap-n);}
static void offset_text(void) {int value=settings.utc_offset;unsigned at=0;if(value<0){offset[at++]='-';value=-value;}number(offset+at,(unsigned)value);}
static bool offset_value(void) {
    unsigned i=0;bool negative=offset[0]=='-';if(negative)i++;
    if(!offset[i])return false;int n=0;
    for(;offset[i];i++){if(offset[i]<'0' || offset[i]>'9' || i>4)return false;n=n*10+offset[i]-'0';}
    settings.utc_offset=negative?-n:n;return true;
}
static void initialize(void) {if(!loaded && api(SYS_CONFIG,0,(uintptr_t)&settings,0)==0){loaded=true;offset_text();if(api(SYS_DISPLAY,3,0,0)==1)tab=3;}}
static void apply(bool save) {
    if(!offset_value()){status("Enter a signed offset in minutes, such as -300 or 330");return;}
    int64_t r=api(SYS_CONFIG,1,(uintptr_t)&settings,save);
    status(r==0?(save?"Settings saved to the system disk":"Settings applied to this session"):(r==-22?"Invalid name or time offset (15-minute steps, -720..840)":"Settings save failed; previous settings retained"));
}
void settings_key(unsigned key) {
    initialize();
    if(key>=512 && key<=515){tab=key-512;app_gui.relayout=true;}
    if(tab==3){
        if(api(SYS_DISPLAY,0,(uintptr_t)&display,0)<0)return;
        if(key>=800 && key<800+display.count)display_selected=key-800;
        if(key==KEY_UP && display_selected)display_selected--;
        if(key==KEY_DOWN && display_selected+1<display.count)display_selected++;
        if(key=='c'){for(unsigned i=0;i<display.count;i++)if(display.modes[i].width==display.current_width && display.modes[i].height==display.current_height)display_selected=i;status("Display selection cancelled.");}
        if(key=='a' || key==13){
            int64_t r=api(SYS_DISPLAY,1,(uintptr_t)&display.modes[display_selected],0);
            status(r==0?"Saved trial mode. Restart; keep it after checking the display.":"Mode save failed. Installed M8 disk required; mode unchanged.");
        }
        if(key=='k'){int64_t r=api(SYS_DISPLAY,2,0,0);status(r==0?"Current resolution confirmed and saved.":"No active trial or confirmation save failed.");}
        return;
    }
    if(key=='1' || key=='2'){settings.theme=key-'1';apply(false);}
    if(key=='s')settings.clock_seconds=!settings.clock_seconds;
    if(key==('s'|KEY_CTRL) || key==13)apply(app_persistent);

}
void settings_paint(void) {
    initialize();begin(ROYALE_APP_BG);label(16,"CHOS Control Panel");
    if(app_width<620){
        int width=(app_width-24)/4;
        static const char *const names[]={"Style","Clock","System","Screen"};
        for(unsigned i=0;i<4;i++)ui_tab(&app_gui,1+i,512+i,12+(int)i*width,44,width-4,names[i],tab==i);
    } else {
    ui_tab(&app_gui,1,512,12,44,140,"Appearance",tab==0);ui_tab(&app_gui,2,513,158,44,140,"Clock",tab==1);ui_tab(&app_gui,3,514,304,44,140,"System",tab==2);ui_tab(&app_gui,4,515,450,44,140,"Display",tab==3);
    }
    if(tab==0) {
        rounded(16,82,app_width-32,42,ROYALE_WIDGET_BOTTOM);rounded(16,142,app_width-32,42,ROYALE_WIDGET_BOTTOM);rounded(app_width-104,88,70,30,0x1c4f82);rounded(app_width-104,148,70,30,0x23565a);
        ui_radio(&app_gui,10,'1',28,90,"1  Deep blue",settings.theme==0,true);ui_radio(&app_gui,11,'2',28,150,"2  Harbor green",settings.theme==1,true);
        label(212,"Select a theme to preview it. Save keeps it after reboot.");
    } else if(tab==1) {
        label(92,"Offset from firmware time, minutes:");ui_textfield(&app_gui,12,280,88,120,offset,sizeof(offset));
        ui_checkbox(&app_gui,13,'s',16,142,"Show clock seconds",settings.clock_seconds,true);
        label(198,"Use -720 to +840 in 15-minute steps (UTC offset).");label(226,"This is a fixed offset; daylight-saving rules are not applied.");
    } else if(tab==3){
        if(api(SYS_DISPLAY,0,(uintptr_t)&display,0)<0){label(88,"Display mode service unavailable.");return;}
        label(84,"Display Properties / firmware-enumerated modes");
        char current[48],n[21];copy(current,"Current: ",sizeof(current));number(n,display.current_width);append(current,n,sizeof(current));append(current," x ",sizeof(current));number(n,display.current_height);append(current,n,sizeof(current));
        label(110,current);
        unsigned first=display_selected>4?display_selected-4:0;
        for(unsigned i=first;i<display.count && i<first+5;i++){
            char mode[48];number(mode,display.modes[i].width);append(mode," x ",sizeof(mode));number(n,display.modes[i].height);append(mode,n,sizeof(mode));
            ui_radio(&app_gui,30+i,800+i,28,146+(int)(i-first)*28,mode,display_selected==i,true);
        }
        ui_text(3,16,294,"Switching uses UEFI at restart. No runtime GOP calls.",ROYALE_APP_INK);
        ui_text(3,16,316,"An unconfirmed trial falls back on the following boot.",ROYALE_APP_INK);
        ui_button(&app_gui,70,'a',16,348,168,28,"Apply at restart",app_persistent && display.count>0);
        ui_button(&app_gui,71,'c',194,348,100,28,"Cancel",true);
        ui_button(&app_gui,72,'k',304,348,190,28,"Keep current mode",app_persistent && display.trial);
        return;
    } else {
        label(88,"Computer name:");ui_textfield(&app_gui,14,160,84,320,settings.computer,sizeof(settings.computer));
        struct chos_info info;if(api(SYS_INFO,(uintptr_t)&info,0,0)==0){char n[21];number(n,info.width);label(156,"Framebuffer width:");text(264,156,n,ROYALE_APP_INK);number(n,info.height);label(184,"Framebuffer height:");text(264,184,n,ROYALE_APP_INK);}
        label(224,"UEFI chooses resolution before the kernel starts.");label(252,"User accounts and authentication are not implemented.");
    }
    ui_button(&app_gui,20,'s'|KEY_CTRL,app_width-208,app_height-64,192,30,app_persistent?"Save settings":"Apply to session",true);
    if(!app_status[0])status(app_persistent?"Changes persist only after Save settings / Ctrl-S":"Live session: settings are lost at shutdown");
}
