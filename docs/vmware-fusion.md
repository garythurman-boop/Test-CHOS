# VMware Fusion on Apple Silicon

**M7D reached the kernel on the user's M1 MacBook Air and failed at MMU init.**
Reported configuration: one CPU, 2 GiB, macOS 27 “Golden Gate,” Fusion “26H1u1,”
normal M7D ISO. Full build and memory/device maps are unavailable. The current
M7E correction passes controlled QEMU tests; the actual Fusion retest is pending.
No Apple Silicon/Fusion host is attached to this workspace.

- [Compatibility matrix and research provenance](fusion-compatibility-matrix.md)
- [Exact manual Fusion validation checklist](fusion-validation.md)
- [Current iteration and QEMU evidence](milestone-7e.md)
- [Versioned test images and checksums](../artifacts/milestone-7e/README.md)

M7C adds input-free diagnostics and a bounded original polled xHCI USB2 boot-HID
path tested only in QEMU. USB interrupts, other controllers, hubs and general HID
remain unsupported. [First evaluation](fusion-first-boot.md) requires no
installation. Other tested input is Virtio PCI; storage is one bus-0 NVMe
controller/namespace; display uses GOP RGB/BGR. Obtain actual target IDs/descriptors
and the release's custom ARM64 optical workflow before claiming support. New
read-only firmware PCI inventory and Terminal `hardware` help capture evidence.
Do not apply x86 VMware assumptions or QEMU addresses to the generic ARM64 kernel.

M7D hardens kernel sizing and diagnostic failure handling without assuming new
Fusion hardware. Actual first-boot screenshots/serial and host/Fusion versions
have been requested; target validation remains pending. Initial testing requires
no disk. Do not proceed to installation on Fusion until boot/input/storage pass.

Retest at the same 2 GiB with the M7E diagnostic image and [capture the complete
MMU failure report or all inventory pages](fusion-first-boot.md).
