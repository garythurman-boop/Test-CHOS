# M7I Fusion test — one normal ISO first

The M7H captures now establish the missing paths: port 6 has two report-only HID
interfaces (`0e0f:0003`, 03/00/00); ports 7/8 are USB2 hubs (`0e0f:0002`). xHCI,
MMU/GIC/GOP and NVMe read-only probes pass. M7I implements report HID and hub-child
discovery; actual keyboard/trackpad behavior must be checked on the Mac.

1. Download [the normal M7I CHOS-ARM64.iso](../artifacts/milestone-7i/CHOS-ARM64.iso)
   using **Download raw file**, and compare `shasum -a 256` with adjacent SHA256SUMS.
2. Reuse the same ARM64 UEFI VM: one CPU, 2 GiB, Secure Boot off. Cold boot with
   **no disk attached first**. Preserve M7H. Record the exact Fusion/macOS build and
   whether optical boot was automatic; previous version strings are user-reported.
3. With initialized input, the usual blue Setup should appear. Check Up/Down,
   Enter/license, Escape/back, Tab/focus and L/F3 Live Desktop. Confirm the pointer
   aligns with buttons, then test Start menu, a native text window, Shift/control
   combinations, ordinary key presses/releases, dragging and left/right clicks.
   This is the real input gate; recognizing descriptors/endpoints is not enough.
4. If either input category is unavailable, the **same ISO automatically opens
   read-only diagnostics**, with no keypress or second ISO required. Capture a full
   page cycle, especially hub ports/routes/child identities, selected interface,
   report descriptor LE32 words/length, HID compiler or xHCI completion failures,
   and the fixed keyboard/mouse/event/ENUM fields. It skips Setup/mounting and
   forces the NVMe write/flush guard. Do not change VM controller settings before
   capturing the baseline. The last successful operation may be a hub operation;
   its zero ENUM reason does not override earlier HID/compiler refusals.
5. The optional [INPUT ISO](../artifacts/milestone-7i/CHOS-ARM64-M7I-INPUT.iso)
   always shows the live panel even when input works. Use it for pointer coordinates,
   button/keyboard events, topology/descriptor captures and the next read-only disk
   test. Only the last numeric event is volatile; no key content/history is saved.

After input passes, attach a **designated disposable NVMe virtual disk** and cold
boot the INPUT ISO. Repeat controller/namespace identify, first/last 4 KiB reads
and diagnostic write guard; these passed on M7H and must not regress. Capture
sector size/capacity and errors. Do not infer reliable persistence from these reads.

Only after actual input, Setup interaction and read-only storage gates pass may
normal Setup installation be tested with exact disk identity/capacity review and
typed ERASE confirmation. Disk-only reboot and saved-file integrity are further
independent gates. M7I Fusion input/installation/persistence are pending until
those observations arrive. [Implementation/limits](milestone-7i.md),
[codes](driver-diagnostics.md), [compatibility matrix](fusion-compatibility-matrix.md).

## M8 installation retest

The user has now confirmed M7I keyboard, mouse/trackpad, Setup and Live Desktop
on Fusion/M1. The M8 development ISO addresses the installation-worker recheck
failure and adds two-phase setup; persistent installation still requires a real
Fusion retest. Preserve the M7I ISO as the known-good backup.

Use a new disposable NVMe virtual disk. In blue Setup review controller 0 /
namespace 1, serial, capacity, sector size, existing partitions and proposed GPT /
ESP / CHOS layout. Type exactly ERASE. If a precise recheck error appears, stop
and capture the entire error screen; do not change the disk identity or disable
the guard. The installer must display verified first-stage completion.

Power off after completion if necessary, disconnect the ISO, then start the VM
from the same virtual disk. Royale configuration should appear automatically.
Finish valid settings, verify Welcome, then reboot again: Setup must not repeat.
Save a Notepad document and check its contents after another cold boot.

Test one-click selection, double-click opening, shortcut dragging and saved
positions, context menus and the visible cursor. Display Properties only exposes
firmware-enumerated modes. Apply schedules a trial at restart. After a visible
trial boot, Keep Current Mode confirms it; otherwise the following cold boot
falls back. Never interpret QEMU screenshots as Fusion installation evidence.
