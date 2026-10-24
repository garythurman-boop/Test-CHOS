#!/usr/bin/env python3
"""Independent FAT32/UEFI payload inspection; no FAT tool or shared C decoder."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib
ROOT=Path(__file__).resolve().parents[1];BUILD=ROOT/'build'
def digest(path):
    with path.open('rb') as file:return hashlib.file_digest(file,'sha256').hexdigest()
run=Path(tempfile.mkdtemp(prefix='fat32-test-',dir=BUILD))
results=[]
for sector in [512,4096]:
    path=run/f'chos-{sector}.img'
    command=[str(BUILD/'mkdisk'),str(path),str(BUILD/'BOOTAA64.EFI'),str(BUILD/'KERNEL.BIN'),str(sector)]
    subprocess.run(command,check=True)
    before=digest(path)
    duplicate=run/f'chos-{sector}-repeat.img'
    repeat=list(command);repeat[1]=str(duplicate);subprocess.run(repeat,check=True)
    assert digest(duplicate)==before,'Disk build is not reproducible'
    # Exclusive-create must refuse a pre-existing image without touching a byte.
    assert subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode!=0
    assert digest(path)==before
    with path.open('rb') as f:
        def read(offset,n):f.seek(offset);data=f.read(n);assert len(data)==n;return data
        header=read(sector,sector);entry_offset=struct.unpack_from('<Q',header,72)[0]*sector
        entry=read(entry_offset,128);first,last=struct.unpack_from('<QQ',entry,32);base=first*sector
        b=read(base,sector);bps=struct.unpack_from('<H',b,11)[0];spc=b[13]
        reserved=struct.unpack_from('<H',b,14)[0];fat_count=b[16];total,fat=struct.unpack_from('<II',b,32)
        root,info,backup=struct.unpack_from('<IHH',b,44)
        assert bps==sector and spc==1 and reserved==32 and fat_count==2 and root==2
        assert b[510:512]==b'\x55\xaa' and b[82:90]==b'FAT32   ' and (last-first+1)==total
        assert read(base+backup*bps,bps)==b
        fs=read(base+info*bps,bps);assert struct.unpack_from('<I',fs)[0]==0x41615252
        assert struct.unpack_from('<I',fs,484)[0]==0x61417272 and fs[508:512]==b'\0\0\x55\xaa'
        assert fs==read(base+(backup+info)*bps,bps)
        tables=read(base+reserved*bps,fat*bps);assert tables==read(base+(reserved+fat)*bps,fat*bps)
        clusters=(total-reserved-fat_count*fat)//spc;assert clusters>=65525
        data=base+(reserved+fat_count*fat)*bps
        used=set()
        def chain(cluster):
            out=bytearray();seen=set()
            while cluster<0x0ffffff8:
                assert 2<=cluster<clusters+2 and cluster not in seen
                seen.add(cluster);assert cluster not in used,'cross-linked cluster'
                used.add(cluster);out+=read(data+(cluster-2)*bps,bps)
                cluster=struct.unpack_from('<I',tables,cluster*4)[0]&0x0fffffff
            return bytes(out)
        def directory(cluster):
            contents=chain(cluster);entries={}
            for offset in range(0,len(contents),32):
                e=contents[offset:offset+32]
                if not e[0]:break
                assert e[0]!=0xe5 and e[11] in [0x10,0x20]
                name=e[:8].decode().rstrip();ext=e[8:11].decode().rstrip()
                if ext:name+='.'+ext
                start=(struct.unpack_from('<H',e,20)[0]<<16)|struct.unpack_from('<H',e,26)[0]
                entries[name]=(start,struct.unpack_from('<I',e,28)[0],e[11])
            return entries
        roots=directory(root);assert set(roots)=={'EFI','CHOS'}
        efi=directory(roots['EFI'][0]);assert set(efi)=={'.','..','BOOT'}
        boot=directory(efi['BOOT'][0]);chos=directory(roots['CHOS'][0])
        assert boot['..'][0]==roots['EFI'][0] and chos['..'][0]==0
        paths={'EFI/BOOT/BOOTAA64.EFI':(boot['BOOTAA64.EFI'],BUILD/'BOOTAA64.EFI'),'CHOS/KERNEL.BIN':(chos['KERNEL.BIN'],BUILD/'KERNEL.BIN')}
        files={}
        for name,(e,source) in paths.items():
            assert e[2]==0x20
            raw=chain(e[0]);assert raw[:e[1]]==source.read_bytes() and raw[e[1]:]==bytes(len(raw)-e[1])
            files[name]=hashlib.sha256(raw[:e[1]]).hexdigest()
        e=chos['DISPLAY.BIN'];assert e[1:]==(32,0x20)
        raw=chain(e[0]);assert len(raw)==4096 and raw[:8]==b'CHOSDSP1'
        assert struct.unpack_from('<III',raw,8)==(1024,768,0)
        assert zlib.crc32(raw[:28])==struct.unpack_from('<I',raw,28)[0]
        assert raw[32:]==bytes(4096-32)
        for c in range(2,clusters+2):
            value=struct.unpack_from('<I',tables,c*4)[0]&0x0fffffff
            assert (value!=0)==(c in used)
    results.append({'passed':True,'sector_bytes':sector,'disk_image':str(path),'sha256':before,'files':files,'checks':['BPB and backup','FSInfo and backup','mirrored FATs','bounded non-cyclic non-crosslinked cluster chains','directory tree','exact original EFI/kernel bytes and zero padding','exclusive new-image creation','identical second fresh build']})
report={'passed':True,'scope':'Build-time disk construction; not guest installation','cases':results}
(BUILD/'fat32-images.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
