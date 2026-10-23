# VMware Fusion on Apple Silicon

**M7E successfully reaches kernel diagnostics and Setup on the user's M1/Fusion VM.**
MMU/protection, EL1, GICv3/timer and 1024×768 GOP are observed. Keyboard/mouse and
NVMe initialization fail. The supplied screenshot places both supported controllers
on nonzero buses, beyond the old scan. M7F fixes that discovery blocker and adds
input/read-only diagnostics; actual M7F peripheral retest is pending.

- [Compatibility matrix and research provenance](fusion-compatibility-matrix.md)
- [Exact manual Fusion validation checklist](fusion-validation.md)
- [Current iteration and QEMU evidence](milestone-7f.md)
- [Versioned test images and checksums](../artifacts/milestone-7f/README.md)

The original EL1/EL2, GICv2/v3, GOP and polled drivers are preserved. xHCI/NVMe
now use checked firmware BDF/MCFG/live config; the old bus-zero scan is a fallback.
One NVMe controller/namespace 1 and bounded USB2 direct-root boot-HID remain the
supported subsets. Other USB controllers, hubs/general HID, ownership takeover,
scratchpads, USB interrupts and recovery are not implemented.

Retest with the **M7F INPUT image** at the same one CPU/2 GiB, no disk first.
Capture every report page and live key/pointer fields. Then follow the ordered
Setup-interaction/read-only-storage gates before any installation. The diagnostic
modes enforce namespace write/flush rejection. Actual target input, controller
capabilities, USB attachment and NVMe namespaces still need evidence.

[Actual target evidence](fusion-observations-m7e.md) · [PCI routing](pci-discovery.md)
· [live input/driver reports](driver-diagnostics.md) · [ordered retest](fusion-first-boot.md).
