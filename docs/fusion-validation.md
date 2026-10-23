# VMware Fusion ARM64 manual validation

**Pending: no Apple Silicon/Fusion host available.** This is a checklist, not a
successful installation record. Keep evidence per gate; stop at failed prerequisites.

1. Record Apple SoC, macOS, exact Fusion version/build, firmware and VM CPU/memory,
   disk/controller, display/input settings. Check that release's supported custom/
   other ARM64 UEFI workflow. Do not select x86 or assume another version's UI.
2. First run the [non-destructive diagnostic evaluation](fusion-first-boot.md), which
   requires no input or disk. For later installation download
   [M7C normal ISO](../artifacts/milestone-7c/CHOS-ARM64.iso) using GitHub
   **Download raw file**; compare adjacent SHA256SUMS. Preserve M6D. Create a new
   disposable ARM64 VM, one CPU, 1 GiB RAM, Secure Boot off. Attach connected optical
   media and a disposable disk at least 1 GiB, selecting NVMe if actually available.
   Attach no valuable disks or host shared volumes.
3. Cold boot. Record automatic optical detection, EFI catalog/path execution and
   loader diagnostics with actual screenshots/logs. Catalog inspection is not boot.
4. If optical boot fails, inspect catalog/options; manually select
   `EFI/BOOT/BOOTAA64.EFI` if the firmware file browser supports it. Record separately
   from automatic ISO boot. If optical EFI boot is unsupported, investigate a
   separate standards-compliant GPT/FAT removable disk. QEMU raw fixtures are not
   validated Fusion VMDKs; do not merely rename/import them and claim compatibility.
5. Record firmware entry EL and successful EL1 transition, or exact error. Current
   EL2 startup excludes VHE, LPA2/52-bit firmware mappings, MMU-off and big-endian
   entry. It does not support every possible ARM firmware regime.
6. Capture actual ACPI/MADT/GTDT resources, boot MPIDR, GIC revision, distributor/
   redistributor and timer flags/PPI. Verify timer scheduling and exception/protection
   self-tests. Capture firmware diagnostics if CHOS cannot reach its Terminal.
7. Confirm blue Setup and keyboard arrows/Enter/Back/F3/Live. If the actual USB controller/protocol is outside the tested polled xHCI
   boot-HID subset, mark **blocked**, capture controller IDs/class/programming interface
   and USB/HID descriptors. A static visible Setup screen is not usable installation;
   injected QMP input is not a Fusion input driver.
8. Choose L/Live, run Terminal `hardware`, capture firmware/EL/GIC/timer/input/NVMe/
   display/PCI output and full loader inventory where available. Test pointer/buttons,
   Start/app launch, keyboard editing, focus, Alt-Tab, drag/resize and minimize/maximize.
9. Check GOP format/stride, supported mode choice, clipping, pointer alignment and
   sustained redraw stability. A usable GOP is distinct from VMware GPU acceleration.
10. Open Setup (Meta-7). Review exact disk identity/capacity/sector size, existing
    partitions and proposed layout. Back/cancel without writes. If no supported
    disk exists, record actual controller IDs and stop rather than substituting QEMU.
11. Install only to the disposable target. Accept agreement, review layout, type
    `ERASE`, enter supported configuration. Record real partition/format/copy/verify
    progress and errors. Completion must follow actual successful verification.
12. Completion Restart must reset. Power off if necessary, detach ISO and cold boot
    disk only. Record firmware path, persistent mount, saved settings and Royale.
13. Save known bytes under a unique filename in Notepad; verify with Explorer and
    Terminal. Save supported settings, shut down, cold boot again without ISO and
    compare file/settings. Independently inspect GPT CRCs, ESP payload and CHFS if
    the disposable image can safely be exported.
14. Repeat input/window/file operations, restart/shutdown. On disposable copies,
    test interruption/retry and invalid-volume read-only fallback. Preserve evidence;
    CHOS has no full rollback, fsck or production crash-safety guarantee.
15. Repeat core boot at 512 MiB if feasible. Measure allocated managed RAM separately
    from reservations. Measure actual Fusion idle CPU/interaction on the recorded
    host; QEMU host-thread observations do not satisfy native performance acceptance.
16. Report pass/fail/blocked separately for ISO, loader, EL, GIC/timer, display,
    keyboard, mouse, storage, installation, first disk-only boot, second cold boot/
    persistence and performance. Include hashes, settings, real screenshots/logs
    and a reproducible failing step.

Accounts/password/ACLs, networking, SMP and full regional/runtime mode setup remain
unfinished regardless of these results. Logoff ends apps, not authenticated users.
CHFS exposes at most 4 MiB. See [matrix](fusion-compatibility-matrix.md),
[Setup safety](setup-environment.md) and [known issues](status.md).
