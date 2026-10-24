# M7I unified ARM64 input candidate

**Start with [CHOS-ARM64.iso](CHOS-ARM64.iso)** — one file for normal blue/Royale
Setup and automatic read-only diagnostics if input initialization is incomplete.
Actual Fusion keyboard/trackpad readiness is still pending the user's Mac test.
This is an intermediate checkpoint, not CHOS 0.1 or universal Fusion certification.

- [Normal ISO — direct download](https://raw.githubusercontent.com/garythurman-boop/Test-CHOS/main/artifacts/milestone-7i/CHOS-ARM64.iso)
- [Optional M7I INPUT ISO](CHOS-ARM64-M7I-INPUT.iso): always-on live input panel,
  static descriptor/topology reports, no filesystem/installer; media writes guarded.
- [Full-page diagnostic ISO](CHOS-ARM64-M7I-DIAGNOSTIC.iso): automatic reports.
- [SHA256SUMS](SHA256SUMS): compare `shasum -a 256 filename.iso`, or download all
  three and run `shasum -a 256 -c SHA256SUMS`.

On GitHub choose **Download raw file**. Cold boot the normal ISO with no disk first:
ARM64 UEFI, one CPU, 2 GiB, Secure Boot off. Check keyboard navigation, live native
text entry, pointer alignment/dragging and both buttons. If fallback appears,
capture its hub/report/compiler pages and live panel; no other image is required.
[Exact Fusion test](../../docs/fusion-first-boot.md).

Only after actual input and repeated INPUT read-only NVMe probes pass should a
confirmed installation on a designated disposable virtual disk be attempted.
Normal Setup is a real installer with typed destructive confirmation. Preserve
M7H. [Architecture, validation and limitations](../../docs/milestone-7i.md).

Rebuild on a Linux development host with the documented Docker image and host
C compiler/ASan: commit source changes, then `bash scripts/checkpoint.sh`.
It runs full validation, sanitizers, clean-source reproduction and packages
verified images into `build/release/`; it does not publish or touch host disks.
