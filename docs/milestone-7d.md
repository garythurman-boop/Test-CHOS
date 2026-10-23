# M7D — kernel bootstrap hardening and Fusion evaluation package

**Intermediate checkpoint; VMware Fusion NOT RUN.** No Apple Silicon/Fusion host
is attached. Actual firmware/device inventory, input, display, storage, installation
and disk-only reboot remain separate pending acceptance gates. No target-specific
USB driver rewrite or Fusion support claim is made from QEMU observations.

## Preserved baseline and implementation

Baseline `761c958920064470df46396636411d80af5e47c8` and all prior images are
preserved; `checkpoint/milestone-7d-start` records that baseline. Runtime source is
`88c0e90e0b94fa9953d631e9b1b651639ac34dbb`.

The previous 512 KiB restriction was coupled across the loader, linker, stack guard
and installer payload validation. It was an implementation limit, not an ARM64
limit. The original loader now validates a versioned 64-byte CHOS flat-image
header before allocation: exact file size, entry instruction span, reserved fields,
page-aligned memory size and bounded arithmetic. Policy limits are 16 MiB file and
64 MiB image memory. It allocates declared image/BSS memory and a separate 64 KiB
stack with two unmapped 4 KiB guards. Physical regions must be aligned, disjoint,
within the supported address width and non-overflowing. These firmware allocations
are excluded from the conventional-page allocator. Header checks are structural,
not cryptographic authentication.

The private loader/kernel handoff carries an explicit version/size contract. An old
loader paired with a new kernel stops before reading appended fields. Kernel image
text permissions, NX, null protection, EL0 separation and page-fault checks remain.
The installer accepts the same validated image-size policy; the ISO builder grows
its FAT boot image when payload headroom requires it. See the complete
[image format, memory map and audit](kernel-image.md).

Diagnostic mode now continues after failure of optional Virtio input initialization.
A real rejected-feature device is tested with no disk and no usable input. Existing
bounded polled xHCI USB input is preserved. Disconnect releases a held mouse button;
the remaining USB keyboard continues operating native applications. USB interrupts,
firmware ownership takeover, hubs/general HID, hotplug re-enumeration and controller
reset recovery remain unimplemented. A compact native Save As dialog now uses an
“Enter / Esc” hint that fits beside its controls; the Royale design is retained.

## Build and validation

```sh
./scripts/container.sh image
./scripts/container.sh make -j4 iso diagnostic-iso
./scripts/container.sh bash scripts/validate.sh
bash scripts/sanitize.sh
```

The diagnostic build retains the legacy internal name `build/CHOS-ARM64-M7C.iso`;
the published identical bytes are named **CHOS-ARM64-M7D.iso**. The normal image is
**CHOS-ARM64.iso**. Neither image is a completed release.

**All 61 QEMU cases and 17 host programs passed**, along with both ELF payload
checks, ASan/UBSan and clean-source image reproduction. The final validation results are recorded in [summary.json](evidence/m7d/summary.json),
[full validator log](evidence/m7d/validation.log),
[sanitizer log](evidence/m7d/sanitizers.log) and
[clean committed-source build](evidence/m7d/source-package-build.log).
The test matrix retains all 50 M7C QEMU cases and adds eleven:

- Optional Virtio failure with no disk/input still reaches automatic diagnostics.
- Held USB mouse unplug releases button state; remaining keyboard remains functional.
- Genuine preserved M7C EFI loader plus new kernel safely rejects the handoff.
- Controlled 1 MiB read-only and 3 MiB BSS growth boots under EL1/GICv2 and EL2/GICv3.
- That enlarged image installs, then boots twice from disk without ISO, preserving data.
- Three actual UEFI boots reject oversized memory, file-length mismatch and invalid entry.

