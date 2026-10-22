#!/usr/bin/env python3
"""Boot the ISO via real AArch64 UEFI, then verify kernel-rendered pixels over QMP.
No -kernel, host firmware callbacks, or guest OS is used to start CHOS.
"""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import socket
import subprocess
import struct
import zlib
import time

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'
SIGNATURE = [0x32bbcd,0x6b45a1,0xf3ba20,0x18ad72,0xe14567,0x2998ef,0xcddc39,0x804020]
# Independent expected glyphs for the CHOS banner.
CHOS = [[14,17,16,16,16,17,14], [17,17,17,31,17,17,17],
        [14,17,17,17,17,17,14], [15,16,16,14,1,1,30]]

def read_ppm(path):
    with path.open('rb') as f:
        assert f.readline().strip() == b'P6'
        line = f.readline()
        while line.startswith(b'#'):
            line = f.readline()
        width,height = map(int,line.split())
        assert f.readline().strip() == b'255'
        data = f.read()
        assert len(data) == width*height*3
    def pixel(x,y):
        offset = (y*width+x)*3
        return int.from_bytes(data[offset:offset+3],'big')
    return pixel

def save_png(ppm):
    with ppm.open('rb') as f:
        assert f.readline().strip() == b'P6'
        width,height = map(int,f.readline().split())
        assert f.readline().strip() == b'255'
        data = f.read()
    def chunk(kind,payload):
        return (struct.pack('>I',len(payload))+kind+payload+
                struct.pack('>I',zlib.crc32(kind+payload)))
    raw=b''.join(b'\0'+data[y*width*3:(y+1)*width*3] for y in range(height))
    png=(b'\x89PNG\r\n\x1a\n'+
         chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))+
         chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
    ppm.with_suffix('.png').write_bytes(png)

def completed(path,phase=0):
    pixel = read_ppm(path)
    expected_signature=list(SIGNATURE)
    if phase: expected_signature[0]={1:0x33ccdd,2:0x33ddee,3:0x33eeff}[phase]
    for i,rgb in enumerate(expected_signature):
        if pixel(40+i*16,408) != rgb:
            return False
    for index,glyph in enumerate(CHOS):
        for row,bits in enumerate(glyph):
            for col in range(5):
                expected = 0xffffff if bits & (16>>col) else 0x164a82
                if pixel(32+index*12+col*2,24+row*2) != expected:
                    return False
    return True

