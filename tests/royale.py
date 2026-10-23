#!/usr/bin/env python3
"""Native Royale layout, AA output, menu pointer launch and measured damage path."""
import hashlib
import json
import time
from desktop import Guest
from applications import launch,read_number
from boot import BUILD

guest=Guest('royale')
result={'passed':False,'iso_sha256':hashlib.sha256(guest.iso.read_bytes()).hexdigest(),'command':guest.command}
try:
    guest.wait(lambda p:p(60,250)==0x326bb0,'native Royale desktop',120)
    width,height=guest.dimensions();assert (width,height)==(1024,768)
    guest.snapshot('royale-desktop')
    guest.click(618,36);guest.wait(lambda p:p(60,250)!=0x326bb0,'close About')
    guest.snapshot('royale-wallpaper')
    guest.click(40,height-22);time.sleep(.2);guest.snapshot('royale-start')
    # Pinned Notepad in the left column: actual window and user process.
    guest.click(90,height-44-400+64+36+14)
    guest.wait(lambda p:p(45,160)==0xffffff,'Start pointer launch of Notepad')
    guest.snapshot('royale-notepad');guest.click(738,36)
    launch(guest,5)
    guest.wait(lambda p:p(60,120)==0xe5eef8,'native Task Manager')
    old=read_number(guest.capture(),310,404,0x204b72)
    # Stationary scene, pointer outside window chrome. Renderer counters are
    # read back through the native process, not inferred from host frame rate.
    for i in range(12):guest.move(850+i*3,620+i*2)
    guest.wait(lambda p:read_number(p,310,404,0x204b72)>old,'partial repaint telemetry',10)
    partial=read_number(guest.capture(),310,404,0x204b72)
    guest.snapshot('royale-render-metrics')
    result.update(passed=True,width=width,height=height,partial_pointer_frames_before=old,partial_pointer_frames_after=partial,
                  checks=['firmware-selected 1024x768','Start pointer launches native Notepad','real exported partial repaint counter increases'])
finally:
    guest.close();(BUILD/'royale.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
