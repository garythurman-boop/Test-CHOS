#!/usr/bin/env python3
"""Disposable NVMe error, interrupted install, retry and capacity acceptance.
No host devices are opened. QEMU blkdebug injects an actual guest I/O failure.
"""
import hashlib,json,tempfile,time
from pathlib import Path
from desktop import Guest
from applications import matches,type_text
from boot import BUILD

def digest(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def new_disk(name,mib):
    p=run/name
    with p.open('wb') as f:f.truncate(mib*1048576)
    return p
def welcome(g):g.wait(lambda p:p(30,160)==0x244da8,'dedicated ISO Setup',120)
def authorize(g):
    for _ in range(4):g.key('ret')
    g.wait(lambda p:matches(p,32,92,'Confirm destruction',0xf2f6ff),'erase confirmation')
    type_text(g,'ERASE')
    # Send Enter without a host dwell so interruption can catch an early stage.
    g.qmp('input-send-event',{'events':[{'type':'key','data':{'down':down,'key':{'type':'qcode','data':'ret'}}} for down in [True,False]]})
run=Path(tempfile.mkdtemp(prefix='setup-flow-',dir=BUILD));result={'passed':False,'iso_sha256':digest(BUILD/'CHOS-ARM64.iso'),'boots':[]}
# Real write failure: an eligible, readable target whose first NVM write fails.
disk=new_disk('io-error.img',128);before=digest(disk);fault=run/'fault.conf'
fault.write_text('[inject-error]\nevent = "write_aio"\nerrno = "5"\nonce = "on"\nimmediately = "on"\n')
g=Guest('setup-io-error',disk=disk,optical=True,live=False,fault_config=fault)
try:
    welcome(g);authorize(g)
    g.wait(lambda p:matches(p,32,148,'GPT write, flush or read-back failed.',0xffd9a0,0),'actual NVM write error displayed',20)
    g.snapshot('setup-io-error');g.key('f3')
    g.wait(lambda p:p(60,250)==0x326bb0,'error exits safely to live diagnostics')
    result['boots'].append({'kind':'nvme_write_error','command':g.command,'passed':True,'error':'GPT write/read-back failure','safe_live_exit':True})
finally:g.close()
assert digest(disk)==before,'The failed first write unexpectedly changed target bytes'
# Stop an actual worker after GPT preparation while FAT32 formatting is displayed.
disk=new_disk('interrupted.img',70);before=digest(disk)
g=Guest('setup-interrupted',disk=disk,optical=True,live=False)
try:
    welcome(g);authorize(g)
    g.wait(lambda p:matches(p,32,92,'Installing CHOS system files',0xf2f6ff),'active graphical install',20)
    g.qmp('stop');g.snapshot('setup-interrupted')
    result['boots'].append({'kind':'interrupt_active_writer','command':g.command,'passed':True,'method':'QMP stop followed by VM termination'})
finally:g.close()
assert digest(disk)!=before,'Interruption did not reach actual disk writes'
# A CHFS success log must not exist on this partially prepared blank destination.
assert b'CHOS Setup log v1' not in disk.read_bytes(),'Interrupted fixture already completed installation'
# A new ISO boot must inventory and explicitly reconfirm the same interrupted disk.
g=Guest('setup-retry',disk=disk,optical=True,live=False)
try:
    welcome(g);g.key('ret');g.key('ret')
    g.wait(lambda p:matches(p,32,240,'Existing primary GPT verified:',0xf2f6ff),'read-only existing GPT inventory')
    g.snapshot('setup-existing-gpt');g.key('esc');g.key('esc');authorize(g)
    g.wait(lambda p:matches(p,32,92,'CHOS has completed the first stage',0xf2f6ff),'retry completes and verifies actual files',30)
    g.snapshot('setup-retry-complete')
    result['boots'].append({'kind':'retry_interrupted_destination','command':g.command,'passed':True,'existing_gpt_inventory':True})
finally:g.close()
# The 70 MiB native retry must boot with the installation ISO entirely absent.
g=Guest('setup-small-installed',disk=disk)
try:
    g.wait(lambda p:matches(p,256,100,'Personalize CHOS',0xf2f6ff,2),'installed pending configuration on a 70 MiB destination',120)
    g.snapshot('setup-small-installed');result['boots'].append({'kind':'near_minimum_installed_boot','disk_mib':70,'command':g.command,'passed':True})
finally:g.close()
# Small target: exact layout error, no arbitrary 128 MiB prerequisite and no writes.
small=new_disk('too-small.img',64);before=digest(small)
g=Guest('setup-too-small',disk=small,optical=True,live=False)
try:
    welcome(g);g.key('r')
    g.wait(lambda p:matches(p,32,92,'Recovery tools are not implemented yet.',0xf2f6ff),'honest recovery status');g.snapshot('setup-recovery-status')
    g.key('ret');g.key('ret');g.key('ret')
    g.wait(lambda p:matches(p,32,654,'Disk too small',0xf2f6ff,0),'capacity rejection')
    g.snapshot('setup-too-small');g.key('ret');g.key('f3')
    g.wait(lambda p:p(60,250)==0x326bb0,'exit from unavailable target to Live Desktop')
    result['boots'].append({'kind':'small_disk_and_recovery_status','command':g.command,'passed':True,'disk_mib':64,'required_bytes':69*1048576+20480})
finally:g.close()
assert digest(small)==before,'Ineligible target changed'
result.update(passed=True,write_failure_disk_unchanged=True,small_disk_unchanged=True,interruption_did_not_report_success=True)
(BUILD/'setup-flow.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
