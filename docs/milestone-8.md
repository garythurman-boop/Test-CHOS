# Milestone 8 — installation and desktop continuation

The user reports actual M7I operation on an Apple M1 MacBook Air with VMware
Fusion: normal ISO boot, original kernel, graphical Setup, keyboard, mouse /
trackpad and Live Royale Desktop work. These are user-observed results, not a
new automated Fusion run. Persistent Fusion installation remains unverified:
the installation worker reports "Disk identity or write protection changed."
M7I artifacts and its source checkpoint remain the known-good backup.

## M8A investigation

The exact old predicate was `!nvme_recheck() || !nvme_writable()` in
`installer/service.c`. This ran in the kernel worker after confirmation, so the
error did not identify a typing failure. Every Identify, serial, namespace,
geometry and writable-state failure was collapsed into error 1.

Initialization trimmed only trailing spaces from the 20-byte Identify Controller
serial. Revalidation reconstructed space padding and compared it with the raw
Identify response. A serial ending with NUL padding therefore passed initial
inventory but failed even when the identical device answered again. This is a
reproduced source-level defect; the user's raw Identify data is unavailable, so
it is not yet proven to be the sole cause of the Fusion error.

Both initial discovery and revalidation now use one bounded canonicalizer.
Trailing spaces/NUL padding are equivalent. Leading/interior spaces remain
significant; embedded NUL/control bytes, blank identities and non-ASCII serials
are rejected. Canonical serial, namespace NGUID/EUI64, sector count, sector size,
namespace write protection, controller-ready status and diagnostic read-only
policy are checked. Failures have distinct codes and readable UI/serial messages.
No disk-identity or write-protection gate is removed.

Confirmation is exactly `ERASE`. It authorizes the existing kernel-held selected
disk and plan through the process/session token; it never supplies a replacement
disk identity. Optical boot, verified readable geometry, serial availability,
writable namespace, explicit destination and layout review remain prerequisites.
An installed boot disk cannot authorize the destructive installer.

Portable tests cover space/NUL padding, full-width serials, empty/malformed
identities, serial/namespace/geometry changes, read-only identity and exact
ERASE-only confirmation. QEMU installation and later sub-milestone results are
recorded below as they complete.

## Remaining sequence

M8B: persisted two-phase setup with actual disk-only reboot. M8C: desktop input
semantics and cursor. M8D: firmware-enumerated display choices and safe preference
handling. M8E: incremental Royale refinement. M8F: integrated regression,
reproducibility and versioned ISO publication. No sub-milestone is accepted from
its design alone; actual Fusion persistent-installation acceptance is separate.

M8A validation: `./scripts/container.sh make -j4 iso test-nvme test-setup`
passed. GCC AddressSanitizer/UndefinedBehaviorSanitizer NVMe parser tests passed.
`./scripts/container.sh python3 tests/install.py --sector 512` passed: real GPT /
FAT32 / CHFS writes, guest-requested reset, disk-only cold boots, settings and
Notepad file persistence, independent on-disk CRC/parser verification, rejection
of wrong confirmation without writes, and installed-boot disk erase exclusion.
This verifies the correction under QEMU, not the user's Fusion failure resolution.

## M8B — durable two-phase installation

The blue optical Setup now performs GPT, FAT32/CHFS format, actual kernel/EFI
copy, read-back verification, flush and continuation-marker commit. It displays
operation-based progress and actual per-file byte counts. It does not use an
elapsed-time estimate. Completion explicitly says to remove the ISO and restart.
Restart invokes the existing ACPI/PSCI guest power path.

A verified `/system/setup-stage` record survives power loss: `CHOS SETUP PENDING
2` resumes Royale configuration only from the matching installed ESP/CHFS disk;
`CHOS SETUP READY 1` bypasses setup. Legacy disks without a marker preserve their
old behavior. Malformed records fail into the existing temporary recovery
session; they never authorize repartitioning. Missing/incomplete first-stage
files likewise do not trigger automatic writes or retries. A retained ISO shows
explicit remove-media guidance and still requires selection, review and ERASE.

The installed graphical phase persists a valid computer name, theme, fixed UTC
offset, clock preference and optional firmware resolution trial. The US keymap
is fixed; locale databases, DST rules and authenticated accounts are unfinished
and labeled. Required configuration failures keep setup pending. A successful
verified commit leads to a separate Welcome to CHOS screen, then the desktop.
The existing CHFS two-root commit mechanism protects individual records, not a
multi-file transaction; an interrupted save may retain old or new complete
settings, with the setup marker controlling whether configuration repeats.

