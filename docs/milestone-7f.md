# M7F — confirmed PCI discovery correction and input/NVMe trial

**M7E boots on the actual M1/Fusion VM. M7F input/storage retest is pending.**
This checkpoint corrects a discovery defect established by the supplied target
screenshots and adds a safe, visible next trial. It does not mark keyboard/mouse,
NVMe reads, installation or persistent boot as working on Fusion.

## Actual evidence and preserved state

The user supplied three real M7E automatic diagnostic pages. They show EL1 firmware
and kernel, ACPI checksums, GICv3/PPI 30 with 31 delivered ticks, MMU/protection/core
tests and 1024×768 GOP passing. MMU table use is 531/1024. The user also reports the
normal ISO reaches Setup. Input count is zero; storage reports `NVME PCI CONTROLLER
NOT FOUND`. [Exact transcribed fields/BDFs and provenance](fusion-observations-m7e.md).
The original images remain in the conversation; no fabricated target capture is
published. Host is the reported M1 MacBook Air, one CPU, 2 GiB, macOS 27 “Golden
Gate,” Fusion “26H1u1”; full Fusion build remains unspecified.

Started from clean `52487b9fa283f337d824b6bcc055501c2a424ce4`, preserved by
`checkpoint/milestone-7f-start`. Runtime/source checkpoint:
`b87f129813c55ad19c586e0d27d918c2ccfeb687`. No newer work was discarded.

## Confirmed blocker and implementation

The target xHCI `15ad:077a` is at **0000:0a:00.0**; NVMe `15ad:07f0` is at
**0000:12:00.0**. The original xHCI and NVMe loops scanned only bus 0. The independent
UEFI inventory found them, but the drivers could not reach their configuration or
capabilities. This is a confirmed discovery cause, not evidence of which controller
owns keyboard/mouse or whether later hardware initialization will succeed.

New shared original PCI code selects firmware BDFs, validates ACPI MCFG resources
and segment/bus/function bounds, maps the config page, then rechecks live ID/class.
It retains the bus-zero fallback when hints are absent, rejects inaccessible/stale
hints explicitly and preserves the bounded polled xHCI/NVMe hardware paths. BAR
validation and PCI decode/bus-master enabling are shared. Loader inventory now
visits all bounded handles and prioritizes endpoints over bridges within the same
32-record ABI. [Full PCI architecture/limits](pci-discovery.md).

xHCI reports IDs/BDF/config/MMIO, capabilities, ports/PORTSC, ownership/reset stage,
connected count, enumeration failure stage, HID interface/protocol and operational
count. The existing HID parser/input API and eight-receive pipeline remain. No
UHCI/EHCI implementation is added without attachment evidence; USB interrupts,
firmware ownership takeover, scratchpads/hubs/general HID/USB3/recovery remain
unsupported. A failed supported attempt returns to diagnostics rather than panicking.

NVMe reports selected resources, CAP/version, ready budget, identify/namespace
results, sectors/format and queue failure stage/completion. Portable capability/
namespace parsers validate supported queue depth, page size, stride, format index,
metadata/protection, capacity and arithmetic. Both diagnostic modes reject writes/
flushes at driver level, skip filesystem/Setup and read only the first/last 4 KiB
without displaying contents. Normal confirmed installation remains functional.
No controller/namespace operational claim follows merely from class detection.

The new **CHOS-ARM64-M7F-INPUT.iso** retains rotating reports with a fixed lower
hardware test panel. It displays actual USB-ready state/path, controller ID,
key code/value, pointer coordinates/buttons and event count. The panel uses the
original event queue; a single last event is volatile and never logged/exported/
persisted. Its layout fits the supported minimum 640×480. The original Royale
apps/window manager/installer design are preserved. [Driver stages and policy](driver-diagnostics.md).

## Build and validation

```sh
./scripts/container.sh image
./scripts/container.sh make -j4 iso diagnostic-iso input-iso
./scripts/container.sh make test-memory test-pci test-nvme
./scripts/container.sh bash scripts/validate.sh
bash scripts/sanitize.sh
```

**All 75 unique QEMU cases and 20 host C programs passed**, along with both
real ELF payload validators, ASan/UBSan, repeatable normal/diagnostic/input/storage
images, independent disk parsers and clean committed-source reproduction of all
three distributable images. All 66 M7E regressions remain: all EL/GIC combinations,
MMU/RO/NX/guards/EL0 isolation, large file/BSS, malformed-image/old-loader rejection,
USB stress/disconnect/failures, NVMe/CHFS, 512/4096 sectors, native installation and
cold persistent boots, installer safety/error/retry, apps/Royale/settings/rendering.

Nine new QEMU instances comprise:

1. Preserved M7E diagnostic image with bridged xHCI/NVMe: expected input/storage
   discovery failure, with disk unchanged. This reproduces the bus-scan defect;
   it does not emulate all VMware internals.
2. New input ISO, nonzero-bus xHCI/NVMe, EL1/GICv3/2 GiB: live key press/release,
   pointer movement/buttons, held-button unplug and remaining keyboard operation;
   identify/read-only first/last reads, zero disk writes and unchanged disk hash.
3. Normal Setup/Live Desktop with bridged USB/NVMe: real keyboard selection and
   native mouse/Notepad interaction; unconfirmed disk remains unchanged.
