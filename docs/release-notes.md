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
