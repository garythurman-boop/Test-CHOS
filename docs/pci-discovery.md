# M7F — firmware BDF discovery and resource validation

## Confirmed target defect

The user's three M7E Fusion diagnostic screenshots identify xHCI `15ad:077a` at
`0000:0a:00.0` and NVMe `15ad:07f0` at `0000:12:00.0`. Both original drivers scanned
only segment 0, **bus 0**, all slots/functions. The loader's independent EFI PCI I/O
inventory already saw these nonzero-bus endpoints, but the drivers never reached
their capability registers. Page 3 reports no input and `NVME PCI CONTROLLER NOT
FOUND`, without xHCI initialization output. This establishes a discovery blocker;
it does not establish controller/HID/namespace usability after correction.

## Corrected path

`drivers/pci/discover.c` shares discovery between xHCI and NVMe. It selects matching
class/subclass/interface records from the bounded firmware inventory, obtains the
requested segment/BDF's config page from checked ACPI MCFG, maps only that 4 KiB
page, and rechecks the **live vendor/device and class** against the hint. Stale,
malformed or inaccessible hints return distinct errors. A known inaccessible
controller is never disguised as absent by falling back to another bus.

MCFG lookup validates checksums/ownership through the existing ACPI parser,
entry/table shape and count (at most 64 windows), reserved entry fields, segment,
StartBus/EndBus, BDF bounds, base alignment, physical span and RAM exclusion.
ACPI MCFG's base corresponds to **bus zero**, even when StartBus is nonzero:
`base + (bus << 20) + (slot << 15) + (function << 12)`. Segment IDs are not hardcoded
to zero for firmware hints. The original bus-zero scan remains a compatibility
fallback only when there is no matching firmware hint; Virtio's existing bus-zero
path is retained. USB discovery no longer depends on a successful Virtio ECAM path.

The EFI loader now visits all of at most 256 handles rather than stopping after
32. The handoff still retains at most 32 records and preserves its private/public
layout. When full, it favors USB/NVMe/Virtio-input endpoints, other storage/display,
then other endpoints over bridges. Duplicate BDFs are ignored. A bridge-heavy
inventory therefore need not hide a later critical endpoint. This is bounded
prioritization, not a complete PCI enumerator or inventory export.

BAR0 decoding accepts only supported 32/64-bit memory BAR encodings, rejects I/O,
reserved encodings, zero/misaligned addresses and 48-bit span overflow. Live PCI
command reads/writes enable and verify memory decoding/bus mastering, preserving
other command bits. Resources remain firmware assigned; CHOS does not resize BARs,
program bridge windows or perform unsafe active-controller BAR sizing probes.
Known MMIO windows remain bounded (xHCI 64 KiB, NVMe 16 KiB); actual BAR extents are
not independently sized. Register offsets are bounded inside those windows.

No VMware vendor whitelist or QEMU addresses are introduced. Class selection is
followed by actual capability validation. The original xHCI/NVMe hardware logic,
polled completion path, shared HID decoder and CHOS input queue remain.

## Reporting and limitations

Each selected driver reports name, segment/bus/devfn, device/vendor DWORD, mapped
config address and discovery result. Encoded devfn is `slot*8+function`; the value
printed as `segment/bus/devfn` is `(segment<<16)|(bus<<8)|devfn`. For example `0xa00`
is segment 0, bus 0a, device/function 00.0. The inventory retains standard BDF lines.
PCI device/vendor DWORD uses vendor in the low 16 bits; the input panel shows the
familiar `vendor:device` order.

Discovery result: 0 live match, 1 ACPI/MCFG/BDF resource rejected, 2 mapping rejected,
3 live ID/class mismatch, 4 no matching hint or bus-zero function, 5 invalid record
count. Discovery and driver selection are not controller initialization or I/O.

One xHCI and one NVMe controller are selected. No exhaustive post-firmware bridge
walk, PCI hotplug, resource reassignment, arbitrary function access syscall or
IOMMU support is added. Critical devices omitted beyond the bounded inventory can
still require broader enumeration. Unsupported UHCI/EHCI are identified by class;
their presence does not identify the keyboard/mouse attachment path.

## Validation

Host tests use the actual VMware IDs and buses 0a/12 with live fake config pages,
checking selection, mismatches, MMIO denial, absent devices, legacy fallback, BAR
encodings/bounds and prioritized inventory. ACPI tests exercise actual MCFG address
math, segment selection, nonzero StartBus and bus/window bounds. These tests run
under ASan/UBSan.

Actual QEMU root ports place xHCI/NVMe on nonzero buses. The preserved M7E diagnostic
ISO reproduces absent input/storage; the new image receives HID events and performs
read-only identification/reads. The normal image tests Setup interaction and a
separate explicitly disposable native install/two disk-only boots. QEMU uses QEMU
controllers, not emulated VMware internals; real M7F Fusion retest remains pending.
