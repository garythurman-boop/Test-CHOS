# M7G Fusion retest — input first, read-only storage second

M7F target testing now confirms boot/MMU/GIC/framebuffer and NVMe initialize,
namespace identification, first/last reads and diagnostic write guard. Input still
fails at xHCI stage 2 with eight ports and base `0x5f600000`. The M7G correction
accepts valid compact register layouts and records each refusal precisely. Actual
M7G controller/HID/input operation remains pending. Keep the M7F ISO/checksum.

## First trial: no disk required

1. Download [CHOS-ARM64-M7G-INPUT.iso](../artifacts/milestone-7g/CHOS-ARM64-M7G-INPUT.iso)
   using GitHub **Download raw file**. Verify the adjacent SHA256SUMS with
   `shasum -a 256 CHOS-ARM64-M7G-INPUT.iso`.
2. Reuse the known ARM64 UEFI configuration: **one CPU, 2 GiB, Secure Boot off**,
   ISO connected at power-on. No network or hard disk is required. Preserve the
   working M7E ISO. Fusion UI labels vary by release; do not select an x86 guest.
3. Cold boot. Wait for the kernel tests, automatic report pages and lower
   **CHOS USB HARDWARE TEST** panel. No keyboard is needed to see the reports.
   The report should now identify and select xHCI at bus `0a`, rather than silently
   miss it. Seeing a selected controller does not establish working input.
4. Capture **every rotating page until page 1 returns**, particularly the xHCI PCI config/ID/MMIO,
   CAPLENGTH, HCSPARAMS1/2, HCCPARAMS1/2, **DBOFF/RTSOFF**,
   **failure stage/reason and readable cause**, root ports and each PORTSC, initialization/ownership
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
No speculative new controller driver is included in M7G.

## Gate 4: Setup interaction without a disk

After actual keyboard/mouse events pass, boot the [normal ISO](../artifacts/milestone-7g/CHOS-ARM64.iso)
with **no disk attached**. Verify Down/Up highlight, Enter/license, Escape/back and keyboard focus;
L/F3 selects Live Desktop. Verify mouse movement/selection, Start menu opening
and application launch, window focus and native typing. Record this separately
from the input-panel result.
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

The user already reported these M7F reads/guard passing; repeat them to rule out
a regression in M7G. These reads identify I/O readiness; they do not validate persistence or filesystem
integrity. No sector contents are displayed. Do not infer namespace support from a
PCI class match or from the word “NVMe” in the firmware inventory.

## Later gates

The normal ISO preserves the approved full-screen Setup and Live Desktop. After
all input/Setup/read-only storage gates pass, installation is a separate explicitly confirmed
action on a disposable disk, with exact identity/capacity review and typed `ERASE`.
Do not install on Fusion before all prerequisites pass. Installed disk-only boot
and file persistence are further independent gates.

The separate [standard diagnostic ISO](../artifacts/milestone-7g/CHOS-ARM64-M7G-DIAGNOSTIC.iso)
retains full-page automatic reports without the fixed input panel; it also enforces
read-only storage. [Driver stage codes](driver-diagnostics.md),
[PCI resource findings](pci-discovery.md), [compatibility matrix](fusion-compatibility-matrix.md).
