# M8 development images

Primary build: **[Download CHOS-ARM64-M8.iso](https://raw.githubusercontent.com/garythurman-boop/Test-CHOS/main/artifacts/milestone-8/CHOS-ARM64-M8.iso)**.
[SHA-256 checksums](SHA256SUMS) · [Milestone report and screenshots](../../docs/milestone-8.md).

This normal image boots blue Setup or Live Desktop and retains automatic
read-only input-failure diagnostics. It includes guarded ERASE-only installation,
a real disk-only reboot into Royale configuration, a larger original cursor,
desktop gestures/saved layout, native context menus and reboot resolution trials.
It is a development build, not CHOS 0.1 or a claim of complete Fusion acceptance.

The user confirmed M7I normal ISO, keyboard, mouse/trackpad, Setup and Live Desktop
on Fusion/M1. M8 installation, persistence, new gestures and display trials still
need actual Fusion testing. The reproduced serial-padding defect is corrected,
but the user's raw NVMe Identify response was unavailable; another target failure
would now produce a precise error. [Fusion instructions](../../docs/fusion-first-boot.md).

Use a disposable supported NVMe virtual disk. Review identity, capacity, geometry,
partitions and proposed layout. Type exactly ERASE. Wait for verified first-stage
completion, detach the ISO and restart from the disk. Complete Royale settings,
verify Welcome, then test a later cold boot and a saved Notepad document.
No authenticated accounts or permissions are claimed. Older marker-less volumes
are not migrated automatically; preserve their existing boot pair and data.

Optional `CHOS-ARM64-M8-INPUT.iso` always opens the read-only input panel.
`CHOS-ARM64-M8-DIAGNOSTIC.iso` rotates hardware inventory pages without needing
input. These do not install CHOS. The primary normal ISO is the installation build.

[Known-good M7I backup](../milestone-7i/CHOS-ARM64.iso) remains unchanged.
Build the standard `build/CHOS-ARM64.iso` with `./scripts/container.sh make iso`,
or run `bash scripts/checkpoint.sh` for validation, sanitizers, clean-source
reproduction and versioned images in `build/release/`. Internal optional-image
filenames retain historical names for existing test-script compatibility.
