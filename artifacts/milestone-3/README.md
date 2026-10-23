# CHOS milestone 3 testing images

Use **CHOS-ARM64.iso** for ordinary testing. Download with GitHub's Download raw
file button. Download SHA256SUMS too, then run `shasum -a 256 -c SHA256SUMS` on
macOS or `sha256sum -c SHA256SUMS` on Linux.

This is an original kernel/process/hardware test environment, not a desktop or
installer. UEFI ARM64, Secure Boot off, one CPU, ACPI 2.x, EL1 and GICv2 required.
512 MiB/1 GiB QEMU configurations pass. Virtio PCI keyboard/mouse provide a cursor
and uppercase character feedback when present. NVMe probing uses ACPI/PCI BARs;
**the ordinary ISO performs no NVM writes or automatic formatting**.

Fusion is unverified. GICv3, EL2 startup and USB/xHCI input are unsupported. Expect
a useful unsupported-platform message if those requirements differ; do not infer
that QEMU compatibility establishes Fusion support.

CHOS-STORAGE-TEST.iso is a specialized, destructive **automated test fixture**.
Do not use it as an installer or attach valuable/existing disks. Use only the
repository test runner, which creates fresh disposable image files. Both the exact
boot opt-in and exact NVMe serial CHOS-TEST-0001 gate write tests. Recognized test
volumes are intentionally updated; blank test volumes are formatted as raw CHFS.
Its separate checksum file is STORAGE-TEST-SHA256SUMS.

This fixture passed three fresh VM boots, native user file operations, independent
data/CRC checks, wrong-identity rejection and a normal-ISO no-write test. GPT/ESP,
installed-disk boot, desktop and applications are not implemented.

See ../../docs/milestone-3.md for exact evidence, ../../docs/status.md for limits,
and ../../docs/vmware-fusion.md for the dedicated Fusion procedure.
