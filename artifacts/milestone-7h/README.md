# M7H ARM64 USB/HID test checkpoint

- **[CHOS-ARM64-M7H-INPUT.iso](CHOS-ARM64-M7H-INPUT.iso)** — start here, without a
  disk. Automatic descriptor/transfer/refusal reports plus live controller/HID/input
  panel. No installation/filesystem mounting; NVMe write/flush guard remains active.
- [CHOS-ARM64-M7H-DIAGNOSTIC.iso](CHOS-ARM64-M7H-DIAGNOSTIC.iso) — full-page rotating
  reports without the fixed live panel; same read-only storage policy.
- [CHOS-ARM64.iso](CHOS-ARM64.iso) — normal blue/Royale Setup and L/F3 Live Desktop.
  Contains a real destructive installer gated by explicit typed disk confirmation.

Choose **Download raw file** on GitHub. Direct [INPUT download](https://raw.githubusercontent.com/garythurman-boop/Test-CHOS/main/artifacts/milestone-7h/CHOS-ARM64-M7H-INPUT.iso)
and [normal ISO download](https://raw.githubusercontent.com/garythurman-boop/Test-CHOS/main/artifacts/milestone-7h/CHOS-ARM64.iso).
Compare `shasum -a 256 filename.iso` with [SHA256SUMS](SHA256SUMS), or download
all three images and run `shasum -a 256 -c SHA256SUMS`.

Reuse ARM64 UEFI, one CPU, 2 GiB RAM, Secure Boot off. Cold boot INPUT without a
disk first. Capture every page for ports 5–8 including USB VID/PID, configuration,
interfaces, endpoints, HID and exact refusal/completion. Then test harmless keys,
modifiers/releases, pointer movement and both buttons. Controller initialized YES
is not an input pass. [Exact Fusion retest](../../docs/fusion-first-boot.md).

M7G controller/NVMe startup is verified in actual supplied Fusion screenshots.
M7H target descriptor/HID/input, installation and persistence are pending. No
VMware USB descriptor format is assumed. Keep the M7G images. Do not attempt a
Fusion installation before actual input/Setup and repeated read-only storage gates
pass; use only a designated disposable virtual disk for later confirmed writes.
[Source audit, test scope, screenshots and known limits](../../docs/milestone-7h.md).
