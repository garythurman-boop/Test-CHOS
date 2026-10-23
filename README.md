# CHOS

CHOS is an original C/AArch64 operating system under development. A native UEFI
loader boots its own kernel, with no Linux/BSD kernel or GRUB in the guest. Debian
is used only as a build/test host. No Microsoft code, artwork or fonts are used.

The current optical image boots dedicated full-screen CHOS Setup. L/F3 selects
the original Live Desktop; installed disks open the desktop directly. Six native
EL0 applications remain available: Explorer, Notepad, Terminal, Control Panel, Task Manager and About.
Protected memory, preemptive scheduling, ELF/syscalls/IPC, NVMe/CHFS and Virtio
input are implemented and tested. **Live ISO files use volatile RAM. Installed sessions use a persistent CHFS
system volume.** Native Setup writes a selected NVMe disk only after explicit
identity/capacity review and typed destructive confirmation.

**CHOS Royale** retains its approved blue design and now uses a shared native C
widget library. Explorer, Notepad, Control Panel and Setup share real controls,
keyboard focus, menus and dialogs. Installed computer/theme/clock settings persist;
invalid configuration is rejected before writing. Normal installed sessions have
no permanent session overlay. See [dedicated Setup results and actual screenshots](docs/milestone-6d-setup.md).
Blue keyboard-driven disk selection and typed confirmation lead to native Royale
configuration/progress/completion. Progress follows real operations and copied
bytes. Original GPT/FAT32/CHFS writers verify the files and installed boot path.

**VMware Fusion compatibility remains unverified.** Original EL2→EL1 startup and
ACPI GICv3 now pass QEMU tests. USB host/HID input remains unsupported. Actual Fusion
device inventory and target validation are pending. Native installation and disk
reboot are tested under QEMU; account/password
and full time-zone/layout/mode setup, networking and CHOS 0.1 remain unfinished.

## Download testing images

- [Current M7A/7B boot/interrupt compatibility ISO](artifacts/milestone-7a-b/CHOS-ARM64.iso).
- [Preserved dedicated full-screen Setup ISO](artifacts/milestone-6d-setup/CHOS-ARM64.iso).
- [Preserved saved-configuration/widget ISO](artifacts/milestone-6d/CHOS-ARM64.iso).
- [Previous Royale foundation ISO](artifacts/milestone-6c/CHOS-ARM64.iso).
- [Previous native installer checkpoint](artifacts/milestone-6b/CHOS-ARM64.iso).
- [Earlier live desktop/application ISO](artifacts/milestone-5/CHOS-ARM64.iso).
- [Desktop core checkpoint](artifacts/milestone-4/CHOS-ARM64.iso).
- [Earlier milestone 3 ISO](artifacts/milestone-3/CHOS-ARM64.iso): ordinary boot
  diagnostics and keyboard/mouse input; **no disk writes**.
- [Milestone 2 core ISO](artifacts/milestone-2/CHOS-ARM64.iso): original isolated
  process/syscall/IPC checkpoint.
- [Older M2A ISO](artifacts/milestone-2a/CHOS-ARM64.iso).

On GitHub choose **Download raw file**, then verify the adjacent SHA256SUMS.
Read [current image notes](artifacts/milestone-7a-b/README.md) before testing. The
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

Validation runs fourteen host C test programs (both ELF payloads) and forty-five real
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
Saved configuration tests verify native Save/shutdown/cold boot, invalid-save
no-write behavior, legacy loading and corrupt-record fallback. A real workspace
VM checks overlapping apps, menus/dialogs, file scrolling and bounded fast typing.
Dedicated Setup tests inject an actual NVMe write error, stop/retry an active
installation, boot a 70 MiB native installation, reject a too-small disk without
writes and verify full-screen configuration at 640×480. Desktop tests explicitly
choose Live Desktop from the ISO's Setup welcome page.
New tests cover real UEFI EL1/EL2 with GICv2/v3, native EL2/GICv3 installation
and two disk-only boots for each sector geometry, ACPI-off refusal and a read-only
native hardware query checked against QMP PCI inventory. Evidence: docs/evidence/m7ab.
`bash scripts/sanitize.sh` additionally runs the portable modules with ASan/UBSan
when host GCC/Clang supports them.

The pinned base image uses current APT package versions; bit-identical ISO builds
are verified within the recorded toolchain, not arbitrary future package updates.
SOURCE_DATE_EPOCH defaults to 2026-01-01 UTC. Build/test containers need no network
after toolchain installation. Tests start/stop their own QEMU processes.

## Documentation

- [Acceptance criteria and evidence](docs/acceptance.md)
- [Dedicated installation environment and safety](docs/setup-environment.md)
- [Current M7A/7B iteration results](docs/milestone-7a-b.md)
- [Account architecture and prerequisites](docs/accounts-design.md)
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
