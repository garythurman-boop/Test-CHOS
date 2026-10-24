#!/usr/bin/env python3
"""Minimum-resolution live USB panel, using an isolated display-config ISO.
No disk is attached, no production image is overwritten, and no key history is
exported. This is QEMU rendering/input evidence, not Fusion verification.
"""
import hashlib,json,os,shutil,subprocess,tempfile,time
from pathlib import Path
from desktop import Guest
from boot import BUILD
root=BUILD.parent
work=Path(tempfile.mkdtemp(prefix='input-panel-',dir=BUILD))
for name in ['scripts/mkiso.py','build/BOOTAA64.EFI','build/KERNEL.BIN','LICENSE','assets/fonts/OFL.txt']:
    target=work/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(root/name,target)
env={**os.environ,'CHOS_INPUT_TEST':'1','CHOS_DISPLAY_MODE':'640x480','SOURCE_DATE_EPOCH':os.environ.get('SOURCE_DATE_EPOCH','1767225600')}
with (BUILD/'input-panel-build.log').open('w') as log:
    subprocess.run(['python3','scripts/mkiso.py'],cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
iso=work/'build/CHOS-ARM64-M7H-INPUT.iso'
g=Guest('input-panel-640',ram=2048,live=False,gic_version=3,input_backend='usb',iso=iso)
try:
    serial=BUILD/(g.label+'.serial.log');deadline=time.monotonic()+120
    while 'Diagnostic mode: no installer, no filesystem writes' not in serial.read_text(errors='replace'):
        assert g.proc.poll() is None
        if time.monotonic()>deadline:raise AssertionError('Minimum panel diagnostic boot stalled')
        time.sleep(.2)
    text=serial.read_text(errors='replace')
    for expected in ['Framebuffer width: 640','Framebuffer height: 480','USB boot keyboard initialized: PASS','USB boot mouse initialized: PASS','Input initialized devices: 2']:
        assert expected in text,expected
    g.wait(lambda p:p(0,260)==0x163a64 and p(0,456)==0x244da8,'minimum-resolution controller and event panel')
    assert g.dimensions()==(640,480),g.dimensions()
    # Last event stays volatile; assert an actual release reaches the visible UI.
    g.key('a')
    digits={'3':[30,1,1,14,1,1,30],'0':[14,17,19,21,25,17,14]}
    def digit(p,x,y,n):
        return all(p(x+c*2,y+r*2)==(0xffffff if bits&(16>>c) else 0x163a64) for r,bits in enumerate(digits[n]) for c in range(5))
    g.wait(lambda p:digit(p,304,396,'3') and digit(p,316,396,'0') and digit(p,448,396,'0'),'640x480 live key release')
    g.snapshot('input-panel-640')
    result={'passed':True,'case':g.label,'command':g.command,'dimensions':[640,480],'iso_sha256':hashlib.sha256(iso.read_bytes()).hexdigest(),'kernel_sha256':hashlib.sha256((BUILD/'KERNEL.BIN').read_bytes()).hexdigest(),'isolated_display_config_variant':True,'controller_keyboard_mouse_initialized':True,'live_key_release_visible':True,'no_disk':True}
finally:g.close()
(BUILD/'input-panel-640.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
