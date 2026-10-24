# M7H — Fusion USB descriptor diagnosis and bounded boot-HID selection

**M7G xHCI startup is now verified on the real Fusion VM. M7H target HID/input remains pending.** This checkpoint preserves original CHOS and its approved Royale desktop, installer, MMU/GIC/NVMe paths. It adds descriptor/transport evidence and corrects independently demonstrated descriptor-selection defects. It does not invent the target USB device formats or mark installation complete.

## What the nine supplied screenshots establish

The archive contains nine PNGs covering eight automatic pages, with one exact duplicate. They show EL1, GICv3/PPI 30 delivery, MMU/protection tests and 1024×768 GOP diagnostics passing. NVMe namespace 1 has 16,777,216 512-byte sectors (8 GiB); identify, first/last 4 KiB reads and the diagnostic write guard pass. xHCI `0000:0a:00.0 15ad:077a` initializes, halts/resets, allocates DMA pages and starts its polled command/event rings.

Raw xHCI values are CAPLENGTH/version `0x120020`, HCSPARAMS1 `0x8001f7f`, HCSPARAMS2 `0x7f`, HCCPARAMS1 `0x38f081`, DBOFF `0x820`, RTSOFF `0xa20`, zero scratchpads and eight ports. The 128-doorbell array ends exactly where runtime registers begin. Both offsets violate the former M7F 4 KiB minimum, now definitively confirming why that old condition rejects this layout. A host regression uses these actual register values. MMIO base is `0x3f600000` in this run; no physical address is hardcoded.

The images show **four connected ports, 5–8**, not two: the fixed panel and final summary both say four. Ports 1–4 show `0x2a0`; connected ports initially show `0x202e1`, then `0x220603` after failure. The logged speed 0 is the pre-reset sample; the latter PORTSC encodes full speed (1). All four reach stage 18 and zero HID devices become operational. There are no live input events. Controller ready is a separate result from HID/input ready.

[Sanitized target transcript and screenshot hashes](evidence/m7h/fusion-observations-user.json). Original frames remain in the uploaded archive/local workspace; they include unrelated host desktop/browser content and are not republished. No target USB VID/PID, interface layout or report descriptor is visible in these M7G captures.

## Exact stage-18 boundary and confirmed source defects

M7G stage 18 combined four outcomes in one boolean: configuration length below 9, length above 512, failed full-configuration control transfer, or rejected shared descriptor selection. Therefore the screenshots **cannot identify the actual failing predicate or VMware descriptor format**. A controller reset or allocator change would not follow from this evidence. M7H splits these outcomes and records actual initialized transfer lengths and xHCI completion codes.

The old parser contains separate, reproducible specification restrictions: it requires exactly one endpoint, rejects an additional OUT endpoint, rejects a second eligible boot interface even after finding the first, and never selects nonzero alternates. Valid composite layouts can therefore be rejected. Host fixtures establish these defects; they are not claimed to reproduce the unknown VMware descriptors.

New original `drivers/usb/descriptors.c` walks at most 512 bytes/64 subordinate descriptors/32 interface-alternate pairs. It validates descriptor bounds, header/total length, interface uniqueness/counts, per-interface endpoint counts/addresses/duplicates and HID subordinate tables. Malformed data (1001–1013) is distinguished from valid layouts outside the driver budget/subset (1101–1108). Unknown non-HID class descriptors are skipped, including class-specific type 0x21 under a non-HID interface. Output selection remains unchanged on refusal.

Selection accepts an interrupt-IN boot keyboard/mouse among other interfaces, tolerates additional OUT/non-selected endpoints, and chooses a usable alternate with preference for alternate zero. SET_INTERFACE is issued only for a selected nonzero alternate. SET_PROTOCOL targets the selected interface with the class/interface recipient. SET_CONFIGURATION uses the advertised configuration value. Optional SET_IDLE is omitted. There are no VMware VID/PID exceptions.

The bounded policy remains **one selected boot interface per physical device, first configuration only**, up to four successful direct-root devices (raised from two to match the observed four connected roots). Additional eligible interface/alternate candidates are counted and left unclaimed; this does not manage both halves of a keyboard/mouse composite device simultaneously. General HID/report protocol, hubs, USB3 and UHCI/EHCI remain unsupported until actual descriptors justify further work.

Source checkpoint: `36b32fef286be3d5047b006a3ea594f95504424b`, following implementation `e8f9a1b`; start checkpoint `a8a0c91` is preserved as `checkpoint/milestone-7h-start`.

New files are `drivers/usb/descriptors.c`, `tests/usb_descriptors.c` and `tests/usb_compat.py`. Updated files include `drivers/usb/{xhci,hid}.c`, `include/usb.h`, the native diagnostic panel, Makefile, ISO/validation/sanitizer scripts and existing USB/bridged/panel tests. Controller startup, native APIs, application and installer implementations remain unchanged.

## Transfer and diagnostic safety

Control data TRBs now request their own completion. The driver checks slot/endpoint/TRB identity, completion code and residual length before consuming data, then requires the status completion. Descriptor reads require the expected byte count. Data buffers are cleared/cleaned before device input and invalidated only after completion; an older buffer tail cannot masquerade as a new descriptor. Short/error completions receive distinct reasons. Existing command and input rings, cache/barrier helpers and HID-to-CHOS event decoder are preserved.

