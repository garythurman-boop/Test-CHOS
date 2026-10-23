#ifndef CHOS_GUI_H
#define CHOS_GUI_H
#include "abi.h"
#include <stdbool.h>
#include <stddef.h>
#define UI_CONTROLS 64u
#define UI_PROMPT_FIELD 900u
#define UI_PROMPT_ACCEPT 901u
#define UI_PROMPT_CANCEL 902u
enum ui_kind { UI_BUTTON,UI_TEXTFIELD,UI_CHECKBOX,UI_RADIO,UI_SCROLLBAR,UI_LISTROW,UI_MENU,UI_TAB };
struct ui_control { unsigned id,key,kind; int x,y,w,h; bool enabled; char *buffer; unsigned capacity,value,total,page; };
struct ui_result { bool consumed,repaint; unsigned id,key,value; };
struct ui_context {
    void (*emit)(void *,const struct chos_draw *);void *output;
    struct ui_control controls[UI_CONTROLS];unsigned count,hot,active,focus,caret;
    int mouse_x,mouse_y;bool down,relayout;
};
void ui_begin(struct ui_context *);
void ui_modal_edit(struct ui_context *,unsigned,char *,unsigned);
void ui_modal_end(struct ui_context *);
void ui_end(struct ui_context *);
struct ui_result ui_event(struct ui_context *,const struct chos_event *);
void ui_label(struct ui_context *,unsigned,int,int,const char *,uint32_t);
void ui_button(struct ui_context *,unsigned,unsigned,int,int,int,int,const char *,bool);
void ui_textfield(struct ui_context *,unsigned,int,int,int,char *,unsigned);
void ui_checkbox(struct ui_context *,unsigned,unsigned,int,int,const char *,bool,bool);
void ui_radio(struct ui_context *,unsigned,unsigned,int,int,const char *,bool,bool);
void ui_scrollbar(struct ui_context *,unsigned,int,int,int,unsigned,unsigned,unsigned);
void ui_list_row(struct ui_context *,unsigned,int,int,int,const char *,bool,uint32_t);
void ui_menu_panel(struct ui_context *,int,int,int,int);
void ui_menu_item(struct ui_context *,unsigned,unsigned,int,int,int,const char *,bool);
void ui_tab(struct ui_context *,unsigned,unsigned,int,int,int,const char *,bool);
void ui_toolbar(struct ui_context *,int,int,int,int);
void ui_dialog(struct ui_context *,int,int,int,int,const char *);
void ui_progress(struct ui_context *,int,int,int,int,unsigned);
bool ui_format_size(char *,unsigned,uint64_t);
#endif
