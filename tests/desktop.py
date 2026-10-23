#!/usr/bin/env python3
"""Drive real guest windows over Virtio input/QMP; save original framebuffer captures."""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import socket
import subprocess
import time
from boot import BUILD, read_ppm, save_png

class Guest:
    def __init__(self,label='desktop',ram=512):
        self.label=label
        self.lock=(BUILD/'boot-test.lock').open('w')
        fcntl.flock(self.lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        self.sockpath=Path(f'/tmp/chos-desktop-{os.getpid()}.sock')
        self.screen=BUILD/f'{label}.ppm'
        self.iso=BUILD/'CHOS-ARM64.iso'
        self.command=['qemu-system-aarch64','-machine','virt,gic-version=2','-cpu','cortex-a72','-accel','tcg','-m',str(ram),'-smp','1',
            '-bios',os.environ.get('CHOS_UEFI_FIRMWARE','/usr/share/qemu-efi-aarch64/QEMU_EFI.fd'),
            '-device','ramfb','-device','virtio-keyboard-pci','-device','virtio-mouse-pci','-device','virtio-scsi-pci',
            '-drive',f'if=none,id=cd,media=cdrom,readonly=on,file={self.iso}',
            '-device','scsi-cd,drive=cd','-boot','order=d','-display','none','-serial',f'file:{BUILD}/{label}.serial.log',
            '-qmp',f'unix:{self.sockpath},server=on,wait=off','-no-reboot','-net','none']
        self.log=(BUILD/f'{label}.qemu.log').open('w')
        self.proc=subprocess.Popen(self.command,stdout=self.log,stderr=subprocess.STDOUT)
        self.sock=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);self.sock.settimeout(10)
        deadline=time.monotonic()+10
        while not self.sockpath.exists():
            if self.proc.poll() is not None or time.monotonic()>deadline:raise RuntimeError('QEMU did not start')
            time.sleep(.1)
        self.sock.connect(str(self.sockpath));self.stream=self.sock.makefile('rwb')
        assert 'QMP' in json.loads(self.stream.readline());self.qmp('qmp_capabilities')
        self.x,self.y=None,None;self.events=[]
    def qmp(self,name,args=None):
        req={'execute':name}
        if args:req['arguments']=args
        self.stream.write(json.dumps(req).encode()+b'\n');self.stream.flush()
        while True:
            value=json.loads(self.stream.readline())
            if 'event' in value:self.events.append(value)
            if 'error' in value:raise RuntimeError(value['error'])
            if 'return' in value:return value['return']
    def capture(self):
        self.qmp('screendump',{'filename':str(self.screen)})
        return read_ppm(self.screen)
    def wait(self,condition,description,seconds=15):
        deadline=time.monotonic()+seconds
        while time.monotonic()<deadline:
            pixel=self.capture()
            if condition(pixel):return pixel
            if self.proc.poll() is not None:raise RuntimeError('Guest exited')
            time.sleep(.15)
        save_png(self.screen)
        raise RuntimeError(f'Timed out: {description}; inspect {self.screen.with_suffix(".png")}')
    def dimensions(self):
        with self.screen.open('rb') as file:
            file.readline();return tuple(map(int,file.readline().split()))
    def move(self,x,y):
        if self.x is None:
            w,h=self.dimensions();self.x,self.y=w-80,h-60
        self.qmp('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':x-self.x}},{'type':'rel','data':{'axis':'y','value':y-self.y}}]})
        self.x,self.y=x,y;time.sleep(.08)
    def button(self,down):
        self.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':down,'button':'left'}}]});time.sleep(.08)
    def click(self,x,y):self.move(x,y);self.button(True);self.button(False)
    def key(self,key,modifiers=()):
        keys=list(modifiers)+[key]
        events=[{'type':'key','data':{'down':True,'key':{'type':'qcode','data':k}}} for k in keys]
        events +=[{'type':'key','data':{'down':False,'key':{'type':'qcode','data':k}}} for k in reversed(keys)]
        self.qmp('input-send-event',{'events':events});time.sleep(.06)
    def power_reason(self):
        while True:
            raw=self.stream.readline()
            if not raw:break
            value=json.loads(raw)
            if 'event' in value:self.events.append(value)
        return next((e.get('data',{}).get('reason') for e in reversed(self.events) if e.get('event')=='SHUTDOWN'),None)
    def snapshot(self,name):
        self.capture();target=BUILD/f'{name}.ppm';target.write_bytes(self.screen.read_bytes());save_png(target)
    def close(self):
        self.sock.close();self.proc.terminate()
        try:self.proc.wait(timeout=5)
        except subprocess.TimeoutExpired:self.proc.kill();self.proc.wait()
        self.sockpath.unlink(missing_ok=True);self.log.close();self.lock.close()

def main():
    started=time.monotonic();guest=Guest()
    result={'passed':False,'iso_sha256':hashlib.sha256(guest.iso.read_bytes()).hexdigest(),'command':guest.command}
    try:
        guest.wait(lambda p:p(60,250)==0x326bb0,'native EL0 client',120)
        guest.snapshot('desktop-initial')
        guest.key('a');guest.wait(lambda p:p(60,250)==0x328c69,'native key event and redraw')
        guest.move(100,38);guest.button(True);guest.move(140,68);guest.button(False)
        guest.wait(lambda p:p(100,280)==0x328c69 and p(30,60)!=0xe5eef8,'window movement')
        # Window now at (68,54), 600 x 330. Maximize then restore.
        guest.click(628,66)
        width,height=guest.dimensions()
        guest.wait(lambda p:p(10,100)==0xe5eef8,'maximize')
        guest.click(width-40,12)
        guest.wait(lambda p:p(100,280)==0x328c69,'restore size and position')
        guest.click(603,66)
        guest.wait(lambda p:p(100,280)!=0x328c69,'minimize')
        guest.click(130,height-22)
        guest.wait(lambda p:p(100,280)==0x328c69,'taskbar restore')
        guest.move(664,380);guest.button(True);guest.move(700,410);guest.button(False)
        guest.wait(lambda p:p(690,370)==0xe5eef8,'client resizing and redraw')
        guest.snapshot('desktop-interaction')
        guest.click(695,66)
        guest.wait(lambda p:p(100,280)!=0x328c69,'close and process exit')
        guest.key('meta_l');guest.key('6')
        guest.wait(lambda p:p(60,250)==0x326bb0,'launcher and reclaimed process/window slots')
        guest.snapshot('desktop-relaunch')
        result.update(passed=True,checks=['EL0 draw/event boundary','keyboard update','move','maximize/restore','minimize/taskbar restore','resize','close/reap','keyboard Start launcher and relaunch'],elapsed_seconds=round(time.monotonic()-started,3))
        print(json.dumps(result,indent=2))
    finally:
        guest.close();(BUILD/'desktop.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
