# VMware Fusion on Apple Silicon: compatibility investigation

**Not verified. No Fusion/Apple Silicon host is attached to this workspace.**
This project has booted only on QEMU's ARM virt machine. Current core requires
EL1 entry, ACPI 2.x, GICv2 and a GTDT timer; GICv3 is explicitly rejected. These
requirements must be checked on Fusion before claiming compatibility. Current images contain real kernel/process/file/input tests followed by a native
desktop, applications and a native installer tested in QEMU. Fusion installation
is still unverified. Use disposable virtual disks only.

## Evidence and open questions

The ISO uses the standard UEFI optical EFI boot catalog, an embedded FAT volume,
and EFI/BOOT/BOOTAA64.EFI with ARM64 PE machine 0xAA64. These are the standards-based
boot requirements; they do not establish acceptance by a particular Fusion version.

References for subsequent verification:

- UEFI specification, ARM/AArch64 binding, removable media boot and GOP protocols:
  https://uefi.org/specifications
- QEMU ARM virt device documentation (the tested machine, not Fusion):
  https://www.qemu.org/docs/master/system/arm/virt.html
- Broadcom Fusion Apple Silicon support knowledge base:
  https://knowledge.broadcom.com/external/article/315602
  Retrieval from this workspace returned HTTP 403 on 2026-10-22. Its current text
  was not available for verification; do not treat a remembered hardware list as
  an authoritative current Fusion device inventory.

No Fusion runtime driver has been implemented. The following must be inventoried
on the actual target release using firmware ACPI tables, PCI IDs/class codes,
USB descriptors and the VM configuration. Never substitute QEMU addresses or
x86 VMware device assumptions:

| Component | M1 approach | Fusion evidence still required |
|---|---|---|
| Firmware/CPU/RAM | native UEFI handoff, CurrentEL, memory map | custom ARM64 VM boot path, EL, GOP, ACPI topology |
| Interrupts/timer | M2A: ACPI-discovered GICv2 and physical timer | GIC revision, interrupt routing, timer resources |
| Optical boot | firmware filesystem on El Torito FAT | ISO detection and removable-path execution |
| Storage | original NVMe polling driver; CHFS tested in QEMU | actual PCI controller IDs, NVMe/SCSI capabilities |
| Display | firmware GOP linear RGB/BGR only | GOP format, mapping lifetime and framebuffer writes |
| Keyboard/pointer | Virtio PCI on QEMU; USB/xHCI unsupported | virtual USB/controller IDs and report protocols |
| Network | none | adapter PCI IDs, required original driver |
| Multiple CPUs | M2A: boot CPU and kernel threads only | PSCI/ACPI CPU startup mechanism |

## Dedicated Fusion validation procedure

1. Record Apple SoC, macOS, Fusion exact version/build and guest configuration.
   Confirm in that release's UI/docs that an ARM64 custom/other OS UEFI optical
   boot workflow exists. If absent, record a blocker rather than selecting x86.
2. Create a disposable ARM64 guest using that supported workflow; start with
   1 GiB RAM and one CPU. Disable Secure Boot for this unsigned development image.
   Attach CHOS-ARM64.iso as a connected CD/DVD. Attach a disposable NVMe virtual disk for eventual installation testing; the
   native installer currently supports only one controller/namespace.
3. Start the VM, select optical media in firmware if necessary, and record whether
   the EFI catalog is found, BOOTAA64.EFI executes, kernel load succeeds and
   core-test completion followed by dedicated CHOS Setup appears (or record the explicit unsupported-platform error). Capture an actual screenshot and firmware
   logs. Record resolution, pixel format, EL and displayed memory count.
4. If optical boot fails, inspect the ISO catalog and firmware boot options. Try
   manually selecting EFI/BOOT/BOOTAA64.EFI from the firmware file browser, if the
   release provides one. This is a diagnostic, not proof automatic ISO boot works.
   If optical EFI boot is unsupported, investigate a standards-compliant GPT/FAT
   removable boot disk carrying the same ARM64 executable as a separate artifact.
5. Inventory devices before writing drivers. Then independently test input,
   framebuffer stability, block reads/writes on a disposable disk, interrupt/timer
   delivery, and multicore startup as those features become implemented.
6. For M6+, require installer disk identity/capacity and destructive confirmation,
   verify GPT/ESP/payload integrity, power off, detach ISO, and boot installed disk.
   Test file persistence across cold reboot and hashes after repeated writes.
7. Record pass/fail separately for ISO recognition, loader, kernel, input, display,
   storage, actual installation, and installed reboot. Also record 512 MiB boot,
   1 GiB desktop footprint and idle CPU only when the desktop exists.

Full verified user-facing Fusion instructions remain pending actual target tests.
If the kernel and input work on the recorded Fusion version, open CHOS Setup
(Meta-7), review the exact destination identity/capacity, accept the setup agreement,
enter a computer name and provide the typed erase confirmation. Wait for success,
detach the ISO and restart. Record every result; do not presume it works because
the corresponding QEMU sequence passed.

The latest ISO writes an installation destination only after explicit native
Setup confirmation. Ordinary live operation uses RAM. Its storage-test companion is
strictly for tests using a disposable NVMe image with the documented serial and
boot opt-in; do not treat it as an installer. QEMU's NVMe/Virtio device IDs and BAR
addresses are recorded in docs/evidence/m3/*.pci.log and are not a Fusion inventory.
Fusion may expose GICv3 or USB devices that are currently unsupported, so a clear
unsupported-platform error is a valid finding, not evidence the ISO installed.

## GPT/ESP fallback evidence (QEMU only)

Milestone 6A's original GPT/FAT32 encoders produce a standards-based EFI disk
carrying BOOTAA64.EFI and the original kernel. QEMU UEFI boots the desktop from
both 512-byte and 4 KiB logical-sector NVMe fixtures, without optical media.
This is evidence for the disk layout/firmware path, not Fusion support or a guest
installation. Fusion disk-container conversion/import and its ARM64 virtual
hardware still require an actual Fusion test. The supplied raw test fixture is
not a validated Fusion VMDK and must not be described as one.

## Dedicated Setup iteration

The current ISO boots Setup directly; L/F3 selects the Live Desktop. Installed
boot opens the regular Royale desktop. Fusion acceptance must independently
exercise blue-page keyboard routing (including F3), destination identity/capacity,
explicit ERASE, graphical progress, real write/flush/read-back failures, completion
Restart, disk-only boot and persisted files/settings. These require actual target
input/storage drivers; QEMU Virtio input support is not a Fusion USB implementation.
The layout minimum is 69 MiB + 20 KiB for 512-byte sectors or 277 MiB + 20 KiB
for 4096-byte sectors. Use a larger disposable target (e.g. 1 GiB) for manual tests;
CHFS currently exposes at most its first 4 MiB. See setup-environment.md.
