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
compatibility or time zone service yet. File operations added at checkpoint
3A are documented below. Graphics, broader time,
process control and IPC extensions must get documented numbers when implemented.

Unknown EL0 synchronous faults terminate only that process. Exited contexts remain
until the bootstrap supervisor switches back to the kernel address space and
reaps them. The reaper releases user regions, private tables and guarded kernel
stacks. Current limits are eight live process records, twelve thread slots and
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
app 1–7 (Setup is 7); returns PID or negative error. 26 THEME selects session colors 0/1.
27 REPLACE takes absolute path, data pointer, length <=4096: validate all input,
then commit one CHFS whole-file replacement atomically. 28 POWER takes 1 reset or
2 shutdown via ACPI-declared PSCI, otherwise reports unsupported. 29 TASK takes
process slot 0–7 plus writable `chos_task` and reports PID/app ID/timer samples.

All structures are in include/abi.h. The extension returns -19 before desktop
startup. Public file replacement works on the mounted acceptance/desktop volume.
Counters describe managed conventional RAM, not the entire physical footprint.

## Native setup service

30 SETUP uses x0=operation, x1=user structure, x2=confirmation token. Only the
trusted built-in Setup role (app 7) may invoke it; other processes receive -13.
Operation 0 copies `chos_setup`: selected serial/capacity/logical-sector size,
boot medium (0 unknown, 1 GPT disk, 2 optical), eligibility, token and progress/
error state. Operation 1 copies `chos_confirm`, verifies owner/token, exact
`ERASE <serial>` text and computer-name syntax, then starts the single kernel
writer. Writes are disabled from disk/unknown boot media. NVMe identity/geometry
is rechecked before destructive work. Invalid confirmations do not write.

INFO now includes the persistent-volume flag and saved computer name. LAUNCH
accepts app 7 for the native installer. These are development ABI extensions;
applications and kernel must be built together.

## Royale additions (M6C)

Existing syscall numbers and structure sizes are preserved. DRAW adds op 3,
ROUNDED_RECT (x/y/w/h/color, radius 5), and op 4, UI_TEXT (w selects font 0 body,
1 bold, 2 title, 3 small, 4 mono; h ignored). Existing TEXT now renders the same
12-pixel advance with A8 coverage. All text remains 79-byte bounded ASCII. UI font
IDs outside 0–4 are rejected; no application supplies glyph or physical pointers.

31 RENDER takes x0 writable `chos_render`: five uint64 values `frames`, `pixels`,
`partial_frames`, `last_us`, `total_us`. Frames/pixels count completed paints and
actual framebuffer copy area. Counter-derived microseconds measure software paint
cost, not CPU utilization. It requires an active desktop and validates the buffer.

INFO's former `reserved` uint32 now reports VOLUME_* from include/abi.h: 0 live,
1 persistent, 2 no disk device, 3 invalid GPT, 4 ESP identity mismatch, 5 invalid
CHFS, 6 invalid installed computer configuration. `persistent` remains authoritative
for mounted persistent files; values 2–6 identify a temporary fallback session.

## M6D configuration and input additions

32 CONFIG: x0 operation (0 get, 1 apply, 2 stage installation), x1 pointer to
64-byte `chos_config`, x2 persistence flag 0/1 for mutations. Get validates a writable
buffer; mutations validate all input. Apply is restricted to built-in Control Panel
and Setup; staging is Setup-only and requires an eligible destination before the
writer starts. Apply with x2=1 requires an installed mounted volume. Errors include
-13 role/operation authority, -14 bad span, -22 malformed settings/operation, -30 no
persistent volume, -16 active installer, -5 save I/O. Stage never writes the disk.
See configuration.md for the versioned disk format and actual limitations.

23 EVENT additionally accepts nonzero x2 for a nonblocking dequeue. Existing x2=0
waits are unchanged. Clients can drain pending input before repainting, then block
at idle. EVENT_POINTER=5 and EVENT_RELEASE=6 carry client coordinates; release is
captured outside the window. Pointer coordinates can be negative on leave. Existing
CLICK=2 is press. KEY_SHIFT=131072 is added to Shift-Tab; KEY_ALT=262144 preserves
Alt combinations (Alt-Tab remains the window-manager shortcut). Buffers are still
validated before dequeue. Pointer events coalesce only with the same event type;
release and close survive a full 128-entry window queue.

## Dedicated Setup extension (M6D continuation)

