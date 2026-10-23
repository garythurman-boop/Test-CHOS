# CHOS storage checkpoint 3A (historical)

Current installer/persistent-system behavior is documented in
[dedicated Setup](setup-environment.md), [configuration](configuration.md), and
[feature status](status.md). The raw-test-only descriptions below record M3A,
not the current desktop/installer feature set.

Original polling NVMe, discovered through validated firmware BDF/ACPI MCFG/live
configuration, with a bus-zero compatibility fallback,
with one controller/namespace, 16-entry admin/I/O queues, 4 KiB DMA bounce buffer,
512/4096-byte sectors and NVM FLUSH. No firmware I/O after ExitBootServices.
The QEMU PCI BAR lies above 512 GiB (see evidence/m3a/*.pci.log), so the MMU now
uses four levels/48-bit virtual addresses. Private user roots clone the low L1
before inserting user tables; supervisor RAM mappings remain shared.

CHFS v1 is an original raw-volume filesystem: 4096-byte blocks, up to 1024 blocks
(4 MiB), 40 named files/directories, 59-byte absolute paths, 32 KiB per file. The
root directory is implicit. First two blocks are alternating 4096-byte metadata
roots with little-endian magic bytes COSCHFS1, version, CRC32, generation, volume
block count, 128-byte allocation bitmap and 40 80-byte entries. Each entry stores
name[60], first block, block count, size, kind and data CRC32. Reserved padding
completes the block. File writes allocate new extents, flush data, write the other
metadata root and flush again. Mount checks CRCs, bounds, paths/parents, allocation
ownership and overlap, then selects the highest valid generation. Read checks data
CRC. Uncertain writes unmount the volume until remount; no automatic repair.

Test evidence: nine QEMU cases and four host C programs pass, including three
fresh VM starts on one disposable NVMe image. The native EL0 file test creates,
writes, seeks, reads, closes, renames and lists an 8192-byte file. That file and a
9000-byte kernel-created file survive subsequent boots. An independent Python
parser verifies metadata/data CRCs and exact bytes; third boot does not change
the image. Expected disk-identity rejection and normal-ISO disk hashes remain
unchanged. Host ASan/UBSan programs cover metadata/data corruption, capacity,
remount and failures/torn writes at each file-replacement write/flush boundary.
This supports the tested QEMU persistence claim, not production storage reliability.

The normal CHOS-ARM64.iso never formats or writes disks. A separately generated
CHOS-STORAGE-TEST.iso contains exact opt-in CHOS/TEST.CFG content. Destructive
acceptance operations additionally require NVMe serial CHOS-TEST-0001 and a blank
or already-valid test volume. tests/storage.py creates fresh disposable files in
build/storage-run-*; it never attaches host block devices. Do not attach valuable
disks to test images. This mechanism is an automated-test gate, not an installer
confirmation UI. The installer remains unimplemented.

Native file handles are process-local, eight per process, numbered 3..10. They are
path-based: rename/unlink invalidates existing path handles, rather than preserving
POSIX unlinked-file semantics. No ownership/permissions, timestamps, links, sparse
files, directory rename, encryption, recovery tooling or general VFS mount table.
Device operations are serialized and bounded polling with 2 s command timeouts;
there is no hotplug, NCQ concurrency, PRP lists, MSI-X or controller error recovery.
Initial failed device setup may retain allocated queue pages until reboot.

This is a raw test volume, with no GPT, ESP, FAT driver or installed-disk boot.
Storage is not mounted in the normal diagnostic ISO. Fusion NVMe, ACPI topology and DMA coherency remain unverified. Input support
added after this checkpoint is Virtio PCI keyboard/mouse for QEMU, not Fusion USB. QEMU is the evidence source, not a
substitute for Fusion testing. Exact logs/screenshots/results: evidence/m3a/.
Reproduce: ./scripts/container.sh bash scripts/validate.sh.

Current combined sequence evidence is in milestone-3.md and evidence/m3/.

M7F adds capability/namespace validation and driver reports. Both diagnostic modes
force namespace write/flush rejection and probe only first/last 4 KiB reads; normal
explicitly confirmed installation remains separate. [Policy and stages](driver-diagnostics.md).
