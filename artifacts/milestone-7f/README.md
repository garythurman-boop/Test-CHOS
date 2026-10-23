# CHOS M7F — Fusion input and read-only NVMe trial

Start with **[CHOS-ARM64-M7F-INPUT.iso](CHOS-ARM64-M7F-INPUT.iso)**. Open the ISO on
GitHub and choose **Download raw file**. This image combines automatic controller/
port reports with a fixed live keyboard/mouse test panel, requiring no disk.

Other images:

- [CHOS-ARM64-M7F-DIAGNOSTIC.iso](CHOS-ARM64-M7F-DIAGNOSTIC.iso): standard rotating
  full-page diagnostics, also read-only storage.
- [CHOS-ARM64.iso](CHOS-ARM64.iso): normal full-screen Setup/Live Desktop. L/F3 selects
  Live Desktop. Do not install on Fusion before actual input/storage gates pass.

All three images are 17,154,048 bytes. Verify [SHA256SUMS](SHA256SUMS):

```sh
shasum -a 256 CHOS-ARM64-M7F-INPUT.iso CHOS-ARM64-M7F-DIAGNOSTIC.iso CHOS-ARM64.iso
```

- Input: `5d12eb31263585faf38e83b9e75243265be70fc3808f08f89f0bd8d266915a28`
- Diagnostic: `4e868f6ea83b3792ca65aca5f4e80f7c3359241612ce45e0e0cf201d3b1fdc8c`
- Normal: `1125324d042468a4190837323b9e7ba60c893a9a2f213d3bdbe6a52fc7be0ffb`

Runtime source: `b87f129`; publication tag: `checkpoint/milestone-7f`.
[Full results/screens](../../docs/milestone-7f.md) ·
[Actual Fusion observations](../../docs/fusion-observations-m7e.md) ·
[PCI correction](../../docs/pci-discovery.md) · [Driver stage codes](../../docs/driver-diagnostics.md).

**Confirmed:** M7E boots on the user's M1/Fusion VM, with MMU/protection/timer/GOP
passing and Setup reached. Input/NVMe fail. The supplied screenshots place xHCI at
`0a:00.0` and NVMe at `12:00.0`; the original drivers scanned only bus 0. M7F fixes
that confirmed discovery blocker and tests nonzero-bus routing under QEMU.
**M7F real Fusion keyboard/mouse/NVMe retest remains pending.** Controller attachment,
capabilities, ownership, HID format and namespace behavior are not inferred from IDs.

Retest with **one CPU, 2 GiB RAM, Secure Boot off**, INPUT ISO connected, no network
or disk required. Allow bounded driver initialization to finish. Capture all rotating
pages and live key/pointer fields. [Exact sequence](../../docs/fusion-first-boot.md).
For a second trial, use only a designated disposable NVMe disk and the **same INPUT
image**: it rejects writes/flushes, skips filesystem/Setup, and probes read-only
Identify/first/last 4 KiB reads without displaying sector contents. No typed event
history/content is logged, exported or persisted.

Normal Setup requires exact disk identity/capacity review and typed `ERASE` before
partitioning a supported disposable disk. Live files are volatile; installed CHFS
exposes at most 4 MiB. Limits include one polled xHCI/NVMe, namespace 1, 32 retained
PCI records, no general bridge walk/hotplug/BAR resizing/IOMMU, bounded USB2 boot-HID
without interrupts/hubs/general HID/ownership takeover/scratchpads/USB3/recovery,
1 GiB PMM/128 ranges, 1,024 tables, unfinished authentication/ACLs/regional setup/
fsck/SMP/networking. This is a test checkpoint, not CHOS 0.1 or completed Fusion
installation compatibility. Previous images remain preserved.

The standard diagnostic build keeps legacy internal `build/CHOS-ARM64-M7C.iso`;
publication renames identical bytes. INPUT is built directly with its M7F filename.
All new runtime captures are QEMU; the user's original Fusion screenshots remain
in the conversation and their diagnostic contents are transcribed with attribution.
