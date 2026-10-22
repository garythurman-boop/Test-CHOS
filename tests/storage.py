#!/usr/bin/env python3
"""Three cold VM boots plus independent on-disk CRC/data checks and no-write gates.
Only explicitly created files under build/ are attached as writable devices.
"""
from pathlib import Path
import hashlib
import json
import struct
import subprocess
import tempfile
import sys
import zlib
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build'
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def boot(*args): subprocess.run([sys.executable,str(ROOT/'tests/boot.py'),*map(str,args)],check=True,cwd=ROOT)
def main():
    run=Path(tempfile.mkdtemp(prefix='storage-run-',dir=BUILD))
    disk=run/'storage-test.img'
    with disk.open('wb') as f: f.truncate(8*1024*1024)
    iso=BUILD/'CHOS-STORAGE-TEST.iso'
    boot('--iso',iso,'--disk',disk,'--storage-phase',1)
    boot('--iso',iso,'--disk',disk,'--storage-phase',2)
    before=digest(disk)
    boot('--iso',iso,'--disk',disk,'--storage-phase',3)
    assert before==digest(disk),'verification-only boot modified disk'
    raw=disk.read_bytes()
    headers=[]
    for index in [0,1]:
        h=bytearray(raw[index*4096:(index+1)*4096])
        recorded=struct.unpack_from('<I',h,12)[0]; h[12:16]=bytes(4)
        if zlib.crc32(h)==recorded and h[:8]==b'COSCHFS1': headers.append((struct.unpack_from('<Q',h,16)[0],h))
    assert headers,'no checksummed CHFS metadata root'
    generation,h=max(headers,key=lambda x:x[0])
    files={}
    for index in range(40):
        offset=160+index*80
        name=bytes(h[offset:offset+60]).split(b'\0',1)[0].decode('ascii')
        first,blocks,size,kind,crc=struct.unpack_from('<IIIII',h,offset+60)
        if kind==1:
            content=raw[first*4096:first*4096+size]
            assert len(content)==size and zlib.crc32(content)==crc
            files[name]=content
    assert struct.unpack('<Q',files['/state'])[0]==2
    assert files['/documents/check.txt']==bytes((i^0x52)&255 for i in range(9000))
    assert files['/user/renamed.txt']==bytes((i^0x9b)&255 for i in range(8192))
    # Refuse destructive tests on a different device identity, even if a test ISO is used.
    refused=run/'refused-disk.img'
    refused.write_bytes(bytes([0xa5])*(8*1024*1024)); refused_before=digest(refused)
    boot('--iso',iso,'--disk',refused,'--disk-serial','ORDINARY-DISK','--expect-disk-reject')
    assert digest(refused)==refused_before,'refused device was modified'
    # Normal ISO must not write, even when a disk has the test serial.
    normal=run/'normal-disk.img'
    with normal.open('wb') as f: f.truncate(8*1024*1024)
    normal_before=digest(normal)
    boot('--disk',normal,'--label','normal-disk-no-write')
    assert digest(normal)==normal_before,'normal ISO modified disk'
    result={'passed':True,'workspace_disk':str(disk),'cold_boots':3,'generation':generation,'disk_sha256':before,
            'data_checks':'independent Python little-endian parser and zlib CRC32',
            'refused_identity_unchanged':True,'normal_iso_disk_unchanged':True,
            'third_boot_unchanged':True,'files':{k:hashlib.sha256(v).hexdigest() for k,v in files.items()}}
    (BUILD/'storage-integrity.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
if __name__=='__main__': main()
