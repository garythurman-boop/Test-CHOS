# M7E ARM64 MMU audit, memory maps and diagnostic codes

Subsequent M7E target screenshots now show MMU/protection/timer success with
531/1024 table pages. The original M7D failed branch was not captured on Fusion;
the reproduced QEMU capacity defect was corrected. M7F proceeds with the separate
[confirmed device-discovery blocker](pci-discovery.md). [Actual screenshots transcribed](fusion-observations-m7e.md).

## Evidence and cause boundary

The user reports M7D normal `CHOS-ARM64.iso` on an Apple M1 MacBook Air, one CPU,
2 GiB RAM, macOS 27 “Golden Gate,” Fusion “26H1u1,” stopping at `MMU INIT FAILED`.
These are user-reported configuration strings, not independently inspected host
metadata. Exact Fusion build, firmware EL, memory map, devices and screenshot have
not been supplied. Reaching that message establishes a kernel-startup failure;
it does not establish working interrupt delivery, input or persistent storage.

The preserved M7D image **also fails in QEMU at 2 GiB**. A separate `b485976` source
archive changes only the two silent table-allocation/capacity branches to explicit
panics. It reaches `MMU TABLE CAPACITY 1024`, proving table exhaustion rather than
contiguous allocation failure **for that QEMU instance**. The accompanying patch,
build log and native screenshot record this controlled experiment. It is not the
Fusion memory map or proof of the same target root cause.

To reproduce the instrumented baseline, extract `git archive b485976` into a new
scratch directory, apply the published `baseline-instrumentation.patch`, build
with the repository container, then run `tests/boot.py --ram 2048` against that
scratch ISO. The validator exits nonzero intentionally on the explicit panic;
inspect its captured framebuffer. Do not substitute this negative image for M7E.

Old RAM mappings used only 4 KiB leaves: the block-mapping branch required AttrIndx
bit 2 (the Device attribute), so Normal RAM never used blocks. Covering 2 GiB alone
needs 1,024 L3 pages plus root/intermediate tables. The fixed 1,024-page pool cannot
cover it. Its allocation also required one contiguous 4 MiB run. Fragmentation is
another possible failure mode, not an observed Fusion diagnosis.

## Corrected table management

- The safety ceiling remains **1,024 table pages** (4 MiB maximum table contents),
  with 8 KiB of kernel address metadata. It is not an arbitrarily enlarged pool.
- Tables allocate **one 4 KiB page on demand**, using only validated conventional
  RAM. No contiguous multi-megabyte allocation is needed. Each table is zeroed;
  table-descriptor pointers are checked against the owned-page registry before use.
- Aligned 2 MiB identity blocks cover Normal RAM **outside managed allocator ranges**
  and suitable device spans. Sparse descriptor edges remain 4 KiB. There are no
  1 GiB blocks and no attempt to map holes as RAM. Large MMIO descriptors are not
  automatically mapped; hardware drivers request bounded device windows explicitly.
- **All managed RAM stays 4 KiB granular.** Kernel threads can acquire/release guards
  on allocator pages after activation without splitting a live block. This retains
  the existing refusal to split active blocks; no unsafe live break-before-make
  shortcut or temporary writable/executable alias was added.
- Text, rodata and bootstrap guards refine/split mappings **before activation**.
  Inactive block splits preserve physical addresses and attributes. Text is EL1
  read-only executable/EL0 NX; rodata is read-only/NX; other RAM is writable/NX.
  WXN and EL0 address-space isolation remain enabled.
- The PMM still manages at most **262,144 pages (1 GiB)** across 128 ranges. Excess
  RAM is identity mapped but reserved from allocation. Larger-memory boot tests
  do not claim that all installed RAM is allocatable. Raising that cap is separate
  work; new managed ranges must remain page-granular or gain a safe live split path.
- Exhaustion remains a bounded, explicit failure. Very sparse/fragmented firmware
  maps can exceed the 1,024-table policy. Partial boot tables are retained on fatal
  startup failure; runtime device mapping failures do not reclaim published tables.

