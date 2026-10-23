# M7G ARM64 test images

These are original CHOS UEFI/kernel images, an intermediate checkpoint rather
than CHOS 0.1. Fusion M7F storage/core boot passes are user-confirmed; M7G input
and Fusion installation/persistence are pending. [Source audit/results](../../docs/milestone-7g.md).

- **[CHOS-ARM64-M7G-INPUT.iso](CHOS-ARM64-M7G-INPUT.iso)** — start here. Automatic
  reports, precise xHCI refusals/registers, fixed live controller/ports/HID/input
  panel. No disk required; diagnostic NVMe writes/flushes rejected, no filesystem.
- [CHOS-ARM64-M7G-DIAGNOSTIC.iso](CHOS-ARM64-M7G-DIAGNOSTIC.iso) — full-page rotating
  reports without the fixed input panel. Same driver-level read-only storage policy.
- [CHOS-ARM64.iso](CHOS-ARM64.iso) — normal full-screen Setup, L/F3 Live Desktop.
  Same USB correction. It contains a real destructive installer gated by typed
  disk identity confirmation. Test input without a disk before using any target.

On GitHub open the ISO and choose **Download raw file**. Direct raw downloads:
[INPUT](https://raw.githubusercontent.com/garythurman-boop/Test-CHOS/main/artifacts/milestone-7g/CHOS-ARM64-M7G-INPUT.iso),
[normal Setup](https://raw.githubusercontent.com/garythurman-boop/Test-CHOS/main/artifacts/milestone-7g/CHOS-ARM64.iso).
Download [SHA256SUMS](SHA256SUMS) alongside the images and run
`shasum -a 256 -c SHA256SUMS` for all three, or compare your selected file's
`shasum -a 256 filename.iso` output to its line.

Use ARM64 UEFI, one vCPU, 2 GiB RAM, Secure Boot disabled. Cold boot the INPUT
image without a disk first. Capture every rotating controller/offset/failure/
port/enum page and live numeric event changes. Controller initialized YES alone
is not a keyboard/mouse pass. [Exact retest instructions](../../docs/fusion-first-boot.md).
Do not install on Fusion until actual input and read-only storage prerequisites
pass. Only designated disposable virtual disks may be used for later confirmed
write tests. No real Fusion screenshot is inferred from QEMU captures.
