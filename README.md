# CHOS

CHOS is an original C/AArch64 operating system under development. A native UEFI
loader boots its own kernel, with no Linux/BSD kernel or GRUB in the guest. Debian
is used only as a build/test host. No Microsoft code, artwork or fonts are used.

The current optical image boots dedicated full-screen CHOS Setup. L/F3 selects
the original Live Desktop; installed disks resume pending graphical configuration
or open the desktop directly. Six native
EL0 applications remain available: Explorer, Notepad, Terminal, Control Panel, Task Manager and About.
Protected memory, preemptive scheduling, ELF/syscalls/IPC, NVMe/CHFS, Virtio
input and a bounded original xHCI boot-HID input path are implemented and tested
under QEMU. USB completion is polled; USB interrupt routing is unfinished. **Live ISO files use volatile RAM. Installed sessions use a persistent CHFS
system volume.** Native Setup writes a selected NVMe disk only after explicit
identity/capacity review and typed destructive confirmation.

**CHOS Royale** retains its approved blue design and now uses a shared native C
widget library. Explorer, Notepad, Control Panel and Setup share real controls,
keyboard focus, menus and dialogs. Installed computer/theme/clock settings persist;
invalid configuration is rejected before writing. Normal installed sessions have
no permanent session overlay. See [dedicated Setup results and actual screenshots](docs/milestone-6d-setup.md).
Blue keyboard-driven disk selection, layout review and exact ERASE confirmation
lead to actual blue-screen partitioning, copying, verification and flush. A real
restart with the ISO removed boots installed Royale configuration, Welcome, then
the persistent desktop. Original GPT/FAT32/CHFS writers verify the payload.

**Milestone 8 continues the working M7I Fusion input baseline.** The user confirms
normal ISO boot, Setup, keyboard, mouse/trackpad and Live Desktop on an M1 MacBook
Air. M8 corrects inconsistent NVMe serial padding and exposes precise guarded
installation errors; the actual Fusion installation failure needs a target retest.
It adds a larger original cursor, desktop selection/double-click/dragging, saved
icon positions, native context menus and firmware-enumerated reboot resolution
trials. It preserves the original USB2 hub/report-HID/absolute-pointer paths and
automatic read-only input fallback. `bash scripts/checkpoint.sh` validates,
sanitizes, reproduces and packages images into `build/release/`.
See [M8 scope and validation](docs/milestone-8.md), [Fusion instructions](docs/fusion-first-boot.md)
and [roadmap](docs/roadmap.md). Authentication/ACLs, full regional setup, runtime
mode switching, networking, SMP and CHOS 0.1 remain unfinished.

M7D replaces the fixed 512 KiB bootstrap ceiling with a validated versioned kernel
image and a separately allocated 64 KiB guarded stack. Controlled large-file/BSS
boots, installation and malformed-image refusal are covered. [Memory/image design](docs/kernel-image.md).

The separate diagnostic image automatically displays firmware/ACPI/PCI/driver
inventory without keyboard/mouse input and never enters Setup or writes a report
to disk. [First Fusion evaluation instructions](docs/fusion-first-boot.md).

## Download testing images

- **[Current M8 development Setup/Live Desktop ISO](https://raw.githubusercontent.com/garythurman-boop/Test-CHOS/main/artifacts/milestone-8/CHOS-ARM64-M8.iso)** — guarded two-phase installation, desktop interaction and resolution trials. [Report/screenshots](docs/milestone-8.md) · [Checksums](artifacts/milestone-8/SHA256SUMS).
- **[Known-good M7I Setup/Live Desktop backup](artifacts/milestone-7i/CHOS-ARM64.iso)** — normal startup plus automatic read-only input-failure diagnostics.
- [Optional always-on M7I input test ISO](artifacts/milestone-7i/CHOS-ARM64-M7I-INPUT.iso).
- [Preserved full-page M7I diagnostic ISO](artifacts/milestone-7i/CHOS-ARM64-M7I-DIAGNOSTIC.iso).
- [Preserved noninteractive M7C diagnostic ISO](artifacts/milestone-7c/CHOS-ARM64-M7C.iso).
- [Preserved M7C Setup/Live Desktop ISO](artifacts/milestone-7c/CHOS-ARM64.iso).
- [Preserved M7A/7B boot/interrupt compatibility ISO](artifacts/milestone-7a-b/CHOS-ARM64.iso).
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
Read [current image notes](artifacts/milestone-7i/README.md) before testing. The
separate STORAGE-TEST ISO is for the automated disposable-disk tests only, with
an explicit boot-volume opt-in and exact device identity gate. It is not an installer.

## Build and validate

Docker Engine with bind mounts is required. The Dockerfile lists packages for a
native Linux toolchain alternative.

```sh
./scripts/container.sh image
./scripts/container.sh make
bash scripts/checkpoint.sh
```

`build/CHOS-ARM64.iso` contains EFI/BOOT/BOOTAA64.EFI and CHOS/KERNEL.BIN on an
El Torito FAT boot volume. Secure Boot must be off. The FAT volume is not an
installed ESP. `kernel.elf` is a debug artifact; the separate static ELF loader
runs original acceptance and application programs embedded in the kernel.

Validation runs twenty host C test programs (both ELF payloads) and eighty-three real
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
native hardware query checked against QMP PCI inventory. M7C adds input-free diagnostics, real USB-only native interactions/ring wrap/
disconnect, and unsupported xHCI/OHCI/non-boot-HID cases. Both ordinary and
diagnostic ISOs are checked for repeatability. M7D adds disk-free optional-driver
failure, mouse disconnect with a working keyboard, mixed-loader version rejection,
large image/BSS boot/install/disk reboot and three malformed-image refusals.
M7E adds 2/8 GiB UEFI boots, input-free 2 GiB discovery and an actual
framebuffer table-capacity refusal. The production mapper also runs host stress
tests with privileged operations stubbed. M7F adds actual production PCI routing/VMware-ID host tests, namespace/CAP parsing,
nonzero-bus input/read-only/normal Setup, failure panels, a preserved M7E discovery
regression and bridged native installation/two disk boots. Evidence: docs/evidence/m7f.
`bash scripts/sanitize.sh` additionally runs the portable modules with ASan/UBSan
when host GCC/Clang supports them.

The pinned base image uses current APT package versions; bit-identical ISO builds
are verified within the recorded toolchain, not arbitrary future package updates.
SOURCE_DATE_EPOCH defaults to 2026-01-01 UTC. Build/test containers need no network
after toolchain installation. Tests start/stop their own QEMU processes.

## Documentation

- [Acceptance criteria and evidence](docs/acceptance.md)
- [Dedicated installation environment and safety](docs/setup-environment.md)
- [Current M8 development checkpoint](docs/milestone-8.md)
- [Preserved M7I input baseline](docs/milestone-7i.md)
- [Preserved M7H USB descriptor/HID checkpoint](docs/milestone-7h.md)
- [Preserved M7G xHCI layout/diagnostic checkpoint](docs/milestone-7g.md)
- [Preserved M7F discovery/input/storage checkpoint](docs/milestone-7f.md)
- [Preserved M7E MMU hardening](docs/milestone-7e.md)
- [Preserved M7D hardening iteration](docs/milestone-7d.md)
- [Kernel image and bootstrap architecture](docs/kernel-image.md)
- [Previous M7C discovery/USB iteration](docs/milestone-7c.md)
- [USB architecture and limits](docs/usb.md)
- [Previous M7A/7B results](docs/milestone-7a-b.md)
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
