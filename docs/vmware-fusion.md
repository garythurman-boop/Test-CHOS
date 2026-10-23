# VMware Fusion on Apple Silicon

**Not verified: no Apple Silicon/Fusion host is attached.** M7A/7B implements
original EL2→EL1 startup and ACPI GICv3 support with real QEMU UEFI boot, installation
and disk-only persistence tests. This removes two kernel limitations; it does not
establish Fusion's hardware or compatibility.

- [Compatibility matrix and research provenance](fusion-compatibility-matrix.md)
- [Exact manual Fusion validation checklist](fusion-validation.md)
- [Current iteration and QEMU evidence](milestone-7a-b.md)
- [Versioned test image and checksum](../artifacts/milestone-7a-b/README.md)

USB host/HID drivers remain absent. Input is Virtio PCI; storage is one bus-0 NVMe
controller/namespace; display uses GOP RGB/BGR. Obtain actual target IDs/descriptors
and the release's custom ARM64 optical workflow before claiming support. New
read-only firmware PCI inventory and Terminal `hardware` help capture evidence.
Do not apply x86 VMware assumptions or QEMU addresses to the generic ARM64 kernel.
