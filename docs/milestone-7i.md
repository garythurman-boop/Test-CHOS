# M7I — unified VMware Fusion input candidate

**This build implements USB2 hubs, report-protocol HID, simultaneous interfaces and absolute pointing for the paths exposed by the supplied Fusion captures. Actual M7I keyboard/trackpad operation on Fusion still requires the user's target test.** QEMU results are separate evidence; this is not a claim of universal Fusion/version/device compatibility or a completed CHOS release.

## Target evidence and scope

The twelve supplied M7H screenshots identify the actual stage-18 blockers: root port 6 (`0e0f:0003`) contains two HID interfaces, class/subclass/protocol `03/00/00`, each with an 8-byte interrupt-IN endpoint. They are report-protocol interfaces, not boot HID; the boot-only parser returns 1103. Ports 7 and 8 are USB hubs (`0e0f:0002`, class 09) at full/high speed and were rejected as non-HID (1102). Port 5 is video-class hardware (`0e0f:000b`), correctly outside the input scope. Last completion 1 means descriptor transfers succeeded. No keyboard/mouse events were delivered in those captures.

The captures do not contain HID report descriptor bytes or the devices behind either hub. M7I retrieves and interprets those automatically, without guessing a VID/PID-specific packet format. [Sanitized observations and source-image hashes](evidence/m7i/fusion-observations-user.json). Original frames contain unrelated host content and remain local rather than being republished.

## Implemented input path

- Original polled xHCI transport with separate physical device slots and HID interface channels. Up to 16 physical nodes, 16 input channels, four HID functions per device and five routing nibbles. Multiple endpoints share one address/configuration and one device context correctly.
- USB2 hub descriptor discovery, configuration, port power/reset/status, route strings, parent port/root propagation, hub slot context and transaction-translator information for low/full-speed children behind high-speed hubs. Device discovery is bounded; no recursive unbounded allocation.
- Independent polling of hub ports, child/subtree disconnect releases and stop-on-error handling. DMA/slots with uncertain ownership are retained, not freed speculatively. Replug/re-enumeration and automatic controller reset recovery remain unfinished; cold reboot is required after detach for rediscovery.
- Bounded original HID short-item compiler: report IDs, global push/pop, collections, usage lists/ranges, constant padding, keyboard arrays/modifiers, variable keyboard fields and standard Generic Desktop pointer axes/buttons/wheel. Output/Feature fields do not shift Input offsets. Descriptor bytes are bounded to 1 KiB; 96 admitted fields, 16 input report IDs, 64-byte wire reports and 24 simultaneous held keys are explicit limits.
- Signed relative motion and logical-range-scaled absolute X/Y. The internal absolute events convert into the existing native relative-event ABI at current framebuffer resolution; native apps are unchanged. Unchanged coordinates do not create redundant motion events.
- Independent held state per report ID/interface, modifiers delivered before letters, and reference-counted USB key/button releases across interfaces. When a composite device actually sends absolute axes, its duplicate relative axes are temporarily suppressed; other physical mice remain independent.
- Boot keyboard/mouse compatibility remains. SET_PROTOCOL is used only for advertised boot functions; report-only interfaces use their declared report format. Nonzero alternates use SET_INTERFACE. Static descriptor dumps contain no live input reports, device strings or typed content.

The compiler deliberately rejects unsupported long/local delimiter items, excessive budgets and unrecognized required layouts. It does not implement general touch/digitizer gestures, consumer/vendor controls or arbitrary feature-report negotiations. Both physical interface selection and report-ID support are tested portably. QEMU supplies standard tablet/report and USB2 hub devices; its hub model does not establish real Fusion high-speed transaction-translator behavior or simultaneous multi-interface behavior on the actual `0e0f:0003` device.

## One distribution ISO and one build workflow

Use **CHOS-ARM64.iso** as the primary image. It boots the approved blue/Royale Setup when keyboard and pointing paths initialize. If usable input is absent/incomplete during startup, it automatically displays the input diagnostic panel and rotating reports, skips Setup/filesystem initialization and forces the NVMe write/flush guard. No keypress or second ISO is needed to obtain failure diagnostics. Endpoint initialization is not evidence that real events have reached applications; the live event panel and target test remain required.

The separate M7I INPUT/full-page diagnostic images remain available for repeatable driver evaluation. They retain their original boot-time read-only guards and perform no installation. The normal healthy-input path retains the real installer, exact disk identity/capacity review and typed destructive confirmation.

On a Linux development host with Docker, the CHOS image and a host C compiler/ASan available:

```sh
./scripts/container.sh image   # once, if the build image is absent
bash scripts/checkpoint.sh
```

