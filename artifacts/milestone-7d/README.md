# CHOS M7D — hardening and Fusion evaluation checkpoint

Download **[CHOS-ARM64-M7D.iso](CHOS-ARM64-M7D.iso)** first for unattended diagnostics.
On GitHub open the ISO and select **Download raw file**. No disk or input is needed;
inventory pages rotate automatically. [First-boot instructions and capture checklist](../../docs/fusion-first-boot.md).

[CHOS-ARM64.iso](CHOS-ARM64.iso) is the separate normal Setup/Live Desktop image.
L/F3 selects Live Desktop. Installation requires a supported disposable target,
identity/capacity review and typed `ERASE`. Do not attempt installation in Fusion
until actual boot, input, display and storage prerequisites have passed.

Both images are 17,154,048 bytes. Verify [SHA256SUMS](SHA256SUMS):

```sh
shasum -a 256 CHOS-ARM64-M7D.iso CHOS-ARM64.iso
```

- Diagnostic: `f29469f9710147154614d3589cb9f8eba99d1b4cf0ced10b4444aa0bfcc94698`
- Normal: `b8448c533756baf6f6a4a47d8d81fac4f1ab70801f839a68874afa8876ccd33e`

Runtime source `88c0e90`; publication tag `checkpoint/milestone-7d`.
[Report, tests and actual screenshots](../../docs/milestone-7d.md).
The build's legacy internal diagnostic filename remains `CHOS-ARM64-M7C.iso`;
this package renames the same bytes to M7D. Previous checkpoints remain preserved.

**VMware Fusion NOT RUN.** No Apple Silicon/Fusion host is attached. Start with a
new ARM64 UEFI VM, one CPU, 1 GiB RAM, Secure Boot off, connected diagnostic ISO,
no disk or network. Send every rotating diagnostic page or last failure screen,
Apple SoC, macOS version and exact Fusion version/build. A QEMU pass is not Fusion
compatibility. [Compatibility matrix](../../docs/fusion-compatibility-matrix.md).

Original dynamic kernel image allocation replaces the old 512 KiB bootstrap limit;
controlled larger kernel/BSS tests include real native installation and disk-only
reboots. Optional Virtio failure no longer prevents diagnostic output. Polled xHCI
boot-HID remains a bounded subset: no USB interrupt delivery, hubs/general HID,
firmware ownership takeover, hotplug re-enumeration or controller reset recovery.

Live files are volatile RAM. Installed CHFS exposes at most 4 MiB. Accounts,
authentication/ACLs, full regional/display settings, filesystem repair/production
reliability, SMP and networking remain unfinished. This is an intermediate
engineering checkpoint, not CHOS 0.1 or completed Fusion validation.
