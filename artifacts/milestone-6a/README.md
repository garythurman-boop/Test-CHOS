# GPT/ESP boot test fixture

`CHOS-GPT-BOOT-512.img.gz` is a compressed 128 MiB raw GPT disk created by CHOS's
original build-time C formatters. It contains an original FAT32 ESP carrying the
same EFI executable/kernel as the milestone-5 ISO. Its CHOS system partition is
unformatted. It is **not the output of a guest installer** and not a Fusion VMDK.

Use GitHub's Download raw file button. Verify SHA256SUMS, decompress using gzip,
and verify RAW-SHA256SUMS. The test scripts construct fresh copies automatically;
this fixture is for inspecting/reproducing the recorded QEMU disk-only boot.
Never write it to a physical disk. It has fixed test GUIDs and a volatile desktop.

Both 512-byte and 4096-byte logical-sector fixtures booted under QEMU UEFI without
an ISO and stayed unchanged. See ../../docs/milestone-6a.md for results, commands
and limitations. The current live ISO is ../milestone-5/CHOS-ARM64.iso.

No graphical installation, persistent installed system volume or VMware Fusion
compatibility is claimed. Native guest setup is the next integration milestone.
