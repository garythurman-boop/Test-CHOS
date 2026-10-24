#!/usr/bin/env python3
"""QEMU USB topology/descriptor regressions, not Fusion HID verification."""
import hashlib,json,time
from desktop import Guest
from boot import BUILD
results=[]
for label,peripherals,count in [
 ('usb-compat-rejections',['usb-tablet,id=tablet0,bus=xhci.0','usb-tablet,id=tablet1,bus=xhci.0','usb-kbd,id=usbkbd,bus=xhci.0','usb-mouse,id=usbmouse,bus=xhci.0'],2),
 ('usb-compat-four',['usb-kbd,id=kbd0,bus=xhci.0','usb-mouse,id=mouse0,bus=xhci.0','usb-kbd,id=usbkbd,bus=xhci.0','usb-mouse,id=usbmouse,bus=xhci.0'],4)]:
 g=Guest(label,ram=2048,live=False,gic_version=3,input_backend='none',iso=BUILD/'CHOS-ARM64-M7H-INPUT.iso',extra_devices=['qemu-xhci,id=xhci,p2=8,p3=0',*peripherals])
 try:
  serial=BUILD/(label+'.serial.log');deadline=time.monotonic()+120
  while 'Diagnostic mode: no installer, no filesystem writes' not in serial.read_text(errors='replace'):
   assert g.proc.poll() is None
   if time.monotonic()>deadline:raise AssertionError('USB compatibility diagnostic boot stalled')
   time.sleep(.2)
  text=serial.read_text(errors='replace')
  for expected in ['USB connected root ports: 4',f'Input initialized devices: {count}','USB device VID:','USB device PID:','USB interface class/subclass/protocol:','USB endpoint max packet:','USB descriptor selection: PASS']:assert expected in text,expected
  if count==2:assert 'USB enumeration refusal reason: 1103' in text and 'USB unsupported: HID subclass is not boot' in text
  else:assert text.count('USB boot keyboard initialized: PASS')==2 and text.count('USB boot mouse initialized: PASS')==2
  g.wait(lambda p:p(0,g.dimensions()[1]-24)==0x244da8,'live USB compatibility panel')
  h=g.dimensions()[1];digits={'0':[14,17,19,21,25,17,14],'1':[4,12,4,4,4,4,14],'2':[14,17,1,2,4,8,31],'3':[30,1,1,14,1,1,30]}
  def digit(p,x,y,n):return all(p(x+c*2,y+r*2)==(0xffffff if bits&(16>>c) else 0x163a64) for r,bits in enumerate(digits[n]) for c in range(5))
  g.key('a');g.wait(lambda p:digit(p,304,h-84,'3') and digit(p,316,h-84,'0') and digit(p,448,h-84,'0'),'key release after four-root enumeration')
  for button,value in [('left','1'),('right','2')]:
   g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':True,'button':button}}]})
   g.wait(lambda p:digit(p,496,h-66,value),'mouse '+button+' press')
   g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':False,'button':button}}]})
   g.wait(lambda p:digit(p,496,h-66,'0'),'mouse '+button+' release')
  g.snapshot(label)
  results.append({'passed':True,'case':label,'command':g.command,'iso_sha256':hashlib.sha256(g.iso.read_bytes()).hexdigest(),'connected_root_ports':4,'initialized_hid_interfaces':count,'keyboard_release_and_both_mouse_buttons':True,'no_disk':True,'unsupported_devices_do_not_block_later_boot_interfaces':count==2})
 finally:g.close()
(BUILD/'usb-compat.json').write_text(json.dumps({'passed':True,'scope':'Real QEMU devices and bounded USB topology; VMware descriptor formats still unknown','cases':results},indent=2)+'\n');print(json.dumps(results,indent=2))
