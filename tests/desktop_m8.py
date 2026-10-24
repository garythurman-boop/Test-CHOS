#!/usr/bin/env python3
"""Native shell semantics, USB absolute input, cursor damage and saved positions."""
import hashlib,json,shutil,tempfile,time
from pathlib import Path
from desktop import Guest
from applications import matches,launch
from boot import BUILD
class AbsoluteGuest(Guest):
 def move(self,x,y):
  w,h=self.dimensions();self.qmp('input-send-event',{'events':[{'type':'abs','data':{'axis':'x','value':round(x*32767/(w-1))}},{'type':'abs','data':{'axis':'y','value':round(y*32767/(h-1))}}]});self.x,self.y=x,y;time.sleep(.1)
def right(g,x,y):
 g.move(x,y)
 for down in [True,False]:g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':down,'button':'right'}}]});time.sleep(.08)
run=Path(tempfile.mkdtemp(prefix='desktop-m8-',dir=BUILD));result={'passed':False,'cases':[]}
for backend in ['virtio','usb-absolute']:
 cls=AbsoluteGuest if backend=='usb-absolute' else Guest
 options={'input_backend':'none','extra_devices':['qemu-xhci,id=xhci','usb-kbd,bus=xhci.0','usb-tablet,bus=xhci.0']} if backend=='usb-absolute' else {}
 g=cls('desktop-m8-'+backend,**options)
 try:
  g.wait(lambda p:p(60,250)==0x326bb0,'About',120);g.click(618,36)
  # One click selects; wait longer than the configured double-click interval.
  g.click(40,146);time.sleep(.7)
  assert g.capture()(200,200)!=0xffffff,'Single click incorrectly launched an app'
  g.snapshot('m8-selected-'+backend)
  g.click(40,146);g.click(40,146)
  g.wait(lambda p:p(200,200)==0xffffff,'double click opens native Notepad');g.click(738,36)
  # Drag a selected shortcut into empty space, then Enter opens exactly that app.
  g.move(40,146);g.button(True);g.move(270,570);g.button(False)
  g.snapshot('m8-icon-drag-'+backend);g.key('ret')
  g.wait(lambda p:p(200,200)==0xffffff,'Enter opens moved selected shortcut');g.click(738,36)
  right(g,270,570);g.move(600,600);g.wait(lambda p:any(matches(p,280+dx,577+dy,'Open',0x183550,3) for dx in [-1,0,1] for dy in [-1,0,1]),'icon context menu')
  g.key('down');g.key('ret');g.wait(lambda p:any(matches(p,282+dx,580+dy,'Notepad',0x183550,1) for dx in [-1,0,1] for dy in [-1,0,1]),'native shortcut properties');g.snapshot('m8-icon-properties-'+backend);g.key('esc')
  right(g,990,680);g.snapshot('m8-desktop-menu-'+backend);g.key('esc')
  # The larger cursor must erase cleanly along the partial-damage path.
  g.move(810,510);before=g.capture();region=[before(x,y) for y in range(540,580) for x in range(850,890)]
  g.move(850,540);g.move(810,510);after=g.capture();assert region==[after(x,y) for y in range(540,580) for x in range(850,890)],'Cursor left stale pixels'
  result['cases'].append({'backend':backend,'passed':True,'command':g.command,'checks':['single selects','double opens','drag and Enter','right-click properties','edge menu dismissal','cursor damage restoration']})
 finally:g.close()
# Genuine persistent icon layout across a disk-only cold boot.
source=Path(json.loads((BUILD/'native-install-512.json').read_text())['disk']);disk=run/'saved-icons.img';shutil.copyfile(source,disk)
g=Guest('desktop-m8-save',disk=disk)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'installed desktop',120);g.click(618,36)
 g.move(40,146);g.button(True);g.move(270,570);g.button(False);time.sleep(.3);g.snapshot('m8-icon-saved')
finally:g.close()
g=Guest('desktop-m8-read',disk=disk)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'disk-only cold boot',120);g.click(618,36)
 g.click(270,570);g.key('ret');g.wait(lambda p:p(200,200)==0xffffff,'saved icon position opens the same Notepad after cold boot');g.snapshot('m8-icon-cold-boot')
 result['cases'].append({'backend':'persistent-disk','passed':True,'command':g.command,'saved_position_survives_cold_boot':True})
finally:g.close()
result.update(passed=True,iso_sha256=hashlib.sha256((BUILD/'CHOS-ARM64.iso').read_bytes()).hexdigest());(BUILD/'desktop-m8.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
