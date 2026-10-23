#include "gui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned commands;
static void emit(void *unused,const struct chos_draw *d){(void)unused;assert(d->op<=DRAW_UI_TEXT && d->w>=0 && d->h>=0);commands++;}
static struct ui_result event(struct ui_context *g,unsigned type,unsigned code,int x,int y){struct chos_event e={.type=type,.code=code,.x=x,.y=y};return ui_event(g,&e);}
int main(void) {
    struct ui_context g={.emit=emit,.mouse_x=-1,.mouse_y=-1};char field[8]="ab";
    ui_begin(&g);ui_button(&g,1,'a',10,10,100,28,"Action",true);ui_button(&g,2,'b',120,10,100,28,"Disabled",false);ui_textfield(&g,3,10,50,100,field,sizeof(field));ui_scrollbar(&g,4,230,10,200,0,100,10);ui_end(&g);
    assert(commands && event(&g,EVENT_POINTER,0,20,20).repaint && g.hot==1);
    assert(event(&g,EVENT_CLICK,0,20,20).consumed && g.active==1);
    assert(!event(&g,EVENT_RELEASE,0,0,0).key && !g.active);
    event(&g,EVENT_CLICK,0,20,20);assert(event(&g,EVENT_RELEASE,0,20,20).key=='a');
    assert(!event(&g,EVENT_CLICK,0,130,20).consumed);
    event(&g,EVENT_CLICK,0,20,60);event(&g,EVENT_RELEASE,0,20,60);event(&g,EVENT_KEY,KEY_LEFT,0,0);event(&g,EVENT_KEY,'X',0,0);assert(!strcmp(field,"aXb"));
    for(unsigned i=0;i<20;i++)event(&g,EVENT_KEY,'z',0,0);
    assert(strlen(field)==sizeof(field)-1);
    event(&g,EVENT_KEY,'u'|KEY_CTRL,0,0);assert(!field[0]);
    event(&g,EVENT_KEY,9,0,0);assert(g.focus==4);event(&g,EVENT_KEY,9,0,0);assert(g.focus==1);event(&g,EVENT_KEY,9|KEY_SHIFT,0,0);assert(g.focus==4);
    event(&g,EVENT_CLICK,0,235,80);struct ui_result r=event(&g,EVENT_POINTER,0,235,1000);assert(r.id==4 && r.value==90);event(&g,EVENT_RELEASE,0,0,0);assert(!g.active);
    ui_begin(&g);for(unsigned i=1;i<=UI_CONTROLS+5;i++)ui_button(&g,i,i,10,10,100,28,"bounded",true);assert(g.count==UI_CONTROLS);ui_end(&g);
    ui_begin(&g);ui_checkbox(&g,1,'s',10,10,"Seconds",true,true);ui_radio(&g,2,'1',10,40,"Blue",true,true);ui_list_row(&g,3,10,70,200,"File",true,0x204b72);ui_menu_item(&g,4,'o',10,100,200,"Open",true);ui_tab(&g,5,1,10,130,100,"Clock",true);ui_progress(&g,10,170,100,20,1000);ui_dialog(&g,10,200,250,100,"Dialog");ui_toolbar(&g,10,310,250,30);ui_end(&g);assert(g.count==5);
    assert(event(&g,EVENT_CLICK,0,20,80).id==3);assert(!event(&g,EVENT_RELEASE,0,20,80).id);
    char path[40]="/documents/";ui_modal_edit(&g,900,path,sizeof(path));event(&g,EVENT_KEY,'R',0,0);event(&g,EVENT_KEY,'E',0,0);
    ui_begin(&g);ui_textfield(&g,900,10,10,300,path,sizeof(path));ui_end(&g);
    const char *tail="NAMED.TXT";for(unsigned i=0;tail[i];i++)event(&g,EVENT_KEY,(unsigned)tail[i],0,0);
    assert(!strcmp(path,"/documents/RENAMED.TXT"));
    ui_modal_end(&g);assert(!event(&g,EVENT_KEY,'x',0,0).consumed && g.relayout);
    puts("GUI press/release cancellation, disabled controls, focus traversal, bounded editing, scrollbar drag and registry limits passed");
}
