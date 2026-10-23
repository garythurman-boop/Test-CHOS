#!/usr/bin/env python3
import hashlib
import json
import time
from desktop import Guest
from applications import launch, type_text
from boot import BUILD
started=time.monotonic();guest=Guest('restart')
result={'passed':False,'iso_sha256':hashlib.sha256(guest.iso.read_bytes()).hexdigest(),'command':guest.command}
try:
    guest.wait(lambda p:p(60,250)==0x326bb0,'boot for reset test',120)
    launch(guest,3);type_text(guest,'reboot\n');type_text(guest,'YES\n')
    deadline=time.monotonic()+8
    while guest.proc.poll() is None and time.monotonic()<deadline:time.sleep(.1)
    assert guest.proc.poll()==0 and guest.power_reason()=='guest-reset','No QMP-confirmed guest reset'
    result.update(passed=True,check='Native Terminal confirmed restart; ACPI PSCI; QMP guest-reset event',elapsed_seconds=round(time.monotonic()-started,3))
    print(json.dumps(result,indent=2))
finally:
    guest.close();(BUILD/'restart.json').write_text(json.dumps(result,indent=2)+'\n')
