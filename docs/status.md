# CHOS feature status and known issues

| Milestone | Current status |
|---|---|
| 1: original UEFI loader/kernel, bootable ISO | Verified under QEMU UEFI |
| 2: memory, exceptions, timer, threads, processes, syscalls/IPC | Core implemented and tested; public ABI remains deliberately small |
| 3: display/input/storage/persistent file operations | Implemented for QEMU GOP + Virtio PCI input + NVMe; Fusion devices unverified/unsupported where listed |
| 4: CHOS Desktop, window manager, taskbar/Start menu | Core implemented/tested; see milestone-4.md for remaining launcher/session details |
| 5: native Explorer/Notepad/Terminal/Control Panel/Task Manager | Not implemented; acceptance ELF programs are not these applications |
| 6: graphical installer, GPT/ESP and installed boot | Not implemented |
| 7: VMware Fusion validation | Not run; no Apple Silicon/Fusion host available |
| 8: release CHOS 0.1 | Not ready |

Current normal images run kernel/EL0 self-tests, then start the original desktop
with a native EL0 window client. No installer is implemented. See milestone-4.md
for the tested desktop core checkpoint and artifacts.

## Limits

- One CPU, EL1 entry, ACPI 2.x and GICv2/GTDT required. No EL2 transition, GICv3,
  DT-only discovery, SMP or full interrupt-controller coverage.
- Eight live process records, twelve threads, static AArch64 ELF subset, no dynamic
  linker/public spawn/wait, no signal model. FP/SIMD traps at EL0; general registers
  only. User mappings are private; all processes currently share one filesystem
  authority, with no per-user file permissions or authentication.
- Physical allocation tracks 1 GiB/128 ranges; heap is 64 KiB. Page-table pool
  reserves 4 MiB, with 48-bit identity mappings and bounded private process tables.
  512 MiB/1 GiB configurations were tested; larger RAM may exhaust table storage.
  No demand paging/swapping. Single-CPU management calls require IRQ exclusion.
- Strict priorities can starve low-priority threads. Completed contexts are reaped
  only after switching away; the desktop supervisor reaps exited application contexts.
- Polling NVMe: one controller/namespace on PCI segment/bus 0, 4 KiB PRP transfers,
  512/4096-byte sectors, fixed queues and 2 s timeouts. No hotplug/IOMMU isolation,
  advanced recovery, queue concurrency or Fusion driver validation. Partial failed
  initialization can retain bounded queue allocations until reboot.
- CHFS is a bounded raw test volume: 4 MiB maximum, 40 entries, 59-byte ASCII paths,
  32 KiB files. No GPT/ESP, FAT runtime driver, file permissions, timestamps, links,
  directory rename, fsck or general mount table. Checkpoint/reboot and fault-model
  tests passed; production reliability and hardware power-cut testing remain open.
- Native file handles are path-based; rename/unlink invalidates old path handles.
  Read/write syscalls are capped at 4 KiB per call. The ordinary ISO does not mount
  or write volumes; native file acceptance runs only in the opt-in storage-test ISO.
- Virtio PCI input: two devices, 32-entry event queues, a 128-event software FIFO
  with counted drops, polling via the timer. Relative mouse and basic uppercase
  US key feedback are tested; raw navigation/modifier events are exposed for future
  clients. No USB/xHCI, tablet absolute-coordinate support, LEDs, IME or keymaps.
- GOP RGB/BGR >=640x480 only, no mode switching/hotplug. The cursor/character
  feedback is not a desktop/window manager.
- Unsigned payload, Secure Boot off, no payload authentication. Boot allocation
  cleanup on early error is incomplete. Final map/ExitBootServices failure halts.
- No measured idle CPU, total resident footprint or application latency claims.
  Timer is nominal 100 Hz and can drift with polling/IRQ latency.

Next: milestone 4 begins with compositor/window primitives and an application
message/graphics boundary, then focus, move/resize, taskbar and launcher. Keep
Fusion USB/GICv3 investigation separate and do not substitute QEMU for target testing.
