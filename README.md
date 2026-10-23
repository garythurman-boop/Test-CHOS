# CHOS

CHOS is an original C/AArch64 operating system under development. A native UEFI
loader boots its own kernel, with no Linux/BSD kernel or GRUB in the guest. Debian
is used only as a build/test host. No Microsoft code, artwork or fonts are used.

The current checkpoint boots an original graphical desktop and six native EL0
applications: Explorer, Notepad, Terminal, Control Panel, Task Manager and About.
Protected memory, preemptive scheduling, ELF/syscalls/IPC, NVMe/CHFS and Virtio
input are implemented and tested. **Live ISO files use volatile RAM. Installed sessions use a persistent CHFS
system volume.** Native Setup writes a selected NVMe disk only after explicit
identity/capacity review and typed destructive confirmation.

**CHOS Royale** adds anti-aliased text, original wallpaper/icons, rounded blue
window frames, a two-column Start menu and clipped pointer repainting. Firmware
mode selection prefers supported 1024×768; installed-volume failures show an
explicit temporary-files warning. See [real screenshots and design](docs/royale.md).

**VMware Fusion compatibility remains unverified.** GICv3, USB/xHCI input and EL2
entry are unsupported. A native installation core and disk reboot are tested under QEMU; account/password
and timezone/layout/mode setup, networking and CHOS 0.1 remain unfinished.

## Download testing images

- [Current Royale/native installation ISO](artifacts/milestone-6c/CHOS-ARM64.iso).
- [Previous native installer checkpoint](artifacts/milestone-6b/CHOS-ARM64.iso).
- [Earlier live desktop/application ISO](artifacts/milestone-5/CHOS-ARM64.iso).
- [Desktop core checkpoint](artifacts/milestone-4/CHOS-ARM64.iso).
- [Earlier milestone 3 ISO](artifacts/milestone-3/CHOS-ARM64.iso): ordinary boot
  diagnostics and keyboard/mouse input; **no disk writes**.
- [Milestone 2 core ISO](artifacts/milestone-2/CHOS-ARM64.iso): original isolated
  process/syscall/IPC checkpoint.
- [Older M2A ISO](artifacts/milestone-2a/CHOS-ARM64.iso).

On GitHub choose **Download raw file**, then verify the adjacent SHA256SUMS.
Read [current image notes](artifacts/milestone-6c/README.md) before testing. The
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
runs original acceptance and application programs embedded in the kernel.

Validation runs ten host C test programs (both ELF payloads) and twenty-four real
AArch64 UEFI QEMU cases, including 512 MiB/1 GiB boots, protection/IPC, keyboard/
mouse window and application interactions, native restart/shutdown, three cold
NVMe/CHFS boots, two GPT/ESP disk-only boots and expected error/safety cases.
Native graphical installs and two installed cold boots per sector geometry
verify persistent file saves and self-erasure refusal. Independent Python code
checks disk CRCs/content and original GPT/FAT32 payload
formats. ISO and builder-created disk variants are checked for reproducibility. Tests use
only disposable image files, never host block devices. Additional Royale tests
check actual UEFI mode selection/fallback, Start-menu pointer launch, partial-paint
telemetry and a corrupt installed-volume warning with an unchanged disk hash.
Evidence: docs/evidence/m6c.
`bash scripts/sanitize.sh` additionally runs the portable modules with ASan/UBSan
when host GCC/Clang supports them.

The pinned base image uses current APT package versions; bit-identical ISO builds
are verified within the recorded toolchain, not arbitrary future package updates.
SOURCE_DATE_EPOCH defaults to 2026-01-01 UTC. Build/test containers need no network
after toolchain installation. Tests start/stop their own QEMU processes.

## Documentation

- [Acceptance criteria and evidence](docs/acceptance.md)
- [Native installation results and limits](docs/milestone-6b.md)
- [Installer-format foundations and disk boot](docs/milestone-6a.md)
- [Desktop/application sequence results](docs/milestone-5.md)
- [Desktop/application controls and limits](docs/desktop.md)
- [Milestone 2 process results](docs/milestone-2.md)
- [Architecture](docs/architecture.md), [syscalls](docs/syscalls.md), [storage](docs/storage.md)
- [Feature status and known issues](docs/status.md)
- [VMware Fusion investigation/test plan](docs/vmware-fusion.md)
- [Release notes](docs/release-notes.md)

Source is organized into boot/uefi, kernel/{arch,memory,process,scheduler,interrupts,
syscall}, drivers/{storage,input,interrupts}, filesystem, userspace/init, include,
scripts, tests, desktop and applications.
