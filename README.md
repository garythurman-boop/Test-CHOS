# CHOS

CHOS is an original C/AArch64 operating system under development. A native UEFI
loader boots its own kernel, with no Linux/BSD kernel or GRUB in the guest. Debian
is used only as a build/test host. No Microsoft code, artwork or fonts are used.

Milestones 1–2 core and milestone 3's **tested QEMU hardware/storage/input subset**
are implemented: exceptions, protected page tables, physical/heap memory,
preemptive threads, isolated EL0 ELF processes, syscalls/IPC, NVMe, original CHFS
persistent file operations and Virtio PCI keyboard/mouse input with framebuffer
cursor/character output. This is not yet a desktop or installable operating system.

**VMware Fusion compatibility remains unverified.** GICv3, USB/xHCI input and EL2
entry are unsupported. No GPT/ESP installer, installed-disk boot, window manager,
built-in desktop applications, networking or CHOS 0.1 release is delivered yet.

## Download testing images

- [Current milestone 3 ISO](artifacts/milestone-3/CHOS-ARM64.iso): ordinary boot
  diagnostics and keyboard/mouse input; **no disk writes**.
- [Milestone 2 core ISO](artifacts/milestone-2/CHOS-ARM64.iso): original isolated
  process/syscall/IPC checkpoint.
- [Older M2A ISO](artifacts/milestone-2a/CHOS-ARM64.iso).

On GitHub choose **Download raw file**, then verify the adjacent SHA256SUMS.
Read [current image notes](artifacts/milestone-3/README.md) before testing. The
separate STORAGE-TEST ISO is for the automated disposable-disk tests only, with
an explicit boot-volume opt-in and exact device identity gate. It is not an installer.

## Build and validate

Docker Engine with bind mounts is required. The Dockerfile lists packages for a
native Linux toolchain alternative.

```sh
./scripts/container.sh image
./scripts/container.sh make
./scripts/container.sh bash scripts/validate.sh
```

`build/CHOS-ARM64.iso` contains EFI/BOOT/BOOTAA64.EFI and CHOS/KERNEL.BIN on an
El Torito FAT boot volume. Secure Boot must be off. The FAT volume is not an
installed ESP. `kernel.elf` is a debug artifact; the separate static ELF loader
runs original acceptance programs embedded in the kernel.

Validation runs five host C test programs and nine real AArch64 UEFI QEMU cases,
including 512 MiB/1 GiB boots, EL0 protection and IPC, real keyboard/mouse events,
three fresh VMs verifying NVMe/CHFS persistence, and expected error/safety cases.
An independent Python parser checks actual disk metadata/data CRCs and byte content.
Two normal and two storage-test ISO builds are compared for repeatability. Tests
create disposable image files under build/ and never attach host block devices.
Evidence is committed in docs/evidence/m3; generated outputs remain ignored.

The pinned base image uses current APT package versions; bit-identical ISO builds
are verified within the recorded toolchain, not arbitrary future package updates.
SOURCE_DATE_EPOCH defaults to 2026-01-01 UTC. Build/test containers need no network
after toolchain installation. Tests start/stop their own QEMU processes.

## Documentation

- [Current sequence results and next milestone](docs/milestone-3.md)
- [Milestone 2 process results](docs/milestone-2.md)
- [Architecture](docs/architecture.md), [syscalls](docs/syscalls.md), [storage](docs/storage.md)
- [Feature status and known issues](docs/status.md)
- [VMware Fusion investigation/test plan](docs/vmware-fusion.md)
- [Release notes](docs/release-notes.md)

Source is organized into boot/uefi, kernel/{arch,memory,process,scheduler,interrupts,
syscall}, drivers/{storage,input,interrupts}, filesystem, userspace/init, include,
scripts and tests. Application/desktop directories will be added with working code.
