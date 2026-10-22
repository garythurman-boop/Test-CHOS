#!/usr/bin/env python3
"""Boot the ISO via real AArch64 UEFI, then verify kernel-rendered pixels over QMP.
No -kernel, host firmware callbacks, or guest OS is used to start CHOS.
"""
import argparse
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
SIGNATURE = [0x43c0de,0x6b45a1,0xf3ba20,0x18ad72,0xe14567,0x2998ef,0xcddc39,0x804020]
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

def completed(path):
    pixel = read_ppm(path)
    for i,rgb in enumerate(SIGNATURE):
        if pixel(40+i*16,408) != rgb:
            return False
    for index,glyph in enumerate(CHOS):
        for row,bits in enumerate(glyph):
            for col in range(5):
                expected = 0xffffff if bits & (16>>col) else 0x164a82
                if pixel(32+index*12+col*2,24+row*2) != expected:
                    return False
    return True

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--iso',type=Path,default=BUILD/'CHOS-ARM64.iso')
    parser.add_argument('--ram',type=int,default=512)
    parser.add_argument('--expect-missing-kernel',action='store_true')
    parser.add_argument('--timeout',type=int,default=120)
    args=parser.parse_args()
    name='missing-kernel' if args.expect_missing_kernel else f'boot-{args.ram}'
    qmp_path=Path('/tmp') / f'chos-{os.getpid()}.sock'
    serial=BUILD/f'{name}.serial.log'
    screen=BUILD/f'{name}.ppm'
    screen.unlink(missing_ok=True)
    firmware=os.environ.get('CHOS_UEFI_FIRMWARE','/usr/share/qemu-efi-aarch64/QEMU_EFI.fd')
    command=['qemu-system-aarch64','-machine','virt','-cpu','cortex-a72',
             '-accel','tcg','-m',str(args.ram),'-smp','1','-bios',firmware,
             '-device','ramfb','-device','virtio-scsi-pci',
             '-drive',f'if=none,id=cd,media=cdrom,readonly=on,file={args.iso.resolve()}',
             '-device','scsi-cd,drive=cd','-boot','order=d',
             '-display','none','-serial',f'file:{serial}',
             '-qmp',f'unix:{qmp_path},server=on,wait=off','-no-reboot','-net','none']
    started=time.monotonic()
    outcome={'passed':False,'ram_mib':args.ram,'command':command,'test':name}
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
                    if completed(screen):
                        outcome['passed']=True
                        break
                time.sleep(.5)
            if not outcome['passed']:
                raise RuntimeError('Boot acceptance timed out; inspect screenshot and logs')
            if not args.expect_missing_kernel:
                save_png(screen)
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
