SHELL := /bin/bash
CC := clang
CFLAGS := -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-stack-protector -Iinclude
KFLAGS := $(CFLAGS) --target=aarch64-none-elf -march=armv8-a -mgeneral-regs-only -ffixed-x18 -fpie -fvisibility=hidden -include include/kernel_visibility.h -mno-outline-atomics
SOURCE_DATE_EPOCH ?= 1767225600
export SOURCE_DATE_EPOCH
.PHONY: all iso test clean
all: iso
build:
	mkdir -p build
build/boot.obj: boot/uefi/main.c boot/uefi/efi.h include/boot.h include/display_mode.h | build
	$(CC) $(CFLAGS) --target=aarch64-pc-windows-msvc -c $< -o $@
build/BOOTAA64.EFI: build/boot.obj
	lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /machine:arm64 /timestamp:0 /out:$@ $<
KSOURCES := kernel/arch/arm64/hardware.c kernel/config/format.c kernel/config/store.c installer/inventory.c installer/policy.c installer/service.c installer/gpt.c installer/fat32.c filesystem/ram.c kernel/arch/arm64/power.c desktop/desktop.c desktop/window-manager/model.c desktop/graphics/render.c desktop/graphics/primitives.c desktop/graphics/fontcache.c desktop/graphics/font.c drivers/input/virtio.c drivers/input/events.c filesystem/vfs.c filesystem/chfs.c drivers/storage/nvme.c kernel/storage_test.c kernel/runtime.c kernel/main.c kernel/console.c kernel/process/elf.c kernel/process/process.c kernel/syscall/syscall.c kernel/arch/arm64/acpi.c kernel/memory/physical.c kernel/memory/heap.c kernel/memory/virtual.c kernel/interrupts/exception.c kernel/scheduler/scheduler.c drivers/interrupts/gicv3.c drivers/interrupts/gicv2_timer.c
KASM := desktop/graphics/assets.S kernel/arch/arm64/entry.S kernel/arch/arm64/vectors.S kernel/arch/arm64/mmu.S kernel/arch/arm64/test_worker.S kernel/process/image.S
KOBJECTS := $(patsubst %.c,build/%.o,$(KSOURCES)) $(patsubst %.S,build/%.o,$(KASM))
build/%.o: %.c $(wildcard include/*.h) Makefile
	mkdir -p $(dir $@)
	$(CC) $(KFLAGS) -c $< -o $@
build/%.o: %.S Makefile
	mkdir -p $(dir $@)
	$(CC) $(KFLAGS) -c $< -o $@
build/kernel.elf: $(KOBJECTS) kernel/arch/arm64/kernel.ld
	ld.lld -T kernel/arch/arm64/kernel.ld --no-undefined -o $@ $(KOBJECTS)
.PHONY: test-memory
test-memory: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/memory.c kernel/memory/physical.c kernel/memory/heap.c -o build/test-memory
	./build/test-memory
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/acpi.c kernel/arch/arm64/acpi.c -o build/test-acpi
	./build/test-acpi
build/KERNEL.BIN: build/kernel.elf
	llvm-objcopy -O binary $< $@
iso: build/BOOTAA64.EFI build/KERNEL.BIN
	python3 scripts/mkiso.py

test: iso
	python3 tests/boot.py
clean:
	rm -rf build

build/user-main.o: userspace/init/main.c include/abi.h Makefile | build
	$(CC) $(CFLAGS) --target=aarch64-none-elf -march=armv8-a -mgeneral-regs-only -ffixed-x18 -mno-outline-atomics -c $< -o $@
build/user-entry.o: userspace/init/entry.S | build
	$(CC) --target=aarch64-none-elf -c $< -o $@
build/init.elf: build/user-main.o build/user-entry.o userspace/init/user.ld
	ld.lld -T userspace/init/user.ld -z max-page-size=4096 --no-undefined -o $@ build/user-entry.o build/user-main.o
build/kernel/process/image.o: build/init.elf build/desktop.elf build/BOOTAA64.EFI LICENSE
.PHONY: test-elf
test-elf: build/init.elf build/desktop.elf
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/elf.c kernel/process/elf.c -o build/test-elf
	./build/test-elf build/init.elf
	./build/test-elf build/desktop.elf

.PHONY: test-filesystem
test-filesystem: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/filesystem.c filesystem/chfs.c -o build/test-filesystem
	./build/test-filesystem

.PHONY: test-input
test-input: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/input.c drivers/input/events.c -o build/test-input
	./build/test-input

ASOURCES := userspace/libraries/gui/widgets.c applications/setup/main.c applications/common/runtime.c applications/desktop/main.c applications/explorer/main.c applications/notepad/main.c applications/terminal/main.c applications/control-panel/main.c applications/task-manager/main.c applications/about/main.c
AOBJECTS := $(patsubst %.c,build/app/%.o,$(ASOURCES))
build/app/%.o: %.c $(wildcard include/*.h) applications/common/app.h Makefile
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iapplications/common --target=aarch64-none-elf -march=armv8-a -mgeneral-regs-only -ffixed-x18 -mno-outline-atomics -c $< -o $@
build/app/runtime.o: kernel/runtime.c Makefile | build
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) --target=aarch64-none-elf -march=armv8-a -mgeneral-regs-only -c $< -o $@
build/desktop.elf: $(AOBJECTS) build/app/runtime.o build/user-entry.o userspace/init/user.ld
	ld.lld -T userspace/init/user.ld -z max-page-size=4096 --no-undefined -o $@ build/user-entry.o $(AOBJECTS) build/app/runtime.o

.PHONY: test-desktop
test-desktop: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/desktop.c desktop/window-manager/model.c desktop/graphics/render.c desktop/graphics/primitives.c desktop/graphics/fontcache.c desktop/graphics/font.c desktop/graphics/assets.S -o build/test-desktop
	./build/test-desktop

build/mkdisk: scripts/mkdisk.c installer/gpt.c installer/fat32.c include/install.h include/filesystem.h | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude scripts/mkdisk.c installer/gpt.c installer/fat32.c -o $@
.PHONY: test-installer
test-installer: build/mkdisk build/BOOTAA64.EFI build/KERNEL.BIN
	mkdir -p build/installer
	$(CC) $(KFLAGS) -c installer/gpt.c -o build/installer/gpt.o
	$(CC) $(KFLAGS) -c installer/fat32.c -o build/installer/fat32.o
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/installer.c installer/gpt.c -o build/test-installer
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/fat32.c installer/gpt.c installer/fat32.c -o build/test-fat32
	./build/test-fat32
	python3 tests/installer.py
	python3 tests/fat32.py

.PHONY: test-setup
test-setup: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/setup.c installer/policy.c -o build/test-setup
	./build/test-setup

build/desktop/graphics/assets.o: assets/royale/fonts.bin assets/fonts/OFL.txt

.PHONY: test-graphics
test-graphics: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/graphics.c desktop/graphics/render.c desktop/graphics/primitives.c desktop/graphics/fontcache.c desktop/graphics/assets.S -o build/test-graphics
	./build/test-graphics

.PHONY: test-config
test-config: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/config.c kernel/config/format.c kernel/config/store.c filesystem/chfs.c -o build/test-config
	./build/test-config

.PHONY: test-gui
test-gui: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/gui.c userspace/libraries/gui/widgets.c -o build/test-gui
	./build/test-gui

.PHONY: test-inventory
test-inventory: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/inventory.c installer/inventory.c installer/gpt.c -o build/test-inventory
	./build/test-inventory

.PHONY: test-gic
test-gic: | build
	$(CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/gic.c drivers/interrupts/gicv3.c -o build/test-gic
	./build/test-gic
