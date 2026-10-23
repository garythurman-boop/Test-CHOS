# CHOS M7E — MMU correction and Fusion retest

Start with **[CHOS-ARM64-M7E-DIAGNOSTIC.iso](CHOS-ARM64-M7E-DIAGNOSTIC.iso)**.
On GitHub open the ISO and choose **Download raw file**. No keyboard/mouse,
virtual disk, network or desktop is required. It retains precise MMU failure codes
or continues automatically to rotating hardware inventory.

[CHOS-ARM64.iso](CHOS-ARM64.iso) is the separate normal Setup/Live Desktop image;
L/F3 selects Live Desktop. Use the diagnostic image for the next Fusion trial.

Both images are 17,154,048 bytes. Verify [SHA256SUMS](SHA256SUMS):

```sh
shasum -a 256 CHOS-ARM64-M7E-DIAGNOSTIC.iso CHOS-ARM64.iso
```

- Diagnostic: `4443a6dc70f0a8a45034083383e82cbb4795c286d550b102d39d71b4768cf322`
- Normal: `3d792bd8352d3977cea00771175c1279ad7789396816af1c54822a87edfa9bfe`

Runtime source: `a6ce6eb`; publication tag: `checkpoint/milestone-7e`.
[Full results/screenshots](../../docs/milestone-7e.md) ·
[MMU audit and error codes](../../docs/mmu-architecture.md) ·
[Compatibility matrix](../../docs/fusion-compatibility-matrix.md).

**Real M7D Fusion failure reported; M7E target retest pending.** The user reports
an M1 MacBook Air, one CPU, 2 GiB RAM, macOS 27 “Golden Gate,” Fusion “26H1u1,”
and normal M7D ISO stopping at `MMU INIT FAILED`. Full build and memory map are
unknown. QEMU reproduces table exhaustion at 2 GiB; the new mapper corrects that
confirmed defect, but this does not establish the Fusion root cause or target fix.

For retesting, keep **one CPU and 2 GiB RAM**, Secure Boot off, attach the diagnostic
ISO and no disk/network. [Detailed sequence](../../docs/fusion-first-boot.md).
Capture the complete retained failure screen (stage/reason/address/descriptor/table/
PMM/register fields), or every rotating inventory page if boot progresses. Include
ISO checksum, exact Fusion About version/build, macOS and automatic/manual boot path.
Do not reduce RAM to hide a capacity failure or attempt destructive installation
before boot/display/input/storage prerequisites pass.

The diagnostic build uses legacy internal `build/CHOS-ARM64-M7C.iso`; the same
bytes are renamed M7E for publication. Controlled limited-capacity/instrumented
baseline ISOs are not distributed. Previous working checkpoints remain preserved.

Normal Setup requires exact disk identity/capacity review and typed `ERASE` before
partitioning a disposable supported NVMe target. Live files are volatile. Limits:
1 GiB PMM tracking/128 ranges, 1,024 translation tables, 48-bit identity regime,
4 MiB CHFS, bounded polled USB boot-HID without interrupts/hubs/general HID/
ownership takeover/recovery, unfinished authentication/ACLs/regional setup/fsck,
SMP and networking. This is an intermediate checkpoint, not CHOS 0.1 or completed
Fusion validation.
