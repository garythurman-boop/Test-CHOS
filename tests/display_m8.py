#!/usr/bin/env python3
"""Persisted native resolution trial, UEFI consumption, confirmation and fallback."""
import hashlib,json,re,shutil,struct,tempfile,zlib
from pathlib import Path
from desktop import Guest
from applications import launch,matches
from boot import BUILD
run=Path(tempfile.mkdtemp(prefix='display-m8-',dir=BUILD));result={'passed':False,'boots':[]}
source=Path(json.loads((BUILD/'native-install-512.json').read_text())['disk']);disk=run/'display.img';shutil.copyfile(source,disk)
def preference(path):
 with path.open('rb') as f:
  f.seek(512);h=f.read(512);f.seek(struct.unpack_from('<Q',h,72)[0]*512);e=f.read(128);base=struct.unpack_from('<Q',e,32)[0]*512
  f.seek(base);b=f.read(512);sector=struct.unpack_from('<H',b,11)[0];data=base+(struct.unpack_from('<H',b,14)[0]+2*struct.unpack_from('<I',b,36)[0])*sector
  f.seek(data+3*sector+96);e=f.read(32);assert e[:11]==b'DISPLAY BIN';cluster=struct.unpack_from('<H',e,26)[0]|struct.unpack_from('<H',e,20)[0]<<16
  address=data+(cluster-2)*sector;f.seek(address);wire=f.read(32)
  assert wire[:8]==b'CHOSDSP1' and zlib.crc32(wire[:28])==struct.unpack_from('<I',wire,28)[0]
  return address,struct.unpack_from('<III',wire,8)
def check(g,kind):result['boots'].append({'kind':kind,'passed':True,'command':g.command,'actual_mode':g.dimensions()})
def properties(g):
 launch(g,4);g.click(540,134)
 g.wait(lambda p:matches(p,68,162,'Display Properties',0x204b72,0),'native Display Properties')
def apply_800(g):
 text=(BUILD/(g.label+'.serial.log')).read_text(errors='replace')
 modes=[tuple(map(int,pair)) for pair in re.findall(r'GOP mode width: (\d+)\s+GOP mode height: (\d+)',text)]
 assert (800,600) in modes,modes
 for _ in range(modes.index((800,600))):g.key('down')
 g.click(140,440);g.wait(lambda p:matches(p,64,484,'Saved trial mode.',0x163b65,3),'trial preference committed')
 g.snapshot('m8-display-apply');assert preference(disk)[1]==(800,600,1)
g=Guest('display-m8-request',disk=disk)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'installed desktop',120);properties(g);apply_800(g);check(g,'firmware_mode_selection_and_saved_trial')
finally:g.close()
# Fresh VM consumes trial before ExitBootServices; no original ISO attached.
g=Guest('display-m8-trial',disk=disk)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'trial cold boot',120);assert g.dimensions()==(800,600);assert preference(disk)[1]==(800,600,2)
 g.snapshot('m8-display-trial');check(g,'actual_800x600_trial_without_confirmation')
finally:g.close()
g=Guest('display-m8-fallback',disk=disk)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'unconfirmed fallback cold boot',120);assert g.dimensions()==(1024,768)
 g.snapshot('m8-display-fallback');properties(g);apply_800(g);check(g,'unconfirmed_trial_falls_back_to_1024x768')
finally:g.close()
g=Guest('display-m8-confirm',disk=disk)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'second trial boot',120);assert g.dimensions()==(800,600)
 properties(g);g.click(450,440);g.wait(lambda p:matches(p,64,484,'Current resolution confirmed',0x163b65,3),'native mode confirmation committed')
 assert preference(disk)[1]==(800,600,0);g.snapshot('m8-display-confirmed');check(g,'native_keep_current_resolution')
finally:g.close()
g=Guest('display-m8-retained',disk=disk)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'confirmed cold boot',120);assert g.dimensions()==(800,600);check(g,'confirmed_mode_survives_cold_boot')
finally:g.close()
# Invalid preference checksum must retain safe boot, never call runtime GOP.
address,_=preference(disk)
with disk.open('r+b') as f:f.seek(address+28);b=f.read(1);f.seek(address+28);f.write(bytes([b[0]^1]))
g=Guest('display-m8-corrupt',disk=disk)
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'corrupt preference safe boot',120);assert g.dimensions()==(1024,768);check(g,'corrupt_preference_falls_back')
finally:g.close()
result.update(passed=True,iso_sha256=hashlib.sha256((BUILD/'CHOS-ARM64.iso').read_bytes()).hexdigest());(BUILD/'display-m8.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
