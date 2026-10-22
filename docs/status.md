# Feature status and known issues

| Milestone | Status |
|---|---|
| 1: native loader, kernel console, bootable ISO, QEMU | Complete; historical report in validation.md |
| 2A: exceptions, physical memory, heap, page tables, timer, kernel threads | Implemented and tested; milestone-2a.md |
| 2B: user processes, ELF loading, public syscalls, IPC | Not implemented; milestone 2 remains in progress |
| 3: runtime input, storage, filesystem, persistence | Not implemented |
| 4: desktop and window manager | Not implemented |
| 5: native applications | Not implemented |
| 6: graphical installer and installed-disk boot | Not implemented |
| 7: VMware Fusion validation | Not run |
| 8: CHOS 0.1 release | Not ready |

## Current limitations

- Single CPU and EL1 entry only. ACPI 2.x, GICv2 and GTDT are required; no DT-only,
  GICv3 or EL2 startup path. QEMU virt with GICv2 is the tested platform. M2A adds
  requirements beyond the simpler M1 firmware/framebuffer boot; Fusion is untested.
- No ELF/user processes, user/kernel process isolation, public syscall ABI or IPC.
  Present protections apply to kernel mappings; all current threads share EL1.
- Eight lifetime thread slots, strict priority starvation, 8 KiB usable stacks,
  retained terminated stacks, and no FP/SIMD context. No general thread reaper.
- Physical allocator manages at most 1 GiB/128 extents. Heap is fixed at 64 KiB;
  page-table pool reserves 4 MiB. Allocation/heap/table APIs need IRQ exclusion
  once scheduling is enabled and are not SMP safe.
- Virtual space is 39-bit identity mapped, with no demand paging, arbitrary
  mappings, page reclamation or swapping. Live large-block splitting is refused.
- No independent clock/calendar service. Timer is nominal 100 Hz and drifts with
  interrupt latency. No instrumented idle CPU or total resident-footprint claim.
- Boot-time self-tests intentionally exercise BRK, null/guard translation faults,
  read-only text and NX heap faults before starting the scheduling test.
- Diagnostic console only. No input driver, interactive shell, applications or
  installer. No kernel storage driver or CHOS filesystem: firmware reads the
  kernel before ExitBootServices. No guest disk writes are possible yet.
- Unsigned loader/payload; disable Secure Boot. No payload authentication.
- Only GOP RGB/BGR framebuffer >=640x480. No hotplug or mode-switch support.
- Memory-map/exit failure after an exit attempt halts without firmware output.
  Earlier boot failure allocation cleanup is incomplete; restart after failure.
- QEMU's missing legacy vgabios-ramfb.bin warning is harmless for the tested native
  UEFI GOP path. Legacy BIOS is not supported or targeted.
- Container base digest is pinned; APT package versions are not. Identical ISO
  rebuilds are verified in the recorded toolchain, not across arbitrary updates.

## Next engineering step

Milestone 2B: define and validate the executable/syscall boundary before permitting
EL0 code to run. Add malformed-ELF tests, kernel-pointer rejection tests, separate
user mappings, fault termination and safe process resource cleanup. Persistence
must later pass cold-reboot and integrity tests before an installer or editor can
claim data was saved. Networking follows stable local operation.