Commit source changes first. The script runs the complete QEMU/portable validation sequence, host sanitizers, a clean archive-of-HEAD reproduction of the three images, byte comparisons and packaging/checksums into **build/release/**. It never publishes externally or opens host block devices. [Workflow log](evidence/m7i/workflow.log), [full tests](evidence/m7i/validation.log), [sanitizers](evidence/m7i/sanitizers.log), [source reproduction](evidence/m7i/source-package-build.log).

## Validation scope

**All 83 QEMU cases and 20 portable test programs passed**, together with both ELF payload checks, host ASan/UBSan, independent disk verification, repeatable images and byte-identical clean committed-source reproduction of all three distributable images. The one-command workflow completed successfully.

The suite retains the previous 78-case coverage and adds five QEMU boots: native absolute pointer/keyboard directly attached, the same behind one hub, behind two nested hubs, held-button child removal with a remaining usable keyboard, and normal-ISO automatic input fallback on a disposable NVMe disk. The three native cases exercise Setup arrows/Enter/Escape, actual typing and absolute window dragging. Fallback checks actual block-write counters and whole-disk hashes.

Existing unsupported-HID tablet fixtures are updated to non-HID CCID readers because standard report-protocol tablets are now supported. Their safety purpose remains: unclaimed non-input functions must not block later keyboard/mouse discovery. No negative case is silently counted as a successful device. Existing native storage/install/reboot, graphics/apps/settings, EL1/EL2, GICv2/v3, protected memory, large images, malformed-image refusal, USB ring-wrap and disconnect regressions remain.

Portable tests cover report-ID state isolation, signed/absolute axes, keyboard modifiers/rollover/release, capacity-atomic rejection, malformed/truncated/mutated descriptors, interface collection, hub descriptor prefixes/routes, framebuffer scaling and diagnostic saturation. The portable program count remains 20; the new HID tests run inside the existing USB executable. Full actual test results, image hashes and resource measurements are recorded in [the summary](evidence/m7i/summary.json).

## Actual QEMU captures and measured footprint

The package includes 157 real guest framebuffer PNGs, 54 fresh build/test metadata
files and 309 hashed evidence files, plus the manifest itself. Scenario names
containing “fusion” are QEMU regression labels; actual Fusion observations remain
separately attributed in the sanitized target transcript.

| Observation | Actual QEMU capture |
|---|---|
| Native typing/absolute dragging through two hubs | [Nested-hub desktop](evidence/m7i/usb-report-nested-drag.png) |
| Direct report-HID absolute pointer | [Direct desktop](evidence/m7i/usb-report-direct-drag.png) |
| Held hub-child removal releases button; keyboard remains | [Disconnect panel](evidence/m7i/usb-hub-disconnect.png) |
| Same normal ISO automatically falls back, with disk unchanged | [Fallback](evidence/m7i/usb-input-fallback.png) |
| Minimum-resolution input panel | [640×480](evidence/m7i/input-panel-640.png) |
| Existing installed persistence | [File reopened](evidence/m7i/installed-notepad-reopen-512-gic3-bridged.png) |

[Complete evidence manifest](evidence/m7i/files.sha256),
[preserved subsystem/payload checks](evidence/m7i/m7i-preservation.json).

Normal kernel file is 291,104 bytes, with 614,400 bytes declared image memory and
the unchanged 73,728-byte guarded stack reservation. Linked kernel text/data/BSS
totals 605,314 bytes; native app ELF remains 66,408 bytes. In the 512 MiB QEMU
two-app test, allocated conventional RAM was 11,664 KiB out of 489,616 KiB managed,
with 15.423-second readiness including firmware/self-tests/Live selection. These
exclude unmanaged/firmware reservations and are not total guest resident memory
or native Fusion performance. Detailed scoped idle/timing data is in the summary.

## Source and next Fusion test

Started at `cd77e1d`, preserved by `checkpoint/milestone-7i-start`. The implementation/source checkpoint is `54eb2d4`. New modules include `drivers/usb/{report,hub}.c`, `include/hid_report.h`, portable HID fixtures, `tests/usb_m7i.py` and `scripts/checkpoint.sh`. Original xHCI/descriptor selection, input normalization, diagnostic capacity/panel and startup fallback are updated. NVMe's existing operations remain unchanged; a small guard setter enables the same read-only policy on automatic fallback. Kernel memory, interrupts, scheduling, application and Royale implementation source are preserved.

[Download images/checksums](../artifacts/milestone-7i/README.md). Boot the normal ISO in the same Fusion VM: one CPU, 2 GiB, ARM64 UEFI, Secure Boot off. Without a disk first, verify Setup arrows/Enter/Escape, then L/F3 Live Desktop, Start menu, native typing, pointer alignment/dragging and left/right clicks. If automatic fallback appears, capture USB topology, report descriptor/compiler/endpoint errors and the panel. No need to choose another image or guess controller settings.

After input passes on Fusion, repeat the already successful read-only NVMe probes with the INPUT image and a designated disposable disk. Actual installation, disk-only reboot and persistent files are subsequent independent gates. Do not claim target installation/data integrity from QEMU or from HID readiness alone. Networking, SMP, authentication/ACLs, general USB/USB3, full keyboard layouts, recovery and CHOS 0.1 remain unfinished.
