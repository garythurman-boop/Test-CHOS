#ifndef CHOS_APP_H
#define CHOS_APP_H
#include "abi.h"
#include <stddef.h>
#include <stdbool.h>
extern unsigned app_role;
extern bool app_persistent;
const char *volume_status(void);
void setup_paint(void);void setup_key(unsigned);void setup_click(int,int);
extern int app_window,app_width,app_height;
extern char app_status[80];
int64_t api(unsigned,uint64_t,uint64_t,uint64_t);
size_t length(const char *);
void copy(char *,const char *,size_t);
bool equal(const char *,const char *);
void number(char *,uint64_t);
void text(int,int,const char *,uint32_t);
void box(int,int,int,int,uint32_t);
void rounded(int,int,int,int,uint32_t);
void ui_text(unsigned,int,int,const char *,uint32_t);
void label(int,const char *);
void status(const char *);
void begin(uint32_t);
void present(void);
void prompt(const char *,const char *);
bool prompt_key(unsigned);
extern char prompt_value[120];
extern bool prompting;
int read_file(const char *,char *,unsigned);
int write_file(const char *,const char *,unsigned);
void explorer_init(void);void explorer_paint(void);void explorer_key(unsigned);void explorer_click(int,int);
bool notepad_close(void);
void notepad_init(void);void notepad_paint(void);void notepad_key(unsigned);void notepad_click(int,int);
void terminal_init(void);void terminal_paint(void);void terminal_key(unsigned);
void settings_paint(void);void settings_key(unsigned);void settings_click(int,int);
void task_paint(void);void task_key(unsigned);
void about_paint(void);void about_key(unsigned);
#endif
