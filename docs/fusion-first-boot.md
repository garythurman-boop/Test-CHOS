# First Fusion ARM64 evaluation — no installation required

**M7E retest after a real M7D failure.** The user reports an M1 MacBook Air,
one CPU, 2 GiB RAM, macOS 27 “Golden Gate,” Fusion “26H1u1,” normal M7D ISO and
`MMU INIT FAILED`. Exact build and firmware/device map remain unknown. No target
host is attached here. Keep the **same 2 GiB RAM** for the first retest so reducing
RAM does not hide a capacity failure. UI labels vary by Fusion release.

1. Download [CHOS-ARM64-M7E-DIAGNOSTIC.iso](../artifacts/milestone-7e/CHOS-ARM64-M7E-DIAGNOSTIC.iso)
   using GitHub **Download raw file**. Compare adjacent SHA256SUMS with
   `shasum -a 256 CHOS-ARM64-M7E-DIAGNOSTIC.iso`. This is the noninteractive diagnostic image.
2. Open Fusion on an Apple Silicon Mac. Create a **new custom/other ARM64 64-bit
   UEFI VM** using that release's supported workflow. Select an ARM64 guest, not
   an x86 compatibility type. If custom optical boot is unavailable, record the
   release/UI as a blocker. Do not claim another guest type provides compatibility.
3. Configure **one virtual CPU and 2 GiB RAM**. Disable **Secure Boot** for the
   unsigned CHOS EFI executable. No guest network is needed. Use default virtual
   display/input settings initially and record their exact configuration.
4. Attach the diagnostic ISO to connected CD/DVD media, connected at power-on.
   **No virtual disk is required for this first test.** If the creation wizard
   insists on one, use a new disposable disk and record its controller; never
   attach a valuable disk. For later installer testing, prefer a fresh NVMe disk
   at least 1 GiB if that controller is actually selectable on the target release.
   CHOS does not yet support SCSI/SATA installation or USB mass storage.
5. Cold start the VM and select optical boot in UEFI if needed. Expect the original
   CHOS ARM64 loader, core exception/memory/GIC/timer tests, then **CHOS HARDWARE
   DIAGNOSTICS** with pages rotating automatically every five seconds. Keyboard
   and mouse input are not required. This image stays in diagnostics; it never
   enters the installer or automatically exports to disk.
6. Capture each automatic page with Fusion's screenshot facility or macOS
   Shift-Command-4. Record a short video if pages change too quickly. Where the VM/
   release supports serial capture, retain firmware output and CHOS's native
   ACPI-SPCR PL011 log. A serial port without a supported firmware description may
   produce no native serial output; the framebuffer remains the primary capture.
7. Capture failures exactly: UEFI loader errors for unsupported firmware/GOP,
   CHOS red panic text for missing ACPI/GIC/timer, or diagnostic warnings for absent/
   unsupported input/storage. Record the last visible screen and how long you
   waited. A stable diagnostic screen is successful discovery, not desktop/input
   compatibility or installation. No arbitrary memory or credentials are dumped.
8. If no loader appears, confirm ARM64/UEFI, Secure Boot off, connected optical
   media and checksum. If supported, select `EFI/BOOT/BOOTAA64.EFI` manually in
   firmware; record that separately from automatic optical boot. A different
   standards-compliant GPT/FAT boot artifact may need investigation if the target
   release cannot boot EFI optical media; no validated Fusion VMDK is supplied.
9. Return the checklist below with exact Fusion/macOS/Apple SoC details and actual
   images/logs. No disk erase or installation is needed for this evaluation.

## Concise capture checklist

- Host SoC/macOS; Fusion version/build; VM CPU/RAM/firmware/Secure Boot/media settings.
- Automatic ISO detection and loader execution, or exact error/manual path.
- Firmware vendor/revision and UEFI revision; entry EL and current kernel EL.
- ACPI table count/validity and mask (bit 0 MADT, 1 GTDT, 2 MCFG, 3 FADT, 4 SPCR).
- GIC version, initialization result, timer PPI/delivered ticks and preemption result.
- Every PCI BDF, vendor:device and class/subclass/programming-interface record.
- USB controller type; initialized boot-HID devices or exact failure stage.
- Storage controllers and actual NVMe initialization result; no installation claim.
- GOP width/height/stride/pixel format and any framebuffer error.
- Missing capabilities, hangs and the final visible screen; all pages where possible.

PCI inventory retains 32 of at most 256 firmware handles. UART support is PL011
from validated ACPI SPCR, not an assumed QEMU address or arbitrary firmware call.
The bounded 16 KiB native report is also available read-only through syscall 34
in ordinary sessions; it is not a disk writer or a general memory dump.

After discovery identifies supported input/storage/display, a separate normal
[CHOS-ARM64.iso](../artifacts/milestone-7e/CHOS-ARM64.iso) contains the same kernel
and starts full-screen Setup. Use L/F3 for Live Desktop first. USB xHCI boot-HID
passes QEMU tests only; Fusion might expose other controllers/protocols. Follow
[full installation checklist](fusion-validation.md) only after those prerequisites
are validated. Keep typed `ERASE` and the exact selected disposable disk review.

## If MMU initialization fails again

Capture the **entire retained “CHOS MMU INIT FAILED / REPORT V1” screen**.
Transcribe stage and reason numbers, physical address, size/argument, descriptor
index/type/attributes, table count/capacity, PMM used/total/error/ranges, map entries,
highest end, MMFR0 and old/new translation registers. `4294967295` for descriptor
index/type means the failure is not tied to one descriptor. Stage/reason names and
[codes](mmu-architecture.md) are included on screen. A register value of zero on a
pre-MMU map rejection can mean it has not been captured yet. No serial is required.
If the screen stops at “MMU STAGE 10” or an exception screen, capture that instead;
that identifies the translation switch rather than a returned mapping error.

If the previous point is passed, capture **every rotating hardware page**, including
MMU table usage, firmware/EL/GIC/timer, GOP and all PCI/input/storage outcomes.
Record ISO checksum, RAM, Secure Boot setting, exact Fusion About version/build,
macOS version and whether optical boot was automatic or manually selected.
Do not attach a valuable disk or attempt installation. Booting diagnostics is not
input/storage/desktop/install acceptance. Preserve the original failed M7D image.
