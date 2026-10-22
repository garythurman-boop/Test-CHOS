# CHOS

CHOS is an original C/AArch64 operating system project. **Milestone 1 is implemented:**
a native UEFI loader boots a freestanding original kernel from an ARM64 ISO.
This is a real firmware-to-kernel handoff, with no Linux/BSD kernel, GRUB, or host OS
in the guest boot path. Debian is used only as the build/test tool container.

This is not yet a usable desktop OS or installation image. There is no installer,
persistent filesystem, user mode, scheduler, input driver, or VMware validation.
The kernel displays diagnostics, then idles with interrupts masked.

## Build and test

Requirements: Docker Engine with bind mounts supported. On a Linux development
machine with the packages listed in Dockerfile installed, run `make` directly.

```sh
./scripts/container.sh image
./scripts/container.sh make
./scripts/container.sh bash scripts/validate.sh
```

Output: `build/CHOS-ARM64.iso` (ARM64 UEFI optical image), `BOOTAA64.EFI`,
`KERNEL.BIN`, and `kernel.elf` (debug/link inspection only). ISO boot volume contains
`EFI/BOOT/BOOTAA64.EFI` and `CHOS/KERNEL.BIN`. Boot it with Secure Boot disabled.
It never writes guest disks. The 16 MiB FAT boot image is not an installed ESP.

The test suite boots real AArch64 EDK II firmware on QEMU `virt`, with ramfb and a
SCSI CD-ROM, at 512 MiB and 1 GiB. It verifies actual kernel framebuffer output via
QMP and tests the loader's missing-kernel diagnostic. `build/*.json`, serial/QEMU
logs, and screenshots contain the evidence. It also compares two generated ISO
hashes. Run a clean rebuild with `./scripts/container.sh make clean` first if needed.

Builds run without container network access after the toolchain image is prepared.
The base image digest is pinned; APT package versions are not locked, so binary
reproducibility is demonstrated within the recorded toolchain, not across all
future Debian package updates. SOURCE_DATE_EPOCH defaults to 2026-01-01 UTC.

## Documentation

- [Architecture and boot contract](docs/architecture.md)
- [Feature status, known issues and next milestone](docs/status.md)
- [Validation evidence](docs/validation.md)
- [VMware Fusion compatibility and test procedure](docs/vmware-fusion.md)
- [Milestone release notes](docs/release-notes.md)

Source is organized into `boot/uefi`, `kernel/arch/arm64`, shared `include`,
`scripts`, and `tests`. Future subsystem directories will be added with their
implementations rather than empty or decorative components. No Microsoft assets
or fonts are used; the diagnostic glyphs are original.
