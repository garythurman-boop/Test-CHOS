#!/usr/bin/env python3
"""Native application acceptance via keyboard/mouse, not host-side app emulation."""
import hashlib
import json
import time
from desktop import Guest
from boot import BUILD, CHOS

# Decode the checked-in A8 cache independently of the guest renderer. This
# checks native raster output; it does not synthesize a screenshot or app state.
import struct
FONT=(BUILD.parent/'assets/royale/fonts.bin').read_bytes()
BASELINE=struct.unpack_from('<I',FONT,24+4*16+4)[0]
TABLE=struct.unpack_from('<I',FONT,24+4*16+12)[0]
def matches(pixel,x,y,s,color,style=4):
    baseline=struct.unpack_from('<I',FONT,24+style*16+4)[0]
    table=struct.unpack_from('<I',FONT,24+style*16+12)[0]
    pen=0
    for index,c in enumerate(s):
        offset,w,h,left,top,advance=struct.unpack_from('<IHHbbH',FONT,table+(ord(c)-32)*12)
        for row in range(h):
            for col in range(w):
                coverage=FONT[offset+row*w+col]
                actual=pixel(x+pen+left+col,y+baseline+top+row)
                distance=max(abs(((actual>>shift)&255)-((color>>shift)&255)) for shift in (0,8,16))
                if coverage>=250 and distance>6:return False
                if coverage==0 and distance<=4:return False
        pen+=advance
    return True

def read_number(pixel,x,y,color):
    value=''
    for offset in range(12):
        digit=next((str(n) for n in range(10) if matches(pixel,x+offset*12,y,str(n),color)),None)
        if digit is None:break
        value+=digit
    assert value, 'No numeric glyphs at expected instrumentation location'
    return int(value)

def type_text(guest,text):
    punctuation={'/':'slash','.':'dot','-':'minus','_':'minus',' ':'spc','\n':'ret'}
    for c in text:
        key=punctuation.get(c,c.lower())
        guest.key(key,('shift',) if c.isupper() or c=='_' else ())

def launch(guest,app):guest.key('meta_l');guest.key(str(app));time.sleep(.3)
def replace_prompt(guest,value):guest.key('u',('ctrl',));type_text(guest,value);guest.key('ret');time.sleep(.25)

