#!/usr/bin/env python3
"""Native boot/report-HID and hub/absolute regressions on real QEMU devices."""
import hashlib,json,time,tempfile
from pathlib import Path
from desktop import Guest
from applications import launch,type_text,matches
from boot import BUILD
results=[]
class AbsoluteGuest(Guest):
 def move(self,x,y):
  w,h=self.dimensions();self.qmp('input-send-event',{'events':[{'type':'abs','data':{'axis':'x','value':round(x*32767/(w-1))}},{'type':'abs','data':{'axis':'y','value':round(y*32767/(h-1))}}]});self.x,self.y=x,y;time.sleep(.1)
def record(g,**kw):return {'passed':True,'case':g.label,'command':g.command,'iso_sha256':hashlib.sha256(g.iso.read_bytes()).hexdigest(),**kw}
for label,hubs in [('usb-report-direct',0),('usb-report-hub',1),('usb-report-nested',2)]:
 devices=['qemu-xhci,id=xhci,p2=8,p3=0'];path=''
 for i in range(hubs):
  port=path+'1';devices.append(f'usb-hub,id=hub{i},bus=xhci.0,port={port}');path=port+'.'
 devices += [f'usb-kbd,id=usbkbd,bus=xhci.0,port={path}1',f'usb-tablet,id=usbmouse,bus=xhci.0,port={path}2']
 g=AbsoluteGuest(label,ram=2048,live=False,gic_version=3,input_backend='none',extra_devices=devices)
 try:
  g.wait(lambda p:p(30,160)==0x244da8,'normal Setup with keyboard and report-protocol absolute pointer',120)
  text=(BUILD/(label+'.serial.log')).read_text(errors='replace')
  assert 'USB HID absolute pointer layout: PASS' in text,text[-4000:]
  assert 'USB report HID initialized: PASS' in text
  assert text.count('USB hub initialized: PASS')==hubs
  g.key('down');g.wait(lambda p:p(30,212)==0x244da8,'keyboard arrow through report/hub path');g.key('up');g.key('ret');g.wait(lambda p:matches(p,32,92,'CHOS Setup Agreement',0xf2f6ff),'Enter opens license');g.key('esc');g.wait(lambda p:p(30,160)==0x244da8,'Escape returns');g.snapshot(label+'-setup')
  g.key('l');g.wait(lambda p:p(60,250)==0x326bb0,'native Live Desktop',120)
  g.click(618,36);launch(g,2);type_text(g,'HUB AND ABSOLUTE INPUT')
  g.wait(lambda p:matches(p,42,118,'HUB AND ABSOLUTE INPUT',0x152b40),'native typing with report mouse and USB keyboard');g.snapshot(label+'-notepad')
  # Real absolute move/button events drag an existing window to a new position.
  g.move(200,36);g.button(True);g.move(320,180);g.button(False)
  g.wait(lambda p:any(matches(p,42+dx,118+dy,'HUB AND ABSOLUTE INPUT',0x152b40) for dx in range(119,122) for dy in range(143,146)),'absolute window dragging with translated native text');g.snapshot(label+'-drag')
  results.append(record(g,hub_depth=hubs,absolute_pointer_native_drag=True,keyboard_arrows_enter_escape_typing=True))
 finally:g.close()
# Disconnect a real emulated hub child while its button is held.
g=Guest('usb-hub-disconnect',ram=2048,live=False,gic_version=3,input_backend='none',iso=BUILD/'CHOS-ARM64-M7I-INPUT.iso',extra_devices=['qemu-xhci,id=xhci,p2=8,p3=0','usb-hub,id=hub,bus=xhci.0,port=1','usb-kbd,id=usbkbd,bus=xhci.0,port=1.1','usb-mouse,id=usbmouse,bus=xhci.0,port=1.2'])
try:
 log=BUILD/(g.label+'.serial.log');end=time.monotonic()+120
 while 'Diagnostic mode: no installer, no filesystem writes' not in log.read_text(errors='replace'):
  assert g.proc.poll() is None
  if time.monotonic()>end:raise AssertionError('Hub disconnect diagnostic boot stalled')
  time.sleep(.2)
 g.wait(lambda p:p(0,g.dimensions()[1]-24)==0x244da8,'hub disconnect input panel')
 h=g.dimensions()[1];digits={'0':[14,17,19,21,25,17,14],'1':[4,12,4,4,4,4,14],'3':[30,1,1,14,1,1,30]}
 def digit(p,x,y,n):return all(p(x+c*2,y+r*2)==(0xffffff if bits&(16>>c) else 0x163a64) for r,bits in enumerate(digits[n]) for c in range(5))
 g.button(True);g.wait(lambda p:digit(p,496,h-66,'1'),'held button before hub child removal')
 g.qmp('device_del',{'id':'usbmouse'});g.wait(lambda p:digit(p,496,h-66,'0'),'hub child removal releases button',15)
 assert 'USB releases on disconnect: 1' in log.read_text(errors='replace')
 g.key('a');g.wait(lambda p:digit(p,304,h-84,'3') and digit(p,316,h-84,'0') and digit(p,448,h-84,'0'),'keyboard behind same hub remains operational')
 g.snapshot(g.label);results.append(record(g,hub_child_disconnect_releases=True,remaining_keyboard_operational=True,no_disk=True))
finally:g.close()
# A normal distribution image safely falls back when no input function is usable.
trial=Path(tempfile.mkdtemp(prefix='input-fallback-',dir=BUILD))/'untouched.img'
with trial.open('wb') as f:f.truncate(128*1048576)
before=hashlib.sha256(trial.read_bytes()).hexdigest()
g=Guest('usb-input-fallback',disk=trial,optical=True,ram=2048,live=False,gic_version=3,input_backend='none',extra_devices=['qemu-xhci,id=xhci','usb-ccid,bus=xhci.0'])
try:
 log=BUILD/(g.label+'.serial.log');end=time.monotonic()+120
 while 'Diagnostic mode: no installer, no filesystem writes' not in log.read_text(errors='replace'):
  assert g.proc.poll() is None
  if time.monotonic()>end:raise AssertionError('Automatic input fallback did not run')
  time.sleep(.2)
 assert 'Automatic input fallback: read-only diagnostics; Setup not started' in log.read_text(errors='replace')
 assert 'Automatic fallback write guard: PASS' in log.read_text(errors='replace')
 assert all(x['stats']['wr_operations']==0 for x in g.qmp('query-blockstats'))
 g.wait(lambda p:p(0,g.dimensions()[1]-24)==0x244da8,'input-independent automatic fallback panel');g.snapshot(g.label)
 results.append(record(g,normal_iso_fallback=True,block_write_operations=0))
finally:g.close()
assert hashlib.sha256(trial.read_bytes()).hexdigest()==before
(BUILD/'usb-m7i.json').write_text(json.dumps({'passed':True,'scope':'QEMU standard report descriptors and USB2 hubs; actual Fusion keyboard/mouse retest pending','cases':results},indent=2)+'\n');print(json.dumps(results,indent=2))
