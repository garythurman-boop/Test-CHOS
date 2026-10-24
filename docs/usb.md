# Original USB2 / HID architecture — M7I

The running guest uses original CHOS C for xHCI, USB2 hubs, boot HID and a bounded
HID report compiler/decoder. No firmware USB service or external OS USB stack is
used after ExitBootServices. [M7I scope and evidence](milestone-7i.md).

`drivers/usb/xhci.c` retains firmware-BDF/MCFG/live-ID discovery and polled command/
event rings. Physical nodes own EP0/address/configuration/output contexts; up to
four HID functions share that slot while owning independent IN rings, report
buffers and held state. Input endpoint contexts are configured together with the
correct slot ContextEntries/flags. Boot functions use SET_PROTOCOL(0); report-only
functions fetch their interface-indexed HID descriptor and compile its layout.
Nonzero alternates use SET_INTERFACE. No VMware VID/PID packet layout is hardcoded.

`drivers/usb/descriptors.c` validates configuration spans/counts/addresses/HID
tables and collects compatible functions/alternates. `drivers/usb/hub.c` checks USB2
hub descriptor prefixes and xHCI route arithmetic. The hardware wrapper configures
hub slot/TT information, powers/resets ports and discovers bounded child trees.
It polls one hub port per 100 ms across active hubs, releases disconnected child/
subtree state and stops an unreachable subtree. Uncertain DMA/slots remain owned
until reboot. Automatic reconnect/re-enumeration/controller reset is unfinished.

`drivers/usb/report.c` compiles bounded short items into Input fields, accounting
for padding, report IDs, collections, global push/pop and usage lists/ranges.
Keyboard arrays/modifiers/variable keys and Generic Desktop relative/absolute
pointer axes/buttons/wheel are supported. Unknown required layouts fail explicitly.
Input state is independent by report ID; Output/Feature items never shift Input
bit offsets. Valid reports translate through the original CHOS event queue.
Absolute values scale by logical min/max into the framebuffer; native apps receive
the existing relative-event ABI. Duplicate relative axes from the same composite
device are suppressed while actual absolute reports are arriving. USB key/button
reference counts prevent another interface's release from clearing a held input.

Limits: one AC64 xHCI, 32 controller root ports, 16 physical nodes, 16 HID channels,
four HID functions/device, five route nibbles, USB2 hubs with at most 15 ports,
first configuration, one IN endpoint per function, 512-byte configurations,
1 KiB report descriptors, 96 admitted fields/16 report IDs/64-byte Input reports,
24 simultaneously held keys. No USB3, legacy controller driver, general touch/
consumer/vendor layouts, report-feature negotiation, USB interrupts, ownership
handoff or scratchpads. High-speed hub TT setup follows the architecture but
actual Fusion high-speed child operation is still unverified. QEMU's hub model
and portable simultaneous-interface fixtures do not establish that target pass.

The 64 KiB volatile diagnostic buffer can include bounded static HID descriptor
words for reproduction; it never logs typed content or live report payloads.
Normal distribution boot falls back to this read-only mode when input startup is
incomplete, before Setup/filesystem initialization; NVMe write/flush is guarded.
No automatic mid-install mode switch is attempted. [Codes](driver-diagnostics.md),
[target facts and test instructions](fusion-first-boot.md).
