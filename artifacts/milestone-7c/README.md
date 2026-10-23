# CHOS M7C discovery / initial USB checkpoint

**Start with [CHOS-ARM64-M7C.iso](CHOS-ARM64-M7C.iso)** for unattended hardware
discovery. It requires no input or disk, rotates inventory pages every five
seconds and stays out of installation. [Exact first-evaluation sequence and capture
checklist](../../docs/fusion-first-boot.md).

[CHOS-ARM64.iso](CHOS-ARM64.iso) is the separate normal Setup/Live Desktop image
using the same original kernel. It starts blue Setup; L/F3 selects Live Desktop.
Use it for interaction and eventual explicitly confirmed disposable-disk install
only after target hardware discovery. It is not the unattended diagnostic mode.

On GitHub choose **Download raw file** for each ISO. Both are 17,154,048 bytes.
Verify [SHA256SUMS](SHA256SUMS), for example:

```sh
shasum -a 256 CHOS-ARM64-M7C.iso CHOS-ARM64.iso
```

- Diagnostic: `9897a8408a67c4dcf60b038555735e7289ad1f2ce6e960bc9244de4c8f52e702`
- Normal: `2981cb1163db69bcb3a5029490463fd6a7ab857f6cf59c4ce2b1b35e62c056a3`

Runtime/source checkpoint: `ac095e6`; publication tag `checkpoint/milestone-7c`.
Previous working images/checkpoints are preserved. See [iteration evidence](../../docs/milestone-7c.md).

**VMware Fusion has not been tested.** No Apple Silicon/Fusion host was available;
the actual release's custom ARM64 boot workflow and devices remain unknown. USB
xHCI boot-HID input passes QEMU tests only. Other controllers, hubs/general HID,
USB interrupts, ownership takeover and hotplug re-enumeration remain unsupported.
See [USB subset](../../docs/usb.md) and [compatibility matrix](../../docs/fusion-compatibility-matrix.md).

For first evaluation: new ARM64 UEFI VM, one CPU, 1 GiB RAM, Secure Boot off,
connected diagnostic ISO, no disk or networking required. Record exact host/macOS/
Fusion versions. Firmware vendor/revisions, EL, ACPI, GIC/timer, PCI class/IDs,
USB/storage classification, GOP and driver results appear automatically. Serial
logging needs validated ACPI SPCR PL011; otherwise capture the screen. A bounded
16 KiB read-only report API is available to native applications in normal mode;
there is no automatic disk export or arbitrary-memory dump.

The normal image's live files are volatile RAM. Native Setup can destroy a selected
disposable NVMe disk only after exact identity/capacity/layout review and typed
`ERASE`. Use a new disk at least 1 GiB when actual target support is established.
Installed CHFS exposes at most 4 MiB. Accounts/password/ACLs, networking, SMP,
full regional/runtime mode setup, fsck and transactional rollback are unfinished.
This is an intermediate development checkpoint, not completed Fusion support or
CHOS 0.1. Full [Fusion install acceptance](../../docs/fusion-validation.md) is separate.