4. Input ISO with no input peripherals/disk: usable automatic test panel.
5. Zero-port xHCI refusal: panel/report survives initialization failure.
6. NVMe with unsupported MQES: detected/selected controller is explicitly rejected
   before queues/data writes, rather than reported absent or operational.
7–9. Disposable bridged native installation and two disk-only persistent boots,
   using USB input at EL1/GICv3/2 GiB and independent on-disk validation.

Host tests use actual VMware IDs and bus 0a/12 with fake live config memory, real
ACPI MCFG math/segments/nonzero StartBus, BAR/overflow/revalidation/mapping failure,
prioritized inventory, and NVMe CAP/namespace malformed/unsupported cases. QEMU
uses QEMU devices, not VMware controller internals; no Fusion pass is inferred.

[Summary](evidence/m7f/summary.json), [full validator](evidence/m7f/validation.log),
[sanitizers](evidence/m7f/sanitizers.log),
[clean source build](evidence/m7f/source-package-build.log),
[linked sizes](evidence/m7f/linked-sizes.log).
Reproduction is within the recorded toolchain, not arbitrary future package versions.
Publication also strengthens the NVMe refusal screenshot wait; its focused fixture
passed with unchanged guest image hashes.

## Artifacts and actual QEMU captures

[Three ISOs, SHA-256 and download instructions](../artifacts/milestone-7f/README.md).
The standard diagnostic target retains legacy internal `build/CHOS-ARM64-M7C.iso`;
publication renames identical bytes to `CHOS-ARM64-M7F-DIAGNOSTIC.iso`. The new
input filename is produced directly. Neither is a completed release.

| Controlled QEMU observation | Capture |
|---|---|
| Preserved M7E nonzero-bus discovery failure | [Baseline page](evidence/m7f/fusion-m7e-baseline-nonzero.png) |
| Live numeric hardware key press | [Input panel](evidence/m7f/fusion-input-key.png) |
| Mouse movement/button held | [Pointer fields](evidence/m7f/fusion-input-mouse.png) |
| Disconnect releases button; keyboard continues | [Panel after unplug](evidence/m7f/fusion-input-disconnect.png) |
| Input-free/controller failure | [Absent](evidence/m7f/fusion-input-absent.png), [refusal](evidence/m7f/fusion-input-refusal.png) |
| Normal Setup and native app | [Setup](evidence/m7f/fusion-bus-setup.png), [Notepad](evidence/m7f/fusion-bus-notepad.png) |
| Bridged installed persistence | [File reopened](evidence/m7f/installed-notepad-reopen-512-gic3-bridged.png) |

The package contains 142 actual QEMU captures and 49 final-run metadata files,
plus the separately attributed target-observation transcript. These are actual guest framebuffer captures. Names containing “fusion” identify the
regression scenario, not the hypervisor used for those captures. Actual target
facts are separately attributed to the user's screenshots.

Normal kernel file is 270,624 bytes; declared memory is 503,808 bytes, plus the
unchanged 73,728-byte guarded bootstrap stack reservation. Linked kernel text/data/
BSS totals 494,730 bytes; the native app ELF remains 66,408 bytes. At 512 MiB the
two-app test measured 11,664 KiB allocated conventional RAM out of 489,744 KiB managed,
with 15.349-second readiness including firmware/self-tests/Live selection under TCG.
These exclude unmanaged/firmware reservations and are not Fusion performance or
total guest resident memory. The final idle probe measured 0.77 host CPU seconds in 10 seconds (7.7% of one
host core), an emulation cost rather than native guest idle percentage. The
controlled growth build had 1,319,200 file bytes and 4,698,112 declared memory bytes.
All detailed probe measurements are recorded in summary.

## Changed files, remaining limits and next milestone

New `drivers/pci/{discover,resource}.c`, `include/pci.h`,
`drivers/storage/nvme_format.c`, `include/nvme_format.h`, host PCI/NVMe tests,
bridged QEMU topology/device tests and PCI/driver/target-observation documentation.
Updated original xHCI/NVMe, ACPI/UEFI inventory, input fallback, diagnostic screen/
mode/event loop, build/ISO/validation/sanitizer scripts and current status/matrix/
instructions. Exact runtime diff is in `b87f129`.

Retest the [INPUT ISO without a disk first](fusion-first-boot.md). Capture every
rotating controller/port/enum page and live keyboard/mouse changes. Then use only
a designated disposable NVMe disk for read-only identification/reads. If xHCI has
no suitable devices or reports unsupported ownership/capabilities, use those actual
results to decide the next controller/HID change. Do not guess attachment from IDs.
Actual Setup input, storage initialization/reads, confirmed installation and
installed persistent boot are separate gates; no destructive Fusion installation
until prerequisites pass.

Other limits remain: one xHCI/one NVMe/namespace 1, bounded polled I/O, 32 retained
PCI records, no general bridge walk/hotplug/resource sizing/IOMMU, USB subset above,
1 GiB PMM/128 ranges, 1,024 table pages, 48-bit identity regime, 4 MiB CHFS, unfinished
authentication/ACLs/entropy/full regional settings/fsck/recovery, SMP and networking.
The existing fatal MMU report's lower rows clip at 640×480; the new input panel fits.
Milestone 7 target acceptance and CHOS 0.1 remain incomplete.
