#!/usr/bin/env python3
"""No-disk diagnostics under actual optional-device refusal; USB mouse disconnect."""
import hashlib,json,time,tempfile,shutil,subprocess,sys
from desktop import Guest
from applications import launch,type_text,matches
from boot import BUILD
from pathlib import Path
results=[]
g=Guest('diagnostic-optional-failure',ram=1024,live=False,input_backend='none',iso=BUILD/'CHOS-ARM64-M7C.iso',gic_version=3,virtualization=True,extra_devices=['virtio-keyboard-pci,iommu_platform=on'])
try:
 serial=BUILD/'diagnostic-optional-failure.serial.log';deadline=time.monotonic()+120
 while 'Diagnostic mode: no installer, no filesystem writes' not in serial.read_text(errors='replace'):
  if time.monotonic()>deadline:raise AssertionError('Optional failure blocked diagnostic output')
  assert g.proc.poll() is None;time.sleep(.2)
 text=serial.read_text(errors='replace');assert 'Optional Virtio input initialization failed' in text,text[-1800:]
 assert 'NVME PCI CONTROLLER NOT FOUND' in text
 # Wait for the complete framebuffer report, not only the earlier serial marker.
 g.wait(lambda p:p(0,g.dimensions()[1]-24)==0x244da8 and any(p(x,78+y)==0xe0efff for y in range(14) for x in range(16,400)), 'complete optional-failure inventory screen',45)
 g.snapshot('diagnostic-optional-failure');results.append({'passed':True,'case':'no disk/network/input/desktop; optional Virtio IOMMU-feature rejection','command':g.command,'iso_sha256':hashlib.sha256(g.iso.read_bytes()).hexdigest()})
finally:g.close()
g=Guest('usb-mouse-disconnect',input_backend='usb',gic_version=3,virtualization=True)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'USB live About',120);g.click(618,36);launch(g,2)
 g.wait(lambda p:p(45,160)==0xffffff,'native editor')
 g.move(746,482);g.button(True);g.move(328,264);g.button(False);time.sleep(.3)
 g.key('s',('ctrl',));g.wait(lambda p:p(40,160)==0xb9d5ee,'compact save dialog')
 g.snapshot('small-dialog-hint');g.key('esc');g.wait(lambda p:p(40,160)!=0xb9d5ee,'dismiss compact dialog')
 g.move(200,36);g.button(True)
 g.qmp('device_del',{'id':'usbmouse'});deadline=time.monotonic()+10
 serial=BUILD/'usb-mouse-disconnect.serial.log'
 while 'USB releases on disconnect: 1' not in serial.read_text(errors='replace'):
  if time.monotonic()>deadline:raise AssertionError('Held mouse button not released')
  time.sleep(.1)
 launch(g,3);type_text(g,'hardware\n')
 g.wait(lambda p:any(matches(p,66,88+i*18,'CHOS firmware/hardware snapshot v1',0xd3e7fa) for i in range(16)),'remaining USB keyboard operates after mouse disconnect')
 g.snapshot('usb-mouse-disconnect');results.append({'passed':True,'case':'mouse unplug releases held button; keyboard/native app remains operational','command':g.command,'iso_sha256':hashlib.sha256(g.iso.read_bytes()).hexdigest()})
finally:g.close()


# A real preserved M7C loader must reject the new private boot-info contract safely.
root=BUILD.parent;work=Path(tempfile.mkdtemp(prefix='mixed-loader-',dir=BUILD))
for folder in ['scripts','tests','assets/fonts','build']:(work/folder).mkdir(parents=True,exist_ok=True)
for name in ['scripts/mkiso.py','tests/boot.py','LICENSE','assets/fonts/OFL.txt']:shutil.copyfile(root/name,work/name)
subprocess.run(['xorriso','-osirrox','on','-indev',str(root/'artifacts/milestone-7c/CHOS-ARM64-M7C.iso'),'-extract','/boot/efi.img',str(work/'old-efi.img')],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True)
subprocess.run(['mcopy','-i',str(work/'old-efi.img'),'::/EFI/BOOT/BOOTAA64.EFI',str(work/'build/BOOTAA64.EFI')],check=True)
shutil.copyfile(BUILD/'KERNEL.BIN',work/'build/KERNEL.BIN')
subprocess.run([sys.executable,'scripts/mkiso.py'],cwd=work,check=True)
subprocess.run([sys.executable,'tests/boot.py','--label','mixed-loader','--expect-contract-reject'],cwd=work,check=True)
for p in (work/'build').glob('mixed-loader.*'):
 if p.suffix in ['.png','.json','.log']:shutil.copyfile(p,BUILD/p.name)
mixed=json.loads((work/'build/mixed-loader.json').read_text());assert mixed['passed']
results.append({'passed':True,'case':'preserved M7C loader plus current kernel rejects boot-info version before extended-field access','result':mixed})
(BUILD/'hardening.json').write_text(json.dumps({'passed':True,'cases':results},indent=2)+'\n')

print(json.dumps(results,indent=2))
