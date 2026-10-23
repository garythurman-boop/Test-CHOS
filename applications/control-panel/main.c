#include "app.h"
void settings_key(unsigned key) {
    if(key=='1' || key=='2') {
        if(api(SYS_THEME,key-'1',0,0)==0)status("Desktop color theme changed for this session");
    }
}
void settings_click(int x,int y) { (void)x;if(y>=80 && y<130)settings_key('1');if(y>=140 && y<190)settings_key('2'); }
void settings_paint(void) {
    begin(0xe5eef8);label(16,"CHOS Control Panel");label(46,"Desktop colors (click or press 1 / 2)");
    rounded(16,82,app_width-32,42,0x1c4f82);text(28,95,"1  Deep blue",0xffffff);
    rounded(16,142,app_width-32,42,0x23565a);text(28,155,"2  Harbor green",0xffffff);
    struct chos_info info;if(api(SYS_INFO,(uintptr_t)&info,0,0)==0){char value[21];number(value,info.width);label(212,"Framebuffer width:");text(264,212,value,0x204b72);number(value,info.height);label(240,"Framebuffer height:");text(264,240,value,0x204b72);}
    label(276,"Resolution is selected by UEFI before kernel boot.");
    status("DISPLAY.CFG requests a firmware mode. Theme is volatile.");
}
