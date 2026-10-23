# M7D bootstrap and diagnostic hardening checkpoint

Versioned flat kernel metadata replaces the fixed 512 KiB bootstrap contract.
UEFI allocates declared image/BSS memory and an independent 64 KiB guarded stack;
old private boot contracts and malformed images are rejected. Native installation
uses the shared larger-file policy. Optional input failures preserve diagnostic
mode; narrow dialog hints fit alongside buttons. Existing polled xHCI is preserved.
The acceptance sequence expands to 61 QEMU cases/17 host programs, including
controlled large-image boot/install/reboot. See [M7D report](milestone-7d.md) for
final results. Fusion remains untested and awaits actual hardware screenshots/logs.
Previous working checkpoints are preserved; this is not full Milestone 7 completion.

# M7C discovery and USB development checkpoint

A separate input-free diagnostic ISO automatically reports firmware/UEFI/EL/ACPI/
GIC/timer/PCI/GOP and initialized/unsupported drivers, with bounded read-only export
and firmware-described PL011 serial output. Original polled xHCI USB2 root boot-HID
input joins the existing events; narrow Notepad controls/menu retain Royale styling.
Acceptance expands to 50 QEMU cases/16 host programs and two reproducible images.
See [M7C report](milestone-7c.md) for final results. Fusion is untested; USB interrupts,
other controllers/hubs/general HID and broad recovery remain unfinished. Previous
images are preserved. This is an intermediate checkpoint, not CHOS 0.1.

# M7A/7B development checkpoint

Original non-VHE EL2→EL1 startup, ACPI GICv3 discovery/controller/CPU interface,
read-only firmware PCI inventory and native `hardware` diagnostics are implemented.
Shared binary-size labels refine existing Setup without changing Royale design.
Expanded regression: 45 QEMU cases/14 host programs plus sanitizers and clean-source
reproduction; see [iteration report](milestone-7a-b.md) for final recorded results.
Fusion remains unverified, USB host/HID absent, accounts design-only. Previous
images are preserved. This is not Milestone 7 completion or a CHOS 0.1 release.

# M6D continuation — dedicated native Setup

This development checkpoint preserves the previous M6D widget/configuration ISO
and original Royale desktop. The ISO now boots directly into full-screen native
Setup: blue welcome/license/disk/layout/typed erase pages, Royale configuration,
actual operation/file-copy progress, complete payload/configuration verification,
installation log and Restart. L/F3 starts Live Desktop; disk-only boot remains the
persistent desktop. Actual layout capacity replaces the arbitrary 128 MiB gate.

See [iteration report](milestone-6d-setup.md), [installation instructions and safety](setup-environment.md),
and [current status](status.md). This is not CHOS 0.1. Account authentication,
complete time-zone/layout/display configuration, repair/rollback, USB/GICv3/EL2,
networking and actual VMware Fusion validation remain unfinished.

The following sections preserve earlier checkpoint notes.

# M6D — persistent configuration and native widget refinement

Preserves the approved Royale design. Adds a shared original C control library,
native application menus/dialogs, tabs/fields and real file list scrolling; saved
computer/theme/clock configuration; Setup Back/error correction; pre-paint modal
input binding, bounded event draining/capture and deterministic pointer/keyboard
batch delivery. Installed sessions lose the permanent overlay. Shadow edge output
is preserved while omitting covered blending. See [M6D evidence and limitations](milestone-6d.md).
Authentication, full installer configuration, Fusion hardware validation and CHOS
0.1 remain unfinished.

# M6C — Royale visual foundation

Continues M6B with licensed anti-aliased fonts, original wallpaper/icons, blue
rounded chrome/taskbar and two-column Start menu, shared native presentation
primitives, pointer damage, real paint counters, supported UEFI mode selection and
explicit installed-volume failure diagnostics. Native installer/persistent reboot
regressions remain acceptance requirements. See [M6C evidence and limitations](milestone-6c.md).
This is an engineering checkpoint, not CHOS 0.1 or Fusion validation.

# CHOS native installation core (milestone 6B)

Adds original EL0 graphical Setup with identity/capacity review and typed erase
confirmation. A preemptible kernel worker installs GPT/FAT32 boot files and a CHFS
system partition from pristine original payloads. Installed boot matches the EFI
partition identity and mounts persistent files. The existing MIT license is retained
and included in the ISO/installed volume; the wizard has a separate setup agreement.

