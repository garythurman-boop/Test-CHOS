# M7E — real Fusion failure investigation and MMU hardening

**M7D failed on the user's M1 MacBook Air; M7E Fusion retest is pending.** This
checkpoint corrects a demonstrated larger-memory QEMU defect and provides precise
framebuffer diagnostics for the next target trial. It does not claim Fusion boot,
input, installation or persistent-storage success.

## Reported target evidence

The user reports one virtual CPU, 2 GiB RAM, macOS 27 “Golden Gate,” Fusion
“26H1u1,” and normal `CHOS-ARM64.iso` from the M7D folder, stopping at
`MMU INIT FAILED`. These are reported strings, not independently queried host
metadata. Exact Fusion build number, firmware/PCI inventory, physical memory map,
automatic/manual optical path and a failure screenshot have not been supplied.
The evidence supersedes the previous “Fusion never tested” status: the kernel
executes far enough to return an MMU initialization failure. Later hardware gates
remain open; do not infer delivered interrupts from successful ACPI discovery.

Baseline `b4859764aebe78cb0d51ade4ebfc5cff0fca3d2a` is preserved by
`checkpoint/milestone-7e-start`. The workspace was clean at entry. Runtime/source
checkpoint is `a6ce6eb1c3987206800b1b8d8e62196dc9bcb019`. The original kernel,
bootloader, Royale desktop, applications, installer and tested polled USB path
remain. No speculative VMware USB/storage driver or unrelated redesign was added.

## Demonstrated defect and correction

The unchanged M7D image also fails with 2 GiB in QEMU. A separate source archive
adds explicit panics only to the old table-allocation/capacity branches; its real
UEFI boot shows **“MMU TABLE CAPACITY 1024.”** The pool allocation succeeded in that
experiment. [Actual capture](evidence/m7e/m7e-baseline-pool.png),
[instrumentation patch](evidence/m7e/baseline-instrumentation.patch).

The old mapper allowed block mappings only for Device memory, forcing all RAM
through 4 KiB leaves. Two GiB needs 1,024 leaf-table pages plus intermediate/root
pages, exceeding the fixed capacity. Its 4 MiB contiguous allocation was a second
fragility, but **fragmentation has not been established as the Fusion cause**.
The target's specific failed branch is still unknown.

The corrected mapper keeps the existing 1,024-page ceiling, allocates individual
4 KiB table pages and uses aligned 2 MiB blocks outside PMM-managed ranges. Managed
RAM remains 4 KiB granular so runtime thread guards never require unsafe live
block splitting. Offline text/rodata/guard refinements preserve permissions. Page
pointer ownership, CPU PA/granule limits, descriptor bounds/overlap, mapping
arithmetic, framebuffer conflicts and complete transition identity coverage are
checked. PMM failures also retain detailed fields. This corrects the reproduced
QEMU defect without an arbitrary larger pool or weaker protections.

The retained framebuffer report provides stage/reason numbers and names, failing
address/span, descriptor index/type/attributes, requested pages, table usage/cap,
PMM usage/error/ranges, map count/highest end and CPU/translation-register fields.
No serial, desktop, keyboard, mouse or disk is required. Unavailable values remain
zero; errors never dereference the failing address. Success continues automatically
to the existing rotating inventory. Full [architecture, audit, codes and remaining
limits](mmu-architecture.md).

## Build and tests

```sh
./scripts/container.sh image
./scripts/container.sh make -j4 iso diagnostic-iso
./scripts/container.sh make test-mmu
./scripts/container.sh bash scripts/validate.sh
bash scripts/sanitize.sh
```

The normal image is `CHOS-ARM64.iso`; the diagnostic build still uses legacy
internal `build/CHOS-ARM64-M7C.iso`. The published identical diagnostic bytes are
renamed **CHOS-ARM64-M7E-DIAGNOSTIC.iso**. The deliberately capacity-limited negative
image and instrumented baseline are not distributable images.

**All 66 unique QEMU cases and 18 host C programs passed**, along with
both real ELF payload validators, ASan/UBSan, normal/diagnostic/storage-test ISO
repeatability, independent disk parsers and clean committed-source reproduction.
The five new QEMU cases cover 2 GiB EL1/GICv2, 2 GiB EL2/GICv3, 8 GiB EL2/GICv3,
2 GiB no-input/no-disk automatic diagnostics and an eight-table capacity failure
verified by actual framebuffer stage 4/reason 2/count 8/cap 8. Host tests compile
production MMU/PMM logic with only privileged operations/output stubbed; synthetic
maps are explicitly stress fixtures rather than imported Fusion maps.

The earlier 61 cases retain all EL/GIC combinations, faults/RO/NX/null/stack/EL0
isolation, USB-only editing/ring wrap/disconnect/remaining-device operation,
unsupported controller handling, NVMe/CHFS and both sector geometries, real native
installation and disk-only reboots, persistent files/settings, malformed media,
installer confirmation/no-write/error/retry safety, Royale/app regressions and
controlled large-file/BSS kernel boots/installation/persistence. No test installs
to a host block device or a Fusion disk.

