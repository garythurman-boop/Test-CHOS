# CHOS status and known issues

| Milestone | Status |
|---|---|
| 1: UEFI, original ARM64 kernel, bootable ISO | Verified under QEMU UEFI |
| 2: protected memory, exceptions/timer, scheduling, EL0 ELF/syscalls/IPC | Core implemented and tested |
| 3: display/input/NVMe/CHFS persistent operations | QEMU GOP + Virtio PCI + NVMe tested; three cold storage boots |
| 4: CHOS Desktop/window manager/taskbar/Start | Working core; Royale widgets and multi-window usage tested; dedicated Search/Help and wallpaper file loading remain |
| 5: six native applications | Functional bounded native apps with installed persistence and shared widgets; see desktop.md |
| 6: graphical installer/GPT/ESP/installed boot | Dedicated full-screen blue/Royale native setup, disk boot and persistent system volume; saved computer/theme/fixed clock offset implemented; account/password/layout and graphical mode configuration unfinished |
| 7: VMware Fusion validation | M7C adds automatic diagnostics and bounded original polled xHCI boot-HID tested in QEMU; actual Fusion and broader USB/interrupt support remain open |
| 8: CHOS 0.1 | Not ready |

Current ISO/results: [M7C diagnostics and USB](milestone-7c.md).
Preserved [M7A/7B boot and interrupts](milestone-7a-b.md).
Preserved [M6D dedicated Setup iteration](milestone-6d-setup.md).
[Setup flow and safety](setup-environment.md). Preserved [M6D widgets/settings checkpoint](milestone-6d.md). [Saved configuration](configuration.md), [native toolkit](gui-toolkit.md). Previous visual checkpoint: [milestone 6C](milestone-6c.md). [Royale visual architecture](royale.md). Previous installer core: [milestone 6B](milestone-6b.md). Earlier foundations: [milestone 6A](milestone-6a.md). Ordinary images
boot dedicated Setup after kernel tests. L/F3 starts Live Desktop; installed disks
start the regular desktop directly. Their live
CHFS volume is RAM backed. Native Setup requires explicit destructive confirmation.
Installed sessions mount the verified CHFS system partition and persist files.
The separately gated storage-test ISO verifies persistent NVMe/CHFS data but is
not an installer or desktop persistence environment.

## Limits

- One CPU, EL1 or supported non-VHE EL2→EL1 entry, ACPI 2.x, GICv2/v3 and GTDT.
  No VHE/LPA2/52-bit/MMU-off/big-endian EL2 handoff, DT-only discovery, SMP,
  GIC ITS/LPI/MSI or confirmed Fusion device inventory. Original xHCI supports a
  bounded polled USB2 root-port boot-HID subset only; no USB interrupts, hubs,
  general HID reports, legacy controllers, ownership takeover or hotplug re-enumeration.
- Eight user processes/twelve threads; private static AArch64 ELF subset, no
  dynamic linker, public arbitrary-executable spawn/wait or signals. Application
  launch selects one embedded ELF entry role. FP/SIMD traps; general registers only.
- All apps share one filesystem authority. No authentication, per-user permissions,
  encryption or signed payload. Logoff ends apps, not an authenticated login session.
- PMM: 1 GiB/128 ranges; kernel heap 64 KiB; translation pool 4 MiB; no swapping or
  demand paging. Single-CPU management calls require IRQ exclusion. Strict scheduler
  priorities can starve lower threads; dead desktop clients are safely reaped.
- NVMe: one segment-0/bus-0 controller/namespace, 512/4096-byte sectors, polling and
  4 KiB transfers, bounded queues/timeouts. No hotplug/IOMMU/advanced recovery.
- CHFS: raw volume up to 4 MiB, 40 entries, 59-byte ASCII paths, 32 KiB files. No
  general GPT mounting, FAT runtime layer, timestamps, links, fsck or directory rename. Cold-boot,
  integrity and injected failure-model tests pass; production reliability is open.
- Native handles are path based; rename/unlink invalidates old path handles. File
  calls cap transfers at 4 KiB. Notepad/GUI copy/cat cap files at 4095 bytes. Text
  editing is basic ASCII with no clipboard/undo, proportional editing or IME.
- Virtio PCI and bounded xHCI USB boot keyboard/relative mouse; fixed queues and counted software drops.
  Basic US modifiers/punctuation/navigation; no Caps Lock, LEDs or keyboard layouts.
- GOP RGB/BGR 640×480 through 1920×1200; UEFI mode selection before boot, no runtime switch/GPU acceleration/full alpha
  compositor or image-file wallpaper loader. Pointer-only damage is clipped; other
  state changes still copy full frames. Cached grayscale fonts are ASCII only.
- Clock follows firmware time plus the generic counter and a saved fixed offset.
  No date setting, DST/IANA time-zone rules or network time. Control Panel saves
  supported computer/theme/clock settings; live sessions remain volatile.
- Task Manager uses real process IDs, memory counters and scheduled timer samples,
  not CPU percentage. Allocator figures exclude non-conventional reservations.
- Native installation and installed CHFS persistence pass QEMU cold-boot tests.
  Full-screen Setup lacks account/password/DST/layout/mode configuration, cancellation of
  an active writer, repair/fsck and full transactional rollback. No Fusion evidence.

The preserved dedicated Setup sequence passed 35 QEMU cases, 13 portable host programs,
ASan/UBSan and clean tracked-source ISO reproduction. Actual captures and hashes:
[evidence/m6d-setup/summary.json](evidence/m6d-setup/summary.json). Native 70 MiB
installation/boot, both logical-sector geometries, typed confirmation, failure/
interruption/retry and 640×480 setup are covered. This does not complete accounts,
repair, production reliability or Fusion compatibility.

The M7A/7B sequence expands acceptance to 45 QEMU cases and 14 host programs,
including EL2/GICv3 native installation/persistence and firmware/native hardware
diagnostics. See the [iteration report](milestone-7a-b.md) for final evidence.
[Account architecture](accounts-design.md) is design only; no new security claim.

M7C adds a separate automatic noninteractive diagnostic ISO, read-only bounded
report export, ACPI-SPCR PL011 logging, USB descriptor/HID/ring host tests and native
USB-only QEMU interactions. Target Fusion remains untested. See [report](milestone-7c.md),
[USB limits](usb.md) and [first-boot instructions](fusion-first-boot.md).
