# Actual Fusion M7E observations supplied for M7F

Source: the user's three native Fusion diagnostic screenshots in this conversation,
plus the M7F request reporting that the normal M7E image reaches Setup. The screenshots
were visually transcribed; their original image files remain in the conversation,
not this repository. This is target evidence, not an automated scan from the cloud
workspace. No image has been generated or altered to represent a Fusion test.

Host previously reported: M1 MacBook Air, macOS 27 “Golden Gate,” Fusion “26H1u1,”
one virtual CPU, 2 GiB. Exact Fusion build remains unspecified.

| Field | Observed value |
|---|---|
| Firmware | EDK II; firmware revision `0x10000`, UEFI revision `0x20046` |
| Firmware/kernel EL | 1 / 1 |
| ACPI | Checksums PASS; 6 tables; mask `0xf` (MADT/GTDT/MCFG/FADT, no SPCR) |
| GOP | 1024×768, stride 1024, pixel format 1 |
| PCI inventory | 32 retained out of 43 firmware handles |
| MMU | PASS; 531/1024 table pages; 29 descriptors; highest end `0x100000000` |
| Image | File 262432 bytes; memory 491520 bytes; usable boot stack 65536 bytes |
| Protection/image checks | PASS; controlled growth zero |
| GIC/timer | GICv3; PPI 30; 31 ticks; delivery/preemption PASS |
| Core exception/EL0/IPC/protection | PASS |
| Input | 0 initialized; unsupported/absent |
| Storage | `NVME PCI CONTROLLER NOT FOUND` |
| Diagnostic mode | No installer, no filesystem writes |
| Setup | Normal ISO reaches Setup, reported by user; interaction blocked |

| Device | Actual BDF | Vendor:device | Class |
|---|---|---|---|
| UHCI | `0000:01:00.0` | `15ad:0774` | `0c0300` |
| EHCI | `0000:01:02.0` | `15ad:0770` | `0c0320` |
| xHCI | `0000:0a:00.0` | `15ad:077a` | `0c0330` |
| NVMe | `0000:12:00.0` | `15ad:07f0` | `010802` |
| Display | `0000:00:0f.0` | `15ad:0406` | `030000` |
| AHCI (unsupported) | `0000:01:03.0` | `15ad:07e0` | `010601` |

Many root-port/bridge functions appear in the retained inventory. The two supported
controllers are both beyond the original drivers' bus-zero scan. Thus the M7E
screenshots confirm a discovery defect before either driver's hardware bring-up.
They do not identify the USB attachment path, root-port state, firmware ownership,
BARs, DMA requirements, capability registers, HID descriptors or NVMe namespaces.
Those remain the purpose of the M7F input/driver report image.
