#include "app.h"
static char document[4096],filename[60];
static unsigned size,cursor,operation;
static bool modified,menu;
static void action(unsigned op) {
    if(op==1 && modified){status("Unsaved changes. Save or Ctrl-N to discard.");return;}
    operation=op;
    prompt(op==1?"Open file (absolute path)":"Save as (absolute path)",filename[0]?filename:"/documents/note.txt");
}
void notepad_init(void) { status(app_persistent?"System disk. Ctrl-O open, Ctrl-S save.":"RAM volume. Ctrl-O open, Ctrl-S save."); }
static void save(void) {
    if(!filename[0]){action(2);return;}
    if(write_file(filename,document,size)==0){modified=false;status(app_persistent?"Saved to persistent system disk":"Saved to RAM volume. Lost at shutdown.");}
    else status("Save failed. Original file retained.");
}
void notepad_key(unsigned key) {
    if(key==600 || key==('f'|KEY_ALT)){menu=!menu;app_gui.relayout=true;if(menu)app_gui.focus=610;return;}
    if(menu && key==27){menu=false;ui_modal_end(&app_gui);return;}
    if(menu && (key&KEY_CTRL))menu=false;
    if(prompting) {
        bool accepted=prompt_key(key);
        if(accepted) {
            if(length(prompt_value)>=sizeof(filename)){status("Path too long");return;}
            if(operation==3){if(equal(prompt_value,"YES"))(void)api(SYS_EXIT,0,0,0);return;}
            if(operation==1) {
                /* Validate into separate memory so a failed open preserves edits. */
                static char incoming[4096];int n=read_file(prompt_value,incoming,sizeof(incoming));
                if(n<0){status("Open failed (file absent or over 4095 bytes)");return;}
                for(int i=0;i<n;i++)if(incoming[i]!='\n' && incoming[i]!='\t' && (incoming[i]<32 || incoming[i]>126)){status("Only plain ASCII text is supported");return;}
                copy(document,incoming,sizeof(document));size=(unsigned)n;cursor=size;modified=false;
                copy(filename,prompt_value,sizeof(filename));status(app_persistent?"Opened file from system disk":"Opened file from RAM volume");
            } else {copy(filename,prompt_value,sizeof(filename));save();}
        }
        return;
    }
    if(key&KEY_CTRL) {
        switch(key&65535) {
        case 'o':if(modified){status("Unsaved changes. Save first or Ctrl-N to discard.");return;}action(1);break;
        case 's':save();break;case 'a':action(2);break;
        case 'n':size=cursor=0;document[0]=filename[0]=0;modified=false;status("New empty document");break;
        }
        return;
    }
    if(key==KEY_LEFT){if(cursor)cursor--;return;}
    if(key==KEY_RIGHT){if(cursor<size)cursor++;return;}
    if(key==8 && cursor) {
        for(unsigned i=cursor-1;i<size;i++)document[i]=document[i+1];cursor--;size--;modified=true;
    } else if((key==13 || (key>=32 && key<127)) && size<4095) {
        for(unsigned i=size+1;i>cursor;i--)document[i]=document[i-1];
        document[cursor++]=key==13?'\n':(char)key;size++;modified=true;
    }
}
void notepad_paint(void) {
    begin(0xffffff);ui_toolbar(&app_gui,0,0,app_width,29);
    ui_button(&app_gui,1,'o'|KEY_CTRL,4,2,80,25,"Open",true);ui_button(&app_gui,2,'s'|KEY_CTRL,92,2,82,25,"Save",true);ui_button(&app_gui,3,'a'|KEY_CTRL,184,2,94,25,"Save As",true);ui_button(&app_gui,4,600,286,2,80,25,"File",true);
    text(10,38,filename[0]?filename:"Untitled",0x2a5579);
    unsigned columns=(unsigned)(app_width-24)/12,rows=(unsigned)(app_height-105)/18;
    if(columns>78)columns=78;
    unsigned cursor_line=0,column=0;
    for(unsigned i=0;i<cursor;i++){if(document[i]=='\n' || ++column==columns){cursor_line++;column=0;}}
    unsigned first=cursor_line>=rows?cursor_line-rows+1:0,line=0,col=0;char buffer[80]={0};
    for(unsigned i=0;i<=size;i++) {
        if(document[i]=='\n' || i==size || col==columns) {
            buffer[col]=0;if(line>=first && line<first+rows)text(10,62+(int)(line-first)*18,buffer,0x152b40);
            line++;col=0;if(document[i]=='\n' || i==size)continue;
        }
        buffer[col++]=document[i];
    }
    if(cursor_line>=first && cursor_line<first+rows)box(10+(int)column*12,62+(int)(cursor_line-first)*18,2,16,0x286bb0);
    if(modified)text(app_width-118,38,"Modified",0x965024);
    if(menu) {
        ui_menu_panel(&app_gui,286,30,200,118);
        ui_menu_item(&app_gui,610,'o'|KEY_CTRL,290,34,192,"Open...       Ctrl-O",true);
        ui_menu_item(&app_gui,611,'s'|KEY_CTRL,290,61,192,"Save          Ctrl-S",true);
        ui_menu_item(&app_gui,612,'a'|KEY_CTRL,290,88,192,"Save as...    Ctrl-A",true);
        ui_menu_item(&app_gui,613,'n'|KEY_CTRL,290,115,192,"New           Ctrl-N",true);
    }
}

bool notepad_close(void) {
    if(!modified)return true;
    operation=3;prompt("Discard unsaved edits? Type YES","");return false;
}
