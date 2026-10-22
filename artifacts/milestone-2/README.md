# CHOS Milestone 2 core testing image

ARM64 UEFI, single CPU, EL1, ACPI 2.x and GICv2. Secure Boot off. Validated at
512 MiB and 1 GiB in QEMU; Fusion remains unverified. This image runs real kernel
and EL0 tests, then idles. No desktop, installer or disk writes. See
../../docs/milestone-2.md. Download raw file in GitHub and verify SHA256SUMS.
