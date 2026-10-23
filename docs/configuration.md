# Saved system configuration (M6D)

Native Setup now saves computer name, theme, fixed clock offset and clock seconds
inside the installed CHFS volume. Control Panel edits them through shared native
widgets and saves a single atomic record. Installed sessions reload the record on
cold boot. Live sessions can apply settings to RAM and label them volatile.

`/system/config` is authoritative when present. Older M6B/M6C installations without
it read `/system/computer` with default theme/clock options. Reading a legacy volume
does not silently migrate or write it. New installations retain the legacy name
file for compatibility; later Control Panel name changes update only the canonical
record. Account/password authentication, permissions and login are not implemented.

## Disk format

Exactly 80 bytes, little-endian, explicitly encoded (not a native-struct dump):

| Offset | Bytes | Meaning |
|---|---|---|
| 0 | 8 | ASCII CHOSCFG1 |
| 8 | 4 | Record length, 80 |
| 12 | 4 | IEEE CRC32 over all bytes with this field zero |
| 16 | 4 | Configuration version, 1 |
| 20 | 4 | Theme 0 blue / 1 harbor green |
| 24 | 4 | Signed offset minutes, -720..840, multiples of 15 |
| 28 | 4 | Show seconds, 0/1 |
| 32 | 32 | NUL-terminated computer name, 1..31 ASCII letters/digits/hyphen |
| 64 | 16 | Reserved zeros |

Clock offset is relative to firmware time. It is a fixed offset, not an IANA time
zone/DST implementation; firmware time may itself have an unspecified zone. The
kernel wraps the adjusted clock at midnight. No network time or RTC write exists.

The kernel rejects invalid versions, sizes, checksums, reserved fields, names and
ranges. CRC detects accidental corruption; it is not authentication. The read-only
INFO volume-state code identifies invalid configuration. A bad record produces a
specific About message and a temporary-RAM warning. It does not format, repair,
overwrite or claim persistence on the affected disk. Keep a backup; no automatic
repair utility or installer rollback is provided.

## Apply and save

SYS_CONFIG provides validated get/apply/stage operations. Only built-in Control
Panel and Setup may apply through this service; Setup alone may stage installation
settings. Persistent apply requires a mounted system volume. Save commits a whole
CHFS replacement before changing active in-memory settings. A reported failure retains the previous active settings. An I/O failure after
writing commit metadata can leave the disk with either the old or new complete
record; rollback is not guaranteed. Reread before assuming which commit survived. Other apps still share filesystem
access: this is not an access-control boundary against malicious apps.

Setup stages its record before the independent typed `ERASE <serial>` confirmation.
Staging performs no disk writes. An invalid name can be corrected with Back. Wrong
confirmation, invalid configuration and boot-disk exclusion retain the existing
no-write behavior. The native worker copies the original boot/kernel/assets and
writes the validated record after explicitly authorized partitioning.

Tests verify atomic write failure/reopen, every single-byte record mutation,
range/clock wrap behavior, invalid-save unchanged disk hashes, both sector sizes,
shutdown/cold boot with saved configuration and user data, legacy read without
migration writes, and a configuration CRC failure inside an otherwise valid CHFS.
See [M6D results](milestone-6d.md).
