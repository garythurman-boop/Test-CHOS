# ARM64 compatibility matrix — M7E checkpoint

**Real M7D Fusion test: kernel MMU initialization failed.** The user reports
an Apple M1 MacBook Air, one CPU, 2 GiB RAM, macOS 27 “Golden Gate,” Fusion
“26H1u1,” and normal M7D `CHOS-ARM64.iso`. Full build identifier, automatic versus
manual optical path, firmware inventory and memory map have not been supplied.
No target host is attached here. **M7E Fusion retest pending.** QEMU/stress results
are separate from the user-reported Fusion failure.

| Subsystem | Existing CHOS implementation | Fusion requirement / missing evidence | Status | Required changes / test evidence |
|---|---|---|---|---|
| MMU | Validated CHOS identity tables, page-granular managed RAM, 2 MiB blocks elsewhere, individual table pages and retained failure codes | Retest M7E on the same 2 GiB configuration; capture stage/reason or discovery pages | M7D user-reported failure; M7E QEMU correction, Fusion pending | [MMU audit](mmu-architecture.md); actual Fusion map/cause unconfirmed |
| Optical boot | Original AA64 PE, EFI El Torito FAT, `EFI/BOOT/BOOTAA64.EFI`, bounded ExitBootServices | Exact release must accept custom ARM64 optical UEFI boot; Secure Boot off | QEMU pass; user reports reaching MMU failure, automatic/manual path unknown | Capture automatic catalog/path execution; ISO/EFI inspection logs and boot JSONs |
| Exception levels | EL1 direct; original non-VHE EL2→EL1 transition preserving firmware tables until kernel translation replaces them | Actual entry EL, HCR/TCR and mapping regime | Both tested in QEMU | `boot-gic2-el2.json`, `boot-gic3-el2.json`; validate target handoff |
| Interrupts | ACPI GICv2 or GICv3; MPIDR-selected GICC/GICR, distributor, CPU interface, ack/EOI | Actual MADT GIC revision/resources and accessible CPU sysregs | Both tested in QEMU | Original GICv3 path; affinity/stride/masking/timeout host tests; actual target MADT required |
| Timer | GTDT non-secure physical PPI, CNTFRQ, nominal 100 Hz | Actual PPI/flags and physical timer permission | QEMU pass at both EL handoffs | Protection, preemption, IPC and input checks on all four EL/GIC combinations |
| ACPI/CPU topology | Checksummed RSDP/XSDT/MADT/GTDT; MCFG segment 0/bus 0; boot CPU only | Actual tables, PSCI conduit and CPU affinity | QEMU pass; no DT-only or SMP | ACPI-off refusal; capture target tables before modifying discovery |
| Storage | Original polling NVMe, one controller/namespace, 512/4096 sectors, GPT/FAT32/CHFS | Actual IDs/class, BARs, namespace, DMA and geometry | QEMU install/persistence pass; Fusion unknown | Capture PCI inventory, then test actual controller; SCSI/SATA absent |
| Input | Virtio PCI and original polled xHCI USB2 boot keyboard/relative mouse | Actual USB controller(s), PCI class/programming interface, topology and HID descriptors, or other interface | Both tested only in QEMU; Fusion unknown; USB interrupts/hubs/general HID/other controllers absent | Capture actual target IDs and descriptors; xHCI support is a subset, not an assumed Fusion requirement |
| Display | GOP RGB/BGR linear framebuffer, pre-exit mode choice, original renderer | Actual format/stride/mode and post-exit mapping lifetime | QEMU pass; Fusion unknown | Test target pointer alignment, clipping, redraw and framebuffer stability; no VMware GPU driver claim |
| Network | No driver/stack | Actual adapter IDs/protocol | Unsupported | Later after boot/input/storage stability |
| Installation | Confirmed native GPT/ESP/CHFS writer, verification, reset, disk-only persistence | All target hardware prerequisites plus optical exclusion/disk identity | QEMU pass on both sectors and EL1/GICv2, EL2/GICv3 | Separate full Fusion checklist; QEMU cannot satisfy it |
| Performance | Allocator/render counters, bounded host vCPU observation | Actual host/VM native memory and CPU measurements | QEMU only | Report scope/units; no inferred Fusion performance |

