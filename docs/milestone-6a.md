# Milestone 6A: installation-format foundations and GPT/ESP boot

Implemented original freestanding C GPT and FAT32 ESP encoders, compiled for
AArch64 and exercised by host tests. A build-time tool assembles a new disposable
boot disk from the genuine CHOS UEFI executable and kernel. Real AArch64 UEFI
then boots CHOS Desktop from that NVMe disk with no ISO or `-kernel` attached.
Both 512-byte and 4096-byte logical-sector configurations pass.

**This is not a guest installer run or completion of milestone 6.** The desktop
still uses volatile RAM. The main CHOS partition is planned but unformatted; no
system-volume installation, native graphical wizard, user-account configuration,
installed application filesystem, or Fusion validation is claimed. The normal
ISO is unchanged from the milestone-5 desktop/application checkpoint.

## Implementation

- `installer/gpt.c`: protective MBR, primary/backup GPT, CRC32, aligned ESP and
  CHOS system partitions, unique GUIDs, backup-first metadata writes, read-back
  verification. Requires explicit confirmation and unchanged device geometry.
- `installer/fat32.c`: original bounded FAT32 ESP formatter, BPB/backup, FSInfo/
  backup, mirrored FATs, directories and exact `EFI/BOOT/BOOTAA64.EFI` and
  `CHOS/KERNEL.BIN` payloads. No third-party filesystem formatter is involved.
- `include/install.h`: layout/payload/workspace interface. Storage transfers remain
  4 KiB; GPT/BPB logical sectors are explicitly 512 or 4096 bytes.
- `scripts/mkdisk.c`: host build utility. Exclusively creates a **new regular image
  file**; refuses existing destinations and device nodes. It never opens a real
  disk for installation. Fixed fixture GUID/time values make test images repeatable.
- `tests/{installer,fat32}.{c,py}`, `tests/diskboot.py`: C failure/bounds tests,
  independent Python wire-format parsing and real UEFI disk boots. The test harness
  also records the exact QEMU storage geometry and real guest screenshots.

GPT reserves a 1 MiB-aligned ESP: 64 MiB for 512-byte sectors, 272 MiB for 4 KiB
sectors, keeping FAT32's minimum cluster count. The remaining system partition
stops before backup metadata. The private system-type GUID is
`c4ec0550-3c1d-4e65-9abd-43484f530001`. This is a project-defined type, not a claim
of external registration. The ESP uses the standard EFI system partition GUID.

## Build and tests

```
./scripts/container.sh make
./scripts/container.sh make test-installer
./scripts/container.sh python3 tests/diskboot.py
./scripts/container.sh bash scripts/validate.sh
bash scripts/sanitize.sh
```

The complete sequence passes **14 QEMU cases** (11 positive, three expected
rejections), eight portable host test executables, both actual ELF inputs,
ASan/UBSan, ISO repeatability, independent CHFS persistence, two fresh-build disk
repeatability comparisons, and independent GPT/FAT/payload parsing.
[Full regression](evidence/m6a/validation-m6a.log),
[GPT](evidence/m6a/installer-gpt.json), [FAT32](evidence/m6a/fat32-images.json),
[disk boots](evidence/m6a/diskboot.json), [sanitizers](evidence/m6a/sanitizers-m6a.log).

Tests reject missing confirmation, wrong sector size, changed/invalid geometry,
undersized/overflowing disks, empty GUIDs, oversized payloads and pre-existing image
paths. Injected write/flush failures must return failure. Independent Python code
checks both GPT/header/array CRCs, bounds/overlap, GUIDs and names, FAT mirrors,
non-cyclic/non-crosslinked chains, directory relationships, exact original payload
bytes, padding and unused cluster allocation. Disk-boot tests verify actual EL0
keyboard-driven redraw and unchanged underlying disks after boot.

A compressed 512-byte-sector **test disk**, raw/compressed checksums and its scope
are in `artifacts/milestone-6a/`. It is a build-created QEMU fixture, not an image
produced by a CHOS installer. Do not call it an installed-OS acceptance result.

## Limits, safety and next work

The low-level encoders can overwrite their supplied device when confirmed. They
are not exposed by a syscall or live installer; the shipping ISO does not call
them. A future graphical installer must display/bind disk identity and capacity,
exclude its boot medium, require explicit final destructive confirmation, recheck
identity immediately before writing, and provide partial-failure recovery. A
boolean/geometry check in a library is not sufficient UI authorization.

Formatting is not transactional across an entire installation. A failure after
writes may leave partial GPT/ESP state. FAT free-space contents are not securely
erased. Do not promise rollback or data recovery. Production installation must
operate only on an explicitly selected disposable destination until validated.
The FAT module is an ESP construction subset, not a runtime FAT driver. Test disk
GUIDs are fixed; do not attach multiple copies as distinct production volumes.

Next: retain pristine loader/kernel payloads across ExitBootServices; expose a
read-only destination inventory; build the native setup wizard and confirmation
boundary; wrap/format/mount the CHOS system partition; copy/configure system files;
then automate a **guest-executed** install followed by a fresh boot without ISO
and persistent file verification. Authentication/settings must be implemented
before the full requested configuration stage can be marked complete.