def main():
    started=time.monotonic();guest=Guest('applications')
    result={'passed':False,'iso_sha256':hashlib.sha256(guest.iso.read_bytes()).hexdigest(),'command':guest.command,'checks':[]}
    try:
        guest.wait(lambda p:p(60,250)==0x326bb0,'native About',120)
        result['managed_conventional_ram_kib']=read_number(guest.capture(),402,174,0x204b72)
        result['desktop_ready_seconds']=round(time.monotonic()-started,3)
        guest.click(618,36);guest.wait(lambda p:p(60,250)!=0x326bb0,'close About')
        launch(guest,2)
        guest.wait(lambda p:p(45,160)==0xffffff,'native Notepad')
        type_text(guest,'CHOS FILE 123')
        guest.wait(lambda p:matches(p,42,118,'CHOS FILE 123',0x152b40),'typed editor content')
        guest.key('s',('ctrl',));guest.wait(lambda p:p(40,380)==0xb9d5ee,'save-as prompt')
        guest.key('ret');time.sleep(.3)
        guest.key('n',('ctrl',));guest.wait(lambda p:not matches(p,42,118,'CHOS FILE 123',0x152b40),'new document')
        guest.key('o',('ctrl',));guest.key('ret')
        guest.wait(lambda p:matches(p,42,118,'CHOS FILE 123',0x152b40),'save/open round trip')
        type_text(guest,'A');guest.key('backspace')
        guest.click(738,36);guest.wait(lambda p:p(40,380)==0xb9d5ee,'unsaved close confirmation')
        guest.key('esc');guest.key('s',('ctrl',))
        guest.wait(lambda p:matches(p,42,118,'CHOS FILE 123',0x152b40),'cancel close and save retains contents')
        guest.key('a',('ctrl',));replace_prompt(guest,'/documents/AS.TXT')
        guest.snapshot('apps-notepad')
        result['checks']+=['Notepad edit/backspace, save/save-as, new/open, unsaved-close cancellation and exact text round trip']
        launch(guest,3)
        guest.wait(lambda p:p(60,120)==0x0a1b2d,'native Terminal')
        for command in ['mkdir /documents/work','copy /documents/note.txt /documents/copy.txt','move /documents/copy.txt /documents/work/moved.txt','cat /documents/work/moved.txt']:
            type_text(guest,command+'\n')
        guest.wait(lambda p:any(matches(p,66,88+i*18,'CHOS FILE 123',0xd3e7fa) for i in range(16)),'cross-process copy, move and read content')
        guest.snapshot('apps-terminal')
        type_text(guest,'del /documents/work/moved.txt\n')
        guest.click(762,58);time.sleep(.3)
        # A deleted path fails to open and leaves the existing editor buffer intact.
        guest.key('o',('ctrl',));replace_prompt(guest,'/documents/work/moved.txt')
        guest.wait(lambda p:matches(p,42,118,'CHOS FILE 123',0x152b40),'failed open preserves editor text')
        result['checks']+=['Terminal mkdir/copy/move/cat/del through native file APIs','cross-process file contents','failed open preserves edits']
        launch(guest,1)
        guest.wait(lambda p:p(60,240)==0xf2f6fa,'native Explorer')
        guest.click(100,170);time.sleep(.25) # /documents
        guest.key('n');type_text(guest,'FOLDER');guest.key('ret')
        # /documents contains welcome.txt, note.txt, AS.TXT, work, FOLDER.
        guest.wait(lambda p:matches(p,70,252,'FOLDER',0x206ba5),'Explorer native mkdir and directory refresh')
        guest.click(100,254);time.sleep(.25)
        guest.wait(lambda p:matches(p,68,86,'/documents/FOLDER',0x204b72,0),'Explorer directory navigation')
        guest.key('u');time.sleep(.2)
        guest.click(100,188) # note.txt
        guest.key('c');type_text(guest,'GUI.TXT');guest.key('ret')
        guest.wait(lambda p:matches(p,70,274,'GUI.TXT',0x203a54),'Explorer copied file appears before move')
        guest.key('r');guest.click(100,276) # GUI.TXT, sixth entry
        guest.key('m');type_text(guest,'RENAMED.TXT');guest.key('ret');time.sleep(.2)
        guest.move(790,540)
        guest.wait(lambda p:matches(p,70,274,'RENAMED',0x203a54),'Explorer copy and move refresh')
        guest.click(100,276);guest.key('d');type_text(guest,'YES');guest.key('ret')
        guest.wait(lambda p:not matches(p,70,274,'RENAMED',0x203a54),'Explorer confirmed deletion')
        guest.snapshot('apps-explorer')
        guest.click(762,58);time.sleep(.2)
        guest.click(738,36);time.sleep(.2) # close unmodified Notepad
        result['checks']+=['Explorer navigation, mkdir/copy/move/confirmed deletion and metadata refresh']
        launch(guest,4)
        guest.wait(lambda p:matches(p,44,72,'CHOS Control Panel',0x204b72,0),'functional Control Panel')
        old=guest.capture()(780,20);guest.key('2')
        guest.wait(lambda p:p(780,20)!=old,'theme changes actual compositor wallpaper')
        guest.snapshot('apps-settings')
        launch(guest,5)
        guest.wait(lambda p:p(60,120)==0xe5eef8,'native Task Manager')
        pixel=guest.wait(lambda p:matches(p,202,150,'4',0x204b72) and matches(p,202,172,'5',0x204b72),'Task Manager live application roles')
        first_pid=read_number(pixel,70,150,0x204b72);second_pid=read_number(pixel,70,172,0x204b72)
        assert first_pid>0 and second_pid==first_pid+1,'Task Manager PID lifecycle is inconsistent'
        result['observed_task_pids']=[first_pid,second_pid]
        result['allocated_conventional_ram_kib_two_apps']=read_number(pixel,334,342,0x204b72)
        guest.snapshot('apps-tasks')
        result['checks']+=['Control Panel changes desktop theme','Task Manager queries live process and memory instrumentation']
        # End the graphical session through its confirmation, then launch Terminal.
        width,height=guest.dimensions();guest.click(40,height-22)
        guest.click(70,height-44-400+378)
        guest.wait(lambda p:p((width-540)//2+10,(height-150)//2+50)==0xc9deee,'session confirmation')
        guest.key('y');guest.wait(lambda p:p(60,250)==0x326bb0,'session reset and process reaping')
        result['checks']+=['confirmed session termination and fresh native About']
        guest.click(618,36);time.sleep(.2);launch(guest,3)
        type_text(guest,'shutdown\n');type_text(guest,'YES\n')
        deadline=time.monotonic()+8
        while guest.proc.poll() is None and time.monotonic()<deadline:time.sleep(.1)
        assert guest.proc.poll()==0,'PSCI shutdown did not exit QEMU successfully'
        assert guest.power_reason()=='guest-shutdown','Missing QMP guest shutdown evidence'
        result['checks']+=['confirmed Terminal shutdown via ACPI-discovered PSCI conduit']
        result.update(passed=True,elapsed_seconds=round(time.monotonic()-started,3))
        print(json.dumps(result,indent=2))
    finally:
        guest.close();(BUILD/'applications.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
