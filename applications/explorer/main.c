#include "app.h"
static char cwd[60]="/",selected[60];
static struct chos_dirent entries[40];static unsigned count,operation,offset;
static bool context,properties;static int context_x,context_y;static unsigned context_index;
static bool child(const char *path) {
    unsigned start=0;
    if(!equal(cwd,"/")) {while(cwd[start]){if(path[start]!=cwd[start])return false;start++;}if(path[start]!='/')return false;}
    start++;if(!path[start])return false;
    for(unsigned i=start;path[i];i++)if(path[i]=='/')return false;
    return true;
}
static void refresh(void) {
    count=0;
    for(unsigned i=0;i<40;i++){struct chos_dirent entry;if(api(SYS_LIST,i,(uintptr_t)&entry,0)==0 && child(entry.path))entries[count++]=entry;}
}
void explorer_init(void) { refresh();status(volume_status()); }
static void action(unsigned op) {
    if(op<1 || op>5)return;
    operation=op;
    if(op==1){if(!equal(cwd,"/")){unsigned n=(unsigned)length(cwd);while(n && cwd[n]!='/')n--;if(!n)n=1;cwd[n]=0;}refresh();return;}
    if(op>2 && !selected[0]){status("Select a file first");return;}
    prompt(op==2?"New folder (absolute path)":(op==3?"Copy to (absolute path)":(op==4?"Move to (absolute path)":"Delete selected? Type YES")),op==5?"":"/documents/");
}
void explorer_key(unsigned key) {
    if(properties){if(key==27 || key==13){properties=false;ui_modal_end(&app_gui);}return;}
    if(context && key==27){context=false;ui_modal_end(&app_gui);return;}
    if(context && key>=1001 && key<=1006){
        context=false;ui_modal_end(&app_gui);
        if(key==1001 && entries[context_index].kind==2){copy(cwd,selected,sizeof(cwd));selected[0]=0;offset=0;refresh();}
        if(key==1002)action(3);
        if(key==1003)action(4);
        if(key==1004)action(5);
        if(key==1005)properties=true;
        return;
    }
    if(!prompting && key==KEY_DOWN){if(offset+1<count)offset++;return;}
    if(!prompting && key==KEY_UP){if(offset)offset--;return;}
    if(prompting) {
        if(prompt_key(key)) {
            int64_t result=-22;
            if(operation==2)result=api(SYS_MKDIR,(uintptr_t)prompt_value,0,0);
            if(operation==4)result=api(SYS_RENAME,(uintptr_t)selected,(uintptr_t)prompt_value,0);
            if(operation==5 && equal(prompt_value,"YES"))result=api(SYS_UNLINK,(uintptr_t)selected,0,0);
            if(operation==3){static char bytes[4096];int n=read_file(selected,bytes,sizeof(bytes));if(n>=0){int64_t existing=api(SYS_OPEN,(uintptr_t)prompt_value,1,0);if(existing>=0){api(SYS_CLOSE,(uint64_t)existing,0,0);result=-17;}else if(existing==-2)result=write_file(prompt_value,bytes,(unsigned)n);}}
            status(result==0?"Operation completed":"Operation failed or cancelled");selected[0]=0;refresh();
        }
        return;
    }
    if(key=='u')action(1);if(key=='n')action(2);if(key=='c')action(3);if(key=='m')action(4);if(key=='d')action(5);if(key=='r')refresh();
}
void explorer_paint(void) {
    refresh();begin(0xf2f6fa);box(0,0,app_width,30,0xd5e5f2);label(8,cwd);
    static const char actions[5][8]={"Up","New","Copy","Move","Delete"};
    static const unsigned keys[5]={'u','n','c','m','d'};
    ui_toolbar(&app_gui,0,32,app_width,36);
    for(unsigned i=0;i<5;i++)ui_button(&app_gui,i+1,keys[i],4+(int)i*100,36,92,29,actions[i],i<2 || selected[0]);
    label(70,"Name");ui_text(0,app_width-110,70,"Bytes",0x204b72);
    unsigned rows=(unsigned)(app_height-120)/22;if(offset>=count)offset=0;if(rows>count-offset)rows=count-offset;
    for(unsigned row=0;row<rows;row++) {
        unsigned i=offset+row;
        int y=86+(int)row*22;
        const char *name=entries[i].path;for(unsigned j=0;entries[i].path[j];j++)if(entries[i].path[j]=='/')name=entries[i].path+j+1;
        ui_list_row(&app_gui,100+i,2,y-3,app_width-24,name,equal(selected,entries[i].path),entries[i].kind==2?0x206ba5:0x203a54);
        char bytes[21];number(bytes,entries[i].size);text(app_width-110,y,entries[i].kind==2?"DIR":bytes,0x44617c);
    }
    unsigned page=(unsigned)(app_height-120)/22;
    ui_scrollbar(&app_gui,500,app_width-20,83,(int)page*22,offset,count,page);
    if(context){
        ui_begin(&app_gui);ui_menu_panel(&app_gui,context_x,context_y,212,170);
        static const char *const titles[]={"Open folder","Copy to...","Move / rename...","Delete...","Properties","Cancel"};
        for(unsigned i=0;i<6;i++)ui_menu_item(&app_gui,650+i,1001+i,context_x+3,context_y+3+(int)i*27,206,titles[i],i==0?entries[context_index].kind==2:((i==1 || i==2)?entries[context_index].kind==1:true));
    }
    if(properties){
        ui_begin(&app_gui);ui_dialog(&app_gui,12,100,app_width-24,160,"Properties");
        ui_text(3,24,134,selected,ROYALE_APP_INK);
        char bytes[21];number(bytes,entries[context_index].size);ui_text(0,24,164,entries[context_index].kind==2?"Directory":bytes,ROYALE_APP_INK);
        ui_text(3,24,190,"CHFS object. Account permissions are not implemented.",ROYALE_APP_INK);
        ui_button(&app_gui,660,27,24,220,100,28,"Close",true);
    }
}
void explorer_widget(unsigned id,unsigned value){
    app_gui.relayout=true;
    if(id==500){unsigned page=(unsigned)(app_height-120)/22;offset=value;if(offset>count)offset=count;if(count>page && offset>count-page)offset=count-page;return;}
    if(id>=100 && id<100+count){unsigned index=id-100;copy(selected,entries[index].path,sizeof(selected));if(entries[index].kind==2){copy(cwd,selected,sizeof(cwd));selected[0]=0;offset=0;refresh();}else status("Selected file. C copy, M move, D delete.");}
}

void explorer_context(int x,int y){
    if(prompting || y<83 || y>=app_height-37 || x<2 || x>app_width-24)return;
    unsigned i=offset+(unsigned)(y-83)/22;if(i>=count)return;
    context_index=i;copy(selected,entries[i].path,sizeof(selected));context=true;properties=false;
    context_x=x>app_width-216?app_width-216:x;context_y=y>app_height-194?app_height-194:y;
    ui_modal_end(&app_gui);app_gui.focus=650;app_gui.relayout=true;
}
void explorer_pointer(const struct chos_event *e){
    if(e->type==EVENT_RESIZE){context=properties=false;return;}
    if(context && e->type==EVENT_CLICK && (e->x<context_x || e->x>=context_x+212 || e->y<context_y || e->y>=context_y+170)){
        context=false;ui_modal_end(&app_gui);app_gui.relayout=true;
    }
}
