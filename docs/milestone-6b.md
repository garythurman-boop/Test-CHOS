# Milestone 6B: native installation core and persistent installed desktop

CHOS now performs a real graphical installation from its live ISO to a selected
NVMe virtual disk. Original code creates GPT/ESP, copies the pristine original
EFI loader/kernel, formats the CHOS system partition, and saves a computer name.
Fresh VMs boot that disk with **no ISO attached**, run the native desktop, save a
Notepad file and recover its exact contents on a second cold boot. Both 512-byte
and 4096-byte logical-sector configurations are covered.

This is the working installation **core**, not completion of every milestone-6
configuration requirement. User accounts/passwords, authentication, timezone,
keyboard layout and display-mode configuration remain unfinished and are clearly
identified in Setup. The system has no login security. Fusion remains unverified;
GICv3 and USB/xHCI are still unsupported.

## Implemented components

- UEFI captures the boot device path/ESP GUID and retains a separate pristine
  kernel payload before ExitBootServices. The original EFI executable is embedded
  as installation data. Runtime-mutated kernel globals are never copied as payload.
- Original native EL0 Setup app: welcome, license acceptance, disk identity/capacity,
  computer name, destructive warning, exact typed disk confirmation, installation
  progress, completion/error handling and restart. Open Meta-7 or CHOS Setup.
- The kernel restricts the destructive service to the trusted built-in Setup app,
  binds the token to the querying process, validates buffers/confirmation/name,
  and re-identifies NVMe serial/namespace geometry immediately before writing.
- Optical boot is mandatory for installation. Unknown boot media and a running
  NVMe boot disk cannot be installer destinations. The CD itself is not accessible
  through the NVMe target driver. Only controller 0/namespace 1 is currently offered.
- A preemptible kernel worker writes GPT/FAT32 using the tested original modules,
  initializes a bounded 4 MiB CHFS volume inside the system partition, saves the
  configuration/welcome file, flushes and verifies payload/metadata. Failures report
  an error and do not claim rollback or completion.
- Installed boot verifies the original GPT layout and firmware ESP identity before
  mounting CHFS. Live ISO sessions remain RAM backed. Installed sessions use the
  persistent system volume; About reports the saved computer name and apps label
  the actual volume type.
- The repository MIT license is preserved in `LICENSE`, included on the ISO and
  copied to `/system/license`. Setup displays a separate original setup agreement. No Microsoft code/assets are used.

## Build, testing and evidence

```
./scripts/container.sh make
./scripts/container.sh bash scripts/validate.sh
bash scripts/sanitize.sh
# Native-install acceptance alone, using fresh disposable image files:
./scripts/container.sh python3 tests/install.py --sector 512
./scripts/container.sh python3 tests/install.py --sector 4096
```

The full sequence runs **20 real QEMU boots**: 17 positive cases and three expected
rejections. Nine portable host test programs, both actual ELF payloads, ASan/UBSan,
ISO/disk repeatability checks and independent disk parsers also pass. All writable
test destinations are newly created image files under build/, never host devices.

Each native installation test boots the ISO, interacts with the actual EL0 wizard,
checks the target is unchanged before confirmation and after wrong confirmation,
then supplies the exact disk serial. After completion, two new VMs boot only NVMe.
Notepad writes/opens persistent data; an independent Python parser reads the actual
system partition, checks CHFS CRCs and verifies both computer name and file bytes.
The installed system's Setup screen refuses self-erasure and leaves the disk hash
unchanged. These are guest installations, not host-built fixture acceptance.

Recorded results/screenshots: [evidence/m6b](evidence/m6b/).
See `native-install-512.json`, `native-install-4096.json`, `validation-m6b.log`,
`sanitizers-m6b.log`, and lossless original guest captures. The current ISO/checksum
is in [artifacts/milestone-6b](../artifacts/milestone-6b/README.md).

## Source changes

New `installer/service.c`, `installer/policy.c`, `include/setup.h`, native
`applications/setup/main.c`, `tests/setup.c`, `tests/install.py`, and the setup agreement.
Updated UEFI payload/media handoff, original GPT probing, NVMe identity recheck,
process-authorized Setup syscall, desktop launcher/volume reporting, application
volume labels, build/sanitizer/regression scripts and documentation.

## Known limits and recovery

Use a disposable virtual disk only. Current geometry requires at least 128 MiB
for 512-byte sectors; use at least 512 MiB for 4 KiB sectors. The ESP uses 64/272 MiB
respectively. CHFS uses only the first 4 MiB of the remaining system partition.
All old destination partitions/files become inaccessible; free space is not
securely wiped. Only one NVMe controller/namespace is supported.

The progress percentage is a bounded write-count estimate, not elapsed time.
There is no full-install transaction or rollback. If a write/flush/verification
fails, the disk may be partial: keep/reboot the ISO, inspect the displayed error
and reinstall only that explicitly selected disposable destination. No repair or
fsck utility is provided. Closing Setup does not cancel an active kernel writer.
Wait for completion before restarting. Remove the ISO before restarting into disk.

Mounting is limited to CHOS's verified layout/ESP identity. An unformatted builder
fixture or unmountable system volume falls back to an explicitly labeled live RAM
session; do not infer persistence unless About says installed/system disk. Detailed
volume-recovery diagnostics remain to be improved. No third-party GPT/FAT runtime
mounting, arbitrary application installation or account isolation exists yet.

Next: finish configuration/authentication and recovery diagnostics; implement and
validate the remaining target hardware support, then run the dedicated Fusion
plan on an actual Apple Silicon/Fusion host. These results do not establish Fusion
compatibility or constitute a CHOS 0.1 release.

Installer error codes: 1 identity/geometry changed, 2 GPT operation, 3 ESP payload,
4 CHFS/configuration, 5 final verification. Native injected-NVMe-failure UI tests
remain open; portable GPT/FAT32 write/flush failure models are covered. Hardware
hotplug/reset recovery is not supported.
