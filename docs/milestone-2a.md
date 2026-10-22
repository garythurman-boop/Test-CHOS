# Milestone 2A — kernel memory and preemptive threads

Implemented and validated in the cloud workspace on 2026-10-22. Full milestone 2
is **in progress**: user processes, ELF loading, public syscalls and IPC are not
implemented. This ISO remains a development kernel diagnostic environment.

## Implemented

- Original 16-slot AArch64 exception vectors, full integer context save/return,
  bounded controlled-fault recovery and panic diagnostics with ESR/FAR/ELR.
- Physical page allocator with reserved-range exclusion, bitmap accounting,
  contiguous allocation within extents, exhaustion and invalid/double-free handling.
- Original aligned 64 KiB heap with splitting/coalescing.
- Kernel-owned 39-bit identity page tables, read-only executable text, RO/NX
  rodata, RW/NX data, Device MMIO, null/stack guards and WXN.
- ACPI RSDP/XSDT/MADT/GTDT parsing with checksums, length/pointer checks and boot
  CPU selection. GIC addresses and timer PPI come from firmware, not QEMU constants.
- GICv2 and physical generic timer, nominal 100 Hz IRQs.
- Eight-slot kernel-thread scheduler: timer preemption, integer contexts,
  highest-priority selection, equal-priority round robin and thread termination.

## Validation

| QEMU AArch64 UEFI case | Outcome | Emulator-to-acceptance time |
|---|---|---|
| boot-512 | Pass | 6.679 s |
| boot-1024 | Pass | 6.686 s |
| platform-reject | Expected rejection verified | 6.18 s |
| missing-kernel | Expected rejection verified | 6.122 s |

Two positive functional boots and two expected error/rejection cases completed;
none skipped. QEMU uses virt/GICv2 for positive boots; GICv3 is the intentional
unsupported-platform case. Times include firmware and TCG and are not native
performance measurements. Current tests have unique M2A markers and record the
ISO hash; the M1 completion signature cannot satisfy them.

Each positive boot checks real BRK return, physical-page data integrity and free,
heap allocation/free, null and stack translation faults, code-write permission
fault, heap execution permission fault, ACPI discovery and actual timer-driven
preemption. Two busy-loop workers contain no yield. Register canaries verify
context preservation. A high-priority thread runs/exits; lower-priority work
must remain blocked until higher-priority work stops, then run and terminate.

Two host test programs pass, also built/run with GCC AddressSanitizer and
UndefinedBehaviorSanitizer:

- Memory: reserved ranges, sparse fragmentation, exhaustion, overlap/overflow,
  extended descriptor stride, >4 GiB addresses, alignment, data integrity,
  invalid/double frees and heap coalescing.
- ACPI: checksums, MPIDR selection, malformed entries, invalid IRQs, out-of-map
  pointers, MMIO rejection and oversized RSDP lengths.

ISO EFI catalog/ARM64 PE headers were inspected. Two ISO rebuilds have identical
hashes; output remains 17,154,048 bytes. Kernel payload: 17,595 bytes; EFI executable:
5,120 bytes. ISO SHA-256:

```
991f7b26534ce8711435345e8256d9182556a1c8c5af4fe2699cda937de82d16
```

The captured 512 MiB screen shows 43 timer ticks and 47 context switches at its
completion snapshot. The allocator manages 122,392 pages and has allocated
1,052 pages (4.109375 MiB): 4 MiB table pool, 64 KiB heap, four three-page thread
stack reservations. These allocations exclude the separate 1 MiB loader/kernel
reservation, boot-info/map allocations, framebuffer and retained firmware memory.
This is not a measurement of total resident memory or idle CPU.

[Actual kernel screenshot](evidence/m2a/boot-512.png),
[expected GICv3 diagnostic](evidence/m2a/platform-reject.png),
[validation log](evidence/m2a/validation.log), and JSON/header/console evidence are
under evidence/m2a. Console transcripts normalize ANSI escapes and CRLF only.
M1 evidence remains under evidence/ and in commit fec1183.

## Reproduce

```sh
./scripts/container.sh image
./scripts/container.sh bash scripts/validate.sh
```

For the sanitizer checks on a host with GCC and sanitizer runtimes:

```sh
gcc -std=c11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -Iinclude tests/memory.c kernel/memory/physical.c kernel/memory/heap.c -o /tmp/chos-memory-tests
/tmp/chos-memory-tests
gcc -std=c11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -Iinclude tests/acpi.c kernel/arch/arm64/acpi.c -o /tmp/chos-acpi-tests
/tmp/chos-acpi-tests
```

Container versions remain Clang/LLD 19.1.7, QEMU 10.0.13, EDK II
2025.02-8+deb13u1, mtools 4.0.48 and xorriso 1.5.6. Debian remains a build host
only; its kernel is not included in the guest. No new runtime dependencies.

## Source changes

New modules are kernel/console.c, kernel/memory/{physical,heap,virtual}.c,
kernel/interrupts/exception.c, kernel/scheduler/scheduler.c,
kernel/arch/arm64/{acpi.c,mmu.S,vectors.S,test_worker.S}, the GICv2/timer driver,
their public internal headers, and host tests in tests/{memory,acpi}.c.
The UEFI loader/boot contract now passes ACPI, main runs staged self-tests, and
the linker/build/test scripts enforce position independence and M2A checks.

## Corrected defects and limitations

Boot testing caught absolute GOT pointers after splitting code into modules;
hidden declarations and a linker assertion now prevent that flat-image defect.
A live large-block split temporarily unmapped table storage; normal RAM now uses
4 KiB mappings before activation, and live block splitting is refused. GIC Group-1
interrupts initially returned special ID 1022; AckCtl corrected acknowledgement.

Single CPU, EL1, ACPI 2.x and GICv2 only. No EL0, public syscall ABI, filesystem,
installer, desktop, networking or Fusion validation. Strict priorities can starve;
terminated stacks/slots are retained, no FP/SIMD context, heap/table pools are fixed,
and allocation APIs require IRQ exclusion after startup. See status.md for bounds.

## Next checkpoint: Milestone 2B

Add a bounded static AArch64 ELF loader, per-process user mappings, safe EL0 entry,
validated syscall buffers, fault-driven process termination and deferred resource
reaping; then introduce basic IPC. Require malformed-ELF and kernel-pointer
rejection tests plus real EL0 execution before calling milestone 2 complete.