The design reduces table demand as RAM grows beyond the managed cap without
changing the scheduler's guard semantics or adding table-growth allocation races.
CHOS remains single CPU. Registry lookup and validation are bounded linear scans;
they are not a general purpose multiprocessor virtual-memory implementation.

## Firmware/map and address validation

Before any PMM test allocation, `pmm_init` validates descriptor stride (at least
40 bytes, multiple of eight), count (at most 4,096), map span/alignment, nonzero
page count, aligned bases, 48-bit bounds and pairwise non-overlap. Extended firmware
strides are supported; adjacent regions and out-of-order disjoint descriptors are
valid. Only type 7 enters the allocator. Range-capacity and empty-map failures have
explicit PMM error codes. No descriptor address is dereferenced to format an error.

The MMU repeats structural checks against the CPU's supported physical width.
ID_AA64MMFR0_EL1 PARange 0..6 is understood, capped at 48 bits; unknown encodings
and unsupported 4 KiB granules stop before activation. Physical output is bounded
by the lesser of CPU PA width and 48 bits. Overflow checks precede multiplication,
rounding and mapping. Firmware is still trusted to supply an actually readable
loader-owned map buffer; numerical validation cannot prove arbitrary pointers
readable under firmware tables.

Types 1..10 retain the existing Normal WB identity mapping policy. Reserved,
MMIO, persistent/unaccepted and other types are not automatically treated as RAM.
Runtime/ACPI memory is mapped but never allocated. There is no post-exit UEFI
runtime-service support or SetVirtualAddressMap call. Firmware descriptor attributes
are reported on failure; a general per-attribute cache/protection policy is still
unfinished. Validating a map does not mean supporting every firmware regime.

Identity coverage is required for the complete loaded kernel/BSS, guarded stack
allocation, boot-info structure, memory-map buffer, pristine installer payload and
every allocated table page. The current SP must remain inside the usable stack.
Kernel text/rodata boundaries retain the versioned image/linker alignment checks.
The private loader contract and original guarded-stack reservation are unchanged.

GOP spans are checked for rounding/overflow and overlap with live kernel/stack,
metadata/payload or managed RAM. RAM-backed GOP retains Normal WB; other GOP
uses Device-nGnRE. Ambiguous mixed RAM/MMIO spans fail rather than aliasing the
same region with inconsistent attributes. GIC windows must not overlap live RAM
or the framebuffer. Later device mappings reject Normal RAM overlap, malformed
spans and incompatible live remaps; idempotent existing device mappings are allowed.

## Architectural switch audit

The original `arch_mmu_switch` remains the controlled identity trampoline:
clean each owned table to PoC using CTR_EL0 line size, DSB, temporarily clear M,
ISB, program MAIR/TCR/TTBR0, invalidate EL1 TLB, DSB/ISB, enable M plus WXN, ISB.
This temporary transition is not a permanent MMU-off workaround. Existing firmware
cache state is preserved; no dirty firmware cache is blindly discarded. Executing
code, SP, tables and required metadata remain identity mapped before/after the
switch under the ARM UEFI physical-address handoff contract.

New tables use 4 KiB granules, T0SZ=16 (48-bit TTBR0 VA), TTBR1 disabled (EPD1),
inner-shareable WB/WA table walks, checked IPS, MAIR Attr0 Normal WB/WA `0xff` and
Attr1 Device-nGnRE `0x04`. AF, AP read-only, PXN/UXN and table/block encodings are
unchanged. No new executable data or writable code mapping is introduced. Later
page/TLB synchronization retains barriers; active block splitting remains refused.
EL2-to-EL1 entry still supports the original non-VHE identity regime; VHE, LPA2/
52-bit firmware tables, MMU-off and big-endian EL2 entry remain unsupported.
The firmware-to-CHOS transition has QEMU EL1/EL2 coverage; actual Fusion register
values and successful switch remain to be observed.

Private EL0 mappings still reserve the existing 1 GiB slot starting at 64 GiB.
A firmware RAM layout occupying that slot is not supported by the current process
layout. Higher/sparser physical identity coverage does not solve that limitation.

