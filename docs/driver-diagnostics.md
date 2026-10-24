# M7I device initialization and input diagnostics

The normal **CHOS-ARM64.iso** now enters automatic read-only input diagnostics
if initialization is incomplete; it does not launch Setup or mount a system volume
in that case. `CHOS-ARM64-M7I-INPUT.iso` always displays the live panel. The same
NVMe guard rejects writes/flushes in either diagnostic path. Static USB descriptor
metadata can be retained in a bounded **64 KiB** volatile buffer, never a disk log.

Use `CHOS-ARM64-M7I-INPUT.iso` for the next Fusion trial. This original UEFI/kernel
image runs normal protection/timer tests, skips Setup/filesystem mounting, attempts
bounded driver initialization and rotates reports automatically. A fixed lower
panel shows live USB keyboard/mouse availability, selected controller ID, actual
active path, stage/reason, last numeric key code/value, pointer position/buttons
and event count. M7G also separates controller initialized, total/initially connected
ports, descriptor-enumerated devices and ready HID interfaces. It fits the supported 640×480 minimum as well as 1024×768.

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
also captures PORTSC after failure. Data is bounded by the 32-root-port/16-HID-channel
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
one AC64 xHCI, bounded USB2 hubs and boot/report-HID interfaces, bounded contexts/rings; no USB
interrupt routing, general touch/vendor HID, firmware ownership takeover, scratchpads,
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

## Retained USB enumeration reasons

M7H boot-selection reasons 1103/1104 remain in the compatibility helper; M7I
attempts the report path for those interfaces and uses 1201–1205 for compiler
refusals.

The fixed panel's bottom `ENUM` fields are last stage / reason / root port. They
are independent of controller stage/reason (8/0 can coexist with HID refusal).
Existing enumeration stages 10–21 are preserved. Reasons 810–817, 819 and 820
identify port reset, speed, slot, allocation, address, initial descriptor,
context update, full descriptor, configuration/protocol and endpoint command
failures respectively. Last xHCI completion is printed without report payloads. Common values are 1
(success), 4 (USB transaction error), 5 (TRB error), 6 (stall) and 13 (short packet).
A parser refusal can follow completion 1: the transfer succeeded, but its descriptor
layout was malformed or unsupported. Controller 8/0 and enumeration 18/1103, for
example, mean a running controller with a non-boot HID interface refusal.

| Code | Refusal |
|---|---|
| 901 | Control request length/direction/DMA span outside policy |
| 902 | Data completion code/residual invalid |
| 903 | Status completion failure or missing data completion |
| 904 | Control error completion on another TRB in this endpoint |
| 905 / 906 / 907 | Control timeout / short descriptor / malformed device descriptor |
| 1001 / 1002 / 1003 | Malformed configuration header / total length / descriptor span |
| 1004 / 1005 / 1006 | Interface length / endpoint length / endpoint outside interface |
| 1007 / 1008 / 1009 | Invalid endpoint address / duplicate address / endpoint count |
| 1010 / 1011 | Duplicate interface+alternate / interface count mismatch |
| 1012 / 1013 | Invalid HID subordinate table / missing boot HID descriptor |
| 1101 | Unsupported 512-byte / 64-descriptor / 32-interface-alternate budget |
| 1102 / 1103 / 1104 | No HID / non-boot subclass / unsupported boot protocol |
| 1105 / 1106 | No interrupt IN / multiple interrupt IN endpoints |
| 1107 / 1108 | Unsupported packet size/transactions / polling interval |

Configuration trace is bounded to 160 scalar fields per boot, never arbitrary
memory or device strings. An explicit budget notice marks omitted detail. VID/PID
and class triplets identify devices; a controller PCI ID does not identify USB
peripherals. Readable malformed/unsupported causes accompany parser reasons.
See [actual target facts and selection policy](milestone-7h.md).

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

## M7I report/HID/hub additions

- Stage 22: hub descriptor/configuration; 23: hub discovery complete; 24: interface
  report descriptor/compiler. A later successful hub operation can clear the last
  ENUM reason; earlier interface errors and keyboard/mouse readiness still matter.
- 1201: malformed/truncated HID items; 1202: report/field/stream/wire-size budget;
  1203: unsupported item or ambiguous state; 1204: no recognized keyboard/pointer
  inputs; 1205: invalid logical range/field.
- 1301: hub descriptor transfer; 1302: invalid/unsupported descriptor prefix/port
  count; 1303: hub context command; 1304: hub port status; 1310: node/channel budget.
  Control errors 901–907 and actual xHCI completion codes remain available.

Report descriptor LE32 lines contain successive groups of at most four static
bytes, little endian, scoped by selected physical device/interface and received
length. Last-group padding is zero and is not part of the descriptor. Live Input
report payloads and typed content are never logged. Descriptor words permit an
unsupported target layout to be reproduced without guessing its format.

Limits are now 16 physical nodes/16 HID channels/four HID functions per device;
first configuration, one IN endpoint/function, USB2-only hub tree, 15 ports/hub,
five route nibbles, 1 KiB report descriptors, 96 admitted fields, 16 report IDs,
64-byte Input reports and 24 held keys. The legacy serial marker “operational boot
HID devices” counts ready HID channels including report-protocol channels. The
panel's HID count describes keyboard/pointer functions, not proof of event delivery.

[Unified workflow, actual test scope and remaining target gate](milestone-7i.md).
