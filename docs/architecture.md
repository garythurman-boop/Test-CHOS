# CHOS architecture

## Boot contract

ARM64 UEFI loads EFI/BOOT/BOOTAA64.EFI from the ISO's El Torito FAT volume.
The original loader opens CHOS/KERNEL.BIN on its own loaded-image device,
validates an original 64-byte versioned image header, allocates/zeros its declared
file/BSS memory plus a separate guarded stack, and captures
GOP and the ACPI 2.x RSDP from configuration tables. No third-party bootloader
or existing OS kernel participates.

The loader synchronizes code/data caches, retrieves a bounded memory map and
calls ExitBootServices, with at most three map-key attempts. No allocations or
console calls occur between the final map and exit. After any exit attempt,
only map/exit may be retried; final failure halts without more firmware calls.
Earlier failures print diagnostics. Cleanup of earlier boot allocations remains
incomplete; reboot firmware after failure.

x0 carries include/boot.h boot information; x1 is the temporary stack top.
The versioned flat kernel is linked at zero and loaded at a page-aligned arbitrary
address; the loader enters at its validated offset 64.
All C declarations have hidden visibility, including external assembly/linker
symbols. The linker rejects a nonempty GOT, which would otherwise embed absolute
addresses requiring an unavailable runtime relocator. Text/rodata are page aligned;
the shared policy currently permits 16 MiB file bytes and 64 MiB memory. A
separate 64 KiB usable stack has lower/upper guard pages; no fixed-offset guard
occupies the image. The private boot size/version is checked before extended fields.
See [kernel image design](kernel-image.md) for the complete contract.

Firmware may enter at EL1 or supported non-VHE EL2. The original transition
converts firmware TTBR0/TCR/MAIR to EL1 while keeping identity mappings and cache
attributes active, configures HCR/counter/timer/trap controls and GIC system-register
access, and ERETs with DAIF masked to EL1h. VHE, LPA2/52-bit, MMU-off and big-endian
EL2 handoffs are rejected. Firmware mappings are used only during early setup.
An original 2 KiB-aligned vector table then handles all 16 vector slots, preserving
x0-x30, SP_EL0, ELR/SPSR and ESR/FAR in a 304-byte frame. Unknown faults display
syndrome/address/PC and halt. Only exact armed self-test instructions recover.

## Memory ownership and protection

The physical allocator validates UEFI descriptor stride, bounds, alignment,
page-count overflow and overlapping extents. Only type-7 conventional RAM enters
its bitmap; firmware, loader, runtime, ACPI and device memory remain reserved.
Up to 128 ranges and 262,144 pages (1 GiB) are tracked; excess conventional RAM
is left unmanaged. Allocations may be contiguous within one extent, never across
holes. Invalid or double frees fail without partial changes. The single-CPU API
requires interrupts masked if used after scheduling starts.

A bounded 64 KiB heap uses aligned headers, splitting, exact-pointer free checks,
and coalescing. It does not expand or tolerate corrupted kernel-owned metadata.

CHOS installs its own identity address space (up to 48-bit VA/PA): 4 KiB managed-RAM
pages and aligned 2 MiB blocks outside managed ranges,
EL1 read-only/executable text, read-only/NX rodata, and read-write/NX data/heap.
All kernel-template pages deny EL0 access. MMIO framebuffer/GIC use Device-nGnRE;
RAM-backed GOP retains Normal WB. WXN is enabled; TTBR1 is disabled. Translation
tables allocate individual pages on demand, with a 1,024-page (4 MiB) ceiling.
Memory types/identity mappings are preserved across a short MMU-off trampoline;
table cache cleans and TLB/barrier operations precede use. QEMU does not validate
all real-hardware cache/coherency behavior. M7D reached an MMU initialization
failure in the user-reported Fusion test; M7E target retest remains pending.

Null, bootstrap stack guard and thread stack guard pages are absent. Normal RAM
managed by the allocator uses page mappings before activation, so adding guards
never splits a live mapping
covering page-table storage. Live block splitting is deliberately rejected until
an alias-based, break-before-make path exists. Current mappings and table allocation are bounded;
private process spaces are bounded, with no general mmap/demand-paging API.

## Hardware discovery, timer and scheduling

No GIC MMIO addresses or PPI numbers are hardcoded in the kernel. A bounded ACPI
parser validates RSDP/XSDT/table checksums and memory ownership. MADT selects the
boot CPU's GICC using MPIDR, plus GICD; GTDT supplies the non-secure physical timer
PPI. GICv2 and GICv3 with a high-active physical timer PPI are supported. GICv3
selects a direct GICC redistributor or bounded MADT GICR ranges using MPIDR affinity,
including VLPIS stride/Last handling. Misaligned, overlapping, RAM-owned or invalid
resources are rejected. Missing ACPI/unsupported resources halt rather than probing
QEMU addresses. ACPI structures are read bytewise to avoid unaligned loads.

