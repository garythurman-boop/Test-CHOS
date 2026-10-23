# CHOS Royale foundation / native installation ISO

Download `CHOS-ARM64.iso` using GitHub's **Download raw file** button. Verify the
adjacent SHA256SUMS. This is an original ARM64 UEFI operating system and real native
installer, tested with QEMU UEFI. **VMware Fusion compatibility is unverified.**
GICv3 and USB/xHCI remain unsupported; do not expect Fusion installation to work yet.

Use one CPU, 512 MiB or 1 GiB RAM, GOP/ramfb, Virtio PCI keyboard/mouse and a
**disposable NVMe virtual disk**. Never attach a valuable destination for Setup.
Current minimums are 128 MiB for 512-byte sectors, 512 MiB for 4 KiB sectors.

1. Boot the ISO with Secure Boot off. Kernel tests run, then the desktop opens.
2. Press Meta-7 or open CHOS Setup. Accept the agreement and review the exact
   selected NVMe serial/capacity. Only one controller/namespace is supported.
3. Set the computer name. Review the destructive warning. Type `ERASE <serial>`
   with the exact displayed identity only if that destination may be overwritten.
4. Wait for successful completion. Remove the ISO, then press R to restart.
5. Installed CHOS mounts its CHFS system partition. About shows the computer name
   and installed-volume status; Notepad files now persist across cold boots.

Live ISO sessions use RAM unless Setup explicitly installs to a confirmed disk.
Setup cannot erase a running NVMe boot disk. Installation-core tests cover both
sector sizes, wrong-confirmation no-write behavior, disk-only boots, actual native
file save/reopen after cold boots, and independent on-disk CRC/content/license checks.

This is not CHOS 0.1 or the complete requested configuration stage. No account/
password security, timezone/keyboard/display setup, fsck/rollback, networking or
Fusion validation is delivered. See ../../docs/milestone-6c.md for exact evidence,
known failure/recovery limits and remaining work. Keep the existing MIT license.

This M6C checkpoint adds anti-aliased licensed fonts, original wallpaper/icons,
rounded blue chrome/taskbar, a native two-column Start menu, supported UEFI mode
selection (prefer 1024×768) and explicit failed-volume warnings. The real native
installer and both sector geometries remain covered by cold-boot tests. Font
license: CHOS/FONT-OFL.TXT on the ISO, `/system/font-ofl` after installation.

Actual guest screenshots and measured test results: ../../docs/evidence/m6c.
Remaining shared widgets, installer configuration, advanced recovery and Fusion
hardware work are listed in ../../docs/royale.md and ../../docs/status.md.
