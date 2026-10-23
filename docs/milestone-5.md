# Milestones 4–5: native desktop and application checkpoint

The original desktop now launches six functional EL0 applications: Explorer,
Notepad, Terminal, Control Panel, Task Manager and About. They use original window,
event, graphics, file, launch and information syscalls. Each process has private
memory, owned windows and file handles. No existing OS kernel or GUI runtime is
involved. [Desktop documentation](desktop.md) gives exact controls and limits.

Added a CHFS RAM block device for the live desktop. It never aliases a host or
virtual disk. Atomic file replacement validates the full user buffer before
CHFS copy-on-write. Native tests verify rejected replacements preserve old data.
The volume is labeled volatile throughout the interface: this is functional
session file storage, not a claim of desktop persistence or installation.

Completed launcher submenu/keyboard handling, task buttons for all eight slots,
unsaved-editor close cancellation, confirmed session termination, and restart/
shutdown using the ACPI FADT-discovered PSCI conduit. Firmware clock time is
sampled before leaving boot services. Search/Run/Help are currently shortcuts to
Explorer/Terminal/About; dedicated search, Run and Help interfaces remain open.
Image-file wallpaper loading, richer text editing, display mode changes and
persistent settings also remain beyond this bounded checkpoint.

## Build and validation

```
./scripts/container.sh make
./scripts/container.sh bash scripts/validate.sh
bash scripts/sanitize.sh
```

Twelve QEMU cases passed: nine positive functional boots and three expected
rejection cases. Six host test executables passed; the ELF validator also checks
the actual application ELF. ASan/UBSan passed on all six portable modules and
both ELF inputs. Both ordinary and storage-test ISO repeatability checks passed.
Independent cold-boot CHFS verification and normal-image disk-no-write checks
still pass. [Complete log](evidence/m5/validation-m5.log).

The [application test](evidence/m5/applications.json) drives the actual guest with
QMP/Virtio events and verifies rendered glyphs after Notepad editing/save/save-as/
open, unsaved-close cancellation, Terminal cross-process copy/move/cat/delete,
and Explorer navigation/mkdir/copy/move/confirmed delete. It checks actual Task
Manager PIDs and memory values and a real Control Panel wallpaper color change.
QMP shutdown reasons independently verify native shutdown and restart; these
are not host-issued quit commands. The earlier desktop move/resize/focus/control
acceptance remains in the regression. Screenshots under evidence/m5 are genuine
unmodified guest framebuffer captures, converted losslessly to PNG.

Measured in this recorded QEMU TCG run, with 512 MiB configured:

- Desktop ready: approximately 14.6 seconds from QEMU launch (includes UEFI/tests).
- Managed conventional RAM: 489,564 KiB.
- Allocated conventional RAM with two apps: 10,388 KiB.

These are not total resident-memory or idle-CPU measurements. Firmware, kernel
loader reservation and other non-conventional ranges are excluded from the
allocator counters. QEMU timing is not a Fusion performance measurement.

## Files and limitations

Application sources live in applications/{common,desktop,explorer,notepad,
terminal,control-panel,task-manager,about}; new storage is filesystem/ram.c.
Updated ABI/VFS, scheduler samples and blocking waits, power/ACPI, window model,
renderer/font, build and integration tests. Tests are applications.py, restart.py,
desktop.py and the original portable/boot/storage tests.

[Download and checksum](../artifacts/milestone-5/README.md).
No graphical installer, GPT/ESP formatting, installed-disk boot, authentication,
networking, GICv3, USB/xHCI or Fusion validation is delivered by this checkpoint.
CHOS 0.1 is not ready. Next: independently tested GPT/ESP installation primitives,
then explicit disk selection/confirmation, payload copying and installed reboot.
