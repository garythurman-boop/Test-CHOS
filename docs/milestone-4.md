# Milestone 4: desktop core checkpoint

Implemented an original software compositor, blue theme, glyphs, clock sampled
from UEFI before ExitBootServices, desktop launcher, taskbar, pointer, and window
manager. Windows support stacking/focus, movement, resizing, minimize/taskbar
restore, maximize/restore, close, and Alt-Tab. Start opens by mouse or the Meta key;
Meta then 1–6 launches a client. At this checkpoint the client is a functional EL0
window/API acceptance application. The six requested applications follow next.
Some Start menu destinations are provisional; session/power entries say `later`.
This is a tested core checkpoint, not completion of every milestone-4 requirement.

Application drawing is a bounded, clipped command stream, owned by process and
window ID. Apps cannot access the framebuffer. Present swaps staged/visible
command lists. Event buffers are validated before dequeue. Resize events coalesce;
close survives a full queue. Dead processes release windows, drawing pages,
private mappings and thread stacks. Eight process records/twelve thread slots are
available. Event waits block until input or a one-second timeout rather than spin.

## Source and build

Added `desktop/desktop.c`, `desktop/window-manager/model.c`,
`desktop/graphics/{font,render}.c`, `include/desktop.h`, native client
`applications/desktop/main.c`, and `tests/desktop.{c,py}`. Updated UEFI boot clock
handoff, ABI, scheduler, process lifecycle, font diagnostics, build and validation.

```
./scripts/container.sh make
./scripts/container.sh bash scripts/validate.sh
```

## Evidence

The complete regression passed: six host test programs, ten QEMU cases (seven
positive boots including desktop interaction, three expected rejection cases),
repeatable normal/storage-test ISOs and independent CHFS cold-boot verification.
The desktop model also passed host ASan/UBSan. Tests cover ownership/stale IDs,
queue saturation, focus, move/resize, bounds and control actions. The native client
rejects invalid creation/drawing/event pointers, invalid command opcodes and bad
window handles before rendering. QMP verifies actual keyboard-driven redraw,
movement, resize, minimize/restore, maximize/restore, close and relaunch.

See [recorded results](evidence/m4/validation-m4.log),
[desktop test](evidence/m4/desktop.json), and
[real guest screenshot](evidence/m4/desktop-interaction.png).
The downloadable ISO and checksum are in `artifacts/milestone-4/`.

## Limits and next step

GOP software drawing only, no acceleration, display mode switching or image-file
wallpaper. Clock follows the firmware's time basis and can drift after its initial
sample; no timezone/date setting yet. The backbuffer is capped at 1920×1200.
No installer, installed boot, USB/xHCI, GICv3, authentication or Fusion validation.
Next: functional native applications with explicit volatile-volume behavior and
atomic file replacement, followed by installation foundations.
