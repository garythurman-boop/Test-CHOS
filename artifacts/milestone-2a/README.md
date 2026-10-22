# CHOS Milestone 2A testing image

This is a development kernel image, not an installable desktop OS.

- Architecture: AArch64 / ARM64; native UEFI boot, Secure Boot disabled.
- Source checkpoint: b1438ec (kernel and test evidence).
- Verified in QEMU virt with GICv2 at 512 MiB and 1 GiB RAM.
- Requires single-CPU EL1 entry, ACPI 2.x, GICv2 and GTDT timer information.
- Displays kernel memory/protection/preemption test results, then idles.
- No user processes, applications, persistent storage, desktop or installer.
- VMware Fusion has not been verified. Unsupported GICv3 is explicitly rejected.
- No guest disk writes are implemented.

Download CHOS-ARM64.iso with GitHub's Download raw file button. Download
SHA256SUMS from this folder and verify the ISO before testing:

```sh
# macOS
shasum -a 256 -c SHA256SUMS
# Linux
sha256sum -c SHA256SUMS
```

See ../../docs/milestone-2a.md for actual results and ../../docs/vmware-fusion.md
for the compatibility test procedure. Later milestones use separate versioned
folders so this test image remains identifiable.
