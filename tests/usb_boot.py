#!/usr/bin/env python3
"""Real xHCI USB keyboard/mouse, with no Virtio input device in the VM."""
import hashlib,json,time
from desktop import Guest
from applications import launch,type_text,matches
from boot import BUILD

def main():
    g=Guest('usb-native',gic_version=3,virtualization=True,input_backend='usb')
    result={'passed':False,'command':g.command,'iso_sha256':hashlib.sha256(g.iso.read_bytes()).hexdigest(),'usb_completion_mode':'polled event ring; no USB interrupt routing claim'}
    try:
        g.wait(lambda p:p(60,250)==0x326bb0,'USB L selects Live Desktop; native About',120)
        g.click(618,36);launch(g,2)
        g.wait(lambda p:p(45,160)==0xffffff,'USB Start shortcut opens Notepad')
        type_text(g,'USB INPUT 123')
        g.wait(lambda p:matches(p,42,118,'USB INPUT 123',0x152b40),'USB modifier, key and release sequence')
        g.key('backspace');type_text(g,'4')
        g.wait(lambda p:matches(p,42,118,'USB INPUT 124',0x152b40),'USB backspace/release')
        g.key('s',('ctrl',));g.wait(lambda p:p(40,380)==0xb9d5ee,'USB Ctrl-S opens native dialog')
        g.key('ret');time.sleep(.3);g.key('n',('ctrl',));g.key('o',('ctrl',));g.key('ret')
        g.wait(lambda p:matches(p,42,118,'USB INPUT 124',0x152b40),'USB edit/save/reopen')
        g.snapshot('usb-native-notepad')
        g.move(300,36);g.button(True);g.move(360,96);g.button(False)
        g.wait(lambda p:matches(p,102,178,'USB INPUT 124',0x152b40),'USB pointer/buttons drag the actual window')
        g.snapshot('usb-native-drag')
        # More than a complete transfer/event-ring cycle of actual USB reports.
        g.key('n',('ctrl',));type_text(g,'a'*270)
        g.wait(lambda p:matches(p,102,250,'a'*42,0x152b40) and p(606,250)==0x286bb0,'all 270 stress characters arrive exactly')
        g.key('s',('ctrl',))
        g.wait(lambda p:p(100,440)==0xb9d5ee,'USB stress save dialog')
        g.key('ret');g.wait(lambda p:p(100,440)!=0xb9d5ee,'USB stress save accepted')
        g.key('n',('ctrl',));g.wait(lambda p:not matches(p,102,178,'aaaa',0x152b40),'USB new after stress save')
        g.key('o',('ctrl',));g.wait(lambda p:p(100,440)==0xb9d5ee,'USB stress open dialog');g.key('ret')
        g.wait(lambda p:matches(p,102,178,'a'*40,0x152b40),'USB reports continue after transfer/event-ring wrap')
        g.snapshot('usb-native-ring-wrap')
        # Compact controls and anchored File menu remain inside a small window.
        g.move(806,542);g.button(True);g.move(388,324);g.button(False)
        g.click(344,130)
        g.wait(lambda p:p(185,148)==0xf5f9fd,'small Notepad File menu',10)
        g.snapshot('usb-native-small-menu')
        g.key('esc')
        # Unplug while a modifier is held. The driver must synthesize its release.
        g.qmp('input-send-event',{'events':[{'type':'key','data':{'down':True,'key':{'type':'qcode','data':'shift'}}}]});time.sleep(.15)
        g.qmp('device_del',{'id':'usbkbd'});time.sleep(.4)
        assert 'USB releases on disconnect: 1' in (BUILD/'usb-native.serial.log').read_text(errors='replace')
        assert 'USB root disconnect; held keys/buttons released' in (BUILD/'usb-native.serial.log').read_text(errors='replace')

        serial=(BUILD/'usb-native.serial.log').read_text(errors='replace')
        for value in ['xHCI reset and polled event ring: PASS','USB boot keyboard initialized: PASS','USB boot mouse initialized: PASS','Input initialized devices: 2']:
            assert value in serial,value
        assert 'Virtio input' not in serial
        assert 'USB completed receive reports: 510' in serial
        result.update(passed=True,keyboard_enumeration=True,mouse_enumeration=True,modifier_press_release=True,mouse_movement_buttons_drag=True,native_file_round_trip=True,transfer_and_event_ring_wrap=True,small_notepad_menu=True,disconnect_releases=True)
    finally:g.close()
    (BUILD/'usb-native.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
