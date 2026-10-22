SHELL := /bin/bash
CC := clang
CFLAGS := -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-stack-protector -Iinclude
KFLAGS := $(CFLAGS) --target=aarch64-none-elf -march=armv8-a -mgeneral-regs-only -ffixed-x18 -fpie -mno-outline-atomics
SOURCE_DATE_EPOCH ?= 1767225600
export SOURCE_DATE_EPOCH
.PHONY: all iso test clean
all: iso
build:
	mkdir -p build
build/boot.obj: boot/uefi/main.c boot/uefi/efi.h include/boot.h | build
	$(CC) $(CFLAGS) --target=aarch64-pc-windows-msvc -c $< -o $@
build/BOOTAA64.EFI: build/boot.obj
	lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /machine:arm64 /timestamp:0 /out:$@ $<
build/entry.o: kernel/arch/arm64/entry.S | build
	$(CC) $(KFLAGS) -c $< -o $@
build/kernel.o: kernel/main.c include/boot.h | build
	$(CC) $(KFLAGS) -c $< -o $@
build/kernel.elf: build/entry.o build/kernel.o kernel/arch/arm64/kernel.ld
	ld.lld -T kernel/arch/arm64/kernel.ld --no-undefined -o $@ build/entry.o build/kernel.o
build/KERNEL.BIN: build/kernel.elf
	llvm-objcopy -O binary $< $@
iso: build/BOOTAA64.EFI build/KERNEL.BIN
	python3 scripts/mkiso.py

test: iso
	python3 tests/boot.py
clean:
	rm -rf build
