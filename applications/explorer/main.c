#include "app.h"
static char cwd[60]="/",selected[60];
static struct chos_dirent entries[40];static unsigned count,operation,offset;
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
    if(!prompting && key==KEY_DOWN){if(offset+1<count)offset++;return;}
    if(!prompting && key==KEY_UP){if(offset)offset--;return;}
    if(prompting) {
        if(prompt_key(key)) {
            int64_t result=-22;
            if(operation==2)result=api(SYS_MKDIR,(uintptr_t)prompt_value,0,0);
            if(operation==4)result=api(SYS_RENAME,(uintptr_t)selected,(uintptr_t)prompt_value,0);
            if(operation==5 && equal(prompt_value,"YES"))result=api(SYS_UNLINK,(uintptr_t)selected,0,0);
            if(operation==3){static char bytes[4096];int n=read_file(selected,bytes,sizeof(bytes));if(n>=0)result=write_file(prompt_value,bytes,(unsigned)n);}
            status(result==0?"Operation completed":"Operation failed or cancelled");selected[0]=0;refresh();
        }
        return;
    }
    if(key=='u')action(1);if(key=='n')action(2);if(key=='c')action(3);if(key=='m')action(4);if(key=='d')action(5);if(key=='r')refresh();
}
void explorer_click(int x,int y) {
    if(y>=36 && y<66){action((unsigned)x/100+1);return;}
    if(y<86)return;
    unsigned index=offset+(unsigned)(y-86)/22;if(index>=count)return;
    copy(selected,entries[index].path,sizeof(selected));
    if(entries[index].kind==2){copy(cwd,selected,sizeof(cwd));selected[0]=0;offset=0;refresh();}
    else status("Selected file. C copy, M move, D delete.");
}
void explorer_paint(void) {
    refresh();begin(0xf2f6fa);box(0,0,app_width,30,0xd5e5f2);label(8,cwd);
    static const char actions[5][8]={"Up","New","Copy","Move","Delete"};
    for(unsigned i=0;i<5;i++){rounded(4+(int)i*100,36,92,29,0xd8e8f6);ui_text(0,16+(int)i*100,43,actions[i],0x204b72);}
    label(70,"Name");ui_text(0,app_width-110,70,"Bytes",0x204b72);
    unsigned rows=(unsigned)(app_height-120)/22;if(offset>=count)offset=0;if(rows>count-offset)rows=count-offset;
    for(unsigned row=0;row<rows;row++) {
        unsigned i=offset+row;
        int y=86+(int)row*22;if(equal(selected,entries[i].path))box(2,y-3,app_width-4,22,0xc4dcf4);
        const char *name=entries[i].path;for(unsigned j=0;entries[i].path[j];j++)if(entries[i].path[j]=='/')name=entries[i].path+j+1;
        text(14,y,name,entries[i].kind==2?0x206ba5:0x203a54);
        char bytes[21];number(bytes,entries[i].size);text(app_width-110,y,entries[i].kind==2?"DIR":bytes,0x44617c);
    }
}
