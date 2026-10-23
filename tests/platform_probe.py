#!/usr/bin/env python3
"""Native hardware query and independently observed PCI/vCPU diagnostics.
QEMU evidence only; the same firmware/native output is intended for Fusion capture.
"""
import hashlib,json,os,tempfile,time
from pathlib import Path
from desktop import Guest
from applications import launch,type_text,matches
from boot import BUILD
run=Path(tempfile.mkdtemp(prefix='hardware-probe-',dir=BUILD));disk=run/'unconfirmed.img'
with disk.open('wb') as f:f.truncate(128*1048576)
def digest():
    with disk.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
before=digest();g=Guest('hardware-probe',disk=disk,optical=True,gic_version=3,virtualization=True)
result={'passed':False,'command':g.command,'iso_sha256':hashlib.sha256(g.iso.read_bytes()).hexdigest()}
try:
    g.wait(lambda p:p(60,250)==0x326bb0,'EL2/GICv3 Live Desktop after explicit Setup choice',120)
    g.click(618,36);launch(g,3);type_text(g,'hardware\n')
    def text_visible(p,text):return any(matches(p,42,66+i*18,text,0xd3e7fa) for i in range(16))
    g.wait(lambda p:text_visible(p,'Firmware EL: 2   Kernel EL: 1'),'native query reports real EL2-to-EL1 transition')
    g.wait(lambda p:text_visible(p,'GIC version: 3   Timer IRQ: 30'),'native GIC/timer query')
    g.wait(lambda p:text_visible(p,'Input devices: 2   NVMe: 1'),'real initialized input/storage inventory')
    g.snapshot('hardware-probe')
    pci=g.qmp('human-monitor-command',{'command-line':'info pci'});(BUILD/'hardware-probe.pci.log').write_text(pci)
    assert '1af4:1052' in pci and '1b36:0010' in pci,'Unexpected test hardware: inspect independent QMP PCI dump'
    serial=(BUILD/'hardware-probe.serial.log').read_text(errors='replace')
    assert 'CHOS: firmware entry EL2' in serial and 'ID 1b36:0010 CLASS 010802' in serial
    # Host thread consumption of the emulated vCPU, not guest idle accounting.
    cpu=g.qmp('query-cpus-fast');assert len(cpu)==1;tid=cpu[0]['thread-id'];hz=os.sysconf('SC_CLK_TCK')
    def ticks():
        parts=Path(f'/proc/{tid}/stat').read_text().rsplit(') ',1)[1].split();return int(parts[11])+int(parts[12])
    t0=time.monotonic();c0=ticks();time.sleep(10);elapsed=time.monotonic()-t0;used=(ticks()-c0)/hz
    result.update(passed=True,firmware_entry_el=2,kernel_el=1,gic_version=3,timer_irq=30,initialized_input_devices=2,nvme_present=True,pci_inventory_verified_against_qmp=True,
                  host_vcpu_idle_observation={'seconds':round(elapsed,3),'host_cpu_seconds':used,'percent_one_host_core':round(100*used/elapsed,3),'scope':'QEMU TCG vCPU thread while native Terminal and desktop are idle; not native Fusion guest CPU utilization'})
finally:g.close()
assert digest()==before,'Read-only hardware diagnostics changed an unconfirmed disk'
result['unconfirmed_disk_unchanged']=True
(BUILD/'hardware-probe.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
