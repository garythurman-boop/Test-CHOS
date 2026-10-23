# M7G — bounded xHCI register layouts and precise refusal diagnostics

**Fusion input remains pending. M7F NVMe initialization and read-only I/O are now user-confirmed on Fusion.** M7G continues the original kernel and polled USB path; no storage rewrite, Royale redesign or speculative UHCI/EHCI implementation.

## Target evidence and exact inference

The user's M7G report describes M7F INPUT and normal ISO tests on an M1 MacBook Air, one vCPU, 2 GiB, ARM64 UEFI, Secure Boot disabled. Diagnostic pages and normal Setup boot. MMU, EL1, GICv3, framebuffer, NVMe discovery/initialization/namespace identification, first/last 4 KiB reads and write guard pass. Keyboard/mouse remain absent. This is user-reported target evidence, separate from our QEMU captures; no new Fusion screenshot is manufactured.

xHCI is `0000:0a:00.0`, `15ad:077a`, MMIO `0x5f600000`, eight root ports, stage 2/reason 2, zero initialized input devices. In **M7F source**, the eight-port diagnostic is emitted only after CAPLENGTH, AC64 and scratchpad checks pass. Immediately afterward a single condition rejects zero slots, zero/>32 ports, or masked DBOFF/RTSOFF outside arbitrary ranges. Eight ports excludes the port-count predicates. Thus the actual failure is in **zero slots or register-offset validation**, before extended capabilities, ownership, halt/reset, page-size checking, DMA allocation or rings.

The old `reason=stage` and generic “DMA reserved” text do not identify a failed predicate. No supplied raw HCSPARAMS1/DBOFF/RTSOFF values distinguish zero slots from the offset checks. **It would be incorrect to claim a particular Fusion DBOFF/RTSOFF value or the exact failed predicate as measured.** M7G records these values and returns a distinct stable reason for every refusal. This remaining uncertainty requires a target retest, not a guessed controller configuration.

## Correction and safety

The source contains a definite architectural defect: DBOFF/RTSOFF are register-block offsets, and xHCI does not require either to be at least `0x1000`. Original code rejected valid compact layouts. The shared, portable production validator now masks reserved low bits, checks the full doorbell array `(MaxSlots+1)*4`, the runtime/interrupter-0 range `0x40`, capability/operational/root-port ranges and pairwise overlap within the existing 64 KiB mapping. CAPLENGTH must be DWORD-aligned and at least 32. No controller vendor/address is hardcoded, no arbitrary below-page minimum remains, and malformed/overlapping ranges fail before operational register access.

Extended-capability headers and the optional legacy control DWORD are independently checked against aperture and actual register regions. QEMU's extended capability at offset `0x20` is allowed in the gap before operational registers. Traversal is limited to 64 headers. BIOS-owned controllers, scratchpads, 32-bit-only DMA and unsupported ports/pages are still refused explicitly; their checks are not bypassed. An initial CNR wait before HCRST now complements the existing post-reset wait. All waits remain bounded.

Reports include PCI command before/decode/DMA enable, raw BAR DWORDs, CAPLENGTH/version, HCSPARAMS1/2, HCCPARAMS1, HCCPARAMS2 on xHCI 1.1+, raw DBOFF/RTSOFF, scratchpad count, extended/legacy state, validated USBCMD/USBSTS/PAGESIZE/CRCR/DCBAAP/CONFIG snapshots before reset/after reset/after run, halt/reset results and DMA success/failure. Failure records stage, distinct numeric reason and readable cause. No ambiguous allocator implication remains. Uncertain DMA ownership is never reclaimed.

The INPUT panel adds controller initialized YES/NO, total/connected root ports, descriptor-enumerated devices and ready HID interfaces. Keyboard/mouse readiness remains distinct from controller readiness. The selected polled controller path can be running with zero usable HID devices. Last key code/value and pointer/buttons/event count remain volatile and never enter logs/export/files. Automatic pages require no input and fit 640×480. Root PORTSC, speed IDs and rejected enumeration stages remain available even without a usable keyboard.

The normal installer uses the same driver correction. Approved full-screen Setup, shared focus handling, desktop, applications and filesystem are preserved. Diagnostic ISOs still reject NVMe write/flush at driver level and never mount, partition, format or enter Setup. No destructive Fusion test is performed or authorized before actual input/storage gates pass.

## Build and validation

```sh
./scripts/container.sh make iso diagnostic-iso input-iso
./scripts/container.sh make test-usb
./scripts/container.sh bash scripts/validate.sh
bash scripts/sanitize.sh
```

Final results are recorded in [the evidence summary](evidence/m7g/summary.json) and [validation log](evidence/m7g/validation.log). The preserved 75-case QEMU suite covers EL1/EL2, GICv2/v3, MMU/RO/NX/guards/user isolation, USB rings/descriptors/real keyboard/mouse/disconnect, both NVMe sector sizes, native install/persistence, applications/Royale, installer safety/errors/retry, large images and malformed-image refusal. Twenty host programs include the production layout validator and ASan/UBSan. Synthetic compact register layouts are **host tests**, not a modified QEMU controller or a VMware register capture. QEMU's normal register layout remains a separate real boot/input regression.

Targeted host tests accept a compact disjoint doorbell `0x800` / runtime `0x600` layout, reject truncated arrays, runtime bounds/overlap, invalid capability length, zero slots/ports, unsupported DMA/scratchpads and malformed extended-capability spans. These fixture values are invented for standards validation and are not attributed to Fusion. The bridged QEMU normal-image test additionally checks Down/Up selection, Enter/license, Escape/back/focus, then Live Desktop mouse/application input. Existing read-only disk hashes and zero-write counters are retained.

Source files: new `drivers/usb/layout.c`; updated `drivers/usb/xhci.c`, `include/usb.h`, `kernel/diagnostics/native.c`, host USB tests, bridged device tests and build/ISO/validation/sanitizer scripts. NVMe, kernel memory/interrupt paths, ring/HID mechanics, application and installer source remain unchanged. Start checkpoint is `a128e622f93dc4325e22c277ab3fb0f5467fd2a4`, preserved by `checkpoint/milestone-7g-start`.

## Deliverables and next target gate

[ISOs/checksums/download instructions](../artifacts/milestone-7g/README.md), [stable reason table](driver-diagnostics.md), [compatibility matrix](fusion-compatibility-matrix.md), [exact Fusion retest](fusion-first-boot.md).

Boot M7G INPUT first without a disk, record every rotating xHCI register/port/enum page and the fixed panel, then try harmless press/release/movement/buttons. A running controller without HID events is not an input pass. If no connected root devices are visible, record the existing UHCI/EHCI inventory before any controlled topology change; their presence alone does not justify another controller driver. New ownership/scratchpad/descriptor blockers need their actual evidence.

After actual keyboard/mouse pass, test normal Setup arrows/Enter/Escape, Live Desktop/Start/menu/window interactions without a disk. Repeat read-only NVMe with a designated disposable disk to guard the already observed M7F success. Only after those gates may explicitly confirmed installation, disk-only boot and persistence be tested separately. Fusion input, installation, installed persistence, general USB/interrupts/hubs/hotplug recovery, authentication/ACLs, regional setup, filesystem recovery, SMP/networking and CHOS 0.1 remain incomplete.
