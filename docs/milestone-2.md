# Milestone 2 core: isolated ELF processes, syscalls and IPC

Build and four UEFI QEMU cases pass at this checkpoint: functional 512 MiB and
1 GiB boots, intentional GICv3 rejection and missing-kernel diagnostics. Identical
ISO hashes were verified. Logs, JSON and actual screenshot: evidence/m2/.

Implemented on top of M2A: strict static ELF64 validation/loading, zeroed BSS,
private process page tables, EL0 entry, bounded heap mappings, versioned public
syscalls, per-process eight-message IPC queues, process-local memory-fault termination,
and deferred reclamation of user pages, tables and guarded kernel stacks.

Real EL0 programs test syscall errors, writable-buffer validation, memory allocation,
mailbox capacity/order, non-destructive failed receive, same-VA private data,
request/reply IPC and normal exits. Four separate processes deliberately attempt
kernel-data reads, code writes, stack execution and FP/SIMD use; each is terminated
and the kernel continues. Page ownership returns to the pre-process baseline after
every reap. EL0 FP/SIMD is trapped until full vector contexts are implemented.

Three host C test programs pass (memory/ACPI/ELF). ELF tests validate the actual
built AArch64 executable, then reject invalid architectures/types/endian headers,
overflows, W+X, overlaps, out-of-range entries, and run 2000 metadata mutations.
The ELF test also passed GCC ASan+UBSan. Existing M2A allocator/ACPI sanitizer
results remain in evidence/m2a. Source groups: kernel/process, kernel/syscall,
userspace/init, MMU/scheduler/exception extensions and tests/elf.c.

Reproduce: ./scripts/container.sh bash scripts/validate.sh

The exact public subset is documented in syscalls.md. No public spawn/wait API,
blocking IPC, SMP, dynamic linker, FP/SIMD contexts, user graphics, or file syscalls
exist at this checkpoint. Core milestone 2 targets are exercised, but this is not
a general-purpose process/application environment. Storage and file APIs follow.
Fusion, installed boot, desktop, installer and networking remain unverified or
unimplemented. No disk persistence claim is made by this checkpoint.

Next: original NVMe block I/O and CHFS filesystem, actual cold-reboot integrity
checks, and native file APIs before input/desktop work. No existing guest OS kernel
or filesystem library was used.
