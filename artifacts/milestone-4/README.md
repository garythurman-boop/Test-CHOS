# CHOS Desktop core checkpoint

Download `CHOS-ARM64.iso` using GitHub's **Download raw file** button. Check
`SHA256SUMS` before use. This is a genuine ARM64 UEFI boot image with an original
kernel and native EL0 window client. The desktop starts after kernel diagnostics.

This checkpoint exercises the desktop/window API, not the full application set or
installer. The Start menu explicitly labels unfinished session/power actions.
QEMU UEFI testing passed; VMware Fusion compatibility remains unverified.
