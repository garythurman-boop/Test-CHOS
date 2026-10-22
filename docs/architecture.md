# Architecture: M1 and direction

## Boot flow

1. ARM64 UEFI reads the ISO's El Torito EFI FAT boot image.
2. The removable-media path EFI/BOOT/BOOTAA64.EFI loads as a native ARM64 PE32+
   EFI application. Minimal local UEFI ABI definitions are in boot/uefi/efi.h;
   the compiler checks key structure offsets. No third-party bootloader is used.
3. The loader opens CHOS/KERNEL.BIN on its own loaded-image device, bounds the
   file to 512 KiB, allocates 1 MiB of EfiLoaderCode, zeroes the reservation,
   reads the file, and verifies the byte count. Errors before handoff print useful
   console diagnostics and return an EFI error to firmware.
4. GOP provides a linear RGB/BGR framebuffer. Other pixel formats, insufficient
   framebuffer sizes, and resolutions below 640x480 are rejected. CHOS does not
   assume a VMware GPU or QEMU MMIO address.
5. The loader cleans data cache and invalidates instruction cache for loaded code,
   using CTR_EL0 line sizes and barriers. It captures a memory map, retries an
   invalid map key at most three times, and exits boot services. No firmware I/O
   or allocation occurs between the final map retrieval and exit.
6. Entry receives x0=boot-info pointer and x1=temporary stack top. The image is
   linked at zero, uses position-independent references, and is loaded at a
   page-aligned firmware-chosen address. Loader reservation includes BSS and a
   temporary stack; linker asserts image plus BSS fits below 512 KiB. IRQ/FIQ/
   asynchronous exceptions are masked. The kernel switches stack, draws its own
   console directly to GOP memory, validates descriptor stride, counts conventional
   memory, reads CurrentEL/CNTFRQ_EL0, paints a completion marker, and waits.

Boot information is defined in include/boot.h. Memory map descriptor stride and
version are preserved. Loader code/data, map, boot-info, framebuffer and firmware
regions must remain reserved until a real memory manager has classified them.
The current diagnostic count is firmware conventional RAM, not total or measured
kernel free memory. No allocator has been implemented.

## Decisions and constraints

- Original lightweight monolithic C kernel; assembly only for CPU entry initially.
- Flat position-independent kernel for M1; kernel.elf is a link artifact, not an
  implemented user executable loader. The flat payload is trusted build input;
  magic/hash/signature verification and Secure Boot are future work.
- Firmware mappings/cache state and entry EL are inherited for this milestone,
  in accordance with AArch64 UEFI handoff. No address space or EL transition is
  configured by CHOS yet. QEMU observed EL1; EL2 behavior is untested.
- No boot-service or runtime-service calls after successful ExitBootServices.
  A map/exit failure after an exit attempt halts silently because firmware may be
  partially shut down. A future emergency framebuffer panic path is required.
- No hardware addresses are hardcoded. Firmware-supported boot device/GOP suffice
  only for M1; runtime storage/input/display drivers remain required.
- No disk-write path exists in M1. Later installer tests must use disposable disks,
  display identity/capacity, exclude source media, and require explicit erasure
  confirmation before GPT/ESP/system partition changes.

## Planned M2 security foundation

Install a 2 KiB-aligned exception vector table before enabling interrupts; add
fault reports with ESR/FAR/ELR and bounded panic output. Discover ACPI/DT topology
before GIC/timer initialization. Add a physical allocator with reserved-range
exclusion, translation tables, NX/W^X and guard pages, and a bounded kernel heap.
Test overlap/overflow and malformed metadata on the host as well as in QEMU.
Then add kernel threads, context switches, preemptive scheduling, EL0 processes,
and a documented syscall ABI with user-pointer/range validation. User executable
loading should accept a strict static AArch64 ELF64 subset, validate segment bounds,
zero BSS, reject writable executable mappings, and isolate address spaces.

Future syscall families cover files/handles, processes, mappings, console/graphics,
time, and IPC. No syscall numbers or functionality are promised before implementation.
