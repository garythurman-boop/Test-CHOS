# Versioned kernel image and bootstrap memory — M7D

The old 512 KiB ceiling was a software contract, not an ARM64 limit. M7C's loader
allocated/zeroed a fixed 1 MiB EfiLoaderCode region, loaded at most 512 KiB, and
passed its end as the initial stack top. The linker required text/data/BSS below
512 KiB; MMU initialization and its self-test removed the page at base+512 KiB.
The installer separately rejected kernel files larger than 512 KiB. Raising just
the linker assertion would allow image/BSS/stack/guard overlap.

## Image v1

`KERNEL.BIN` remains an original little-endian flat AArch64 image, with no GRUB,
Linux, ELF kernel loader or external boot manager. The first 64 bytes now declare
its allocation contract. Linker-generated values are offsets/sizes, not runtime
absolute pointers. The ordinary entry code begins at offset 64; the branch at
zero also leads there. No runtime relocation table is needed.

| Offset | Size | Meaning |
|---|---|---|
| 0 | 4 | AArch64 branch to offset 64 (`0x14000010`) |
| 4 | 4 | Reserved zero |
| 8 | 8 | ASCII `CHOSIMG1` |
| 16 | 4 | Version 1 |
| 20 | 4 | Header size 64 |
| 24 | 8 | Exact file bytes, linker `__file_end` |
| 32 | 8 | Required page-aligned memory bytes including BSS, linker `__image_end` |
| 40 | 8 | Entry offset 64, with a complete instruction inside file bytes |
| 48 | 16 | Reserved zero |

Original portable `boot/image.c` validates all fields before firmware allocation.
It rejects truncated headers, unsupported versions, reserved bits, mismatched file
length, impossible/unaligned memory sizes and invalid entries. It bounds arithmetic
before use. Current policy caps file bytes at 16 MiB and memory at 64 MiB; these
are centralized, adjustable resource limits in `include/kernel_image.h`, not
stack locations. Header checking provides structural validation, not signatures,
authentication or verification of arbitrary machine code.

The linker script is preprocessed with that same header. It still forbids GOT
entries and checks file/memory policy. Text and rodata keep page alignment. BSS
is NOLOAD and does not inflate the optical/disk payload. The ISO builder grows its
FAT12/16 EFI volume from 16 to 32 MiB when payload headroom requires it; larger
volumes require an explicit future policy/normalizer extension. Current distributed
images and the controlled 1 MiB growth image fit the 16 MiB EFI volume.

## Independent image and guarded stack allocations

UEFI allocates exactly the declared memory as EfiLoaderCode, and separately
allocates 18 EfiLoaderData pages for the bootstrap stack: one lower guard page,
16 usable pages (64 KiB), one upper guard page. All image/BSS/stack bytes are
zeroed before loading the file. Bases/sizes must be page aligned, disjoint, bounded
by current policy and below 48-bit physical addresses without overflow.

The final firmware memory map retains both allocations as non-conventional RAM;
the existing PMM admits only conventional type-7 pages, so neither is reused.
Firmware identity mappings remain active through EL1 entry or the existing
non-VHE EL2→EL1 transition. The new loader passes entry=base+64 and SP at the
upper guard boundary, leaving 64 KiB below it for actual stack use.

Private boot information retains existing assembly offsets 144/148. A formerly
reserved prefix word now encodes its size and contract version. The kernel checks
that word before reading appended image/stack fields and checks the image base
against its linked position. This explicitly rejects an old loader/new kernel
pair rather than assuming an older boot-information allocation has new fields.
The public EL0 syscall ABI remains version 1.

Kernel MMU initialization still starts with supervisor-only WB/NX mappings for
firmware RAM, overlays executable/read-only kernel text and read-only/NX rodata,
and leaves writable data/BSS/stack non-executable. It unmaps both actual bootstrap
stack guards. There is no base+512 KiB guard inside a growing image. Null/thread/
user guards, private process mappings and page-table ownership remain unchanged.
Initial BSS/stack zeroing and instruction cache synchronization precede use.
QEMU tests do not prove all real-hardware cache/coherency behavior.

The pristine file snapshot retained for Setup contains only original file bytes,
not mutated live data or runtime BSS. GPT/FAT32 installation accepts the shared
file-size policy and retains read-back/flush/payload verification. Installations
write the matching loader and kernel together. Old M7C disks/ISOs retain their own
working old loader/kernel pair; there is no supported single-file upgrade or
legacy image-format conversion path. Final boot-failure allocation cleanup remains
incomplete: reboot firmware after a loader failure.

## Controlled validation

`tests/growth.py` copies source to a fresh disposable build directory and builds:

```sh
make -j4 iso KERNEL_TEST_RO=1048576 KERNEL_TEST_BSS=3145728
```

Dedicated assembly emits 1 MiB of known file-backed bytes and 3 MiB of BSS; normal
builds emit zero growth bytes. A phony growth-object dependency prevents stale
Make variable changes. The kernel verifies the entire pattern and BSS zeroing,
writes the final BSS byte, and fault-tests the last growth pages as read-only/NX.
Production probe aliases resolve to the original zero-growth addresses and emit
identical production bytes. Tests independently check probe offsets exceed the
old 512 KiB ceiling and 4 MiB for the BSS probe.

Real UEFI boots cover EL1/GICv2 and EL2/GICv3, followed by an actual native install
and two disk-only boots of the large payload with persistent file operations.
Three more real UEFI boots reject over-policy memory, file-size mismatch and an
invalid entry before handoff. Host tests check truncation, all fixed fields,
limits, arithmetic, alignment and allocation overlap with ASan/UBSan. A preserved
M7C EFI executable plus current small kernel independently verifies explicit
boot-contract rejection. See [final M7D report](milestone-7d.md) for measurements,
source revision and actual results.

Remaining limits: 48-bit physical mappings, fixed 4 KiB kernel pages, one CPU,
1 GiB PMM tracking, on-demand translation tables capped at 4 MiB, bounded private user spaces and no
swapping/demand paging. Dynamic bootstrap sizing does not implement SMP or a
general kernel module loader. Further growth still needs measured page-table,
PMM, media-capacity and memory-pressure acceptance rather than removing all bounds.

M7E updates table construction and failure reporting without changing this image
format or private boot contract. See [MMU audit](mmu-architecture.md).
