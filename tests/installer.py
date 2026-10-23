#!/usr/bin/env python3
"""Independently inspect GPT images written by the original CHOS C implementation."""
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import uuid
import zlib
ROOT=Path(__file__).resolve().parents[1];BUILD=ROOT/'build'
run=Path(tempfile.mkdtemp(prefix='gpt-test-',dir=BUILD))
subprocess.run([str(BUILD/'test-installer'),str(run/'gpt')],check=True)
results=[]
for sector,size in [(512,128*1024*1024),(4096,512*1024*1024)]:
    path=run/f'gpt-{sector}.img'
    with path.open('rb') as f:
        def read(lba,n):f.seek(lba*sector);return f.read(n)
        assert path.stat().st_size==size
        mbr=read(0,512);assert mbr[510:512]==b'\x55\xaa' and mbr[450]==0xee
        assert struct.unpack_from('<II',mbr,454)==(1,size//sector-1)
        roots=[]
        for current,alternate in [(1,size//sector-1),(size//sector-1,1)]:
            h=bytearray(read(current,sector));assert h[:8]==b'EFI PART'
            revision,length,checksum=struct.unpack_from('<III',h,8);assert (revision,length)==(0x10000,92)
            h[16:20]=b'\0'*4;assert zlib.crc32(h[:length])==checksum
            assert struct.unpack_from('<QQ',h,24)==(current,alternate)
            first,last=struct.unpack_from('<QQ',h,40)
            table,count,entry_size,table_crc=struct.unpack_from('<QIII',h,72)
            assert (count,entry_size)==(128,128)
            entries=read(table,count*entry_size);assert len(entries)==16384 and zlib.crc32(entries)==table_crc
            assert table+len(entries)//sector<=first or table>last
            assert read(1,8)==b'EFI PART'
            esp=entries[:128];system=entries[128:256]
            assert str(uuid.UUID(bytes_le=esp[:16]))=='c12a7328-f81f-11d2-ba4b-00a0c93ec93b'
            assert str(uuid.UUID(bytes_le=system[:16]))=='c4ec0550-3c1d-4e65-9abd-43484f530001'
            assert len({bytes(h[56:72]),esp[16:32],system[16:32],bytes(16)})==4
            ef,el=struct.unpack_from('<QQ',esp,32);sf,sl=struct.unpack_from('<QQ',system,32)
            assert first<=ef<=el<sf<=sl<=last
            assert ef*sector==1048576 and (el-ef+1)*sector==(64 if sector==512 else 272)*1048576
            assert esp[56:128].decode('utf-16le').rstrip('\0')=='CHOS EFI'
            assert system[56:128].decode('utf-16le').rstrip('\0')=='CHOS System'
            assert entries[256:]==bytes(16384-256)
            roots.append((bytes(h[56:72]),first,last,entries))
        assert roots[0]==roots[1]
    results.append({'logical_sector_bytes':sector,'disk_bytes':size,'passed':True,'checks':['protective MBR','primary and backup header CRC32','reciprocal LBA geometry','entry-array CRC32 and identity','partition GUIDs, alignment, limits, names and nonoverlap']})
report={'passed':True,'scope':'GPT encoder only; no ESP formatting, installation or installed boot','cases':results}
(BUILD/'installer-gpt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
