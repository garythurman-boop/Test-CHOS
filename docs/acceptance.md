# Current acceptance evidence

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
