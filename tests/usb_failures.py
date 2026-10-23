#!/usr/bin/env python3
"""Unsupported USB/controller cases must finish noninteractive diagnostics."""
import hashlib,json,time
from desktop import Guest
from boot import BUILD
cases=[('usb-controller-refusal',['qemu-xhci,p2=0,p3=0'],'xHCI failure stage:'),('usb-nonboot-refusal',['qemu-xhci,id=xhci','usb-tablet,bus=xhci.0'],'USB device enumeration failed/unsupported'),('usb-ohci-refusal',['pci-ohci,id=ohci','usb-kbd,bus=ohci.0'],'USB OHCI unsupported')]
results=[]
for name,devices,expected in cases:
    g=Guest(name,live=False,gic_version=3,virtualization=True,input_backend='none',iso=BUILD/'CHOS-ARM64-M7C.iso',extra_devices=devices)
    try:
        serial=BUILD/(name+'.serial.log');deadline=time.monotonic()+120
        while 'Diagnostic mode: no installer, no filesystem writes' not in serial.read_text(errors='replace'):
            assert g.proc.poll() is None
            if time.monotonic()>deadline:raise AssertionError('Diagnostic boot stalled: '+name)
            time.sleep(.2)
        text=serial.read_text(errors='replace');assert expected in text,(name,text[-2000:]);assert 'Input initialized devices: 0' in text
        g.snapshot(name)
        results.append({'case':name,'passed':True,'expected_diagnostic':expected,'command':g.command,'iso_sha256':hashlib.sha256(g.iso.read_bytes()).hexdigest()})
    finally:g.close()
(BUILD/'usb-failures.json').write_text(json.dumps({'passed':True,'cases':results},indent=2)+'\n');print(json.dumps(results,indent=2))
