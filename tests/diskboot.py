#!/usr/bin/env python3
"""UEFI boots CHOS from our GPT/FAT32 NVMe disk, with no optical media attached."""
import hashlib
import json
from pathlib import Path
import time
from desktop import Guest
from boot import BUILD
def digest(path):
    with path.open('rb') as file:return hashlib.file_digest(file,'sha256').hexdigest()
inputs=json.loads((BUILD/'fat32-images.json').read_text());results=[]
for case in inputs['cases']:
    path=Path(case['disk_image']);sector=case['sector_bytes'];before=digest(path)
    started=time.monotonic();guest=Guest(f'gpt-boot-{sector}',disk=path,sector_bytes=sector)
    result={'passed':False,'logical_sector_bytes':sector,'disk_sha256':before,'command':guest.command}
    try:
        assert not any('cdrom' in arg or 'scsi-cd' in arg or arg=='-kernel' for arg in guest.command)
        guest.wait(lambda p:p(60,250)==0x326bb0,'native desktop from GPT NVMe boot',120)
        guest.key('a');guest.wait(lambda p:p(60,250)==0x328c69,'native input after NVMe-only boot')
        guest.snapshot(f'gpt-boot-{sector}')
        result.update(passed=True,elapsed_seconds=round(time.monotonic()-started,3))
    finally:
        guest.close();result['disk_unchanged']=digest(path)==before
        assert result['disk_unchanged'],'live desktop modified boot disk'
        results.append(result)
        (BUILD/'diskboot.json').write_text(json.dumps({'passed':all(x['passed'] for x in results),'scope':'UEFI boot from builder-created GPT/ESP disks, not a guest installer run','cases':results},indent=2)+'\n')
print((BUILD/'diskboot.json').read_text())