The GICv3 driver initializes distributor and redistributor Group-1 NS interrupts,
priority/trigger/masks, affinity routing, CPU SRE/PMR/BPR/IGRPEN and ICC_IAR1/EOIR1.
Wake and RWP waits use bounded physical-counter timeouts. Only the boot CPU and
physical timer interrupt are used; no ITS/LPI/MSI or SMP support is implied.
The GICv2 driver enables and acknowledges the timer interrupt; AckCtl permits
Group-1 acknowledgement in the unified view. The physical generic timer is rearmed
at nominal 100 Hz based on CNTFRQ_EL0. Rearming from the current counter allows
latency drift; no precise wall-clock or idle-CPU claim is made.

The scheduler supports up to twelve threads including bootstrap and EL0 tasks. Each added
thread gets two usable stack pages and one guard. IRQ frames carry full integer
contexts and exception return restores the selected thread. Highest numeric
priority wins; equal priorities rotate. Strict priority may starve lower work.
A private EL1 SVC immediate terminates a returning kernel thread; it is not a public
user syscall ABI. Exited stacks/slots are reaped after switching away, with guard mappings restored before page reuse.
FP/SIMD context is absent; every kernel C file uses general registers only.

## Processes and services

The static ELF validator rejects malformed/overflowing headers, overlapping pages,
non-AArch64/non-little-endian files, dynamic/interpreter segments and writable
executable mappings. Private process roots clone kernel L0 and the low L1 before
adding user L2/L3 mappings. The same user virtual addresses resolve to different
physical pages. Shared kernel mappings retain supervisor permissions. Context
switches update TTBR0 and invalidate the TLB without ASIDs; this is correct but
not optimized. User-copy routines validate the whole span before using kernel
physical aliases. Code/BSS/stacks are zero-initialized; heap allocations are bounded.

Public SVC #0 provides the versioned ABI in syscalls.md. Process management remains
kernel-controlled plus exit/PID/yield calls. Mailboxes are bounded nonblocking
sender/value queues. Unknown EL0 faults terminate only that process; the supervisor
reaps private pages/tables and its guarded kernel stack after switching away.
EL0 FP/SIMD is explicitly trapped until vector state is saved per task.

NVMe PCI discovery uses MCFG/bus-0 enumeration and firmware-assigned BARs, including
addresses above 512 GiB. Four-level page tables replaced M2's three-level layout
for that hardware. The original driver uses split admin/I/O queues and cache
maintenance for bounded DMA, with flushes before/after COW metadata commits.
See storage.md for the CHFS wire format, file APIs and safety gates.

Modern Virtio PCI input negotiates VERSION_1, validates capability chains and
common/notify/ISR ranges, posts device-writable event buffers and validates used
indices/completion IDs/lengths before decoding. Two independent input queues feed
a bounded raw event FIFO and pointer/key state. The idle loop polls and draws a
small original cursor and last uppercase character. USB/desktop work is not implied.

## Original components

The following sections record the subsequent desktop and installer implementation.
No existing OS kernel, filesystem, GUI framework or input stack is linked into CHOS.

## Desktop and native apps

The kernel window service retains bounded drawing commands and owns the single
GOP backbuffer. Ownership checks precede any draw/event access. Staging prevents
partially submitted frames appearing. The original WM model is portable C and
host tested; hardware input and the renderer are separate. IRQ exclusion serializes
single-CPU compositor/process mutations. Native app event waits sleep, with a
one-second refresh deadline; bootstrap supervision reaps dead processes off-stack.

Six built-ins share one static ELF selected by entry role, but each launch gets
private mappings/state. Original user libraries wrap file/window/info APIs. The
normal live session explicitly formats only newly allocated RAM as CHFS; no real
block device is passed into that path. Persistent NVMe acceptance remains opt-in
and separate. Whole-file REPLACE avoids truncate-before-save data loss.

## Installation-format foundations

Original GPT and FAT32 ESP construction modules are portable/freestanding C. They
operate on an explicit 4 KiB block-device abstraction, with separately declared
512/4096-byte logical sector geometry. Encoders verify exact metadata/payload
read-back and propagate every write/flush error. GPT backup metadata is written
first; complete installations are not transactionally atomic.

These modules are compiled for AArch64 and linked into the live kernel by
milestone 6B. The separate host mkdisk tool creates only new regular fixture files. Real
UEFI disk boot verifies firmware consumption independently of the host parsers.
Guest installer authority, boot-medium exclusion, disk identity binding and system
partition mounting are enforced by the separate native service; library confirmation
alone does not fulfill them. See milestone-6b.md for integration and remaining limits.

## Native installer integration

Milestone 6B links the original GPT/FAT32 modules into the kernel. A trusted native
EL0 Setup role requests read-only disk inventory, then a process-bound typed
confirmation. Optical boot and fresh NVMe identity/geometry checks gate writes.
A preemptible kernel worker owns disk I/O while the live VFS remains RAM backed.
The kernel retains pristine payload bytes before first execution, rather than
installing a mutated running image. The ELF applications are inside that payload.