Twenty QEMU boots and nine host programs pass, including native installations to
512/4096-byte NVMe devices, two installed cold boots per geometry, independent file/
configuration/license checks, rejected confirmation and self-erasure refusal.
ASan/UBSan and ISO/builder-disk repeatability checks also pass.

This is an installation-core checkpoint, not CHOS 0.1. Accounts/passwords, timezone/
keyboard/display configuration, recovery tooling and Fusion validation remain open.
See milestone-6b.md and artifacts/milestone-6b for the current ISO and exact evidence.

---

# CHOS installation-format foundations (milestone 6A)

Original GPT/FAT32 ESP encoders and an exclusive-new-file build utility now create
repeatable boot disk fixtures. Independent parsers verify metadata and exact EFI/
kernel payloads. Actual QEMU UEFI boots the native desktop from 512/4096-byte NVMe
fixtures with no ISO; underlying disks stay unchanged. Full sequence: 14 QEMU
cases, eight portable host programs, both ELF inputs and sanitizers pass.

This is not a guest installer or installed system-volume/persistent-desktop result.
The current normal ISO remains the milestone-5 native application image. Graphical
setup, system partition integration, configuration/accounts and Fusion remain open.
See milestone-6a.md for evidence, safety limits and next engineering steps.

---

# CHOS desktop/application development sequence

Adds an original software window manager/compositor, owned native graphics/events,
blocking application waits, taskbar/launcher, clock and six functional EL0 apps.
Notepad supports safe whole-file saves; Explorer/Terminal perform real CHFS file
operations on an explicitly volatile RAM volume. Control Panel changes the actual
theme; Task Manager reads real instrumentation. Confirmed restart/shutdown uses
ACPI-discovered PSCI and is verified by QMP guest power events.

Twelve QEMU cases, six portable host programs (both ELF inputs), sanitizer checks,
ISO repeatability and independent cold storage integrity passed. See milestone-5.md.
No installer/installed boot, desktop disk persistence, Fusion validation or CHOS 0.1
release is claimed. RAM files disappear on restart/shutdown.

---

# CHOS milestones 2 core and 3 development sequence

Milestone 2 adds isolated static ELF processes, public syscalls, IPC, memory-fault
termination and safe context/page reclamation. Milestone 3 adds NVMe, original
CHFS persistent files and native file handles, then original Virtio PCI keyboard/
relative mouse queues, bounded event delivery and cursor/character feedback.

Validated on QEMU ARM64 UEFI at 512 MiB/1 GiB; actual events and three fresh-VM
storage boots are checked. Both normal and specialized test ISO builds are
repeatable in the recorded toolchain. See milestone-3.md for final evidence.

Ordinary ISO: no disk writes. Specialized storage ISO: explicit boot opt-in plus
exact disposable-device identity. No installer, GPT/ESP, installed boot, desktop,
applications or Fusion compatibility claim. CHOS 0.1 is not released.

---

# CHOS development milestone 2A

Adds original exception vectors and fault reporting, bitmap physical allocator,
64 KiB heap, kernel-owned page tables with RO/NX/guard protections, bounded ACPI
parsing, GICv2 and physical timer interrupts, and preemptive priority kernel threads.
Tests exercise actual faults, context preservation, non-yielding workers, priorities
and thread exit. Four QEMU cases and two host test programs pass; details and
limitations are in milestone-2a.md. Full milestone 2 remains unfinished.

Diagnosed and corrected absolute GOT references in the flat image, live block
splitting that unmapped page-table storage, and GIC Group-1 acknowledgement.
The image now explicitly rejects unsupported GICv3 instead of guessing hardware.

User mode, ELF loading, public syscalls, IPC, process reaping and later OS features
remain outstanding. This is not CHOS 0.1 and does not establish Fusion compatibility.

---

# CHOS development milestone 1

First original ARM64 kernel and native UEFI boot image. Includes minimal UEFI ABI,
boot-volume file loading, cache synchronization, memory-map handoff and boot-services
exit, independent kernel stack, framebuffer diagnostics with an original font,
and deterministic ISO packaging with QEMU functional/error-path tests.

The first boot experiment exposed execute protection on EfiLoaderData. Switching
the kernel reservation to EfiLoaderCode corrected the fault; subsequent tests pass.

This is a development boot milestone, not CHOS 0.1. Desktop, installer, filesystem,
processes, networking and VMware Fusion compatibility are not delivered here.
See status.md for limitations, architecture.md for decisions and validation.md
for the exact acceptance results. No existing OS kernel is included.
