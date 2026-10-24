#!/usr/bin/env python3
"""A real native graphical install, two installed cold boots and persistent data.
Only a freshly created disposable image is attached as the destination.
"""
import argparse
import hashlib
import json
import shutil
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
parser=argparse.ArgumentParser();parser.add_argument('--sector',type=int,choices=[512,4096],default=512)
parser.add_argument('--gic-version',type=int,choices=[2,3],default=2);parser.add_argument('--virtualization',action='store_true')
parser.add_argument('--bridged',action='store_true')
args=parser.parse_args();sector=args.sector
suffix=f'-gic{args.gic_version}-el2' if args.virtualization else (f'-gic{args.gic_version}' if args.gic_version!=2 else '')
machine={'gic_version':args.gic_version,'virtualization':args.virtualization}
if args.bridged:
    from bus_topology import BRIDGED
    machine.update(BRIDGED);suffix+='-bridged'
run=Path(tempfile.mkdtemp(prefix=f'native-install-{sector}-',dir=BUILD));disk=run/'target.img'
with disk.open('wb') as file:file.truncate((128 if sector==512 else 512)*1024*1024)
result={'passed':False,'disk':str(disk),'iso_sha256':digest(BUILD/'CHOS-ARM64.iso'),'boots':[],'logical_sector_bytes':sector,'gic_version':args.gic_version,'virtualization':args.virtualization}
before=digest(disk);guest=Guest(f'native-install-{sector}{suffix}',disk=disk,optical=True,sector_bytes=sector,live=False,**machine)
try:
    guest.wait(lambda p:p(30,160)==0x244da8,'dedicated full-screen Setup from ISO',120)
    guest.snapshot(f'setup-blue-{sector}{suffix}')
    guest.key('ret');guest.key('ret')
    guest.wait(lambda p:matches(p,32,112,'NVMe',0xf2f6ff),'native destination inventory')
    guest.snapshot(f'setup-destination-{sector}{suffix}')
    assert digest(disk)==before,'Disk changed before selection/confirmation'
    guest.key('ret');guest.snapshot(f'setup-layout-{sector}{suffix}');guest.key('ret')
    guest.wait(lambda p:matches(p,32,92,'Confirm destruction',0xf2f6ff),'exact disk erase confirmation')
    type_text(guest,'WRONG');guest.key('ret')
    guest.wait(lambda p:matches(p,32,703,'Confirmation refused',0xffe7a5,0),'wrong erase rejection')
    assert digest(disk)==before,'Rejected typed confirmation modified disk'
    guest.snapshot(f'setup-confirmation-{sector}{suffix}')
    guest.key('u',('ctrl',));type_text(guest,'ERASE');guest.key('ret')
    guest.wait(lambda p:matches(p,32,92,'Installing CHOS system files',0xf2f6ff),'graphical disk installation in progress',90)
    guest.snapshot(f'setup-progress-{sector}{suffix}')
    guest.wait(lambda p:matches(p,32,92,'CHOS has completed the first stage',0xf2f6ff),'verified installation completion',90)
    guest.snapshot(f'setup-complete-{sector}{suffix}')
    guest.key('r');deadline=time.monotonic()+8
    while guest.proc.poll() is None and time.monotonic()<deadline:time.sleep(.1)
    assert guest.proc.poll()==0 and guest.power_reason()=='guest-reset','Setup Restart did not produce a guest reset'
    result['boots'].append({'kind':'dedicated_native_installer_from_iso','command':guest.command,'passed':True,'confirmation_no_write_checks':True,'completion_restart_button':True})
finally:guest.close()
pending_disk=run/'pending-stage-two.img';shutil.copyfile(disk,pending_disk)
result['pending_disk']=str(pending_disk)
# Cold boot is a separate VM with no optical device; pending marker survives RAM loss.
guest=Guest(f'installed-pending-{sector}{suffix}',disk=disk,sector_bytes=sector,**machine)
try:
    guest.wait(lambda p:matches(p,256,100,'Personalize CHOS',0xf2f6ff,2),'disk-only graphical continuation',120)
    guest.snapshot(f'setup-phase-two-{sector}{suffix}')
    result['boots'].append({'kind':'pending_phase_two_interrupted_cold_boot','command':guest.command,'passed':True})