Installed boot matches the firmware ESP GUID against verified CHOS GPT metadata,
then mounts a bounded CHFS partition and reads the saved computer name. The ISO
uses RAM by default; installed disk sessions use persistent native file APIs. No
login or per-user authority is implied by a saved computer name. Incomplete or
unmountable volumes must not be reported as successfully mounted installations.

## M6C decisions

Retain the original command-list window system and private EL0 application model.
Use build-time licensed outline fonts baked to a compact A8 cache rather than
adding a guest font library. Preserve the existing editor's 12-pixel advance and
introduce proportional text as an additive command. Cache original procedural
wallpaper in a second RGB surface; repaint-time drawing allocates nothing.
Begin dirty painting with pointer-only damage; leave app/window changes fully
invalidated until independently tested. See [Royale](royale.md).

Enumerate/choose supported GOP modes while boot services are active, never call
firmware graphics after ExitBootServices. Reject modes above allocation limits.
Installed-volume mount failures expose an explicit error state and temporary RAM
warning. A mount failure must not trigger repair/format; the disposable-corruption
acceptance verifies the disk hash is unchanged.

## M6D decisions

Keep the original kernel WM and isolated native applications. Centralize control
rendering/input in a small user-space C library with a bounded hit registry, rather
than adding a heavyweight framework or moving application logic into the kernel.
Bind and dismiss modal editing before the next paint, and force registry rebuilds
after layout changes; drain bounded input batches before repainting. Release
capture, pointer coalescing and deterministic pointer-before-keyboard poll batches
handle real multi-window input. See [toolkit](gui-toolkit.md).

Save one explicitly encoded configuration record in CHFS, not multiple independent
preference files. Validate before writes and apply active settings after a successful
commit. Support legacy reads without migration writes. CRC provides corruption
detection, not authentication; a final flush error has uncertain old/new durability.
No account/login or ACL claim is made. See [configuration](configuration.md).

## Dedicated setup session (M6D continuation)

Optical boot launches only the native Setup role on an exclusive borderless client
using the existing kernel/display/input/storage services. Desktop wallpaper is
allocated only after an explicit Live Desktop choice. Installed boot retains the
regular desktop and persistent mount path. The existing installer worker and all
identity/confirmation checks are preserved. Original read-only partition inventory,
measured payload copying, complete CHFS read-back and installation logging extend
that worker. UI stage changes synchronize with actual displayed native frames,
using a bounded cooperative kernel-thread yield, without synthetic progress timers.
See [setup-environment.md](setup-environment.md) for the flow, layout, authority,
failure model and limits. No second OS/kernel, firmware disk writer or new GUI
framework is introduced.

## M7A/7B diagnostics and refinement

Read-only EFI PCI I/O enumeration runs before final GetMemoryMap; at most 32
records and bounded firmware vendor text enter private boot info. Static offsets
protect the assembly entry contract. A kernel-owned 688-byte versioned snapshot
reports actual EL, selected GICR, ACPI resources, initialized drivers and GOP, not
assumed platform constants. Syscall 33 validates the entire user destination before
copying. No firmware protocol calls occur after ExitBootServices. Full USB/HID
and ACPI dump facilities are not present. See [matrix](fusion-compatibility-matrix.md).

Native Setup uses the shared integer-only binary-size formatter for capacities,
partitions, logical sectors and real byte progress. No geometry or theme redesign.
Account authority/entropy/hash/session prerequisites are specified separately in
[accounts-design.md](accounts-design.md); none are implied by this checkpoint.

## M7C automatic discovery and USB input

The original loader retains firmware and UEFI revisions and recognizes a bounded
optical DIAG.CFG marker. The diagnostic variant uses the same kernel with explicit
no-installer mode; it rotates a framebuffer report without input and does not
mount/format the system filesystem or export to disk. Native log storage is bounded
and read-only to apps. ACPI inventory validates table extents/checksums; PL011
UART output uses only a validated SPCR MMIO address mapped by CHOS, with bounded
transmit waits. There are no post-exit firmware protocol calls or guessed UARTs.

Original xHCI/ring/boot-HID modules share PMM/MMIO/cache/event facilities and feed
the existing input FIFO. Eight interrupt-IN receive TDs per device tolerate GUI
rendering delays; each has a distinct bounded DMA region. Descriptor publication
uses release fences, completion consumption acquire fences, and native cache/DMA
barriers. IRQ routing, broad device support and recovery remain explicit limits.
See [USB architecture](usb.md) and [M7C](milestone-7c.md).

## M7E MMU update

Translation-table allocation is now individual and bounded rather than a contiguous
4 MiB reservation. Normal RAM outside managed allocator ranges uses 2 MiB blocks
where alignment permits. Managed RAM remains 4 KiB granular for runtime stack guards;
text/rodata and bootstrap guards are refined before activation. Granule/PA/map/
identity validation and retained framebuffer reports precede the switch. The table
ceiling remains 1,024 pages. [Full audit and limits](mmu-architecture.md).
