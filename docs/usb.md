# Original USB boot-HID architecture and limits

M7C adds original C xHCI bring-up and USB boot-keyboard/mouse decoding. No Linux,
BSD, firmware USB driver or third-party USB stack is linked or called by the
running kernel. **Only the documented QEMU controller/device configurations are
verified. Actual Fusion controller and HID descriptors remain unknown.**

## Boundaries

`drivers/usb/xhci.c` discovers PCI class 0c/03/30 through the existing ACPI MCFG
segment-0/bus-0 path. It uses firmware-assigned BARs, kernel MMIO mappings, PMM
pages and explicit DMA cache maintenance. No QEMU MMIO address or device vendor ID
is used for controller selection. Other USB classes (UHCI/OHCI/EHCI/unknown) are
identified in automatic inventory and left unsupported.

`drivers/usb/ring.c` implements portable fixed-page command/transfer producer
rings and event consumption. It validates indices, cycle bits, alignment and
bounded DMA spans. Command/transfer rings have 255 data TRBs plus a toggling Link;
event rings have 256 entries. Native wrappers publish cleaned DMA pages, use
barriers/doorbells, invalidate completed pages and advance ERDP/EHB. Only bounded
outstanding work is submitted: one command/control operation, eight interrupt-IN
receive transfers with distinct bounded buffers per active device. Completion slot/endpoint/TRB pointers and residual
lengths are checked before decoding. These bounds do not implement an IOMMU or
protect against a hostile DMA device/hypervisor.

`drivers/usb/hid.c` validates bounded configuration/interface/endpoint descriptors
and decodes explicitly selected HID **boot protocol** into existing CHOS
`input_event` codes. Six-key keyboard arrays and eight modifiers generate press/
release changes; malformed, duplicate and rollover reports preserve prior state.
Three-button relative mouse reports generate existing movement/button events;
an optional fourth wheel byte is decoded, but the desktop has no wheel-scrolling
feature. This is not a general HID report-descriptor parser, tablet or NKRO driver.
`drivers/input/virtio.c` remains working and calls the USB initializer/poller;
both paths feed the same input FIFO/window manager without a second desktop API.

## Controller lifecycle

Enable PCI memory decoding before reading capability registers. Validate capability,
port/slot/context size, register windows, 4 KiB page support and extension bounds.
Reject scratchpads and unhandled firmware ownership requirements explicitly. Halt
and reset with generic-counter timeouts. Allocate/zero/clean private DMA pages;
enable bus mastering before programming DCBAA/CRCR/event-table registers, which
may cause DMA reads immediately. Set the event-ring table and start the controller.
Actual QEMU testing caught the ordering defect when bus mastering was delayed
until after event-table programming; the corrected order is regression tested.

Reset connected USB2 root ports, enable slots, allocate input/output/endpoint
contexts and control buffers, address devices, retrieve bounded device/configuration
descriptors, select configuration and boot protocol, and configure interrupt-IN
endpoints. Use an eight-entry receive pipeline to avoid single-transfer starvation during
rendering; buffers occupy distinct 512-byte regions of one owned DMA page. Use
hardware interval scheduling and poll completion events from the
existing kernel input loop. A stopped/disconnected endpoint releases held HID
keys/buttons and stops resubmission. Timeout/unsupported-device paths log errors;
DMA pages with uncertain hardware ownership are retained until reboot, never
freed for another owner. No automatic controller-reset recovery or hotplug
re-enumeration is implemented.

## Supported subset and remaining work

One xHCI controller, up to two successful direct-root boot-HID devices, AC64,
32/64-byte contexts, no scratchpads, USB low/full/high speed, endpoint packets at
most 64 bytes, configuration at most 512 bytes. Initial USB2 port reset and control
transfers are supported. Hubs, composite/multiple boot interfaces, USB3 devices,
legacy controller drivers, firmware ownership takeover and broad HID layouts are
not implemented. Failed root-port enumeration is bounded and reported.

**USB completion is polled. MSI/MSI-X/INTx routing and USB interrupt delivery are
not implemented or tested.** Existing GIC/timer interrupt delivery remains tested
separately. Future host-controller drivers should keep their register/ring/DMA
mechanisms separate and share descriptor/HID/event code; general report parsing,
USB interrupt routing, ownership handoff and safe reset recovery require their own
acceptance evidence before expanding support claims.

Tests include host descriptor/length/type/DMA/cycle/wrap/error cases with ASan/UBSan;
real QEMU USB-only keyboard modifiers/releases and mouse/buttons; native editing/
save/reopen/dragging; more than a full hardware transfer/event-ring wrap; disconnect;
and noninteractive unsupported-controller/non-boot-device cases. Check the
[iteration report](milestone-7c.md) for final results, not assumptions.
