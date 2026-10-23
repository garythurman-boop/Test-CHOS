#!/usr/bin/env python3
"""Boot actual alternate-mode and unsupported-mode ISOs; restore release ISO."""
import hashlib
import json
import os
import subprocess
from desktop import Guest
from boot import BUILD
results=[]
try:
    for request,expected in [('800x600',(800,600)),('1500x900',(800,600))]:
        subprocess.run(['python3','scripts/mkiso.py'],env={**os.environ,'CHOS_DISPLAY_MODE':request},check=True)
        guest=Guest('mode-'+request)
        try:
            guest.wait(lambda p:p(60,250)==0x326bb0,'requested/fallback firmware mode',120)
            actual=guest.dimensions();assert actual==expected,(request,actual,expected)
            guest.snapshot('royale-mode-'+request)
            results.append({'request':request,'actual':actual,'passed':True,'iso_sha256':hashlib.sha256(guest.iso.read_bytes()).hexdigest(),'command':guest.command})
        finally:guest.close()
finally:subprocess.run(['python3','scripts/mkiso.py'],env={k:v for k,v in os.environ.items() if k!='CHOS_DISPLAY_MODE'},check=True)
(BUILD/'display-modes.json').write_text(json.dumps({'passed':True,'cases':results},indent=2)+'\n');print(json.dumps(results,indent=2))
