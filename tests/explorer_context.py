#!/usr/bin/env python3
"""Real Explorer right-click selection, folder navigation, properties and copying."""
import hashlib,json,time
from desktop import Guest
from applications import launch,matches,type_text
from boot import BUILD
def right(g,x,y):
 g.move(x,y)
 for down in [True,False]:g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':down,'button':'right'}}]});time.sleep(.08)
g=Guest('explorer-context-m8');result={'passed':False,'command':g.command}
try:
 g.wait(lambda p:p(60,250)==0x326bb0,'About',120);g.click(618,36);launch(g,1)
 g.wait(lambda p:matches(p,46,142,'documents',0x206ba5),'root folder')
 right(g,100,146);g.move(600,550);g.snapshot('m8-explorer-folder-menu');g.key('ret')
 g.wait(lambda p:matches(p,44,64,'/documents',0x204b72,0),'context Open folder')
 right(g,100,146);g.click(150,267);g.move(600,550)
 g.wait(lambda p:matches(p,56,190,'/documents/welcome.txt',0x204b72,3),'selected filesystem object properties');g.snapshot('m8-explorer-file-properties');g.key('esc')
 right(g,100,146);g.click(150,186)
 g.wait(lambda p:matches(p,44,378,'Copy to (absolute path)',0x163b65,1),'context Copy dialog')
 g.key('u',('ctrl',));type_text(g,'/documents/copied.txt');g.key('ret');g.move(600,550)
 g.wait(lambda p:matches(p,46,164,'copied.txt',0x203a54),'context copy creates actual file');g.snapshot('m8-explorer-copy')
 # A repeat copy to an existing destination must fail instead of overwriting.
 right(g,100,146);g.click(150,186);g.key('u',('ctrl',));type_text(g,'/documents/copied.txt');g.key('ret')
 g.wait(lambda p:matches(p,40,462,'Operation failed or cancelled',0x163b65,3),'copy overwrite protection')
 result.update(passed=True,iso_sha256=hashlib.sha256(g.iso.read_bytes()).hexdigest(),checks=['right-click folder selection','Open folder','file properties','real copy','existing destination overwrite refusal'])
finally:g.close();(BUILD/'explorer-context-m8.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
