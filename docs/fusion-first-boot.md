# M7F Fusion retest — input first, read-only storage second

M7E already boots on the user's **M1 MacBook Air / one CPU / 2 GiB**. Supplied
screenshots show working EL1, MMU/protection, GICv3/timer and 1024×768 GOP diagnostics;
the normal image reaches Setup. Input is absent and NVMe is not initialized. The
actual endpoints are xHCI `0000:0a:00.0` and NVMe `0000:12:00.0`, beyond the original
bus-zero scans. [Observed evidence](fusion-observations-m7e.md).

## First trial: no disk required

1. Download [CHOS-ARM64-M7F-INPUT.iso](../artifacts/milestone-7f/CHOS-ARM64-M7F-INPUT.iso)
   using GitHub **Download raw file**. Verify the adjacent SHA256SUMS with
   `shasum -a 256 CHOS-ARM64-M7F-INPUT.iso`.
2. Reuse the known ARM64 UEFI configuration: **one CPU, 2 GiB, Secure Boot off**,
   ISO connected at power-on. No network or hard disk is required. Preserve the
   working M7E ISO. Fusion UI labels vary by release; do not select an x86 guest.
3. Cold boot. Wait for the kernel tests, automatic report pages and lower
   **CHOS USB HARDWARE TEST** panel. No keyboard is needed to see the reports.
   The report should now identify and select xHCI at bus `0a`, rather than silently
   miss it. Seeing a selected controller does not establish working input.
4. Capture **every rotating page**, particularly the xHCI PCI config/ID/MMIO,
   capabilities/HCSPARAMS2, root ports and each PORTSC, initialization/ownership
   errors, enumeration port/stage, HID interface/protocol and ready counts.
   The supplied target has no SPCR table, so native serial is not expected; screens
   are the primary evidence. Record the last stable error/exception if boot stops.
5. In the fixed panel, press/release ordinary test keys, move the mouse and press/
   release left/right buttons. Capture numeric last-key code/value, changing pointer
   coordinates/buttons and event count. The driver must actually decode events;
   a “controller selected” line alone is not a pass. Event content/history is not
   logged or persisted. The panel remains visible as report pages rotate.
6. Record ISO checksum, exact Fusion About version/build, macOS version and whether
   optical boot was automatic or manually selected. Previously reported strings
   are macOS 27 “Golden Gate,” Fusion “26H1u1”; full build is still unspecified.

If xHCI runs but no HID devices are ready, capture all port/status/enum pages.
UHCI/EHCI presence does not prove attachment. Do not change controller settings or
attach alternate devices until the baseline evidence is recorded; later controlled
comparisons can then identify the actual path. Firmware ownership, scratchpads,
USB3/hub/general HID and alternate controllers remain bounded unsupported paths.
No speculative new controller driver is included in M7F.

## Gate 4: Setup interaction without a disk

After actual keyboard/mouse events pass, boot the [normal ISO](../artifacts/milestone-7f/CHOS-ARM64.iso)
with **no disk attached**. Verify Setup keyboard navigation/mouse interaction;
L/F3 selects Live Desktop. Record this separately from the input-panel result.
Do not attempt installation. Then return to the INPUT ISO for storage testing.

## Gates 5–6: read-only NVMe

After input and Setup-interaction prerequisites pass, attach **an explicitly designated
disposable virtual disk** using the observed virtual NVMe controller. Boot the
**same INPUT ISO**, which enforces a driver-level write/flush rejection and skips
Setup/filesystem mounting. No partitioning or formatting occurs. Capture:

- Actual NVMe BDF/ID, mapped config/BAR and selected-driver result.
- CAP, version, timeout budget and the exact failure stage/completion DWORD if any.
- Controller/namespace operational state, namespace sectors and sector bytes.
- `NVMe first/last 4KiB reads: PASS` and `NVMe diagnostic write guard: PASS`.

These reads identify I/O readiness; they do not validate persistence or filesystem
integrity. No sector contents are displayed. Do not infer namespace support from a
PCI class match or from the word “NVMe” in the firmware inventory.

## Later gates

The normal ISO preserves the approved full-screen Setup and Live Desktop. After
all input/Setup/read-only storage gates pass, installation is a separate explicitly confirmed
action on a disposable disk, with exact identity/capacity review and typed `ERASE`.
Do not install on Fusion before all prerequisites pass. Installed disk-only boot
and file persistence are further independent gates.

The separate [standard diagnostic ISO](../artifacts/milestone-7f/CHOS-ARM64-M7F-DIAGNOSTIC.iso)
retains full-page automatic reports without the fixed input panel; it also enforces
read-only storage. [Driver stage codes](driver-diagnostics.md),
[PCI resource findings](pci-discovery.md), [compatibility matrix](fusion-compatibility-matrix.md).