## Retained framebuffer report V1

`MMU STAGE n` appears before each major phase. If construction returns failure,
`CHOS MMU INIT FAILED / REPORT V1` remains on screen; it needs no input, disk,
serial or desktop. It displays numeric/name stage and reason, address, requested
span/argument, descriptor index/type/attributes, pages requested, table count and
capacity, PMM used/total/error/ranges, map entry count/highest end, MMFR0 and captured
old TCR/TTBR0/SCTLR plus planned new TCR. Descriptor index/type `4294967295` means
not associated with one descriptor; descriptor attributes are meaningful only when
a valid descriptor index is present. Uncaptured values are zero (notably an early
PMM-map rejection before CPU register capture). For an unrepresentable descriptor
span, size is saturated to UINT64_MAX and pages requested retains the original
page count. The last stage also identifies a fault/hang inside the switch.

| Stage | Meaning |
|---|---|
| 0 | Reset |
| 1 | CPU features/firmware state |
| 2 | UEFI map and required identity coverage |
| 3 | Root table allocation |
| 4 | RAM descriptor mappings |
| 5 | Framebuffer |
| 6 | GIC |
| 7 | Kernel text |
| 8 | Kernel rodata |
| 9 | Null/bootstrap stack guards and table/SP coverage |
| 10 | Translation-register switch |
| 11 | MMU ready |
| 12 | Later explicit device mapping |

| Reason | Meaning |
|---|---|
| 0 | No failure |
| 1 | Physical page allocation failure |
| 2 | Table capacity (or PMM range capacity on a pre-MMU rejection) |
| 3 | Invalid/out-of-range physical address |
| 4 | Unsupported address-width encoding |
| 5 | Size/rounding/descriptor overflow |
| 6 | Malformed UEFI descriptor/map shape |
| 7 | Overlapping descriptors |
| 8 | Mapping conflict/unsafe active split |
| 9 | Alignment |
| 10 | Unsupported 4 KiB granule |
| 11 | Missing identity coverage |
| 12 | Unowned table pointer |
| 13 | Unexpected firmware MMU state |

PMM error: 0 OK, 1 map shape, 2 zero-page descriptor, 3 alignment, 4 overflow,
5 overlap, 6 range capacity, 7 no allocatable pages, 8 physical address.
Diagnostic drawing reads only captured scalars and bounded static text; it does
not chase the failing physical address. Firmware GOP still must be valid/readable.
The current report targets 1024×768. At the supported 640×480 fallback, the
lower register/attribute/footer rows clip; primary stage/reason/address/table/PMM
fields remain visible. Capture all available fields. A paged compact fatal report
is future work. The report buffer logs codes too, but native serial is not assumed available before
mapping. An unexpected exception retains the existing separate ESR/FAR/ELR screen.

## Validation scope

The host test compiles **production** `virtual.c` and `physical.c`. Only privileged
register/cache/TLB operations and console output are stubbed. It tests 2/8 GiB
layouts, >4 GiB/sparse addresses, hundreds of descriptors, MMIO holes, one-page-only
free runs, table budget exhaustion, malformed/overlapping/overflowing descriptors,
CPU granule/PA rejection, framebuffer overflow/conflict/RAM attributes, unowned
table pointers, identity coverage and actual RO/NX/guard entries. ASan/UBSan also
runs this production mapper test.

Five additional actual UEFI QEMU cases boot 2 GiB at EL1/GICv2, 2 GiB and 8 GiB at
EL2/GICv3; run 2 GiB unattended diagnostics; and verify stage 4/reason 2/table count
8/capacity 8 from a deliberately limited diagnostic build's framebuffer without
serial/input/disk. That controlled failure ISO is never distributed. Normal policy
is 1,024; `make KERNEL_TABLE_CAPACITY=8 diagnostic-iso` is only a negative test.
All earlier installation/persistence/GUI/input/large-kernel regressions remain.
These are synthetic stress tests and QEMU reproductions, **not Fusion memory maps**.
