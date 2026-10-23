# CHOS acceptance evidence

Current sequence: [M7D bootstrap/diagnostic hardening](milestone-7d.md),
[setup architecture](setup-environment.md). Earlier tables below retain their
checkpoint-specific evidence; each new sequence reruns the earlier core coverage.

These results are distinct. All VM results below are QEMU AArch64 UEFI, not Fusion.

| Criterion | Result | Evidence |
|---|---|---|
| ARM64 UEFI ISO generated and repeatable | Pass | evidence/m6b/validation-m6b.log; artifacts/milestone-6b/SHA256SUMS |
| ISO booted original kernel/EL0 desktop | Pass, 512 MiB and 1 GiB core coverage | evidence/m6b/boot-512.json, boot-1024.json, applications.json |
| Native graphical installer executed | Pass, 512/4096-byte NVMe geometry | evidence/m6b/native-install-512.json, native-install-4096.json |
| Guest installed GPT/ESP/CHFS/payload/configuration to disk | Pass | same native-install records; independent CRC/configuration/license parser |
| Installed disk booted without ISO and retained native file saves across a second cold boot | Pass, both geometries | same records; installed-*.serial.log and screenshots |
| Full setup configuration/accounts/password/timezone/layout/mode | Incomplete | milestone-6b.md and status.md |
| VMware Fusion ISO/input/storage/installation/reboot | Not tested | No Apple Silicon/Fusion host; actual devices unknown and USB host/HID unsupported |
| CHOS 0.1 release readiness | Not ready | status.md |

The suite covers twenty QEMU boots (seventeen positive, three expected refusals),
nine portable host programs, both original ELF payloads, sanitizers, independent
wire-format checks, repeatability and explicit disk safety gates. Builder-created
GPT fixtures remain separate from the six native-installer/installed-boot cases.

## M6C additions

The complete M6B acceptance remains required. Add ten-host-program coverage with
font bounds/coverage/alpha/clipping and display request validation. QEMU adds a
native Start-menu pointer launch and partial-paint counter case; two alternate
mode request/fallback cases; and a failed CHFS mount on a copied installed disk
that must warn and leave the entire disk unchanged. The expanded validator runs
24 VMs. See [M6C](milestone-6c.md) and [Royale](royale.md). The GUI polish/installer
configuration/Fusion milestones remain partial until their own evidence exists.

## M6D additions

Retain all M6C cases. The validator now requires twelve host programs and 29 real
VM boots. Add configuration write/flush failure boundaries, CRC/range/wrap tests,
GUI input/field/focus/capture tests, and pixel-equivalent visible shadows. Native
installation must reject invalid preferences without writing, correct them with
Back, save chosen settings and preserve user files after disk-only boots. Four
configuration VMs verify invalid-save hashes, saved preferences/shutdown/cold boot,
legacy read without migration writes and invalid-record fallback without changes.
A workspace VM must demonstrate real simultaneous/overlapping native apps, menus,
dialogs, file selection/scrolling, minimize/maximize/restore, bounded input checked
through a second process, Task Manager and Setup inventory without disk writes.
See [M6D](milestone-6d.md). Full Fusion and account/authentication acceptance remains open.

## M6D dedicated Setup acceptance

The expanded validator requires 35 QEMU cases and 13 portable host programs,
both real ELF payloads, repeatable ISO/disk generation and independent disk parsers.
The six native installation/disk-only boots retain both 512/4096-byte-sector
coverage and now require direct full-screen ISO Setup, blue disk/confirmation
screens, actual graphical progress, persisted settings, verified payloads/log and
completion Restart producing QMP guest-reset. Ordinary desktop tests explicitly
select L at the Setup welcome page; installed tests have no optical media attached.

Five additional disposable-disk VMs require an actual blkdebug/NVMe write error
and safe Live exit; an active-writer interruption; read-only inventory and explicit
retry of that disk; installed boot from a 70 MiB target; and a too-small 64 MiB
refusal plus honest recovery-status page with unchanged disk bytes. One additional
640×480 VM exercises native configuration/confirmation and safe Live exit without
writes. The workspace case arranges Explorer, Notepad and Task Manager together
and verifies full-screen Setup can return to those existing apps. Current results
and actual captures are recorded in milestone-6d-setup.md and evidence/m6d-setup.
Fusion and authentication/account acceptance remain open.

## M7A/7B additions

Preserve all 35 prior cases. Replace the former GICv3 refusal with ACPI-off refusal.
Require three additional real UEFI boots: EL1/GICv3, EL2/GICv2, EL2/GICv3, each
with exception/MMU/protection/scheduler/IPC/input checks and observed firmware EL.
Require six additional native install/disk-only/reopen boots using EL2/GICv3 at
512/4096-byte sectors, preserving explicit ERASE, payload verification, Restart
and saved settings/files. Add native hardware query/QMP PCI cross-check and an
unchanged unconfirmed-disk hash. Total: 45 QEMU cases, 14 host programs including
portable GICv3 resources/affinity/routing/mask/timeout tests, both ELF validators
and ASan/UBSan. Require clean tracked-source ISO reproduction. Actual Fusion
ISO/input/storage/install/reboot criteria remain pending separately; this is an
intermediate checkpoint, not Milestone 7 completion or CHOS 0.1.

## M7C additions

Retain all 45 earlier cases. Add five QEMU VMs: input-free diagnostic inventory
with USB/unsupported storage and unchanged disk; USB-only native input/edit/save/
reopen/drag, transfer/event ring wrap, compact menu and disconnect; and three
unsupported xHCI/OHCI/non-boot-device cases that must finish diagnostics. Require
16 host programs including sanitized diagnostic export and USB descriptor/HID/
ring/DMA bounds, both ELF validators, two reproducible ISO variants and a clean
tracked-source rebuild. Native USB interrupts are unimplemented and have no pass
claim; existing GIC/timer interrupts remain a separate tested path. Actual Fusion
boot/input/storage/install/reboot acceptance remains pending.

## M7D additions

Retain the earlier 50 cases. Add three no-disk/USB/mixed-loader cases and eight
controlled large-image cases (two core boots, native install plus two disk-only
boots, three malformed-header refusals): 61 real QEMU instances. Require 17 host
programs, sanitized image/region validation, both real ELF payloads, repeatability
and clean tracked-source reproduction. Test far growth pages with RO/NX faults,
zeroed large BSS, both dynamic bootstrap guards and installed payload integrity.
The no-disk case must show diagnostics despite actual optional-driver initialization
failure. The mixed preserved-loader case must show an explicit boot version error.
Actual Fusion remains a separate pending gate; installation there is not authorized
by a QEMU pass alone and must follow prerequisite validation.
