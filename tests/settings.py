#!/usr/bin/env python3
"""Native saved configuration, failed-save no-write, cold boot, legacy and corruption.
Every writable disk is a fresh copy of this run's native installation result.
"""
import hashlib,json,shutil,struct,tempfile,time,zlib
from pathlib import Path
from desktop import Guest
from applications import launch,type_text,matches,replace_prompt
from boot import BUILD

def digest(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def inspect(path):
    with path.open('rb') as f:
        f.seek(512);header=f.read(512);table=struct.unpack_from('<Q',header,72)[0]
        f.seek(table*512+128+32);first=struct.unpack('<Q',f.read(8))[0]*512
        roots=[]
        for i in [0,1]:
            f.seek(first+i*4096);raw=bytearray(f.read(4096));crc=struct.unpack_from('<I',raw,12)[0];raw[12:16]=bytes(4)
            if raw[:8]==b'COSCHFS1' and zlib.crc32(raw)==crc:roots.append((struct.unpack_from('<Q',raw,16)[0],i,raw))
        generation,index,raw=max(roots);files={};records={}
        for n in range(40):
            at=160+n*80;name=raw[at:at+60].split(b'\0',1)[0].decode();block,blocks,size,kind,crc=struct.unpack_from('<IIIII',raw,at+60)
            if kind==1:
                f.seek(first+block*4096);value=f.read(size);assert len(value)==size and zlib.crc32(value)==crc
                files[name]=value;records[name]=(at,block)
    return first,index,raw,files,records
def config(path):
    data=bytearray(inspect(path)[3]['/system/config']);assert len(data)==80 and data[:8]==b'CHOSCFG1'
    crc=struct.unpack_from('<I',data,12)[0];data[12:16]=bytes(4);assert zlib.crc32(data)==crc
    return struct.unpack_from('<IIiI',data,16)+(data[32:64].split(b'\0',1)[0].decode(),)
def shutdown(g):
    launch(g,3);type_text(g,'shutdown\n');type_text(g,'YES\n')
    deadline=time.monotonic()+12
    while g.proc.poll() is None and time.monotonic()<deadline:time.sleep(.1)
    assert g.proc.poll()==0 and g.power_reason()=='guest-shutdown'

install=json.loads((BUILD/'native-install-512.json').read_text());source=Path(install['disk'])
run=Path(tempfile.mkdtemp(prefix='settings-',dir=BUILD));disk=run/'settings.img';shutil.copyfile(source,disk)
result={'passed':False,'iso_sha256':install['iso_sha256'],'boots':[]}
g=Guest('settings-write',disk=disk)
try:
    g.wait(lambda p:p(60,250)==0x326bb0,'installed About',120);launch(g,4)
    g.wait(lambda p:matches(p,68,94,'CHOS Control Panel',0x204b72,0),'native Control Panel')
    # Invalid computer name must never reach the disk transaction.
    g.click(410,134);g.wait(lambda p:matches(p,68,166,'Computer name:',0x204b72,0),'System tab')
    g.click(280,174);g.key('u',('ctrl',));type_text(g,'BAD/NAME');before=digest(disk);g.key('s',('ctrl',))
    g.wait(lambda p:matches(p,64,484,'Invalid name',0x163b65,3),'invalid configuration feedback')
    assert digest(disk)==before
    g.key('u',('ctrl',));type_text(g,'CHOS-SAVED')
    g.click(240,134);g.wait(lambda p:matches(p,68,170,'Offset from firmware',0x204b72,0),'Clock tab')
    g.click(370,180);g.key('u',('ctrl',));type_text(g,'330')
    g.click(80,230) # clock seconds was enabled during Setup; disable it
    g.key('s',('ctrl',));g.wait(lambda p:matches(p,64,484,'Settings saved',0x163b65,3),'persistent save')
    g.snapshot('settings-saved');shutdown(g);result['boots'].append({'kind':'native_invalid_save_and_valid_save_shutdown','command':g.command,'passed':True})
finally:g.close()
assert config(disk)==(1,1,330,0,'CHOS-SAVED')
corrupt=run/'bad-config.img';shutil.copyfile(disk,corrupt)
g=Guest('settings-read',disk=disk)
try:
    g.wait(lambda p:p(60,250)==0x326bb0,'saved settings cold boot',120)
    g.wait(lambda p:matches(p,44,294,'CHOS-SAVED',0x204b72,0),'canonical saved computer name')
    w,h=g.dimensions();assert not matches(g.capture(),w-230,34,'CHOS Royale',0xc4e8fa,2),'Installed desktop still has permanent overlay'
    g.snapshot('settings-cold-boot');g.click(618,36);g.wait(lambda p:p(60,250)!=0x326bb0,'About closed for clean installed capture');g.snapshot('installed-clean-desktop');launch(g,6)
    g.wait(lambda p:p(60,250)==0x326bb0,'native About relaunch')
    launch(g,2);g.key('o',('ctrl',));g.key('ret');g.wait(lambda p:matches(p,66,140,'INSTALLED CHOS 123',0x152b40),'user data alongside saved configuration')
    # Compatibility case: explicitly remove the new record on this disposable
    # disk. The next boot must read M6B/M6C's legacy computer-name file.
    launch(g,3);type_text(g,'del /system/config\n');type_text(g,'shutdown\n');type_text(g,'YES\n')
    deadline=time.monotonic()+12
    while g.proc.poll() is None and time.monotonic()<deadline:time.sleep(.1)
    assert g.proc.poll()==0 and g.power_reason()=='guest-shutdown'
    result['boots'].append({'kind':'cold_boot_config_and_user_data_then_explicit_legacy_case','command':g.command,'passed':True})
finally:g.close()
assert '/system/config' not in inspect(disk)[3]
before=digest(disk);g=Guest('settings-legacy',disk=disk)
try:
    g.wait(lambda p:p(60,250)==0x326bb0,'legacy configuration boot',120)
    g.wait(lambda p:matches(p,44,294,'CHOSTEST',0x204b72,0),'legacy computer name')
    g.snapshot('settings-legacy');result['boots'].append({'kind':'legacy_read_without_migration_write','command':g.command,'passed':True})
finally:g.close()
assert digest(disk)==before
# Damage only the internal configuration checksum while keeping CHFS file/root
# CRCs valid, distinguishing bad configuration from filesystem corruption.
first,index,raw,files,records=inspect(corrupt);at,block=records['/system/config'];wire=bytearray(files['/system/config']);wire[16]^=2
struct.pack_into('<I',raw,at+76,zlib.crc32(wire));raw[12:16]=bytes(4);struct.pack_into('<I',raw,12,zlib.crc32(raw))
with corrupt.open('r+b') as f:f.seek(first+block*4096);f.write(wire);f.seek(first+index*4096);f.write(raw)
before=digest(corrupt);g=Guest('settings-corrupt',disk=corrupt)
try:
    g.wait(lambda p:p(60,250)==0x326bb0,'bad configuration temporary session',120)
    g.wait(lambda p:matches(p,40,332,'System configuration invalid',0x163b65,3),'specific configuration validation error')
    g.snapshot('settings-invalid-record');result['boots'].append({'kind':'bad_config_crc_safe_fallback','command':g.command,'passed':True})
finally:g.close()
assert digest(corrupt)==before
result.update(passed=True,saved_configuration=[1,1,330,0,'CHOS-SAVED'],invalid_save_unchanged=True,legacy_boot_unchanged=True,corrupt_boot_unchanged=True)
(BUILD/'settings.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