finally:guest.close()
guest=Guest(f'installed-configure-{sector}{suffix}',disk=disk,sector_bytes=sector,**machine)
try:
    guest.wait(lambda p:matches(p,256,100,'Personalize CHOS',0xf2f6ff,2),'pending graphical setup resumes',120)
    guest.key('u',('ctrl',));type_text(guest,'BAD/NAME');before_invalid=digest(disk);guest.key('ret')
    guest.wait(lambda p:matches(p,256,658,'Invalid computer name',0xffdea2,0),'invalid stage-two configuration feedback')
    assert digest(disk)==before_invalid,'Invalid phase-two configuration modified disk'
    guest.click(380,184);guest.key('u',('ctrl',));type_text(guest,'CHOSTEST')
    guest.click(462,225);guest.click(602,264);guest.click(260,308)
    guest.snapshot(f'setup-configuration-{sector}{suffix}');guest.click(920,703)
    guest.wait(lambda p:matches(p,256,100,'Welcome to CHOS',0xf2f6ff,2),'verified configuration and first run welcome')
    guest.snapshot(f'setup-welcome-{sector}{suffix}');guest.key('ret')
    guest.wait(lambda p:p(60,250)==0x326bb0,'first installed desktop')
    result['boots'].append({'kind':'graphical_configuration_verified_then_welcome_and_desktop','command':guest.command,'passed':True})
finally:guest.close()
# Subsequent boots bypass setup; existing file/settings persistence checks remain.
guest=Guest(f'installed-write-{sector}{suffix}',disk=disk,sector_bytes=sector,**machine)
try:
    guest.wait(lambda p:p(60,250)==0x326bb0,'installed boot with no ISO',120)
    guest.wait(lambda p:matches(p,44,294,'CHOSTEST',0x204b72,0),'persisted computer configuration')
    guest.snapshot(f'installed-desktop-{sector}{suffix}')
    launch(guest,2);guest.wait(lambda p:p(60,180)==0xffffff,'installed native Notepad')
    type_text(guest,'INSTALLED CHOS 123');guest.key('s',('ctrl',));guest.key('ret');time.sleep(.5)
    guest.snapshot(f'installed-notepad-save-{sector}{suffix}');result['boots'].append({'kind':'installed_boot_and_native_file_save','command':guest.command,'passed':True})
finally:guest.close()
guest=Guest(f'installed-read-{sector}{suffix}',disk=disk,sector_bytes=sector,**machine)
try:
    guest.wait(lambda p:p(60,250)==0x326bb0,'installed cold reboot',120)
    launch(guest,2);guest.key('o',('ctrl',));guest.key('ret')
    guest.wait(lambda p:matches(p,66,140,'INSTALLED CHOS 123',0x152b40),'native editor persisted data after cold boot')
    guest.snapshot(f'installed-notepad-reopen-{sector}{suffix}')
    saved=digest(disk);launch(guest,7);guest.key('ret');guest.key('ret')
    guest.wait(lambda p:matches(p,32,654,'Disabled: this is not an optical-media boot.',0xf2f6ff,0),'refusal to reinstall over own boot disk')
    guest.key('ret');type_text(guest,'ERASE\n');time.sleep(.3)
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
    assert b'GPT primary/backup' in contents['/system/install-log']
    assert contents['/system/computer']==b'CHOSTEST'
    assert contents['/system/setup-stage']==b'CHOS SETUP READY 1\n'
    assert contents['/system/license']==(BUILD.parent/'LICENSE').read_bytes()
    assert contents['/system/font-ofl']==(BUILD.parent/'assets/fonts/OFL.txt').read_bytes()
    assert contents['/documents/note.txt']==b'INSTALLED CHOS 123'
    config=bytearray(contents['/system/config']);assert len(config)==80 and config[:8]==b'CHOSCFG1'
    crc=struct.unpack_from('<I',config,12)[0];config[12:16]=bytes(4);assert zlib.crc32(config)==crc
    assert struct.unpack_from('<IIiI',config,16)==(1,1,60,1)
    assert config[32:64].split(b'\0',1)[0]==b'CHOSTEST'
result.update(passed=True,filesystem_generation=generation,independent_disk_parser=True,disk_sha256=digest(disk),limitations=['accounts/password setup absent','fixed clock offset supported; DST/keymap/display setup incomplete','VMware Fusion unverified'])
(BUILD/f'native-install-{sector}{suffix}.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
