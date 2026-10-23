#!/usr/bin/env python3
"""Observed nonzero-bus regression with QEMU devices; no Fusion success claim."""
import hashlib,json,tempfile,time
from pathlib import Path
from desktop import Guest
from applications import launch,type_text,matches
from boot import BUILD
from bus_topology import BRIDGED
results=[]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
run=Path(tempfile.mkdtemp(prefix='fusion-devices-',dir=BUILD));disk=run/'read-only-trial.img'
with disk.open('wb') as f:f.truncate(128*1048576)
before=sha(disk)
digits={'0':[14,17,19,21,25,17,14],'1':[4,12,4,4,4,4,14],'2':[14,17,1,2,4,8,31],'3':[30,1,1,14,1,1,30],'4':[2,6,10,18,31,2,2]}
def digit(p,x,y,n,bg=0x163a64):return all(p(x+c*2,y+r*2)==(0xffffff if bits&(16>>c) else bg) for r,bits in enumerate(digits[n]) for c in range(5))
def diagnostic(g):
 serial=BUILD/(g.label+'.serial.log');deadline=time.monotonic()+120
 while 'Diagnostic mode: no installer, no filesystem writes' not in serial.read_text(errors='replace'):
  assert g.proc.poll() is None
  if time.monotonic()>deadline:raise AssertionError('Diagnostic boot stalled '+g.label)
  time.sleep(.2)
 g.wait(lambda p:p(0,g.dimensions()[1]-24)==0x244da8 and p(18,g.dimensions()[1]-180)==0xffffff,'complete USB test panel',45)
 return serial.read_text(errors='replace')
def record(g,**kw):return {'passed':True,'case':g.label,'command':g.command,'iso_sha256':sha(g.iso),**kw}
# Preserved actual M7E image reproduces the observed discovery blind spot.
g=Guest('fusion-m7e-baseline-nonzero',disk=disk,optical=True,live=False,gic_version=3,iso=BUILD.parent/'artifacts/milestone-7e/CHOS-ARM64-M7E-DIAGNOSTIC.iso',**BRIDGED)
try:
 serial=BUILD/(g.label+'.serial.log');deadline=time.monotonic()+120
 while 'Diagnostic mode: no installer, no filesystem writes' not in serial.read_text(errors='replace'):
  assert g.proc.poll() is None
  if time.monotonic()>deadline:raise AssertionError('Preserved baseline boot stalled')
  time.sleep(.2)
 text=serial.read_text(errors='replace');assert 'Input initialized devices: 0' in text and 'NVME PCI CONTROLLER NOT FOUND' in text
 g.wait(lambda p:digit(p,g.dimensions()[0]-64,16,'2',0x102b50),'baseline page showing discovery failure',45)
 g.snapshot('fusion-m7e-baseline-nonzero');results.append(record(g,expected_old_discovery_failure=True))
finally:g.close()
assert sha(disk)==before
g=Guest('fusion-bus-input',disk=disk,optical=True,live=False,gic_version=3,iso=BUILD/'CHOS-ARM64-M7G-INPUT.iso',**BRIDGED)
try:
 text=diagnostic(g)
 for s in ['xHCI halt: PASS','xHCI reset/CNR: PASS','xHCI four aligned DMA pages allocated: PASS','xHCI DBOFF raw:','xHCI RTSOFF raw:','xHCI PCI command DMA:','USB boot keyboard initialized: PASS','USB boot mouse initialized: PASS','NVMe initialized: PASS','NVMe first/last 4KiB reads: PASS','NVMe diagnostic write guard: PASS']:assert s in text,s
 # Firmware assigns both endpoints below root ports, not on bus zero.
 import re
 bdfs=[int(x,16) for x in re.findall(r'Driver PCI segment/bus/devfn: 0x([0-9a-f]+)',text)]
 assert len(bdfs)==2 and all((x>>8)&255 for x in bdfs),bdfs
 h=g.dimensions()[1];g.qmp('input-send-event',{'events':[{'type':'key','data':{'down':True,'key':{'type':'qcode','data':'q'}}}]})
 g.wait(lambda p:digit(p,304,h-84,'1') and digit(p,448,h-84,'1'),'live hardware key code and press')
 g.snapshot('fusion-input-key')
 g.qmp('input-send-event',{'events':[{'type':'key','data':{'down':False,'key':{'type':'qcode','data':'q'}}}]})
 g.move(300,340);g.button(True);g.wait(lambda p:digit(p,304,h-66,'3') and digit(p,316,h-66,'0') and digit(p,400,h-66,'3') and digit(p,412,h-66,'4') and digit(p,496,h-66,'1'),'live pointer/button event');g.snapshot('fusion-input-mouse')
 g.qmp('device_del',{'id':'usbmouse'});g.wait(lambda p:digit(p,496,h-66,'0'),'disconnect releases button in live panel')
 g.key('a');g.wait(lambda p:digit(p,304,h-84,'3') and digit(p,316,h-84,'0') and digit(p,448,h-84,'0'),'remaining keyboard key release');g.snapshot('fusion-input-disconnect')
 stats=g.qmp('query-blockstats');assert all(x['stats']['wr_operations']==0 for x in stats),stats
 results.append(record(g,nonzero_endpoint_devfns=bdfs,live_keyboard_and_mouse=True,disconnect_release=True,read_only_identification_and_reads=True,block_write_operations=0))
