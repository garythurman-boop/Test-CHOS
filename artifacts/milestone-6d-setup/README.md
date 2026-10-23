# CHOS M6D continuation — dedicated full-screen Setup

Download **CHOS-ARM64.iso** with GitHub's **Download raw file** button. Verify the
adjacent SHA256SUMS. This is an original standalone ARM64 UEFI OS development
image, not a browser/host UI. The previous M6D widget/configuration image remains
unchanged in `../milestone-6d/`.

**VMware Fusion is not verified.** No Apple Silicon/Fusion host is attached;
GICv3, EL2 entry and USB/xHCI are unsupported. QEMU success is not Fusion support.
This is not CHOS 0.1 or a complete authenticated desktop OS.

For the tested QEMU configuration use UEFI, one AArch64 CPU, 512 MiB or 1 GiB,
GOP/ramfb, Virtio PCI keyboard/mouse and a **disposable** NVMe disk. Disable
Secure Boot. The current layout requires 72,372,224 bytes at 512-byte sectors or
290,476,032 bytes at 4096-byte sectors. Automated native installs use 70/128 MiB
512-byte targets and 512 MiB 4096-byte targets. CHFS uses at most 4 MiB of the
system partition, regardless of disk capacity.

1. Boot the ISO. After kernel self-tests, full-screen CHOS Setup opens directly.
   Enter installs; L/F3 starts Live Desktop. Recovery status explains that repair
   tools are not implemented. There is no automatic erase or install timeout.
2. Accept the CHOS agreement, review the exact serial/capacity/sector size and
   existing partitions, then preview the replacement GPT/EFI/CHOS layout.
3. Choose a valid computer name, theme and fixed clock preferences. These are
   staged in RAM. Escape/Back corrects them. Account/password authentication,
   keyboard-layout selection and graphical display configuration are unfinished.
4. Review the full-screen warning and type `ERASE <exact serial>`, then Enter.
   Only this explicit confirmation authorizes real writes. Never attach a valuable
   disk. An active writer cannot be cancelled; interruption may leave a partial disk.
5. Observe real native progress and final verification. On success, detach the
   ISO and choose Restart/R. The installed disk boots directly into the persistent
   Royale desktop. User files and supported system preferences survive cold boots.
6. Control Panel Save/Ctrl-S persists supported settings on installed volumes.
   Live Desktop uses RAM files; successful installation does not convert that
   existing live session into an installed session. Reboot from the target disk.

`/system/install-log` records completed checks on successful installations. Errors
identify the failed operation and advise preserving the disk/ISO before retry.
There is no fsck/repair, rollback or account security. Networking/SMP and broader
hardware support remain unfinished.

See [actual screenshots and test results](../../docs/milestone-6d-setup.md),
[setup architecture/safety](../../docs/setup-environment.md), and
[Fusion test plan](../../docs/vmware-fusion.md). Source remains MIT; the bundled
Noto font cache/fonts use SIL OFL 1.1, included on the ISO and installed disk.
