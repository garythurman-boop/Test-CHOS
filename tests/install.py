#!/usr/bin/env python3
"""A real native graphical install, two installed cold boots and persistent data.
Only a freshly created disposable image is attached as the destination.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import time
import zlib
from desktop import Guest
from applications import launch,type_text,replace_prompt,matches
from boot import BUILD

def digest(path):
    with path.open('rb') as file:return hashlib.file_digest(file,'sha256').hexdigest()
parser=argparse.ArgumentParser();parser.add_argument('--sector',type=int,choices=[512,4096],default=512);sector=parser.parse_args().sector
run=Path(tempfile.mkdtemp(prefix=f'native-install-{sector}-',dir=BUILD));disk=run/'target.img'
with disk.open('wb') as file:file.truncate((128 if sector==512 else 512)*1024*1024)
result={'passed':False,'disk':str(disk),'iso_sha256':digest(BUILD/'CHOS-ARM64.iso'),'boots':[],'logical_sector_bytes':sector}
before=digest(disk);guest=Guest(f'native-install-{sector}',disk=disk,optical=True,sector_bytes=sector)
try:
    guest.wait(lambda p:p(60,250)==0x326bb0,'live ISO desktop',120)
    launch(guest,7);guest.wait(lambda p:matches(p,68,94,'Welcome to CHOS Setup',0x204b72,0),'native setup welcome')
    guest.key('ret');guest.key('ret')
    guest.wait(lambda p:matches(p,68,276,'GPT',0x204b72,0),'optical-media destination eligibility')
    guest.snapshot(f'setup-destination-{sector}')
    assert digest(disk)==before,'Disk changed before selection/confirmation'
    guest.key('ret');replace_prompt(guest,'CHOSTEST')
    guest.key('ret');type_text(guest,'WRONG\n');time.sleep(.3)
    assert digest(disk)==before,'Rejected typed confirmation modified disk'
    guest.key('ret');type_text(guest,'ERASE CHOS-GPT-BOOT\n')
    guest.wait(lambda p:matches(p,68,142,'CHOS',0x204b72,0),'native installation completion',90)
    guest.snapshot(f'setup-complete-{sector}');result['boots'].append({'kind':'native_installer_from_iso','command':guest.command,'passed':True,'confirmation_no_write_checks':True})
finally:guest.close()
# The ISO is entirely absent from the next two VMs. This is not the host builder.
guest=Guest(f'installed-write-{sector}',disk=disk,sector_bytes=sector)
try:
    guest.wait(lambda p:p(60,250)==0x326bb0,'installed boot with no ISO',120)
    guest.wait(lambda p:matches(p,44,294,'CHOSTEST',0x204b72,0),'persisted computer configuration')
    guest.snapshot(f'installed-desktop-{sector}')
    launch(guest,2);guest.wait(lambda p:p(60,180)==0xffffff,'installed native Notepad')
    type_text(guest,'INSTALLED CHOS 123');guest.key('s',('ctrl',));guest.key('ret');time.sleep(.5)
    guest.snapshot(f'installed-notepad-save-{sector}');result['boots'].append({'kind':'installed_boot_and_native_file_save','command':guest.command,'passed':True})
finally:guest.close()
guest=Guest(f'installed-read-{sector}',disk=disk,sector_bytes=sector)
try:
    guest.wait(lambda p:p(60,250)==0x326bb0,'installed cold reboot',120)
    launch(guest,2);guest.key('o',('ctrl',));guest.key('ret')
    guest.wait(lambda p:matches(p,66,140,'INSTALLED CHOS 123',0x152b40),'native editor persisted data after cold boot')
    guest.snapshot(f'installed-notepad-reopen-{sector}')
    saved=digest(disk);launch(guest,7);guest.key('ret');guest.key('ret')
    guest.wait(lambda p:matches(p,92,298,'Installation disabled',0x204b72,0),'refusal to reinstall over own boot disk')
    guest.key('ret');type_text(guest,'ERASE CHOS-GPT-BOOT\n');time.sleep(.3)
    assert digest(disk)==saved,'Boot disk exclusion failed'
    result['boots'].append({'kind':'installed_cold_boot_persistent_read_and_self_erase_refusal','command':guest.command,'passed':True})
finally:guest.close()
# Independent wire parser: verify actual stored configuration/file and data CRCs.
with disk.open('rb') as file:
    file.seek(sector);header=file.read(sector);table=struct.unpack_from('<Q',header,72)[0]
    file.seek(table*sector+128);entry=file.read(128);system_start=struct.unpack_from('<Q',entry,32)[0]*sector
    roots=[]
    for i in [0,1]:
        file.seek(system_start+i*4096);raw=bytearray(file.read(4096));checksum=struct.unpack_from('<I',raw,12)[0];raw[12:16]=bytes(4)
        if raw[:8]==b'COSCHFS1' and zlib.crc32(raw)==checksum:roots.append((struct.unpack_from('<Q',raw,16)[0],raw))
    assert roots;generation,raw=max(roots,key=lambda x:x[0]);contents={}
    for i in range(40):
        at=160+i*80;name=raw[at:at+60].split(b'\0',1)[0].decode();first,blocks,size,kind,crc=struct.unpack_from('<IIIII',raw,at+60)
        if kind==1:
            file.seek(system_start+first*4096);data=file.read(size);assert len(data)==size and zlib.crc32(data)==crc;contents[name]=data
    assert contents['/system/computer']==b'CHOSTEST'
    assert contents['/system/license']==(BUILD.parent/'LICENSE').read_bytes()
    assert contents['/system/font-ofl']==(BUILD.parent/'assets/fonts/OFL.txt').read_bytes()
    assert contents['/documents/note.txt']==b'INSTALLED CHOS 123'
result.update(passed=True,filesystem_generation=generation,independent_disk_parser=True,disk_sha256=digest(disk),limitations=['accounts/password setup absent','timezone/keymap/display configuration incomplete','VMware Fusion unverified'])
(BUILD/f'native-install-{sector}.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
