#!/usr/bin/env python3
"""Corrupt a COPY of a successful native installation; preserve all disk bytes.
The known CHFS roots are invalidated on that disposable file only.
"""
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tempfile
from desktop import Guest
from applications import matches
from boot import BUILD
source=Path(json.loads((BUILD/'native-install-512.json').read_text())['disk'])
run=Path(tempfile.mkdtemp(prefix='volume-error-',dir=BUILD));disk=run/'corrupt.img';shutil.copyfile(source,disk)
with disk.open('r+b') as file:
    file.seek(512);header=file.read(512);table=struct.unpack_from('<Q',header,72)[0]
    file.seek(table*512+128+32);first=struct.unpack('<Q',file.read(8))[0]*512
    for root in [0,1]:file.seek(first+root*4096);file.write(b'BADROOT!')
def digest():
    with disk.open('rb') as file:return hashlib.file_digest(file,'sha256').hexdigest()
before=digest();guest=Guest('volume-error',disk=disk)
result={'passed':False,'iso_sha256':json.loads((BUILD/'native-install-512.json').read_text())['iso_sha256'],'command':guest.command,'corrupted_root_offsets':[first,first+4096]}
try:
    guest.wait(lambda p:p(60,250)==0x326bb0,'disk boot after system-volume corruption',120)
    guest.wait(lambda p:matches(p,44,270,'System volume unavailable',0xa13030,1),'explicit native mount error')
    guest.snapshot('royale-volume-error')
finally:guest.close()
assert digest()==before,'Failed mount modified the installed disk'
result.update(passed=True,disk_sha256_before=before,disk_sha256_after=digest(),check='Corrupt installed CHFS boots temporary session, displays error, performs no disk writes')
(BUILD/'volume-error.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
