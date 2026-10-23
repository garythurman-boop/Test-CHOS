#!/usr/bin/env python3
"""Bake licensed Noto outlines into bounded grayscale glyph caches at build time.
The guest reads A8 coverage only; Pillow/FreeType are not guest dependencies.
"""
from pathlib import Path
import hashlib
import json
import struct
from PIL import ImageFont
ROOT=Path(__file__).resolve().parents[1]
# id, file, px, baseline, line height, fixed advance (0 => proportional)
STYLES=[('body','NotoSans-Regular.ttf',14,14,20,0),
        ('bold','NotoSans-Bold.ttf',14,14,20,0),
        ('title','NotoSans-Bold.ttf',18,18,25,0),
        ('small','NotoSans-Regular.ttf',12,12,17,0),
        ('mono','NotoSansMono-Regular.ttf',16,13,18,12)]
FIRST,COUNT=32,95
header=struct.pack('<8sIIII',b'CHOSFNT1',len(STYLES),FIRST,COUNT,24)
records=bytearray();pixels=bytearray();styles=[]
start=24+len(STYLES)*16+len(STYLES)*COUNT*12
for index,(name,file,size,baseline,line,fixed) in enumerate(STYLES):
    font=ImageFont.truetype(str(ROOT/'assets/fonts'/file),size,layout_engine=ImageFont.Layout.BASIC)
    styles.append(struct.pack('<IIII',size,baseline,line,24+len(STYLES)*16+index*COUNT*12))
    for char in range(FIRST,FIRST+COUNT):
        mask,(left,top)=font.getmask2(chr(char),mode='L',anchor='ls')
        width,height=mask.size
        advance=fixed or int(font.getlength(chr(char))+.5)
        assert width<=32 and height<=40 and -32<=left<=32 and -40<=top<=40
        records+=struct.pack('<IHHbbH',start+len(pixels),width,height,left,top,advance)
        pixels+=bytes(mask)
blob=header+b''.join(styles)+records+pixels
out=ROOT/'assets/royale/fonts.bin';out.write_bytes(blob)
manifest={'format':'CHOSFNT1','coverage':'8-bit grayscale','characters':[FIRST,FIRST+COUNT-1],
          'styles':[dict(id=i,name=s[0],size=s[2],baseline=s[3],line_height=s[4],fixed_advance=s[5]) for i,s in enumerate(STYLES)],
          'source_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((ROOT/'assets/fonts').glob('*.ttf'))},
          'sha256':hashlib.sha256(blob).hexdigest(),'bytes':len(blob),'license':'SIL OFL 1.1 (assets/fonts/OFL.txt)'}
(ROOT/'assets/royale/fonts.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({'glyph_cache_bytes':len(blob),'sha256':manifest['sha256']},indent=2))
