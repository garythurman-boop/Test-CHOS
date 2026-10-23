# Milestone 6C: Royale foundation and boot recovery diagnostics

This continues the native M6B kernel, desktop, applications and installation core.
The stable starting commit is `e6a7dbe670eb696d3eb6957c154e4b5834584e93`, tagged
`checkpoint/milestone-6b`. This iteration advances both requested tracks; it does
not declare the entire installer, Fusion validation or CHOS 0.1 complete.

## Operating system progress

- UEFI enumerates up to 256 GOP modes, validates geometry/pixel format and prefers
  advertised 1024×768. An optional `CHOS/DISPLAY.CFG` requests `WIDTHxHEIGHT`.
  Unsupported requests retain the firmware mode with a diagnostic. QueryMode pool
  results are freed before the final memory map/ExitBootServices sequence.
- Actual supported RGB/BGR modes remain bounded to 640×480–1920×1200. Control Panel
  shows the active dimensions; mode switching is boot-time only. Enumeration is
  firmware specific, not proof of Fusion display support. Build a requested ISO
  with `CHOS_DISPLAY_MODE=800x600 python3 scripts/mkiso.py` in the toolchain.
- Disk boot distinguishes missing device, invalid GPT, mismatched ESP identity,
  invalid CHFS and invalid computer configuration. An unavailable installed volume
  produces a visible temporary-file warning and a read-only system-state report.
  Failed mounts do not format, repair or silently claim persistence.
- The existing native installer preserves typed confirmation and boot-disk exclusion.
  The font redistribution notice is carried into the ISO/kernel/installed filesystem.
- Additive graphics commands and a read-only render statistics syscall retain old
  syscall numbers and fixed-advance editing behavior.

## GUI progress

Anti-aliased cached fonts; alpha/rounded/gradient/shadow/blit primitives; original
wallpaper and icons; Royale title bars, vector controls and taskbar; two-column
Start menu with native pointer launches; proportional app labels/status text and
rounded toolbar/dialog/Setup controls; clipped pointer repainting. See [Royale
architecture and real before/after captures](royale.md).

## Build and validation

```sh
./scripts/container.sh image
./scripts/container.sh make
./scripts/container.sh bash scripts/validate.sh
bash scripts/sanitize.sh
```

The expanded suite requires 24 real QEMU boots (21 positive cases, three expected
rejections), ten portable host programs, ASan/UBSan, ISO/disk repeatability and
independent GPT/FAT32/CHFS parsers. Tests use only disposable image files. Native
installation is exercised at both 512- and 4096-byte sectors; each is followed by
two fresh disk-only VMs checking a saved Notepad file and self-erasure refusal.
New cases check native Start-menu pointer launch/partial-paint counters, alternate
800×600/unsupported-mode fallback, and a COPY of an installed disk with both CHFS
roots corrupted. The corrupt copy must show the mount warning and remain byte
identical after boot. Machine-readable results and unchanged-disk hashes accompany
the screenshots in [evidence/m6c](evidence/m6c).

Recorded result: **all 24 QEMU cases and ten host programs passed**, including
ASan/UBSan. Release ISO is 17,154,048 bytes, SHA-256:

```
6b1c1c662db25a4c16da609c3e80ad865a2bc266f7ae605cfb6b104306a01a56
```

At 512 MiB, the two-application acceptance reported 14,664 KiB allocated
from 489,364 KiB managed conventional RAM. Desktop readiness was
15.752 seconds including UEFI and QEMU TCG. The dedicated pointer test observed
partial-paint count 0→10. Backbuffer plus wallpaper cache occupy 6 MiB at
1024×768; kernel linked text/data/BSS total 365,402 bytes. These are scoped
measurements, not total physical footprint, native hardware timings or idle CPU.
Both alternate-mode tests observed 800×600, including unsupported 1500×900 fallback.

| Acceptance | Result |
|---|---|
| ISO generated/UEFI optical catalog | PASS |
| QEMU UEFI boot/desktop/apps | PASS |
| Native installer execution | PASS |
| Native installation to NVMe | PASS, 512/4096-byte sectors |
| Fresh installed disk boot and persistent file reopen | PASS, both geometries |
| VMware Fusion boot/install/reboot | NOT RUN |

[Download notes and ISO](../artifacts/milestone-6c/README.md).

Source changes: boot/uefi; desktop/desktop.c and window-manager/model.c; new
primitives/fontcache/assets under desktop/graphics; graphics/display-mode/Royale
headers; additive ABI/syscall dispatch; installer/service.c; native application
presentation; licensed font assets/generator; graphics/Royale/mode/recovery tests;
Makefile and validation/ISO/sanitizer scripts. Application operations, process
isolation, storage formats and the existing kernel architecture are preserved.

## Known problems and next work

No Apple Silicon/Fusion host is attached: Fusion boot/input/display/storage/install
and reboot remain **unverified**. GICv3, EL2 and USB/xHCI remain unsupported. Native
Setup still lacks accounts/passwords, timezone/layout selection, active-write
cancellation and repair/rollback. Boot-time display configuration is implemented,
but no graphical mode chooser or installed DISPLAY.CFG writer exists yet. No login
security, network stack, filesystem expansion or CHOS 0.1 release is claimed.

The next coherent iteration should extend shared widget focus/pressed states and
installer configuration/recovery, with native regression tests, while researching
and implementing the specific Fusion hardware gaps. Full renderer/window damage,
wallpaper loading and richer icons remain separate tested improvements.
