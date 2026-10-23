#!/usr/bin/env python3
"""Noninteractive inventory; no input device and no installer disk writes."""
import hashlib,json,tempfile,time
from pathlib import Path
from desktop import Guest
from boot import BUILD
run=Path(tempfile.mkdtemp(prefix='diagnostic-',dir=BUILD));disk=run/'untouched.img'
with disk.open('wb') as f:f.truncate(128*1048576)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before=sha(disk)
g=Guest('diagnostic-no-input',disk=disk,optical=True,live=False,gic_version=3,virtualization=True,iso=BUILD/'CHOS-ARM64-M7C.iso',input_backend='none',extra_devices=['qemu-xhci','ich9-ahci'])
result={'passed':False,'command':g.command,'iso_sha256':sha(g.iso)}
try:
    serial=BUILD/'diagnostic-no-input.serial.log'
    deadline=time.monotonic()+120
    while 'Diagnostic mode: no installer, no filesystem writes' not in serial.read_text(errors='replace'):
        assert g.proc.poll() is None
        if time.monotonic()>deadline:raise AssertionError('Diagnostic report did not complete')
        time.sleep(.2)
    text=serial.read_text(errors='replace')
    for expected in ['Firmware revision:','UEFI revision:','Firmware entry EL: 2','Current kernel EL: 1','ACPI XSDT/checksums: PASS','GIC discovered version: 3','Timer initialized PPI: 30','GIC delivery and preemption: PASS','Input initialized devices: 0','Input unsupported/absent','USB xHCI','Storage unsupported','NVMe initialized: PASS']:
        assert expected in text,expected
    # Read the actual page digit from the original kernel bitmap font.
    digits={1:[4,12,4,4,4,4,14],2:[14,17,1,2,4,8,31]}
    def page(pixel,number):
        width,_=g.dimensions();x=width-64
        return all(pixel(x+c*2,16+r*2)==(0xffffff if bits&(16>>c) else 0x102b50) for r,bits in enumerate(digits[number]) for c in range(5))
    for i,number in enumerate([1,2,1]):
        g.wait(lambda p:page(p,number),'automatic diagnostic page '+str(number),45)
        g.snapshot('diagnostic-page-'+str(i+1))

    assert 'Welcome to CHOS Setup' not in text
    result.update(passed=True,no_input_devices=True,automatic_inventory=True,automatic_page_rotation_verified=True,unsupported_storage_detected=True,usb_controller_detected=True,native_spcr_serial=True)
finally:g.close()
assert sha(disk)==before
result['unconfirmed_disk_unchanged']=True
(BUILD/'diagnostic-no-input.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