The new QEMU installation acceptance performs a guest reset, a separate disk-only
pending boot, termination before graphical completion, another disk-only boot
that resumes Setup, persisted configuration, Welcome, subsequent desktop boot,
Notepad save/read across cold boot and independent GPT/CHFS/CRC inspection.
The 512-byte preflight passed; full matrix results follow after integration.

## M8C / M8E — desktop and application interaction

Royale's existing colors, wallpaper, typography, window chrome, taskbar and native
widgets are retained. The original cursor is now a 28-pixel antialiased light
arrow with dark outline, shadow and tip hotspot; a 36-pixel option is available.
Only normal-arrow artwork is implemented in this checkpoint. Partial damage
includes the old/new cursor extents, with no per-frame allocation.

Desktop application shortcuts select on one click and open on a second click
within a configurable 250/500/750 ms interval and four-pixel tolerance. Enter opens
the selected shortcut. A four-pixel movement threshold distinguishes dragging;
positions are clipped above the taskbar. Arrange Icons and Auto Arrange are real
operations. Grid snapping and arbitrary desktop file objects remain unfinished.
A checksummed `/system/desktop` record persists icon positions, click timing,
Auto Arrange and cursor size on installed volumes. Live changes remain volatile;
write failures are displayed rather than reported as saved.

Right-click menus support pointer hover, matching press/release activation,
keyboard navigation, dismissal and screen-edge placement. Desktop menus expose
arrangement, refresh, Display Properties, theme settings, timing and cursor size.
New Folder is visibly unavailable because desktop filesystem objects are not
implemented. Shortcut menus provide Open and real shortcut properties; Rename /
Delete are disabled. Explorer receives a distinct right-click event and provides
folder navigation, copy, move/rename, confirmed deletion and filesystem-object
properties. File Open associations and cut/paste are unfinished. Copy refuses an
existing destination; it retains the existing bounded 4095-byte copy limit.
All real file operations use native CHFS syscalls. Accounts/permissions are not
yet implemented; properties do not imply otherwise.

Native testing caught a flat-kernel absolute string-pointer table crash before
publication. Fixed-width inline arrays replace the tables. Every kernel link now
checks object relocations and rejects absolute address constants. Only the two
specified image-header size/offset fields are allowed; this is not a general
runtime relocation mechanism.

## M8D — real firmware modes and safe reboot trials

The UEFI loader enumerates supported GOP width/height/stride/format entries into
a bounded, deduplicated boot-information list. Only those choices appear in
Display Properties. This QEMU firmware currently exposes 640x480, 800x600 and
1024x768; desired larger modes are not invented. There is no post-ExitBootServices
GOP call and no runtime display driver capable of changing the mode.

Installed M8 ESPs reserve a dedicated aligned 4 KiB allocation for a checksummed
32-byte `CHOS/DISPLAY.BIN`. A native request revalidates NVMe identity, GPT,
FAT metadata, directory entries and original EFI/kernel payload before touching
that slot. It never allocates FAT clusters or edits boot directory metadata at
runtime. Older ESP layouts refuse the operation. Live ISO Apply is disabled.

Apply saves a one-boot trial. Before mode selection, firmware consumes the trial
using the UEFI File Protocol and flushes its attempted state. Failure to write
that state prevents attempting the mode. After reboot, Keep Current Mode commits
the actual current resolution. Without confirmation, the next cold boot requests
1024x768 where supported; malformed preferences also retain safe defaults. Apply
and Cancel do not change the running framebuffer. If neither requested nor
fallback mode is usable, the loader retains an already supported active firmware
mode. A failed/unreadable ESP preference may require reinstalling a disposable
target; no general FAT repair utility is claimed.

## Validation and release gate

Commands: `./scripts/container.sh make -j4 iso test-installer test-desktop
 test-graphics`, `./scripts/container.sh python3 tests/install.py --sector 512`,
new `tests/desktop_m8.py`, `tests/explorer_context.py`, `tests/display_m8.py`, and
`bash scripts/checkpoint.sh` for the full existing suite, host sanitizers and
clean committed-source reproduction. Integration results and source/image hashes
will be added after these gates complete. M8 Fusion persistent installation and
new desktop/resolution behavior require actual user testing; QEMU success alone
does not close that acceptance criterion.

The display preflight found a standards-level FAT interoperability issue: UEFI
updates access/modify timestamps and may set a parent directory's archive bit
when writing the trial record. Verification now allows those non-identity fields
while retaining exact names, type bits, allocation, cluster chains, lengths,
boot metadata and payload checks. Host tests distinguish legitimate timestamps
from a changed directory cluster. No arbitrary directory/payload changes are
accepted.