finally:g.close()
assert sha(disk)==before,'Diagnostic changed target disk'
g=Guest('fusion-bus-setup',disk=disk,optical=True,live=False,gic_version=3,**BRIDGED)
try:
 g.wait(lambda p:p(30,160)==0x244da8,'normal Setup with bridged USB/NVMe',120);g.snapshot('fusion-bus-setup')
 g.key('down');g.wait(lambda p:p(30,212)==0x244da8,'USB arrow selects Live Desktop')
 g.key('up');g.wait(lambda p:p(30,160)==0x244da8,'USB arrow restores Install')
 g.key('ret');g.wait(lambda p:matches(p,32,92,'CHOS Setup Agreement',0xf2f6ff),'USB Enter opens license')
 g.key('esc');g.wait(lambda p:p(30,160)==0x244da8,'USB Escape restores welcome focus')
 g.snapshot('fusion-bus-navigation')
 g.key('l');g.wait(lambda p:p(60,250)==0x326bb0,'USB Setup L selects native Live Desktop',120)
 g.click(618,36);launch(g,2);type_text(g,'BRIDGED USB INPUT')
 g.wait(lambda p:matches(p,42,118,'BRIDGED USB INPUT',0x152b40),'actual native keyboard/mouse interaction');g.snapshot('fusion-bus-notepad')
 results.append(record(g,setup_keyboard_navigation=True,native_mouse_keyboard=True,unconfirmed_disk_unchanged=True))
finally:g.close()
assert sha(disk)==before,'Unconfirmed normal Setup changed disk'
for label,devices in [('fusion-input-absent',['qemu-xhci']),('fusion-input-refusal',['qemu-xhci,p2=0,p3=0'])]:
 g=Guest(label,ram=2048,live=False,gic_version=3,input_backend='none',iso=BUILD/'CHOS-ARM64-M7G-INPUT.iso',extra_devices=devices)
 try:
  text=diagnostic(g);assert 'Input initialized devices: 0' in text
  if label.endswith('refusal'):assert 'xHCI failure stage: 2' in text and 'xHCI failure reason: 205' in text
  else:assert 'xHCI reset and polled event ring: PASS' in text and 'USB connected root ports: 0' in text
  g.snapshot(label);results.append(record(g,no_disk_or_input=True))
 finally:g.close()
g=Guest('fusion-nvme-refusal',ram=2048,disk=disk,optical=True,live=False,gic_version=3,input_backend='none',iso=BUILD/'CHOS-ARM64-M7G-INPUT.iso',storage_pci_bus='nvmeroot',early_devices=['pcie-root-port,id=nvmeroot,chassis=2'],nvme_options='mqes=7')
try:
 text=diagnostic(g);assert 'NVME CAPABILITIES UNSUPPORTED' in text and 'NVMe failure stage: 4' in text
 # Wait for the actual controller error line, not an unrelated first report page.
 glyphs=[[17,25,21,19,17,17,17],[17,17,17,17,17,10,4],[17,27,21,21,17,17,17],[31,16,16,30,16,16,31]]
 def error_visible(p):
  return any(all(p(16+i*12+c*2,78+line*18+r*2)==(0xe0efff if bits&(16>>c) else 0x102b50) for i,glyph in enumerate(glyphs) for r,bits in enumerate(glyph) for c in range(5)) for line in range((g.dimensions()[1]-264)//18))
 g.wait(error_visible,'visible NVME capability refusal line',60)
 g.snapshot('fusion-nvme-refusal');results.append(record(g,controller_detected_but_capabilities_rejected=True,read_only=True))
finally:g.close()
assert sha(disk)==before
(BUILD/'fusion-devices.json').write_text(json.dumps({'passed':True,'scope':'QEMU nonzero-bus regression and live diagnostic panel; real Fusion M7G retest pending','cases':results,'all_unconfirmed_disks_unchanged':True},indent=2)+'\n');print(json.dumps(results,indent=2))