def platform_rejected(path,disk=False):
    pixel=read_ppm(path)
    if pixel(40,408)!=0xff0000:
        return False
    # Exact ACPI diagnostic prefix, rather than accepting any kernel panic.
    glyphs=[[14,17,17,31,17,17,17],CHOS[0],[30,17,17,30,16,16,16],[14,4,4,4,4,4,14]]
    if disk: glyphs=[CHOS[3],[31,4,4,4,4,4,4],CHOS[2],[30,17,17,30,20,18,17]]
    for index,glyph in enumerate(glyphs):
        for row,bits in enumerate(glyph):
            for col in range(5):
                expected=0xffffff if bits&(16>>col) else 0x300808
                if pixel(32+index*12+col*2,132+row*2)!=expected:
                    return False
    return True

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--iso',type=Path,default=BUILD/'CHOS-ARM64.iso')
    parser.add_argument('--ram',type=int,default=512)
    parser.add_argument('--expect-missing-kernel',action='store_true')
    parser.add_argument('--gic-version',choices=['2','3'],default='2')
    parser.add_argument('--expect-platform-reject',action='store_true')
    parser.add_argument('--label')
    parser.add_argument('--expect-disk-reject',action='store_true')
    parser.add_argument('--disk',type=Path)
    parser.add_argument('--storage-phase',type=int,choices=[1,2,3])
    parser.add_argument('--disk-serial',default='CHOS-TEST-0001')
    parser.add_argument('--timeout',type=int,default=120)
    args=parser.parse_args()
    lock=(BUILD/'boot-test.lock').open('w')
    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    name=('platform-reject' if args.expect_platform_reject else
          ('missing-kernel' if args.expect_missing_kernel else f'boot-{args.ram}'))
    if args.expect_disk_reject: name='disk-reject'
    if args.label:
        if not args.label.replace('-','').isalnum(): parser.error('invalid label')
        name=args.label
    if args.storage_phase: name=f'storage-{args.storage_phase}'
    qmp_path=Path('/tmp') / f'chos-{os.getpid()}.sock'
    serial=BUILD/f'{name}.serial.log'
    screen=BUILD/f'{name}.ppm'
    screen.unlink(missing_ok=True)
    firmware=os.environ.get('CHOS_UEFI_FIRMWARE','/usr/share/qemu-efi-aarch64/QEMU_EFI.fd')
    command=['qemu-system-aarch64','-machine',f'virt,gic-version={args.gic_version}','-cpu','cortex-a72',
             '-accel','tcg','-m',str(args.ram),'-smp','1','-bios',firmware,
             '-device','ramfb','-device','virtio-scsi-pci',
             '-drive',f'if=none,id=cd,media=cdrom,readonly=on,file={args.iso.resolve()}',
             '-device','scsi-cd,drive=cd','-boot','order=d',
             '-display','none','-serial',f'file:{serial}',
             '-qmp',f'unix:{qmp_path},server=on,wait=off','-no-reboot','-net','none']
    if args.disk:
        command+=['-drive',f'if=none,id=nvme0,format=raw,cache=writeback,file={args.disk.resolve()}',
                  '-device',f'nvme,drive=nvme0,serial={args.disk_serial}']
    started=time.monotonic()
    outcome={'passed':False,'ram_mib':args.ram,'command':command,'test':name,'iso_sha256':hashlib.sha256(args.iso.read_bytes()).hexdigest()}
    with (BUILD/f'{name}.qemu.log').open('w') as log:
        proc=subprocess.Popen(command,stdout=log,stderr=subprocess.STDOUT)
        sock=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM)
        sock.settimeout(5)
        try:
            deadline=started+args.timeout
            while not qmp_path.exists():
                if proc.poll() is not None or time.monotonic()>deadline:
                    raise RuntimeError('QEMU failed to start; inspect QEMU log')
                time.sleep(.1)
            sock.connect(str(qmp_path))
            stream=sock.makefile('rwb')
            greeting=json.loads(stream.readline())
            assert 'QMP' in greeting
            def qmp(execute,arguments=None):
                request={'execute':execute}
                if arguments: request['arguments']=arguments
                stream.write(json.dumps(request).encode()+b'\n'); stream.flush()
                while True:
                    response=json.loads(stream.readline())
                    if 'error' in response:
                        raise RuntimeError(response['error'])
                    if 'return' in response:
                        return response['return']
            qmp('qmp_capabilities')
            while time.monotonic()<deadline:
                if proc.poll() is not None:
                    raise RuntimeError('QEMU exited before acceptance condition')
                if serial.exists() and 'Synchronous Exception' in serial.read_text(errors='replace'):
                    raise RuntimeError('Guest exception; inspect serial log')
                if args.expect_missing_kernel:
                    if serial.exists() and 'CHOS: CHOS/KERNEL.BIN missing' in serial.read_text(errors='replace'):
                        outcome['passed']=True
                        break
                else:
                    qmp('screendump',{'filename':str(screen)})
                    if args.expect_platform_reject or args.expect_disk_reject:
                        if platform_rejected(screen,args.expect_disk_reject):
                            outcome['passed']=True
                            break
                    elif read_ppm(screen)(40,408)==0xff0000:
                        save_png(screen)
                        debug=qmp('human-monitor-command',{'command-line':'info pci'})
                        (BUILD/f'{name}.pci.log').write_text(debug)
                        raise RuntimeError('CHOS panic; inspect screenshot')
                    if not args.expect_platform_reject and not args.expect_disk_reject and completed(screen,args.storage_phase or 0):
                        outcome['passed']=True
                        break
                time.sleep(.5)
            if not outcome['passed']:
                debug=qmp('human-monitor-command',{'command-line':'info registers'})
                (BUILD/f'{name}.registers.log').write_text(debug)
                raise RuntimeError('Boot acceptance timed out; inspect screenshot and logs')
            if not args.expect_missing_kernel:
                save_png(screen)
            outcome['milestone']='3A'
            if args.storage_phase: outcome['storage_phase']=args.storage_phase
            outcome['result_kind']=('expected_rejection' if args.expect_platform_reject or args.expect_missing_kernel or args.expect_disk_reject else 'functional_boot')
            if outcome['result_kind']=='functional_boot':
                outcome['kernel_checks']=['BRK exception return','physical page integrity and double-free rejection',
                    'heap allocate/free','null translation fault','stack guard translation fault',
                    'read-only code permission fault','NX heap instruction fault','ACPI GICv2/GTDT discovery',
                    'timer-driven non-yielding worker preemption','register context canaries',
                    'strict priority and round-robin selection','thread exit and continued scheduling','EL0 static ELF execution and syscalls',
                    'per-process private mappings and IPC','invalid user buffer rejection',
                    'kernel/text/NX-stack violation termination','deferred process and thread reclamation']
            if args.storage_phase:
                outcome['storage_checks']={1:['NVMe block read/write/flush','CHFS format/create/read/rename','EL0 file-handle and invalid-buffer tests'],2:['cold boot generation 1 integrity','persistent delete/rewrite','EL0 file persistence'],3:['cold boot generation 2 integrity','EL0 file persistence','read-only verification boot']}[args.storage_phase]
            if args.disk:
                pci=qmp('human-monitor-command',{'command-line':'info pci'})
                (BUILD/f'{name}.pci.log').write_text(pci)
            outcome['elapsed_seconds']=round(time.monotonic()-started,3)
            print(json.dumps(outcome,indent=2))
        finally:
            sock.close()
            proc.terminate()
            try: proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill(); proc.wait()
            qmp_path.unlink(missing_ok=True)
            (BUILD/f'{name}.json').write_text(json.dumps(outcome,indent=2)+'\n')

if __name__=='__main__': main()
