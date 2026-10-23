#include "app.h"
static char history[16][80],command[120],cwd[60]="/";
static unsigned lines;
static void line(const char *s) {
    if(lines==16){for(unsigned i=0;i<15;i++)copy(history[i],history[i+1],80);lines--;}
    copy(history[lines++],s,80);
}
static bool absolute(const char *s,char path[60]) {
    if(!s[0])return false;
    if(equal(s,"..")){copy(path,cwd,60);unsigned n=(unsigned)length(path);while(n && path[n]!='/')n--;path[n?n:1]=0;return true;}
    if(s[0]=='/'){if(length(s)>=60)return false;copy(path,s,60);return true;}
    size_t n=length(cwd),m=length(s);bool root=equal(cwd,"/");
    if(n+m+(root?0:1)>=60)return false;
    copy(path,cwd,60);if(!root)path[n++]='/';copy(path+n,s,60-n);return true;
}
static void execute(void) {
    char words[3][120]={{0}};unsigned token=0,index=0;
    for(unsigned i=0;command[i];i++) {
        if(command[i]==' '){if(index){words[token][index]=0;if(++token==3)break;index=0;}continue;}
        if(index+1<120)words[token][index++]=command[i];
    }
    if(token<3)words[token][index]=0;
    char a[60],b[60];bool pa=absolute(words[1],a),pb=absolute(words[2],b);
    if(!words[0][0])return;
    if(equal(words[0],"help")) {
        line("help dir cd mkdir copy move del cls");line("cat systeminfo reboot shutdown");line("run explorer|notepad|terminal|settings");line("run tasks|about");line("Files live on a volatile RAM volume.");return;
    }
    if(equal(words[0],"cls")){lines=0;return;}
    if(equal(words[0],"pwd")){line(cwd);return;}
    if(equal(words[0],"dir")) {
        const char *base=pa?a:cwd;line(base);
        unsigned count=0;
        for(unsigned i=0;i<40;i++) {
            struct chos_dirent e;if(api(SYS_LIST,i,(uintptr_t)&e,0))continue;
            size_t n=equal(base,"/")?0:length(base);bool match=true;
            for(size_t j=0;j<n;j++)if(base[j]!=e.path[j])match=false;
            if(!match || e.path[n]!='/')continue;
            const char *name=e.path+n+1;bool child=true;for(size_t j=0;name[j];j++)if(name[j]=='/')child=false;
            if(child){line(name);count++;}
        }
        if(!count)line("No entries (or directory absent)");return;
    }
    if(equal(words[0],"cd")) {
        if(!pa){line(cwd);return;}bool exists=equal(a,"/");
        for(unsigned i=0;i<40;i++){struct chos_dirent e;if(api(SYS_LIST,i,(uintptr_t)&e,0)==0 && e.kind==2 && equal(e.path,a))exists=true;}
        if(exists)copy(cwd,a,sizeof(cwd));else line("Directory not found");return;
    }
    if(equal(words[0],"systeminfo")) {
        struct chos_info info;if(api(SYS_INFO,(uintptr_t)&info,0,0)){line("System information unavailable");return;}
        line("CHOS pre-alpha ARM64. Original kernel.");char value[21];number(value,info.processes);line("Live user processes:");line(value);
        number(value,info.used_pages*4);line("Allocated conventional RAM KiB:");line(value);return;
    }
    if(equal(words[0],"run")) {
        static const char names[6][12]={"explorer","notepad","terminal","settings","tasks","about"};
        for(unsigned i=0;i<6;i++)if(equal(names[i],words[1])){line(api(SYS_LAUNCH,i+1,0,0)>0?"Launched":"Launch failed (process limit)");return;}
        line("Unknown application");return;
    }
    if(equal(words[0],"reboot") || equal(words[0],"shutdown")) {
        line("RAM files will be lost. Confirm Y in prompt.");
        prompt(equal(words[0],"reboot")?"Restart? Type YES":"Shut down? Type YES","");return;
    }
    int64_t result=-22;
    if(equal(words[0],"mkdir") && pa)result=api(SYS_MKDIR,(uintptr_t)a,0,0);
    else if(equal(words[0],"move") && pa && pb)result=api(SYS_RENAME,(uintptr_t)a,(uintptr_t)b,0);
    else if(equal(words[0],"del") && pa)result=api(SYS_UNLINK,(uintptr_t)a,0,0);
    else if((equal(words[0],"copy") || equal(words[0],"cat")) && pa) {
        static char bytes[4096];int n=read_file(a,bytes,sizeof(bytes));
        if(n<0){line("Read failed (missing or over 4095 bytes)");return;}
        if(equal(words[0],"cat")) {
            char row[64];unsigned k=0;
            for(int i=0;i<=n;i++) {
                if(bytes[i]=='\n' || i==n || k==48){row[k]=0;line(row);k=0;if(bytes[i]=='\n' || i==n)continue;}
                row[k++]=bytes[i];
            }
            return;
        }
        if(pb)result=write_file(b,bytes,(unsigned)n);
    } else {line("Unknown command or missing arguments");return;}
    line(result==0?"OK":"Operation failed");
}
void terminal_init(void) {line("CHOS Terminal. Type help.");status("RAM volume. Commands execute through native APIs.");}
void terminal_key(unsigned key) {
    if(prompting) {
        if(prompt_key(key)) {
            if(equal(prompt_value,"YES")) {
                unsigned action=equal(command,"reboot")?1:2;
                if(api(SYS_POWER,action,0,0)<0)line("Firmware power service unsupported");
            } else line("Cancelled");
            command[0]=0;
        }
        return;
    }
    size_t n=length(command);
    if(key==13){line(command);execute();if(!prompting)command[0]=0;return;}
    if(key==8 && n)command[n-1]=0;
    else if(key>=32 && key<127 && n+1<sizeof(command)){command[n]=(char)key;command[n+1]=0;}
}
void terminal_paint(void) {
    begin(0x0a1b2d);unsigned rows=(unsigned)(app_height-64)/18;if(rows>16)rows=16;
    unsigned first=lines>rows?lines-rows:0;
    for(unsigned i=first;i<lines;i++)text(10,10+(int)(i-first)*18,history[i],0xd3e7fa);
    text(10,app_height-50,">",0x8ddbb5);text(30,app_height-50,command,0xffffff);
}
