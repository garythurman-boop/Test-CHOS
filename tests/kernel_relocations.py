#!/usr/bin/env python3
"""Flat kernel has no runtime relocator: reject absolute address constants."""
import json,re,subprocess,sys
from pathlib import Path
if sys.argv[1:]==['--negative-self-test']:
 source=Path('build/absolute-pointer-test.S');obj=source.with_suffix('.o')
 source.write_text('.data\n.quad flat_bad_pointer\n.text\nflat_bad_pointer: ret\n')
 subprocess.run(['clang','--target=aarch64-none-elf','-c',str(source),'-o',str(obj)],check=True)
 probe=subprocess.run([sys.executable,__file__,str(obj)],text=True,capture_output=True)
 assert probe.returncode!=0 and 'R_AARCH64_ABS64' in probe.stdout,probe
 Path('build/kernel-relocations-negative.json').write_text(json.dumps({'passed':True,'expected_absolute_pointer_refusal':True,'output':probe.stdout+probe.stderr},indent=2)+'\n')
 print('Flat kernel: deliberately absolute pointer rejected');raise SystemExit(0)
paths=sys.argv[1:] or [str(p) for root in ['kernel','drivers','desktop','installer','filesystem','boot'] for p in Path('build',root).rglob('*.o')]
hits=[];header_offsets=0
for path in paths:
 section=''
 for line in subprocess.check_output(['llvm-readelf','-r',path],text=True).splitlines():
  if line.startswith('Relocation section'):section=line.split("'")[1]
  if re.search(r'\bR_AARCH64_ABS(?:64|32|16)\b',line):
   # The custom image header intentionally stores link-time sizes/offsets,
   # rather than pointers. Permit only these two exact header fields.
   fields=line.split()
   header=path.endswith('/arch/arm64/entry.o') and section=='.rela.text.entry' and ((fields[0]=='0000000000000018' and fields[4]=='__file_end') or (fields[0]=='0000000000000020' and fields[4]=='__image_end')) and fields[-1]=='0'
   if header:header_offsets+=1
   else:hits.append({'object':path,'section':section,'relocation':line.strip()})
if hits:print(json.dumps(hits,indent=2));raise SystemExit('Unsupported absolute relocation in flat kernel')
Path('build/kernel-relocations.json').write_text(json.dumps({'passed':True,'objects':len(paths),'absolute_address_relocations':0,'permitted_image_header_offsets':header_offsets},indent=2)+'\n')
print(f'Flat kernel: {len(paths)} objects checked; no absolute address relocations')
