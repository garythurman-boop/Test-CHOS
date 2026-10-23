# CHOS status and known issues

| Milestone | Status |
|---|---|
| 1: UEFI, original ARM64 kernel, bootable ISO | Verified under QEMU UEFI |
| 2: protected memory, exceptions/timer, scheduling, EL0 ELF/syscalls/IPC | Core implemented and tested |
| 3: display/input/NVMe/CHFS persistent operations | QEMU GOP + Virtio PCI + NVMe tested; three cold storage boots |
| 4: CHOS Desktop/window manager/taskbar/Start | Working core; dedicated Search/Run/Help and image wallpaper remain |
| 5: six native applications | Functional bounded live-session versions; see desktop.md |
| 6: graphical installer/GPT/ESP/installed boot | Native graphical installation core, disk boot and persistent system volume tested; account/password/timezone/layout/mode configuration unfinished |
| 7: VMware Fusion validation | Not run: no Apple Silicon/Fusion host available |
| 8: CHOS 0.1 | Not ready |

Current ISO/results: [milestone 6B](milestone-6b.md). Earlier foundations: [milestone 6A](milestone-6a.md). Ordinary images
start a genuine original desktop and native EL0 apps after kernel tests. Their live
CHFS volume is RAM backed. Native Setup requires explicit destructive confirmation.
Installed sessions mount the verified CHFS system partition and persist files.
The separately gated storage-test ISO verifies persistent NVMe/CHFS data but is
not an installer or desktop persistence environment.

## Limits

- One CPU, EL1 entry, ACPI 2.x, GICv2/GTDT. GICv3 is rejected. No EL2 startup,
  DT-only discovery, SMP, USB/xHCI or confirmed Fusion device inventory.
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
  editing is basic ASCII with no clipboard/undo, proportional fonts or IME.
- Virtio PCI keyboard/relative mouse only; fixed queues and counted software drops.
  Basic US modifiers/punctuation/navigation; no Caps Lock, LEDs or keyboard layouts.
- GOP RGB/BGR 640×480 through 1920×1200; no mode switch, GPU acceleration, alpha
  compositor or image-file wallpaper loader. Full software frame copy when dirty.
- Clock follows firmware time plus the generic counter; no date/timezone setup or
  persistence. Settings only change the actual session color theme/display report.
- Task Manager uses real process IDs, memory counters and scheduled timer samples,
  not CPU percentage. Allocator figures exclude non-conventional reservations.
- Native installation and installed CHFS persistence pass QEMU cold-boot tests.
  Setup lacks account/password/timezone/layout/mode configuration, cancellation of
  an active writer, repair/fsck and full transactional rollback. No Fusion evidence.
