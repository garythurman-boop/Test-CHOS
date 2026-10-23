# CHOS acceptance evidence

Current sequence: [M6D dedicated Setup](milestone-6d-setup.md),
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
| VMware Fusion ISO/input/storage/installation/reboot | Not tested | No Apple Silicon/Fusion host; GICv3 and USB/xHCI unsupported |
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
