#!/usr/bin/env python3
"""Failed graphical commit stays pending; corrupt continuation never erases disk."""
import hashlib,json,shutil,struct,tempfile,zlib
from pathlib import Path
from desktop import Guest
from applications import matches
from boot import BUILD
run=Path(tempfile.mkdtemp(prefix='setup-recovery-m8-',dir=BUILD));source=json.loads((BUILD/'native-install-512.json').read_text());result={'passed':False,'cases':[]}
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def stage_record(path):
 with path.open('rb') as f:
  f.seek(512);h=f.read(512);f.seek(struct.unpack_from('<Q',h,72)[0]*512+128);e=f.read(128);base=struct.unpack_from('<Q',e,32)[0]*512
  roots=[]
  for index in [0,1]:
   f.seek(base+index*4096);raw=bytearray(f.read(4096));crc=struct.unpack_from('<I',raw,12)[0];raw[12:16]=bytes(4)
   if raw[:8]==b'COSCHFS1' and zlib.crc32(raw)==crc:roots.append((struct.unpack_from('<Q',raw,16)[0],index,raw))
  generation,index,raw=max(roots)
  for n in range(40):
   at=160+n*80
   if raw[at:at+60].split(b'\0',1)[0]==b'/system/setup-stage':
    first,blocks,size,kind,crc=struct.unpack_from('<IIIII',raw,at+60);f.seek(base+first*4096);data=f.read(size);assert zlib.crc32(data)==crc
    return base,index,raw,at,first,data
 raise AssertionError('No continuation record')
disk=run/'failed-commit.img';shutil.copyfile(source['pending_disk'],disk);before=digest(disk)
fault=run/'fault.conf';fault.write_text('[inject-error]\nevent = "write_aio"\nerrno = "5"\nonce = "on"\nimmediately = "on"\n')
g=Guest('setup-stage-two-write-failure',disk=disk,fault_config=fault)
try:
 g.wait(lambda p:matches(p,256,100,'Personalize CHOS',0xf2f6ff,2),'pending setup',120)
 g.click(920,703);g.move(600,550)
 g.wait(lambda p:matches(p,256,658,'Configuration commit failed.',0xffdea2,0),'failed configuration save stays pending')
 g.snapshot('m8-stage-two-write-failure');result['cases'].append({'kind':'actual_configuration_write_failure','passed':True,'command':g.command})
finally:g.close()
assert digest(disk)==before and stage_record(disk)[-1]==b'CHOS SETUP PENDING 2\n'
g=Guest('setup-stage-two-write-retry',disk=disk)
try:
 g.wait(lambda p:matches(p,256,100,'Personalize CHOS',0xf2f6ff,2),'cold boot still pending',120);g.snapshot('m8-stage-two-retry');result['cases'].append({'kind':'failed_commit_cold_boot_continuation','passed':True,'command':g.command})
finally:g.close()
# An absent marker must also refuse persistent completion, with an otherwise
# structurally valid CHFS root (including its allocation bitmap and CRC).
missing=run/'missing-stage.img';shutil.copyfile(disk,missing)
base,index,raw,at,first,data=stage_record(missing)
blocks=struct.unpack_from('<I',raw,at+64)[0]
for block in range(first,first+blocks):raw[32+block//8]&=~(1<<(block%8))
raw[at:at+80]=bytes(80);struct.pack_into('<I',raw,12,zlib.crc32(raw))
with missing.open('r+b') as f:f.seek(base+index*4096);f.write(raw)
before_missing=digest(missing);g=Guest('setup-missing-continuation',disk=missing)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'temporary desktop after absent stage',120)
 g.wait(lambda p:matches(p,40,332,'System configuration invalid',0x163b65,3),'missing continuation recovery status')
 assert all(b['stats']['wr_operations']==0 for b in g.qmp('query-blockstats'))
 g.snapshot('m8-stage-missing-recovery');result['cases'].append({'kind':'missing_stage_no_automatic_writes','passed':True,'command':g.command})
finally:g.close()
assert digest(missing)==before_missing
# Keep filesystem CRCs correct while deliberately corrupting stage semantics.
base,index,raw,at,first,data=stage_record(disk);bad=b'X'+data[1:];struct.pack_into('<I',raw,at+76,zlib.crc32(bad));struct.pack_into('<I',raw,12,zlib.crc32(raw))
with disk.open('r+b') as f:f.seek(base+first*4096);f.write(bad);f.seek(base+index*4096);f.write(raw)
before=digest(disk);g=Guest('setup-corrupt-continuation',disk=disk)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'temporary desktop after invalid stage',120)
 g.wait(lambda p:matches(p,40,332,'System configuration invalid',0x163b65,3),'precise recovery status')
 assert all(b['stats']['wr_operations']==0 for b in g.qmp('query-blockstats'))
 g.snapshot('m8-stage-invalid-recovery');result['cases'].append({'kind':'invalid_stage_no_automatic_writes','passed':True,'command':g.command})
finally:g.close()
assert digest(disk)==before
result.update(passed=True,failed_commit_disk_unchanged=True,invalid_stage_disk_unchanged=True);(BUILD/'setup-recovery-m8.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
