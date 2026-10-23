# CHOS native application checkpoint

Download **CHOS-ARM64.iso** with GitHub's **Download raw file** button, then check
SHA256SUMS. Boot using AArch64 UEFI with Secure Boot disabled. Kernel tests run
first; CHOS Desktop then opens. See ../../docs/desktop.md for controls.

This image includes six functional native EL0 applications. The desktop volume
is explicitly RAM backed: **files are lost at restart/shutdown**. Attached disks
are not modified. This is not an installer or CHOS 0.1; Fusion compatibility is
unverified. Use the recorded QEMU hardware configuration for reproducible tests.