## Research provenance

On 2026-10-23 the workspace HTTPS proxy rejected retrieval attempts for Broadcom
[315602](https://knowledge.broadcom.com/external/article/315602), the Fusion 13
guide, UEFI 2.11 and Arm GIC documentation with tunnel `403 Forbidden`.
Their current contents were unavailable. Follow-up references, not verified claims:

- [Fusion documentation](https://techdocs.broadcom.com/us/en/vmware-cis/desktop-hypervisors/fusion-pro/13-0.html)
- [UEFI specifications](https://uefi.org/specifications): AArch64 binding, GOP,
  PCI I/O protocol, removable path and optical EFI boot.
- [Arm GIC architecture](https://developer.arm.com/documentation/ihi0069/latest/)
- [QEMU virt](https://www.qemu.org/docs/master/system/arm/virt.html), the tested
  machine, not a description of Fusion.

Read-only Git retrieval worked. Reviewed `open-vm-tools` at
[`1634dfb42f723d2c1b3b6cf2696efc82407fae5b`](https://github.com/vmware/open-vm-tools/tree/1634dfb42f723d2c1b3b6cf2696efc82407fae5b)
(README and ARM64 guest-interface source), and `edk2-platforms` at
[`f72b7cdf572f634c7b15ace80f8d4d8941df7735`](https://github.com/tianocore/edk2-platforms/tree/f72b7cdf572f634c7b15ace80f8d4d8941df7735)
(platform inventory). Neither establishes a target Fusion device topology. No
code from these projects is linked into CHOS. Linux drivers and x86 VMware lists
are not an ARM64 inventory or a foundation for this kernel.

## Actual capture infrastructure

The original loader enumerates standard EFI PCI I/O handles read-only before the
final memory map. It prints BDF, vendor/device and class/programming-interface IDs,
retains at most 32 records and frees the handle buffer before exit. Firmware vendor
and entry EL are retained. The kernel adds actual current EL, ACPI GIC resources,
selected redistributor, timer, initialized input/storage and GOP geometry. Terminal
`hardware` shows a bounded snapshot through syscall 33 (first eight PCI entries in
visible history; 32 in the ABI). M7C adds a separate automatic diagnostic ISO, bounded 16 KiB read-only log export,
firmware/UEFI revisions, ACPI table mask, driver status and PL011 UART discovery
from SPCR. The independent bounded USB driver does not establish Fusion support.

The QEMU probe compares firmware PCI IDs with independent QMP inventory and verifies
an unconfirmed disk remains byte-identical. Full target ACPI tables and USB/HID
reports still need firmware diagnostics; this snapshot is not a complete dump tool.
See [manual checklist](fusion-validation.md) and [iteration report](milestone-7e.md).

M7C also tests a no-input VM with USB and unsupported storage inventory and an
unchanged disposable disk, USB-only native editing/save/reopen/mouse/ring-wrap/
disconnect, zero-port xHCI refusal, non-boot tablet rejection and OHCI inventory.
See [USB subset](usb.md) and [input-free Fusion package instructions](fusion-first-boot.md).

M7D changes bootstrap sizing/validation and diagnostic resilience, not Fusion
hardware support. QEMU validates a versioned file/BSS allocation contract with an
independent guarded stack, controlled large-file/BSS boots and installation,
malformed/old-loader rejection, and no-disk diagnostics despite optional input
negotiation failure. The existing polled xHCI driver is preserved. Actual Fusion
PCI/USB/GIC/storage/display observations are still required before target corrections
or destructive installation tests. [Kernel report](kernel-image.md).

M7E reproduces the old 2 GiB table-pool exhaustion under QEMU with a two-branch
instrumented preserved baseline. The new image passes larger-memory QEMU tests;
that is a demonstrated correction of an independent defect, not proof of the
Fusion root cause. The first Fusion retest must capture the new failure codes or
all automatic inventory pages before any installation is attempted.
