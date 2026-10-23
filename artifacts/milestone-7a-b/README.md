# CHOS M7A/7B — ARM64 boot/interrupt checkpoint

[Download CHOS-ARM64.iso](CHOS-ARM64.iso) (17,154,048 bytes).
On GitHub choose **Download raw file**; an HTML preview is not the ISO.
Verify [SHA256SUMS](SHA256SUMS):

```sh
shasum -a 256 CHOS-ARM64.iso
# 3dd72a4a960cd7aead275e8a399830a50acea0b3107dd078310cadc82c499549
```

Source runtime checkpoint: `e31283e`; published checkpoint tag:
`checkpoint/milestone-7a-b`. Previous M6D images are preserved.

Real original ARM64 UEFI loader/kernel, original Royale desktop/apps and confirmed
native GPT/ESP/CHFS Setup. EL1 and supported non-VHE EL2 startup, ACPI GICv2/v3,
Virtio PCI input, one bus-0 NVMe namespace and GOP RGB/BGR are supported paths.
45 QEMU cases/14 host programs, sanitizers and clean-source ISO reproduction passed.
[Full report/evidence](../../docs/milestone-7a-b.md).

**VMware Fusion is unverified.** No Apple Silicon host was available. USB host/HID
is absent and actual Fusion storage/display/firmware topology is unknown. This is
a testable development ISO, not a completed Fusion-compatible release. Follow the
[separate Fusion checklist](../../docs/fusion-validation.md) and [matrix](../../docs/fusion-compatibility-matrix.md).

Use one CPU, 1 GiB RAM (core tested at 512 MiB), ARM64 UEFI and Secure Boot off.
Boot ISO: dedicated blue Setup. L/F3: Live Desktop. Live files are volatile.
Only a disposable NVMe target should be attached for install testing. Review exact
identity/capacity/layout and type `ERASE`; installation destroys that selected disk.
Native Setup writes GPT/EFI/System only after confirmation, copies/verifies real
payload, and offers Restart. Detach ISO and cold boot installed disk, save/reopen
files and repeat a cold boot. Installed settings/files persist on bounded CHFS;
no account/password security, full regional setup, fsck or transactional rollback.
CHFS currently exposes at most 4 MiB of the system partition.

Terminal `hardware` reports actual firmware/kernel EL, GIC/timer, initialized
input/storage/display and PCI IDs. A static visible screen is not input support.
Record failures/screenshots as well as successes; never substitute QEMU results
for Fusion install/reboot acceptance. No networking, SMP or CHOS 0.1 release claim.
