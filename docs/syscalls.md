# CHOS process ABI v1

Static AArch64 ELF64, little-endian, ET_EXEC/machine 183/version 1. Only one to
eight PT_LOAD program headers are accepted: 4 KiB-aligned virtual/file offsets,
alignment 4096, readable segments, no W+X, no interpreter or dynamic linking.
Segments are at most 64 KiB each inside [0x1000000000,0x1000200000). The aligned
entry must lie in file-backed executable bytes. Headers, arithmetic, segment
bounds and page overlaps are checked before allocation. BSS is zero-filled.

Each process has private TTBR0 tables and the same user virtual layout. The
kernel's identity mappings remain supervisor-only. The two-page user stack ends
at 0x1040000000 with absent surrounding pages; bounded heap allocations begin at
0x1000400000. EL0 FP/SIMD traps terminate the process until context support exists.

Use SVC #0, x8 syscall number, x0/x1 arguments; x0 returns the value or a negative
error. Other integer registers are preserved. Code compiled for this initial ABI
uses general registers only. Initial x0/x1 are application role/peer arguments used
by the embedded acceptance programs, not a general argc/argv environment.

| x8 | Operation | Arguments / result |
|---|---|---|
| 0 | ABI version | returns 1 |
| 1 | Diagnostic console write | x0 buffer, x1 length <=64; returns count |
| 2 | Monotonic timer ticks | returns nominal 100 Hz tick count, not wall time |
| 3 | Exit | x0 signed status; does not return |
| 4 | Yield | reschedule, returns zero |
| 5 | Allocate user pages | x0 count 1..16; returns private virtual address |
| 6 | Free user allocation | x0 exact allocation address; returns zero |
| 7 | Send IPC | x0 destination PID, x1 64-bit value; returns zero |
| 8 | Receive IPC | x0 writable 16-byte {sender,value} destination; returns zero |
| 9 | Get PID | returns stable monotonic process identifier |

Errors: -3 missing/exited PID; -11 empty/full nonblocking mailbox; -12 allocation
failure; -14 invalid/protected user span; -22 invalid argument/free; -38 unsupported
syscall. Each mailbox holds eight messages. Buffer validation occurs before dequeue;
a failed receive leaves the message available. The kernel checks the complete
span before copying through kernel aliases. No caller-supplied physical address is
trusted. Copy spans are bounded to 4096 bytes internally; console is capped at 64.

Kernel code creates processes from a validated embedded ELF. There is no public
spawn/wait API, filesystem-based executable loading, dynamic linker, POSIX
compatibility, time zone service or GUI API yet. File operations added at checkpoint
3A are documented below. Graphics, broader time,
process control and IPC extensions must get documented numbers when implemented.

Unknown EL0 synchronous faults terminate only that process. Exited contexts remain
until the bootstrap supervisor switches back to the kernel address space and
reaps them. The reaper releases user regions, private tables and guarded kernel
stacks. Current limits are four live process records, eight thread slots and
sixteen regions/tables per process. All management operations run with IRQs masked;
SMP and hostile-device DMA isolation are outside this milestone.

## File extension at checkpoint 3A

Only available with the test-mounted CHFS volume; otherwise returns -19 ENODEV.
NUL-terminated paths are limited to 59 bytes; full spans are checked before use.

| x8 | Operation | Arguments / result |
|---|---|---|
| 10 | Open | x0 path, x1 flags: read=1/write=2/create=4/truncate=8; returns fd 3..10 |
| 11 | Read | x0 fd, x1 writable buffer, x2 length <=4096; returns count |
| 12 | Write | x0 fd, x1 readable buffer, x2 length <=4096; returns count |
| 13 | Close | x0 fd; returns zero |
| 14 | Mkdir | x0 path; returns zero |
| 15 | Unlink/remove empty directory | x0 path; returns zero |
| 16 | Rename file | x0 old path, x1 new path; returns zero |
| 17 | Seek absolute | x0 fd, x1 offset <=32768; returns offset |
| 18 | File size | x0 fd; returns bytes |
| 19 | List metadata slot | x0 index 0..39, x1 writable chos_dirent[1]; returns zero or -2 for empty |

Additional errors: -2 missing path/slot; -5 I/O or filesystem update failure;
-9 bad handle/access mode; -19 unmounted; -24 no free handles; -36 path too long.
Read/write reject invalid buffers before changing offsets. Directory entries are
{char path[60]; uint32 size,kind}; kind 1=file, 2=directory. Paths are absolute,
case-sensitive and ASCII. There is no per-user file access control yet.

## Desktop extension (development ABI)

20 WINDOW copies `chos_window` (48-byte title, signed width/height) and returns
an owned, non-reused positive handle. One window per process, eight windows total.
21 DRAW takes handle + `chos_draw` pointer; CLEAR resets staging, RECT/TEXT append
bounded commands (128/frame, 79-byte text). Coordinates are clipped to the client.
22 PRESENT swaps staging/visible command lists. 23 EVENT takes handle + writable
`chos_event`; validates before dequeue. Empty waits block and return -11 after an
event wake or one-second timeout; retry to receive. RESIZE gives client dimensions,
KEY gives ASCII/navigation plus KEY_CTRL, CLICK gives client coordinates, CLOSE
requests voluntary exit (an editor may confirm/cancel).

24 INFO copies real `chos_info` counters/dimensions. 25 LAUNCH selects built-in
app 1–6; returns PID or negative error. 26 THEME selects session colors 0/1.
27 REPLACE takes absolute path, data pointer, length <=4096: validate all input,
then commit one CHFS whole-file replacement atomically. 28 POWER takes 1 reset or
2 shutdown via ACPI-declared PSCI, otherwise reports unsupported. 29 TASK takes
process slot 0–7 plus writable `chos_task` and reports PID/app ID/timer samples.

All structures are in include/abi.h. The extension returns -19 before desktop
startup. Public file replacement works on the mounted acceptance/desktop volume.
Counters describe managed conventional RAM, not the entire physical footprint.
