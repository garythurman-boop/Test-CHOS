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