The clean `git archive` build uses committed runtime source `a6ce6eb`, rebuilds both
ISOs and all host programs, and compares complete image SHA-256 values. Reproduction
is within the recorded toolchain; future package revisions are not pinned.

[Machine-readable summary](evidence/m7e/summary.json),
[full validation](evidence/m7e/validation.log),
[sanitizers](evidence/m7e/sanitizers.log),
[clean source build](evidence/m7e/source-package-build.log),
[linked sizes](evidence/m7e/linked-sizes.log).

## Artifacts and actual captures

[Both images, checksums and download instructions](../artifacts/milestone-7e/README.md).
The package contains 125 actual framebuffer captures and 48 test/experiment metadata
files. All screenshots are genuine QEMU captures, not generated GUI images.
There is no supplied Fusion screenshot or automated target pass.

| Observation | Capture |
|---|---|
| Preserved M7D generic failure at 2 GiB | [Baseline](evidence/m7e/m7e-baseline-2048.png) |
| Instrumented baseline confirms exhausted tables | [Capacity 1024](evidence/m7e/m7e-baseline-pool.png) |
| Corrected 2 GiB and 8 GiB kernel | [2 GiB EL2](evidence/m7e/mmu-2048-el2.png), [8 GiB](evidence/m7e/mmu-8192-el2.png) |
| No-input/no-disk 2 GiB inventory | [Diagnostic page](evidence/m7e/mmu-diagnostic-2048.png) |
| Bounded resource failure with precise codes | [Eight-table negative test](evidence/m7e/mmu-capacity-failure.png) |
| Preserved native desktop | [Native Notepad](evidence/m7e/apps-notepad.png), [compact dialog](evidence/m7e/small-dialog-hint.png) |
| Native installed cold-boot persistence | [Saved file reopened](evidence/m7e/installed-notepad-reopen-4096-gic3-el2.png) |

The normal kernel file is **262,432 bytes**, declared image memory **491,520 bytes**,
plus the unchanged 73,728-byte guarded bootstrap stack allocation. Linked kernel
text/data/BSS is **484,346 bytes**; native app total remains **66,408 bytes**.
The 512 MiB two-app case measures **11,664 KiB allocated conventional RAM out of
489,764 KiB managed**, versus 14,720 KiB allocated at M7D. These exclude unmanaged/
firmware reservations and are not total guest resident memory. Readiness is
**14.871 seconds** including firmware, self-tests and Live selection under TCG;
these are emulator observations, not Fusion performance. The controlled enlarged
kernel file is 1,311,008 bytes, with 4,685,824 bytes declared memory and far
protection probes at offsets 1,175,652 and 4,681,728. Final idle observation
and controlled-growth sizes/table usage are recorded in summary/serial evidence.

Initial table construction used **254 pages at 512 MiB**, **529 at 2 GiB** (both
EL handoffs), and **536 at 8 GiB**, with the ceiling unchanged at 1,024. These
counts precede later explicit device mappings. The final idle probe used 0.61
host CPU seconds in 10 seconds (6.1% of one host core), a TCG observation rather
than native guest/Fusion utilization.

## Files and next milestone

New `include/mmu_status.h`, `kernel/memory/report.c`, `tests/mmu.c`,
`tests/mmu_boot.py`, this report and `docs/mmu-architecture.md`. Updated original
`kernel/memory/virtual.c`, `physical.c`, startup/error handling, private PMM metadata,
Makefile/test/sanitizer orchestration and current architecture/status/compatibility/
first-boot documentation. The UEFI loader and EL2/MMU assembly remain unchanged;
they were audited rather than replaced. Exact runtime changes are in `a6ce6eb`.

**Next: retest the diagnostic image on the same M1 VM with one CPU and 2 GiB**,
Secure Boot off and no disk/network. [Detailed retest instructions](fusion-first-boot.md).
Capture the complete retained failure report or every rotating inventory page,
ISO checksum, exact Fusion About version/build, macOS version and automatic/manual
boot path. Passing the old failure point is a separate result from functional
keyboard/mouse/display/storage. Diagnose the observed next blocker before adding
hardware code; no destructive installation until prerequisites pass.

Limits include 1 GiB tracked PMM/128 ranges, 1,024 table pages, 48-bit supported
identity regime, unsupported EL2 VHE/LPA2 cases, reserved 64 GiB EL0 slot, polled
USB subset without interrupts/ownership/hubs/general HID/recovery, bounded NVMe,
4 MiB CHFS, unfinished authentication/ACLs/entropy/full regional settings/fsck,
SMP and networking. The fatal report’s lower register/footer rows clip at the
640×480 GOP fallback; primary failure fields remain visible. CHOS 0.1 and full Milestone 7 remain incomplete.
