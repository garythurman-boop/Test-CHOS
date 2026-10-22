#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
make iso
sha256sum build/CHOS-ARM64.iso > build/hash-1
make iso
sha256sum build/CHOS-ARM64.iso > build/hash-2
cmp build/hash-1 build/hash-2
xorriso -indev build/CHOS-ARM64.iso -report_el_torito plain > build/iso-inspection.log 2>&1
llvm-readobj --file-headers build/BOOTAA64.EFI > build/efi-inspection.log
llvm-readelf -h -r build/kernel.elf > build/kernel-inspection.log
python3 tests/boot.py --ram 512
python3 tests/boot.py --ram 1024
CHOS_TEST_MISSING_KERNEL=1 python3 scripts/mkiso.py
python3 tests/boot.py --iso build/CHOS-missing-kernel.iso --expect-missing-kernel
# Leave the staging image consistent with the distribution ISO.
make iso
sha256sum build/CHOS-ARM64.iso > build/SHA256SUMS
printf 'All three QEMU tests and ISO repeatability check passed.\n'