The 64-byte `chos_setup` inventory structure is unchanged. SETUP operation 2
requests exit to Live Desktop, available only to Setup before writes or after
worker completion/failure; an active writer returns -16. The supervisor waits
for the requesting process to exit before changing the session. Operation 3 copies
`chos_destination`: minimum/ESP/CHFS bytes, copied/total payload bytes, target
eligibility reason, existing layout category, total GPT partition count and four
bounded partition summaries (byte start/length and ASCII name). The complete
structure is validated before copying; no writes occur during inventory.

The development phase enum is now 0 idle, 1 GPT prepare, 2 EFI format, 3 kernel
copy, 4 UEFI boot copy, 5 CHFS configuration, 6 verification, 7 verified completion.
The former development phase-4 completion meaning has changed: rebuild Setup and
kernel together. This extension is not a stable external application ABI yet.
Progress semantics and capacity policy are in [setup-environment.md](setup-environment.md).

Setup windows use full framebuffer dimensions and exclusive input, without
window-manager decorations or taskbar. Other roles cannot obtain this mode.
F3 is `KEY_F3=260`; Start/Alt-Tab are suppressed while Setup owns the screen.
LAUNCH returns -16 while an exclusive Setup window is present. Existing native
apps retain their normal owned-window coordinates and geometry.

Setup PRESENT additionally supplies x1 = the phase captured when that frame was
painted (0–7), rather than sampling worker state at submission time. Invalid
values return -22 before swapping command lists. EVENT_REFRESH=7 requests a
repaint after a worker stage change. Other apps keep x1=0 and their existing
PRESENT behavior. The compositor acknowledges the submitted phase only after
copying the actual full-screen frame to GOP; a stale frame cannot acknowledge a
newer operation. Refreshes are queued before waking the client, avoiding a lost
wake if a stage changes during painting.

## M7A/7B hardware snapshot

33 HARDWARE: x0 points to a writable 688-byte `chos_hardware` (include/hardware.h);
returns 0, or -14 for an invalid/protected span. Structure version is 1. It contains
firmware entry/current kernel EL, GIC/timer, initialized input/NVMe, sector size,
GOP dimensions/stride/format, ACPI GICD/selected GICR/ECAM, NVMe BAR, bounded firmware
vendor/serial strings and at most 32 16-byte PCI BDF/vendor/device/class records.
`pci_total` counts valid enumerated records, `pci_count` retained records. The
original firmware enumeration is read-only and bounded at 256 handles. Unavailable
resources are zero; storage/input fields indicate actual driver initialization,
not mere device presence. All apps may query the immutable kernel snapshot; it
provides diagnostics, not new device access or authentication authority. Terminal
`hardware` shows the first eight PCI records within its bounded text history.

## M7C bounded diagnostic export

34 DIAGNOSTICS: x0 writable destination, x1 capacity 1..1024, x2 report byte
offset. Returns bytes available (0 at end), -22 for invalid capacity/offset, -14
for invalid/protected destination. The entire requested span is validated and
zero-padded after the available bytes. Read repeatedly with increasing offsets
until zero. Kernel storage is an append-only 16 KiB sanitized ASCII report, with
512 bytes maximum per append; full storage discards further text. No arbitrary
address, filesystem or device write is accepted. Original apps query and reject
invalid spans on startup. On this single-CPU kernel, syscall/driver access is
serialized by exception/input execution. The report contains firmware/device and
initialization status, never raw memory, password input or HID key contents.

## M8 additions

`SYS_DISPLAY` (35): operation 0 copies `chos_display_info` (actual boot-time GOP
modes, current width/height and trial status). Operations 1/2 require Control Panel
or Setup on a persistent installed M8 volume: 1 accepts an enumerated
`chos_display_mode` pointer and saves a reboot trial, 2 confirms the current trial.
There is no runtime firmware call. Internal operation 3 lets Control Panel consume
a shell request to open its Display tab. Invalid pointers/roles/modes and failed
identity/GPT/ESP checks are refused.

`SYS_SETUP` additions: operation 4 queries persisted graphical-setup continuation;
5 validates, saves and verifies configuration and commits the ready marker.
These remain restricted to application role 7. Destructive operation 1 accepts
exactly ERASE and still requires optical boot, the selected kernel-held disk,
owner PID, session token, inventory and final NVMe identity/writable revalidation.

`EVENT_CONTEXT` (8) is a right-click in a client area, with client coordinates.
Explorer uses it for native filesystem-object menus. It is distinct from primary
pointer press/release; other applications can ignore it. Desktop shortcut gestures
remain in the shell and do not bypass filesystem syscalls.
