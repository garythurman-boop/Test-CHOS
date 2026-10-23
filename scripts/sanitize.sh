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
cc "${flags[@]}" tests/desktop.c desktop/window-manager/model.c desktop/graphics/render.c desktop/graphics/font.c -o build/sanitized/desktop
for test in memory acpi filesystem input desktop; do "build/sanitized/$test"; done
build/sanitized/elf build/init.elf
build/sanitized/elf build/desktop.elf
