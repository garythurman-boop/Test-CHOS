#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
make test-memory test-elf test-filesystem test-input test-desktop test-setup test-graphics test-config test-gui test-inventory test-gic test-usb test-diagnostic test-image test-mmu
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
python3 tests/desktop.py
python3 tests/applications.py
python3 tests/royale.py
python3 tests/workspace.py
python3 tests/restart.py
python3 tests/boot.py --gic-version 3 --label boot-gic3-el1
python3 tests/boot.py --gic-version 2 --virtualization --label boot-gic2-el2
python3 tests/boot.py --gic-version 3 --virtualization --label boot-gic3-el2
python3 tests/boot.py --no-acpi --expect-platform-reject
CHOS_TEST_MISSING_KERNEL=1 python3 scripts/mkiso.py
python3 tests/boot.py --iso build/CHOS-missing-kernel.iso --expect-missing-kernel
# Leave the staging image consistent with the distribution ISO.
make iso
sha256sum build/CHOS-ARM64.iso > build/SHA256SUMS
CHOS_TEST_STORAGE=1 python3 scripts/mkiso.py
sha256sum build/CHOS-STORAGE-TEST.iso > build/storage-hash-1
CHOS_TEST_STORAGE=1 python3 scripts/mkiso.py
sha256sum build/CHOS-STORAGE-TEST.iso > build/storage-hash-2
cmp build/storage-hash-1 build/storage-hash-2
python3 tests/storage.py
make iso
make test-installer
python3 tests/diskboot.py
python3 tests/install.py --sector 512
python3 tests/install.py --sector 4096
python3 tests/install.py --sector 512 --gic-version 3 --virtualization
python3 tests/install.py --sector 4096 --gic-version 3 --virtualization
python3 tests/platform_probe.py
python3 tests/setup_flow.py
python3 tests/settings.py
python3 tests/volume_error.py
python3 tests/setup_mode.py
python3 tests/display_modes.py
make diagnostic-iso
sha256sum build/CHOS-ARM64-M7C.iso > build/diagnostic-hash-1
make diagnostic-iso
sha256sum build/CHOS-ARM64-M7C.iso > build/diagnostic-hash-2
cmp build/diagnostic-hash-1 build/diagnostic-hash-2
python3 tests/diagnostic_boot.py
python3 tests/usb_boot.py
python3 tests/usb_failures.py
python3 tests/hardening.py
python3 tests/growth.py
python3 tests/mmu_boot.py
printf 'Sixty-six QEMU cases, eighteen host programs (both ELF payloads checked), both ISO repeatability checks and independent disk verification passed.\n'
