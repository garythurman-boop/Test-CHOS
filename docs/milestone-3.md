# CHOS milestones 2–3 development sequence

Two successive working checkpoints were implemented and boot-tested: milestone 2
core (cd35ad4), then storage checkpoint 3A (56d0fed), followed by milestone 3 input
and combined regression. These complete the core implementation for the documented
QEMU device set, not VMware Fusion acceptance or the complete operating system.

## Features delivered

- M2: static AArch64 ELF loader, isolated EL0 address spaces, versioned syscalls,
  bounded IPC, user heap, fault termination and deferred context/page reclamation.
- M3 storage: ACPI MCFG/PCI discovery, original NVMe queues/DMA/flush, original CHFS
  COW filesystem and native file handles/read/write/seek/close/list/mkdir/remove/rename.
- M3 display/input: existing original GOP renderer plus modern Virtio PCI keyboard/
  relative mouse queues, bounded raw event FIFO, original cursor and character feedback.
- Both ISO variants reproducibly build; versioned images, checksums and evidence
  are committed for GitHub download. No existing OS kernel or guest stack is used.

Source groups: kernel/process and kernel/syscall, userspace/init, filesystem,
drivers/storage, drivers/input, MMU/scheduler/exception/UEFI boot updates, internal
headers, scripts and tests. See architecture.md, syscalls.md and storage.md for
interfaces, wire layouts, safety and bounds.

## Actual validation results

| QEMU AArch64 UEFI case | Outcome | Emulator-to-acceptance time |
|---|---|---|
| boot-512 | Pass | 7.187 s |
| boot-1024 | Pass | 7.184 s |
| platform-reject | Expected rejection verified | 6.181 s |
| missing-kernel | Expected rejection verified | 6.157 s |
| storage-1 | Pass | 7.183 s |
| storage-2 | Pass | 7.185 s |
| storage-3 | Pass | 7.183 s |
| disk-reject | Expected rejection verified | 6.675 s |
| normal-disk-no-write | Pass | 7.184 s |

**Nine completed cases: six functional boots and three expected rejections. None
failed or skipped in the final run.** Positive cases include real QMP-injected `A`
key, relative motion and left-button events received through device DMA queues.
Acceptance requires both core completion and a separate input acknowledgement;
a previous milestone marker cannot satisfy this check.

Core tests exercise BRK, physical/heap integrity, null/guard faults, RO text, NX
heap, non-yielding thread preemption, register canaries, priorities/exits, real EL0
syscalls and IPC. Separate EL0 programs intentionally read kernel data, write code,
execute stack memory and use trapped FP/SIMD. Each terminates without killing the
kernel; allocations return to the baseline after reaping.

Five host test programs pass: physical/heap memory, ACPI/MCFG, strict ELF validation
(with 2000 metadata mutations), CHFS and input decoding/FIFO. All five also passed
GCC AddressSanitizer and UndefinedBehaviorSanitizer. Logs: evidence/m3/.

### Persistent storage evidence

A fresh disposable NVMe file is attached to three separate QEMU instances. Boot 1
creates files and runs an EL0 native file test; boot 2 verifies prior data then
deletes/replaces a file; boot 3 verifies the second generation without changing
the disk hash. Native file tests include invalid buffers without offset/size
changes, seek, two-page writes/reads, close/double-close, rename, listing and delete.

An independent Python wire-format parser (not the CHFS implementation) validates
both metadata CRCs, selects generation 17, checks each file's data CRC and
compares exact 9000-byte and 8192-byte patterns. It verifies the native file
survived cold boots. Final disk SHA-256:

```
8a66e1603543f77dabbdb2df723e67c8b9f7d2f443800c3462a005620882ae1e
```

The refused-device image stays byte-identical. The ordinary ISO stays read-only
even with a disk carrying the test serial. CHFS host tests simulate failures/torn
writes at each replacement write/flush boundary and accept only old or new valid
file data on remount. These demonstrate the tested persistence model, not general
production reliability or physical-hardware power-cut resilience.

### Artifacts and reproduction

```sh
./scripts/container.sh image
./scripts/container.sh bash scripts/validate.sh
```

Normal ISO SHA-256: `908bf5c30df05b1700c8867bc8e5bfb545a8510443610a9ddf30ce711b0c2ef8`

Storage-test ISO SHA-256: `ac5efbb48ee85c2b130eca8c3f5f513b1e65dc8d946d0d95b2ae694b34c215c6`

Each ISO is 17,154,048 bytes; flat kernel is 56,912 bytes;
EFI executable is 5,120 bytes. The base toolchain remains Clang/LLD 19.1.7,
QEMU 10.0.13, EDK II 2025.02-8+deb13u1, mtools 4.0.48 and xorriso 1.5.6.
Repeated normal and storage-test ISO hashes match within that toolchain.

[Normal image and testing notes](../artifacts/milestone-3/README.md),
[core/input screenshot](evidence/m3/boot-512.png),
[third cold-boot screenshot](evidence/m3/storage-3.png),
[raw results and logs](evidence/m3/).
PCI logs capture firmware BARs, including NVMe MMIO above 512 GiB, which required
four-level 48-bit translation tables. No QEMU-specific BAR address is hardcoded.

## Limits and next milestone

The ordinary ISO does not format/write/mount disks. Storage writes require the
special opt-in ISO and disposable identity; this is an automated fixture, not a
human installation workflow. No GPT/ESP, FAT runtime layer, installed boot,
desktop/window manager, native desktop apps, network or release CHOS 0.1 exists.
User file ownership/permissions, richer process control, FP/SIMD contexts and
production recovery tools remain outstanding.

Fusion is unverified: GICv3, EL2 entry and USB/xHCI input are currently unsupported.
NVMe and Virtio tests are QEMU results. Do not infer Apple Silicon/Fusion support.
See status.md for fixed capacities, polling limitations and performance caveats.

Next milestone 4: original compositor/window model, protected application graphics/
event interface, focus/stacking/move/resize, taskbar and launcher. Native desktop
apps and the real graphical installer follow verified infrastructure.
