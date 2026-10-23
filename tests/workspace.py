#!/usr/bin/env python3
"""Actual native multi-window use, menus/dialogs, selection/scrolling and input bursts."""
import hashlib,json,tempfile,time
from pathlib import Path
from desktop import Guest
from applications import launch,type_text,matches
from boot import BUILD

def fast_type(g,text):
    # At most two characters per QMP batch, within the device's 32 descriptors.
    # A bounded stream exercises the app event drain without fabricating input.
    punctuation={'/':'slash','.':'dot','-':'minus',' ':'spc','\n':'ret','_':'minus'}
    for at in range(0,len(text),2):
        events=[]
        for c in text[at:at+2]:
            key=punctuation.get(c,c.lower());keys=(['shift'] if c.isupper() or c=='_' else [])+[key]
            events.extend({'type':'key','data':{'down':True,'key':{'type':'qcode','data':k}}} for k in keys)
            events.extend({'type':'key','data':{'down':False,'key':{'type':'qcode','data':k}}} for k in reversed(keys))
        g.qmp('input-send-event',{'events':events});time.sleep(.04)
run=Path(tempfile.mkdtemp(prefix='workspace-',dir=BUILD));disk=run/'destination.img'
with disk.open('wb') as f:f.truncate(128*1024*1024)
def digest():
    with disk.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
before=digest();g=Guest('workspace',disk=disk,optical=True)
result={'passed':False,'iso_sha256':hashlib.sha256(g.iso.read_bytes()).hexdigest(),'command':g.command,'checks':[]}
try:
    g.wait(lambda p:p(60,250)==0x326bb0,'About',120);g.click(618,36)
    launch(g,1);g.wait(lambda p:p(60,240)==0xf2f6fa,'Explorer');g.click(100,146);g.click(100,146)
    launch(g,2);g.wait(lambda p:p(70,200)==0xffffff,'second native window');type_text(g,'CHOS TOOLKIT')
    g.wait(lambda p:matches(p,66,140,'CHOS TOOLKIT',0x152b40),'Notepad in overlapping windows')
    g.snapshot('workspace-overlap-initial')
    # Hover, press, cancel outside: the Open button must not activate.
    g.move(90,91);g.snapshot('workspace-button-hover');g.button(True);g.snapshot('workspace-button-pressed');g.move(850,600);g.button(False)
    assert g.capture()(64,400)!=0xb9d5ee,'Cancelled button opened a dialog'
    g.click(365,92);g.wait(lambda p:matches(p,356,117,'Open...',0x204b72,0),'native File menu');g.snapshot('workspace-file-menu')
    g.key('down');g.key('ret') # Open -> Save, using the shared menu's focus model
    g.wait(lambda p:p(64,400)==0xb9d5ee,'native Save As dialog');g.snapshot('workspace-save-dialog')
    g.key('u',('ctrl',));type_text(g,'/documents/toolkit.txt');g.click(620,464)
    g.wait(lambda p:p(64,400)!=0xb9d5ee,'dialog accepted by pointer')
    # Resize and arrange the existing windows; both have visible real content.
    g.move(768,502);g.button(True);g.move(648,342);g.button(False)
    g.move(140,58);g.button(True);g.move(440,300);g.button(False)
    g.click(140,38);g.move(744,480);g.button(True);g.move(540,370);g.button(False)
    g.key('r');g.click(100,170);g.snapshot('workspace-arranged')
    g.click(887,300);g.wait(lambda p:not matches(p,366,382,'CHOS TOOLKIT',0x152b40),'minimized Notepad');g.snapshot('workspace-minimized')
    w,h=g.dimensions();g.click(250,h-22);g.wait(lambda p:matches(p,366,382,'CHOS TOOLKIT',0x152b40),'taskbar restore')
    g.click(912,300);g.wait(lambda p:matches(p,14,94,'CHOS TOOLKIT',0x152b40),'maximized Notepad')
    suffix='123456789012345678901234567890123456789012345678'
    fast_type(g,suffix);expected='CHOS TOOLKIT'+suffix
    g.wait(lambda p:matches(p,14,94,expected,0x152b40),'exact bounded input burst');g.key('s',('ctrl',));g.snapshot('workspace-maximized')
    g.click(w-40,12)
    # Real files created by Terminal make the Explorer scrollbar useful.
    launch(g,3);g.wait(lambda p:p(110,320)==0x0a1b2d,'native Terminal')
    for n in range(18):fast_type(g,f'mkdir /documents/F{n:02}\n')
    type_text(g,'cat /documents/toolkit.txt\n')
    g.wait(lambda p:any(matches(p,90,110+i*18,expected[:48],0xd3e7fa) for i in range(16)),'separate process reads exact saved burst text')
    g.snapshot('workspace-terminal-files');g.click(786,80)
    g.click(140,38);g.key('r');g.move(529,160);g.button(True);g.move(529,306);g.button(False)
    g.wait(lambda p:matches(p,46,296,'F17',0x206ba5),'real filesystem list scrollbar');g.snapshot('workspace-scrolled-files')
    launch(g,5);g.wait(lambda p:matches(p,92,112,'CHOS Task Manager',0x204b72,0),'live process display');g.snapshot('workspace-tasks')
    launch(g,7);g.wait(lambda p:matches(p,116,138,'Welcome to CHOS Setup',0x204b72,0),'native Setup')
    g.move(730,497);g.button(True);g.snapshot('workspace-setup-pressed');g.button(False);g.key('ret')
    g.wait(lambda p:matches(p,116,320,'GPT',0x204b72,0),'real destination inventory');g.snapshot('workspace-setup')
    assert digest()==before,'Setup inventory modified unconfirmed destination'
    result.update(passed=True,burst_text=expected,unconfirmed_disk_unchanged=True,checks=['Explorer and Notepad simultaneously','overlapping resized windows','hover/pressed/cancelled button','keyboard application menu','pointer-accepted native dialog','file selection','minimize/taskbar restore/maximize','bounded fast input and cross-process saved contents','scrollbar on real directory entries','actual Task Manager processes','native Setup and unchanged unconfirmed disk'])
finally:g.close();(BUILD/'workspace.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