Per-device reports identify root port, post-reset speed, device VID/PID/class triplet/configuration count, advertised configuration length/value/interface count, actual interface number/alternate/class triplet, endpoint address/type/max packet/interval and HID descriptor presence/length. Class/subclass/protocol triplets are packed in hexadecimal as `class<<16 | subclass<<8 | protocol` (for example `0x30101` is HID/boot/keyboard). Only initialized, bounded fields are printed: no USB strings, arbitrary memory or input report payloads. A per-boot budget of 160 descriptor fields bounds output; a visible notice marks trace truncation and failure summaries continue.

The fixed INPUT panel retains controller/connection/enumeration/HID and live key/pointer/button fields, adding last enumeration stage/reason/root port. Numeric key events remain volatile and never enter serial/export/files. Automatic pages need neither input nor storage. The diagnostic modes continue rejecting NVMe writes/flushes and skip Setup, filesystem mounting and installation.

## Validation and reproducibility

```sh
./scripts/container.sh make iso diagnostic-iso input-iso test-usb
./scripts/container.sh bash scripts/validate.sh
bash scripts/sanitize.sh
```

**All 78 QEMU cases and 20 host executables passed**, including both ELF payload validators, portable ASan/UBSan, independent disk checks and repeatable normal/diagnostic/input/storage images. A clean tracked-source build reproduced all three distributable ISOs. [Final evidence summary](evidence/m7h/summary.json), [complete validator](evidence/m7h/validation.log), [sanitizers](evidence/m7h/sanitizers.log), [clean-source build](evidence/m7h/source-package-build.log).

The suite retains all previous 76 QEMU cases and adds two actual QEMU USB boots: two unsupported tablets before a usable keyboard/mouse, and four accepted boot-HID devices. Both check real key release and left/right mouse button events without a disk. These are QEMU topology/transport/input tests, not a VMware descriptor capture. Existing cases cover modifiers, arrows/Enter/Escape/focus, native editing/save/reopen, pointer interaction, disconnect releases and ring wrap, all EL/GIC combinations, protected memory, large images, both sector geometries, installer safety/retry and installed persistence.

Portable tests add composite/non-HID placement, multiple HID candidates, OUT/IN ordering, alternates, endpoint lengths/counts/addresses/packets/intervals, HID absence/malformed tables, unsupported protocols, output atomicity and every-prefix diagnostic bounds. Nondefault alternate/composite selection is validated with descriptor fixtures; no attached QEMU device or real Fusion device has yet verified that wire configuration. Twenty host executables remain; the new fixtures run within the existing USB executable and under ASan/UBSan.

[Preservation checks](evidence/m7h/m7h-preservation.json) verify unchanged controller startup, HID event decoding, ring, memory, interrupt, NVMe, filesystem, desktop, application and installer paths. UEFI and application ELF payloads remain byte-identical to M7G. A formatting-only GCC warning correction is also verified to preserve all three image hashes.

## Actual QEMU screenshots and footprint

The package contains 146 actual QEMU PNGs, 53 fresh build/test metadata files and
292 hashed evidence files, plus the manifest itself. Scenario filenames containing
“fusion” are QEMU regressions; the supplied Fusion screenshots are separately
attributed by hashes/transcription and are not relabeled QEMU captures.

| QEMU observation | Capture |
|---|---|
| Two unsupported interfaces do not block later keyboard/mouse | [Four-root selection](evidence/m7h/usb-compat-rejections.png) |
| Four operational boot-HID devices and real events | [Four HID devices](evidence/m7h/usb-compat-four.png) |
| Enumeration fields and live key release at minimum resolution | [640×480 panel](evidence/m7h/input-panel-640.png) |
| Bridged normal Setup focus/navigation and native typing | [Setup](evidence/m7h/fusion-bus-navigation.png), [Notepad](evidence/m7h/fusion-bus-notepad.png) |
| Installed persistence on a bridged USB/NVMe configuration | [File reopened](evidence/m7h/installed-notepad-reopen-512-gic3-bridged.png) |

[Evidence manifest](evidence/m7h/files.sha256), [focused final descriptor fixtures
under Clang and ASan/UBSan](evidence/m7h/descriptors-focused.log).

Normal kernel file is 278,816 bytes; declared image memory is 512,000 bytes plus
the unchanged 73,728-byte guarded bootstrap stack allocation. Linked kernel text/
data/BSS totals 504,322 bytes; the native app ELF remains 66,408 bytes. The 512 MiB
two-app case reports 11,664 KiB allocated conventional RAM out of 489,728 KiB
managed, with 15.448-second readiness including firmware/self-tests/Live selection.
These exclude unmanaged/firmware reservations and are QEMU measurements, not
Fusion performance or total guest resident memory. Detailed timing/idle observations
remain scoped in the evidence summary.

## Deliverables, limits and next target test

[Images/checksums](../artifacts/milestone-7h/README.md), [reason codes](driver-diagnostics.md), [compatibility matrix](fusion-compatibility-matrix.md), [Fusion instructions](fusion-first-boot.md).

Boot M7H INPUT without a disk first. Capture every page for ports 5–8, especially VID/PID, configuration total, class/subclass/protocol, all interface/endpoint/HID summaries, selected interface or exact refusal reason and last completion code. Then test ordinary keys/modifiers/releases, arrows/Enter/Escape, pointer movement and both buttons. Selection or device enumeration alone is not input success.

If valid non-boot devices appear, their actual HID report descriptors are needed before implementing a report parser. If multiple required interfaces share one physical device, the next increment must share its slot/configuration and manage multiple endpoints correctly. Do not guess either from controller IDs. Successful M7G controller/NVMe operation is preserved, but M7H target HID, normal Setup/desktop interaction, installation, disk-only persistence and release 0.1 remain pending. Do not run a destructive Fusion installation until actual input and repeated read-only storage prerequisites pass on a designated disposable disk.
