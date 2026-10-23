#!/usr/bin/env python3
"""Controlled independent large file/BSS build and real UEFI/refusal/install boots."""
import hashlib,json,os,shutil,struct,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BUILD=ROOT/'build'
work=Path(tempfile.mkdtemp(prefix='growth-',dir=BUILD));tree=work/'source'
shutil.copytree(ROOT,tree,ignore=shutil.ignore_patterns('build','.git','artifacts','evidence','__pycache__'))
def run(*args,env=None):subprocess.run(list(map(str,args)),cwd=tree,env=env,check=True)
with (BUILD/'growth-build.log').open('w') as log:
 subprocess.run(['make','-j4','iso','KERNEL_TEST_RO=1048576','KERNEL_TEST_BSS=3145728'],cwd=tree,stdout=log,stderr=subprocess.STDOUT,check=True)
b=tree/'build';payload=(b/'KERNEL.BIN').read_bytes();header=struct.unpack('<IIQIIQQQQQ',payload[:64]);file_size,memory=header[5:7]
assert file_size==len(payload) and file_size>1048576 and memory>4*1048576
names=subprocess.check_output(['llvm-nm','--defined-only',str(b/'kernel.elf')],text=True)
probe_offsets={name:int(line.split()[0],16) for name in ['growth_ro_probe','growth_bss_probe'] for line in names.splitlines() if line.split()[-1]==name}
assert probe_offsets['growth_ro_probe']>512*1024 and probe_offsets['growth_bss_probe']>4*1048576
run(sys.executable,'tests/boot.py','--label','growth-el1')
run(sys.executable,'tests/boot.py','--label','growth-el2','--gic-version','3','--virtualization')
serial=(b/'growth-el2.serial.log').read_text(errors='replace')
for expected in ['Controlled file growth bytes: 1048576','Controlled BSS growth bytes: 3145728','Kernel image contract and growth checks: PASS']:
 assert expected in serial,expected
run(sys.executable,'tests/install.py','--sector','512','--gic-version','3','--virtualization')
installed=json.loads((b/'native-install-512-gic3-el2.json').read_text());assert len(installed['boots'])==3 and all(x['passed'] for x in installed['boots'])
# Invalid headers never reach the kernel. Only this disposable build copy changes.
negative=[]
for name,offset,value in [('memory-limit',32,64*1048576+4096),('file-mismatch',24,len(payload)+1),('bad-entry',40,0xffffffffffffffff)]:
 bad=bytearray(payload);struct.pack_into('<Q',bad,offset,value);(b/'KERNEL.BIN').write_bytes(bad)
 run(sys.executable,'scripts/mkiso.py');run(sys.executable,'tests/boot.py','--label','growth-refuse-'+name,'--expect-invalid-kernel')
 result=json.loads((b/('growth-refuse-'+name+'.json')).read_text());assert result['passed'];negative.append(result)
(b/'KERNEL.BIN').write_bytes(payload)
result={'passed':True,'scope':'QEMU only; original header allocation, full large file/BSS validation, both stack guards, RO/NX, installer and two disk-only boots','file_bytes':file_size,'memory_bytes':memory,'ro_growth_bytes':1048576,'bss_growth_bytes':3145728,'normal_iso_sha256':hashlib.sha256((BUILD/'CHOS-ARM64.iso').read_bytes()).hexdigest(),'large_iso_sha256':json.loads((b/'growth-el2.json').read_text())['iso_sha256'],'positive_boots':2,'far_protection_probe_offsets':probe_offsets,'installed_boots':installed['boots'],'expected_refusals':negative}
# Export evidence, not the deliberately malformed images or large disposable disk.
for p in b.glob('*'):
 if p.is_file() and (p.name.startswith(('growth-','native-install-512-gic3-el2','installed-read-512-gic3-el2','installed-write-512-gic3-el2','setup-progress-512-gic3-el2','setup-complete-512-gic3-el2','installed-notepad-reopen-512-gic3-el2')) and p.suffix in ['.json','.log','.png']):shutil.copyfile(p,BUILD/('large-'+p.name))
(BUILD/'growth.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
