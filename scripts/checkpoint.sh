#!/bin/bash
# One Linux-host workflow: validate, sanitize, reproduce, package. No publishing
# credentials or host block devices are used. Commit source changes first.
set -euo pipefail
cd "$(dirname "$0")/.."
git diff --quiet
git diff --cached --quiet
mkdir -p build
./scripts/container.sh bash scripts/validate.sh > build/checkpoint-validation.log 2>&1
bash scripts/sanitize.sh > build/checkpoint-sanitizers.log 2>&1
chos_reproduce_dir=$(mktemp -d build/reproduce-checkpoint-XXXXXX)
python3 - "$chos_reproduce_dir" <<'PY'
import io,subprocess,sys,tarfile
files=[p for p in subprocess.check_output(['git','ls-files'],text=True).splitlines() if not p.startswith(('artifacts/','docs/evidence/'))]
with tarfile.open(fileobj=io.BytesIO(subprocess.check_output(['git','archive','HEAD',*files]))) as archive:archive.extractall(sys.argv[1],filter='data')
PY
./scripts/container.sh bash -c 'cd "$1"; make -j4 iso diagnostic-iso input-iso' _ "$chos_reproduce_dir" > build/checkpoint-reproduction.log 2>&1
for chos_image in CHOS-ARM64.iso CHOS-ARM64-M7I-INPUT.iso CHOS-ARM64-M7C.iso; do
    cmp "build/$chos_image" "$chos_reproduce_dir/build/$chos_image"
done
python3 - "$chos_reproduce_dir" <<'PY'
import hashlib,json,shutil,subprocess,sys
from pathlib import Path
out=Path('build/release');out.mkdir(exist_ok=True)
images=[('CHOS-ARM64.iso','CHOS-ARM64-M8.iso'),('CHOS-ARM64-M7I-INPUT.iso','CHOS-ARM64-M8-INPUT.iso'),('CHOS-ARM64-M7C.iso','CHOS-ARM64-M8-DIAGNOSTIC.iso')]
sums=[]
for src,name in images:
 p=Path('build')/src;target=out/name;shutil.copyfile(p,target);sums.append(hashlib.sha256(target.read_bytes()).hexdigest()+'  '+name+'\n')
(out/'SHA256SUMS').write_text(''.join(sums))
Path('build/checkpoint-reproduction.json').write_text(json.dumps({'passed':True,'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'clean_source_directory':sys.argv[1],'all_three_images_byte_identical':True},indent=2)+'\n')
PY
printf 'Checkpoint passed: full QEMU suite, 20 portable test programs, sanitizers and three clean-source image reproductions. Files: build/release/\n'
