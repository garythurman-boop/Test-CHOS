# Feature status and known issues

| Milestone | Status |
|---|---|
| 1: build, native UEFI loader, kernel console, ISO, QEMU | Implemented and tested; see validation.md |
| 2: memory, exceptions, interrupts, scheduler, user processes, syscalls | Not implemented |
| 3: runtime input, storage, filesystem, persistence | Not implemented |
| 4: desktop and window manager | Not implemented |
| 5: native applications | Not implemented |
| 6: graphical installer and installed-disk boot | Not implemented |
| 7: VMware Fusion validation | Not run |
| 8: CHOS 0.1 release | Not ready |

## Known issues

- Only the boot diagnostic console exists. No interactive UI or applications.
- No kernel-owned exception vectors/MMU, allocator, protection or user isolation.
  IRQs remain masked and inherited firmware mappings are retained.
- No kernel storage driver; firmware loads the kernel before boot services exit.
  FAT support here belongs to firmware, not to a CHOS filesystem implementation.
- Unsigned loader/payload; Secure Boot must be disabled. No image authentication.
- Only GOP RGB/BGR framebuffers at least 640x480 are supported. No GOP means a
  firmware error, not a fallback terminal. Hotplug/mode changes are unsupported.
- Memory map/ExitBootServices failure after the first exit attempt halts without
  further firmware output. Boot allocation cleanup on earlier error returns is
  incomplete; restart firmware after a failed load rather than retry indefinitely.
- The fixed 1 MiB temporary image/stack allocation is not a memory manager. Stack
  guards, overflow detection, and separate executable/data permissions are absent.
- QEMU reports missing legacy vgabios-ramfb.bin in this slim tool image. Native
  UEFI GOP still works, as demonstrated by kernel framebuffer checks. This is
  not evidence of legacy BIOS support (not a CHOS target).
- No VMware runtime device assumptions are justified by the QEMU test.
- No measured idle CPU, total memory footprint, desktop latency, or installer
  reliability claims. WFE is used, but performance needs real instrumentation.
- Builds are repeatable in the recorded toolchain; package repository updates
  can change compiler/firmware versions in newly built toolchain images.

## Next milestone

M2 starts with exception reporting and boot-memory ownership tests, then physical
allocation and kernel-owned page tables. Add GIC/timer support only after hardware
discovery. Keep every step boot-tested before progressing to threads and syscalls.
Storage/persistence must pass reboot and corruption tests before an installer or
editor is allowed to claim saved data. Networking follows stable local operation.
