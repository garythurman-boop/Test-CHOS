#!/usr/bin/env python3
"""Real UEFI larger-memory boots and an isolated bounded-capacity failure image.
These are controlled QEMU tests, not an imported Fusion memory map/retest.
"""
import hashlib,json,shutil,subprocess,sys,tempfile,time
from pathlib import Path
from desktop import Guest
from boot import BUILD
root=BUILD.parent
for ram,extra,label in [(2048,[],'mmu-2048-el1'),(2048,['--gic-version','3','--virtualization'],'mmu-2048-el2'),(8192,['--gic-version','3','--virtualization'],'mmu-8192-el2')]:
 subprocess.run([sys.executable,'tests/boot.py','--ram',str(ram),'--label',label,*extra],check=True)
# No keyboard, mouse, disk, network or functional desktop is needed for the report.
g=Guest('mmu-diagnostic-2048',ram=2048,live=False,input_backend='none',iso=BUILD/'CHOS-ARM64-M7C.iso',gic_version=3,virtualization=True)
try:
 serial=BUILD/'mmu-diagnostic-2048.serial.log';deadline=time.monotonic()+120
 while 'Diagnostic mode: no installer, no filesystem writes' not in serial.read_text(errors='replace'):
  assert g.proc.poll() is None
  if time.monotonic()>deadline:raise AssertionError('2 GiB diagnostics stalled')
  time.sleep(.2)
 text=serial.read_text(errors='replace')
 assert 'Kernel MMU: PASS' in text and 'MMU table pages:' in text and 'Input initialized devices: 0' in text
 g.wait(lambda p:p(0,g.dimensions()[1]-24)==0x244da8,'complete no-input inventory page',45)
 g.snapshot('mmu-diagnostic-2048')
 diagnostic={'passed':True,'command':g.command,'iso_sha256':hashlib.sha256(g.iso.read_bytes()).hexdigest(),'no_disk_or_input':True}
finally:g.close()
# Deliberately smaller resource policy; never distribute this negative-test ISO.
work=Path(tempfile.mkdtemp(prefix='mmu-capacity-',dir=BUILD));tree=work/'source'
shutil.copytree(root,tree,ignore=shutil.ignore_patterns('build','.git','artifacts','evidence','__pycache__'))
with (BUILD/'mmu-capacity-build.log').open('w') as log:
 subprocess.run(['make','-j4','diagnostic-iso','KERNEL_TABLE_CAPACITY=8'],cwd=tree,stdout=log,stderr=subprocess.STDOUT,check=True)
iso=tree/'build/CHOS-ARM64-M7C.iso'
g=Guest('mmu-capacity-failure',ram=2048,live=False,input_backend='none',iso=iso,gic_version=3,virtualization=True)
# Independent 5x7 diagnostic numeral shapes.
digits={'2':[14,17,1,2,4,8,31],'4':[2,6,10,18,31,2,2],'8':[14,17,17,14,17,17,14]}
def digit(p,x,y,n):return all(p(x+c*2,y+r*2)==(0xffffff if bits&(16>>c) else 0x300808) for r,bits in enumerate(digits[n]) for c in range(5))
try:
 g.wait(lambda p:digit(p,176,132,'4') and digit(p,176,160,'2') and digit(p,272,336,'8') and digit(p,416,336,'8') and p(34,656)==0x9ed7ff,'retained RAM/table-capacity failure report',120)
 g.snapshot('mmu-capacity-failure')
 failure={'passed':True,'command':g.command,'iso_sha256':hashlib.sha256(iso.read_bytes()).hexdigest(),'stage':4,'reason':2,'table_pages':8,'capacity':8,'framebuffer_verified_without_serial':True,'no_disk_or_input':True}
finally:g.close()
result={'passed':True,'scope':'Controlled QEMU layouts, not actual Fusion memory-map reproduction','functional_memory_mib':[2048,2048,8192],'diagnostic':diagnostic,'capacity_failure':failure}
(BUILD/'mmu-boots.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
