# Milestone 8 — installation and desktop continuation

The user reports actual M7I operation on an Apple M1 MacBook Air with VMware
Fusion: normal ISO boot, original kernel, graphical Setup, keyboard, mouse /
trackpad and Live Royale Desktop work. These are user-observed results, not a
new automated Fusion run. Persistent Fusion installation remains unverified:
the installation worker reports "Disk identity or write protection changed."
M7I artifacts and its source checkpoint remain the known-good backup.

## M8A investigation

The exact old predicate was `!nvme_recheck() || !nvme_writable()` in
`installer/service.c`. This ran in the kernel worker after confirmation, so the
error did not identify a typing failure. Every Identify, serial, namespace,
geometry and writable-state failure was collapsed into error 1.

Initialization trimmed only trailing spaces from the 20-byte Identify Controller
serial. Revalidation reconstructed space padding and compared it with the raw
Identify response. A serial ending with NUL padding therefore passed initial
inventory but failed even when the identical device answered again. This is a
reproduced source-level defect; the user's raw Identify data is unavailable, so
it is not yet proven to be the sole cause of the Fusion error.

Both initial discovery and revalidation now use one bounded canonicalizer.
Trailing spaces/NUL padding are equivalent. Leading/interior spaces remain
significant; embedded NUL/control bytes, blank identities and non-ASCII serials
are rejected. Canonical serial, namespace NGUID/EUI64, sector count, sector size,
namespace write protection, controller-ready status and diagnostic read-only
policy are checked. Failures have distinct codes and readable UI/serial messages.
No disk-identity or write-protection gate is removed.

Confirmation is exactly `ERASE`. It authorizes the existing kernel-held selected
disk and plan through the process/session token; it never supplies a replacement
disk identity. Optical boot, verified readable geometry, serial availability,
writable namespace, explicit destination and layout review remain prerequisites.
An installed boot disk cannot authorize the destructive installer.

Portable tests cover space/NUL padding, full-width serials, empty/malformed
identities, serial/namespace/geometry changes, read-only identity and exact
ERASE-only confirmation. QEMU installation and later sub-milestone results are
recorded below as they complete.

## Remaining sequence

M8B: persisted two-phase setup with actual disk-only reboot. M8C: desktop input
semantics and cursor. M8D: firmware-enumerated display choices and safe preference
handling. M8E: incremental Royale refinement. M8F: integrated regression,
reproducibility and versioned ISO publication. No sub-milestone is accepted from
its design alone; actual Fusion persistent-installation acceptance is separate.

M8A validation: `./scripts/container.sh make -j4 iso test-nvme test-setup`
passed. GCC AddressSanitizer/UndefinedBehaviorSanitizer NVMe parser tests passed.
`./scripts/container.sh python3 tests/install.py --sector 512` passed: real GPT /
FAT32 / CHFS writes, guest-requested reset, disk-only cold boots, settings and
Notepad file persistence, independent on-disk CRC/parser verification, rejection
of wrong confirmation without writes, and installed-boot disk erase exclusion.
This verifies the correction under QEMU, not the user's Fusion failure resolution.