The enlarged build verifies all pattern/BSS bytes, image contract, both stack guards,
and RO/NX probes beyond the former 512 KiB boundary, including the last enlarged
BSS page. Portable image tests cover malformed headers, alignment, overflow, bounds
and overlapping allocations. All 17 host programs, both real ELF payload checks,
ASan/UBSan, repeatable normal/diagnostic/storage images, independent disk parsers,
and a clean tracked-source build are required before publication. Reproduction is
within the recorded toolchain; future APT package versions are not pinned.

## Actual captures and artifacts

[Download images and verify checksums](../artifacts/milestone-7d/README.md).
The package contains 118 PNGs and 43 test metadata files from the final run.
Every PNG is an actual QEMU guest framebuffer capture. No Fusion screenshot exists.

| Check | Evidence |
|---|---|
| Automatic no-input inventory | [Page 1](evidence/m7d/diagnostic-page-1.png), [page 2](evidence/m7d/diagnostic-page-2.png) |
| Optional device failure | [Diagnostic screen](evidence/m7d/diagnostic-optional-failure.png) |
| Old loader refusal | [Version mismatch](evidence/m7d/mixed-loader.png) |
| Compact native dialog | [Save As hint](evidence/m7d/small-dialog-hint.png) |
| USB disconnect continuity | [Remaining keyboard interaction](evidence/m7d/usb-mouse-disconnect.png) |
| Enlarged kernel boot | [EL1](evidence/m7d/large-growth-el1.png), [EL2](evidence/m7d/large-growth-el2.png) |
| Enlarged installed system | [Setup](evidence/m7d/large-setup-progress-512-gic3-el2.png), [persistent file](evidence/m7d/large-installed-notepad-reopen-512-gic3-el2.png) |

The ordinary kernel file is **258,336 bytes**, with **479,232 bytes** declared image
memory and a separate **73,728-byte** guarded stack reservation. This saves 484 KiB
of bootstrap reservation compared with the old fixed 1 MiB allocation. Linked
kernel text/data/BSS totals 470,922 bytes; the native app totals 66,408 bytes.
The controlled kernel file is **1,306,912 bytes**, with **4,673,536 bytes** declared
image memory. Actual protection probes occur at image offsets 1,170,415 (RO) and
4,669,440 (NX BSS), beyond the previous limit.

At 512 MiB the native two-app test measured **14,720 KiB allocated conventional RAM
out of 489,780 KiB managed**; these exclude firmware/unmanaged reservations and do
not represent total guest resident memory. Desktop readiness was **14.848 seconds**
including firmware, self-tests and explicit Live selection under TCG. The idle
probe used **0.61 host CPU seconds over 10 seconds (6.1% of one host core)**.
This is QEMU emulation cost, not native CHOS or Fusion CPU utilization.

## Changed sources and next milestone

New `include/kernel_image.h`, `boot/image.c`, `kernel/image_test.c`,
`kernel/arch/arm64/growth.S`, `tests/image.c`, `tests/hardening.py`, `tests/growth.py`
and image-format documentation. Updated UEFI loader/private boot info, kernel entry,
linker/MMU/startup, FAT32 payload limits, shared dialog/About, Makefile, ISO builder,
boot/validation/sanitizer scripts and feature/compatibility documentation. Exact
runtime diff is in source commit `88c0e90`.

Next: follow the [non-destructive Fusion first-boot sequence](fusion-first-boot.md).
Collect every rotating diagnostic page or last failure screen plus Apple SoC,
macOS version and exact Fusion version/build. Use a new ARM64 UEFI VM, one CPU,
1 GiB RAM, Secure Boot off, diagnostic ISO and no disk/network. Implement only the
missing hardware path supported by those observations. Do not attempt a Fusion
installation until boot, display, input and target storage prerequisites pass.

Accounts/authentication/ACLs, full regional/display setup, filesystem repair and
production reliability, USB recovery/interrupt delivery, SMP and networking remain
unfinished. The current CHFS system volume exposes at most 4 MiB. Large-kernel tests
do not expand that filesystem or demonstrate production storage reliability.
