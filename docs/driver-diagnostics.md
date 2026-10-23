# M7G device initialization and input diagnostics

Use `CHOS-ARM64-M7G-INPUT.iso` for the next Fusion trial. This original UEFI/kernel
image runs normal protection/timer tests, skips Setup/filesystem mounting, attempts
bounded driver initialization and rotates reports automatically. A fixed lower
panel shows live USB keyboard/mouse availability, selected controller ID, actual
active path, stage/reason, last numeric key code/value, pointer position/buttons
and event count. It fits the supported 640×480 minimum as well as 1024×768.

The panel consumes existing `input_next` events; there is no second input API.
Single last-event fields are volatile. Key codes/content are never appended to
the diagnostic buffer, serial stream, filesystem or configuration. The panel does
not record an input history or show decoded text. Standard diagnostic mode remains
available separately without the live panel. Both modes require no input/disk.

## States and xHCI stages

PCI inventory, live config/driver selection, controller initialization, peripheral
identification and operational I/O are distinct. A class match is never an input
success. Reports include PCI config/BDF/IDs, MMIO base, HCCPARAMS1/HCSPARAMS2, root
port count, each port's initial PORTSC, connected-port count, actual enumeration
stage/failure, HID protocol/interface and operational HID count. Failed enumeration
also captures PORTSC after failure. Data is bounded by the existing 32-port/two-HID
limit; initialized devices continue using the known polled event ring.

xHCI controller stage: 1 discovery/BAR/memory decode, 2 capabilities/offsets/ports,
3 extended capabilities/ownership, 4 halt, 5 reset/4 KiB page support, 6 DMA/rings/
bus master, 7 start, 8 controller running. M7G uses distinct stable reasons below; discovery failures use shared PCI
result codes 1–5 at stage 1. A
running controller with zero accepted HID devices is not a successful input path.

Enumeration stage: 10 connection/reset, 11 speed, 12 enable slot, 13 DMA allocation,
14 address device, 15 initial device descriptor, 16 packet/context evaluation,
17 device/configuration header, 18 configuration/HID parsing, 19 configuration/boot
protocol, 20 interrupt endpoint configuration, 21 HID ready. Last rejected port and
stage are reported; held states are released on disconnect. The panel's active
path says XHCI POLLED when the controller runs; separate keyboard/mouse READY
fields establish usable input. Enumerated counts successful device/configuration
header retrieval; HID counts accepted boot interfaces. Connected is the initial
root-port connection count, not a hotplug topology service.

UHCI/EHCI/OHCI remain inventoried but unsupported. The target screenshot proves
these controllers exist, not which owns the keyboard/mouse. No duplicate HID/input
API or speculative alternate controller driver was added. Existing limits remain:
one AC64 xHCI, two direct-root USB2 boot-HID devices, bounded contexts/rings; no USB
interrupt routing, hubs/general HID, firmware ownership takeover, scratchpads,
USB3 input, hotplug re-enumeration or controller reset recovery. Timeouts are bounded;
uncertain DMA pages remain reserved. Unexpected MMIO exceptions still use the
kernel exception screen; automatic controller recovery is not implemented. No target HID success is claimed yet.

## Stable xHCI refusal reasons (M7G)

| Code | Cause |
|---|---|
| 101 / 102 / 103 | Invalid BAR / MMIO mapping / PCI memory decode |
| 201 | Invalid capability length or aperture |
| 202 / 203 | Unsupported 32-bit-only DMA / scratchpad requirement |
| 204 / 205 | Zero slots / zero or more than 32 ports |
| 206 / 207 / 208 | Doorbell outside aperture / runtime outside aperture / overlapping blocks |
| 301 / 302 / 303 | Extended capability bounds / BIOS ownership unsupported / traversal limit |
| 401 | Halt timeout |
| 501 / 502 / 503 | Reset timeout / CNR timeout / no supported 4 KiB page |
| 601 / 602 | DMA allocation / PCI bus-master enable failure |
| 701 | Run timeout |

The failure log includes a readable cause. Stage 2 performs no allocation, reset
or ownership operation. The M7F target report's eight-port line narrows its old
stage-2 refusal to slots/offsets; raw register values are still needed to identify
the exact old predicate. [Audit and correction](milestone-7g.md).

## NVMe stages and read-only policy

NVMe reports live PCI selection/resources, CAP/version, ready timeout budget,
namespace sectors/sector bytes and controller/namespace operational result.
Failure stage: 1 discovery, 2 BAR/map, 3 PCI enable, 4 capabilities, 5 disable,
6 queue/buffer allocation, 7 enable/controller identify, 8 namespace identify/format,
9 I/O completion queue, 10 I/O submission queue, 11 operational. The last completion
DWORD is reported on failure. This does not dump sector contents or user files.

Portable CAP validation requires NVM command-set support, at least 16 queue entries,
4 KiB pages and supported doorbell stride. Ready waits derive from CAP.TO (minimum
500 ms), with an explicit 10-second driver budget. Command completion waits remain
2 seconds. Namespace 1 requires a valid supported LBA-format index, no extended
format/protection/metadata, 512/4096-byte sectors and non-overflowing NSZE/NCAP.
Unsupported capabilities fail explicitly; validation is not disabled for VMware.
One controller, namespace 1, polled 16-entry queues, bounded 4 KiB transfers remain.

**Both diagnostic modes force a driver-level write/flush rejection**, regardless of
namespace write-protect flags. They issue Identify/create-queue commands and read
the first/last 4 KiB, reporting success/failure without displaying sector contents.
They never mount/format, partition or enter Setup. Native serial requires validated
SPCR PL011, absent in the supplied Fusion pages; use framebuffer screenshots.

The normal image retains explicit destructive confirmation and existing installed
operation. Do not use it to install on Fusion until real input and independent
read-only storage gates pass. QEMU installer regressions use only disposable files.
Reports do not establish production data integrity, general namespace support,
IOMMU support, interrupt-driven storage or recovery.

See [actual target observations](fusion-observations-m7e.md),
[PCI correction](pci-discovery.md), [retest instructions](fusion-first-boot.md).
