#!/bin/bash
# Host-only validation for original portable kernel modules.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/sanitized
flags=(-std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude)
cc "${flags[@]}" tests/memory.c kernel/memory/physical.c kernel/memory/heap.c -o build/sanitized/memory
cc "${flags[@]}" tests/acpi.c kernel/arch/arm64/acpi.c -o build/sanitized/acpi
cc "${flags[@]}" tests/elf.c kernel/process/elf.c -o build/sanitized/elf
cc "${flags[@]}" tests/filesystem.c filesystem/chfs.c -o build/sanitized/filesystem
cc "${flags[@]}" tests/input.c drivers/input/events.c -o build/sanitized/input
cc "${flags[@]}" tests/desktop.c desktop/window-manager/model.c desktop/graphics/render.c desktop/graphics/font.c desktop/graphics/primitives.c desktop/graphics/fontcache.c desktop/graphics/assets.S -o build/sanitized/desktop
for test in memory acpi filesystem input desktop; do "build/sanitized/$test"; done
build/sanitized/elf build/init.elf
build/sanitized/elf build/desktop.elf

cc "${flags[@]}" tests/installer.c installer/gpt.c -o build/sanitized/installer
cc "${flags[@]}" tests/fat32.c installer/gpt.c installer/fat32.c -o build/sanitized/fat32
build/sanitized/fat32
gpt_test_dir=$(mktemp -d "$PWD/build/gpt-sanitized-XXXXXX")
build/sanitized/installer "$gpt_test_dir/disk"

cc "${flags[@]}" tests/setup.c installer/policy.c -o build/sanitized/setup
build/sanitized/setup

cc "${flags[@]}" tests/graphics.c desktop/graphics/render.c desktop/graphics/primitives.c desktop/graphics/fontcache.c desktop/graphics/assets.S -o build/sanitized/graphics
build/sanitized/graphics

cc "${flags[@]}" tests/config.c kernel/config/format.c kernel/config/store.c filesystem/chfs.c -o build/sanitized/config
build/sanitized/config
cc "${flags[@]}" tests/gui.c userspace/libraries/gui/widgets.c -o build/sanitized/gui
build/sanitized/gui

cc "${flags[@]}" tests/inventory.c installer/inventory.c installer/gpt.c -o build/sanitized/inventory
build/sanitized/inventory

cc "${flags[@]}" tests/gic.c drivers/interrupts/gicv3.c -o build/sanitized/gic
build/sanitized/gic

cc "${flags[@]}" tests/diagnostic.c kernel/diagnostics/buffer.c -o build/sanitized/diagnostic
build/sanitized/diagnostic
cc "${flags[@]}" tests/usb.c drivers/usb/hid.c drivers/usb/ring.c -o build/sanitized/usb
build/sanitized/usb

cc "${flags[@]}" tests/image.c boot/image.c -o build/sanitized/image
build/sanitized/image

cc "${flags[@]}" -DCHOS_MMU_TEST tests/mmu.c kernel/memory/virtual.c kernel/memory/physical.c -o build/sanitized/mmu
build/sanitized/mmu

cc "${flags[@]}" tests/pci.c drivers/pci/discover.c drivers/pci/resource.c -o build/sanitized/pci
build/sanitized/pci
cc "${flags[@]}" tests/nvme_format.c drivers/storage/nvme_format.c -o build/sanitized/nvme
build/sanitized/nvme
