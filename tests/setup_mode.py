#!/usr/bin/env python3
"""Actual 640x480 dedicated Setup configuration and safe exit, no writes."""
import hashlib,json,os,subprocess,tempfile
from pathlib import Path
from desktop import Guest
from applications import matches
from boot import BUILD
run=Path(tempfile.mkdtemp(prefix='setup-mode-',dir=BUILD));disk=run/'destination.img'
with disk.open('wb') as f:f.truncate(70*1048576)
def digest():
    with disk.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
before=digest();result={'passed':False}
try:
    subprocess.run(['python3','scripts/mkiso.py'],env={**os.environ,'CHOS_DISPLAY_MODE':'640x480'},check=True)
    g=Guest('setup-640',disk=disk,optical=True,live=False)
    try:
        g.wait(lambda p:p(30,160)==0x244da8,'minimum-resolution setup welcome',120)
        assert g.dimensions()==(640,480)
        g.snapshot('setup-blue-640')
        for _ in range(4):g.key('ret')
        g.wait(lambda p:matches(p,32,92,'Confirm destruction',0xf2f6ff),'minimum-mode confirmation')
        g.snapshot('setup-confirm-640');g.key('f3')
        g.wait(lambda p:p(60,250)==0x326bb0,'explicit live selection at minimum mode')
        result.update(passed=True,command=g.command,actual_mode=[640,480],iso_sha256=hashlib.sha256(g.iso.read_bytes()).hexdigest())
    finally:g.close()
finally:subprocess.run(['python3','scripts/mkiso.py'],env={k:v for k,v in os.environ.items() if k!='CHOS_DISPLAY_MODE'},check=True)
assert digest()==before
result['unconfirmed_disk_unchanged']=True
(BUILD/'setup-mode.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
