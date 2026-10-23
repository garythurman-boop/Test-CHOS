# Dedicated native CHOS Setup

The optical image boots the existing original CHOS kernel and hardware services,
then launches only the native Setup ELF role. It allocates one software framebuffer
and a RAM-backed diagnostic filesystem. It does **not** launch About, the taskbar,
icons or other desktop applications. The wallpaper surface is allocated lazily
only when a desktop is actually displayed. No second kernel or firmware disk I/O
path is introduced.

## Flow

1. Full-screen blue welcome: Enter installs, L opens Live Desktop, R reports the
   currently unavailable recovery tools. Up/Down selects an option. F3 exits to
   Live Desktop. There is no timeout or implicit destructive action.
2. Read the original CHOS agreement. Enter accepts MIT terms and continues.
3. Review the single supported NVMe controller/namespace, serial, capacity,
   logical sector size, eligibility and existing partition summary. Inventory is
   read only. Primary GPT header/array CRCs, partition bounds/nonoverlap and names
   are checked; the backup is not asserted valid by inventory alone. At most four
   partition summaries are displayed. Unknown/MBR/invalid layouts are identified.
4. Preview the replacement GPT, FAT32 ESP and CHOS system layout. Nothing is
   written. Escape goes back.
5. Full-screen Royale configuration collects computer name, theme, fixed clock
   offset and seconds preference. This happens **before** disk preparation so
   configuration can be corrected or cancelled without leaving a partial disk.
   Accounts/authentication, other keyboard layouts and display-mode settings are
   explicitly unfinished. No username/password fields pretend to implement them.
6. Full-screen blue confirmation repeats the serial/capacity and requires
   `ERASE <exact serial>`. Incorrect text changes no disk bytes. Escape corrects
   settings; F3 returns to Live Desktop. Only Enter with a valid process/token,
   target and configuration starts the writer.
7. GPT preparation uses the blue screen. The graphical phase then displays the
   actual worker stage: FAT32/CHFS formatting, system payload copy, UEFI boot payload
   copy, CHFS configuration, final read-back and log verification.
8. Completion appears only after verification. Restart/R uses ACPI-discovered
   PSCI. Detach the ISO; disk-only boot mounts CHFS and opens the regular desktop.

Launching Setup from Live Desktop also opens the dedicated full-screen surface.
Existing applications remain allocated behind it. Leaving Setup restores them;
leaving the initial optical Setup starts the desktop and About. Normal installed
boot goes directly to the desktop. Installed-volume failures have an explicit
RAM-only fallback warning; they never trigger formatting or repair.

## Native presentation and input

Setup uses the same original renderer, drawing-command syscall and shared C
widgets as native applications. Only the trusted Setup role gets a borderless,
exclusive full-screen client. Its resize event gives the complete GOP dimensions;
coordinates start at (0,0). The WM routes mouse and keyboard to this client and
suppresses Start, Alt-Tab and window controls. F3 is an explicit navigation key.
Other app types cannot request the exclusive surface through an arbitrary flag.

The blue pages use cached ASCII monospace text, a restrained blue selection and
keyboard instructions along the bottom. The graphical pages reuse Royale fonts,
colors, fields, radios, checkboxes, buttons and progress bars, with an original
CHOS mark and stage sidebar. No Windows artwork, assets, fonts or source is used.
The default desktop wallpaper, icons, menu organization and blue taskbar remain.

The worker reports six real stages. Overall progress is a stage count, with the
current file's fraction during copy; it is **not** a byte-weighted percentage of
whole installation time. File counters count successfully written payload bytes;
read-back is performed separately. Unmeasured metadata operations name the active
operation, rather than showing a fake timer or remaining-time estimate.

On a stage change the worker queues a refresh, wakes Setup and cooperatively yields until the
compositor has displayed that stage's submitted frame. Setup submits the phase
it actually painted; the compositor does not infer it from newer worker state. There is no minimum stage
duration. A 200-tick upper bound (nominally two seconds) and owner-liveness check prevent a lost UI from
stalling disk work indefinitely. This is presentation synchronization, not fake
progress. Progress snapshots and WM notification queue mutations use IRQ exclusion, matching
the single-CPU kernel synchronization contract. The private kernel yield SVC is
distinct from EL0's public SVC #0.

## Storage layout and safety

The fixed layout reserves 1 MiB before a FAT32 ESP, 64 MiB ESP with 512-byte logical
sectors or 272 MiB ESP with 4096-byte sectors, at least 4 MiB CHFS and five 4 KiB
blocks for backup GPT structures. Those ESP sizes satisfy FAT32 cluster-count
requirements with one logical sector per cluster. They are a documented format
policy, not an assertion that these are the smallest possible FAT32 volumes.

Exact minimum device capacities for this layout are:

| Logical sector | Minimum bytes | Near-minimum test target |
|---|---:|---:|
| 512 | 72,372,224 (69 MiB + 20 KiB) | 70 MiB |
| 4096 | 290,476,032 (277 MiB + 20 KiB) | host boundary test; 512 MiB native test |

A 128 MiB/512-byte-sector disposable fixture is a valid destination for the entire
current payload; it is not evidence of a future desktop release's capacity needs.
The system partition spans the remaining disk, but CHFS currently uses at most
its first 4 MiB. Extra capacity is not yet available to user files. The pristine
boot/kernel payload includes the native applications; it is copied from the
loader-retained source snapshot rather than mutated running kernel memory.

Eligibility requires optical boot, a readable target, supported geometry and
layout capacity, serial identity and no namespace write-protection attribute.
Reads of both disk ends and existing metadata precede authorization. Actual write
capability is established by the **confirmed** installation operations and their
flush/read-back results, not by enumeration or a destructive pre-confirmation
probe. NVMe serial/namespace geometry and write protection are rechecked just
before writes. Disk/unknown boot media cannot authorize erasure of the boot disk.

The original GPT writer verifies primary and backup headers/arrays. The original
FAT32 writer verifies mirrored FATs, directories, boot sectors and exact loader/
kernel payloads. Final CHFS verification reopens and compares the complete
configuration, computer name, welcome file, MIT license and font OFL license.
`/system/install-log` records those completed checks and is itself read back.
UEFI boot uses `EFI/BOOT/BOOTAA64.EFI` on the installed ESP; no firmware NVRAM
boot entry or firmware call after ExitBootServices is required.

An error identifies the failed operation and warns that the destination may be
incomplete. Keep the ISO, power off and preserve any needed disk image before
retrying. The writer cannot be cancelled while active. An interrupted install is
not transactional and does not promise to preserve previous data. Recovery/fsck,
rollback, encrypted credentials and general partition editing remain unimplemented.
Successful-install logs are persisted only after a usable CHFS exists; early
failure/interruption does not promise an on-disk recovery log.

All automated writes attach newly created regular disposable QEMU image files.
No test opens a host block device or relies on a real user-data disk. QEMU evidence
does not establish VMware Fusion compatibility; EL2/GICv3/USB remain blockers.
