# CHOS Milestone 6D — native widgets and saved configuration

Download CHOS-ARM64.iso with GitHub's **Download raw file** button. Verify the
adjacent SHA256SUMS. The version is recorded by this milestone directory, commit
and stable checkpoint tag. This is a genuine original ARM64 UEFI OS with a native
installer, tested under QEMU. **VMware Fusion compatibility is unverified**;
GICv3, EL2 and USB/xHCI input remain unsupported.

The approved Royale appearance is retained. Shared native controls now support
actual app menus, dialogs, file selection/scrolling and saved system settings.
Normal installed desktops no longer have a permanent session overlay.

Use one CPU, 512 MiB or 1 GiB, GOP/ramfb, Virtio PCI keyboard/mouse and a disposable
NVMe disk. Minimum test targets: 128 MiB at 512-byte sectors, 512 MiB at 4 KiB sectors.
Secure Boot must be off. Never give Setup a valuable disk.

1. Boot the ISO; after kernel tests the native desktop appears. Meta-7 opens Setup.
2. Accept the agreement and review the exact disk serial/capacity/partition layout.
3. Set a computer name and supported theme/fixed clock preferences. Back corrects
   invalid input. No account/password security is implemented.
4. Review the destructive warning and type `ERASE <serial>` only for the displayed
   disposable destination. Invalid input does not write the disk.
5. Wait for successful completion, remove the ISO and use Restart/R. The installed
   disk boots independently. User files and chosen settings persist on CHFS.
6. Control Panel's Appearance/Clock/System tabs edit supported preferences. Save
   settings / Ctrl-S persists on installed disks; live Apply is RAM-only.

Native tests cover both sector geometries, wrong confirmation and invalid settings,
boot-disk exclusion, exact file persistence, saved configuration, legacy loading and
safe failed-mount diagnostics. The workspace test uses real overlapping native apps,
menus/dialogs, scrollable files, minimized/maximized windows, Task Manager and Setup.
Actual lossless screenshots: ../../docs/evidence/m6d.

See ../../docs/milestone-6d.md for commands, results, source changes and limits.
Accounts/authentication, full time-zone/layout/display setup, repair/fsck/rollback,
networking and Fusion validation remain unfinished. This is not CHOS 0.1. CHOS
retains MIT; bundled Noto cache/fonts use SIL OFL 1.1, included on the ISO and disk.
