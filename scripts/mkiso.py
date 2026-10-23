#!/usr/bin/env python3
"""Create a deterministic FAT EFI El Torito image; never opens host block devices."""
import datetime
import fcntl
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'

def run(*args, **kwargs):
    subprocess.run(list(map(str, args)), check=True, **kwargs)

def main():
    if os.environ.get("CHOS_INPUT_TEST"):os.environ["CHOS_DIAGNOSTIC"]="1"
    BUILD.mkdir(exist_ok=True)
    lock=(BUILD/'iso-build.lock').open('w')
    fcntl.flock(lock,fcntl.LOCK_EX)
    epoch = int(os.environ.get('SOURCE_DATE_EPOCH', '1767225600'))
    stamp = datetime.datetime.fromtimestamp(epoch, datetime.timezone.utc).strftime('%Y%m%d%H%M%S00')
    stage = BUILD / 'iso-root'
    if stage.exists():
        shutil.rmtree(stage)
    (stage / 'boot').mkdir(parents=True)
    image = stage / 'boot' / 'efi.img'
    # Leave FAT/directory headroom as the versioned kernel grows. These bounded
    # volumes remain FAT12/16, matching the timestamp normalizer below.
    required=(BUILD/'BOOTAA64.EFI').stat().st_size+(BUILD/'KERNEL.BIN').stat().st_size+1048576
    volume_bytes=16*1048576
    while volume_bytes<required:volume_bytes*=2
    if volume_bytes>32*1048576:raise ValueError('EFI payload exceeds bounded FAT volume policy')
    with image.open('wb') as f:
        f.truncate(volume_bytes)
    run('mformat', '-i', image, '-v', 'CHOS_EFI', '-N', '43484f53', '::')
    run('mmd', '-i', image, '::/EFI', '::/EFI/BOOT', '::/CHOS')
    inputs = [(BUILD / 'BOOTAA64.EFI', '::/EFI/BOOT/BOOTAA64.EFI'),
              (BUILD / 'KERNEL.BIN', '::/CHOS/KERNEL.BIN')]
    if os.environ.get('CHOS_TEST_MISSING_KERNEL'):
        inputs = inputs[:1]
    inputs.append((ROOT / 'LICENSE', '::/CHOS/LICENSE.TXT'))
    inputs.append((ROOT / 'assets/fonts/OFL.txt', '::/CHOS/FONT-OFL.TXT'))
    if os.environ.get('CHOS_TEST_STORAGE'):
        config=BUILD/'storage-test.cfg'
        config.write_text('CHOS STORAGE TEST V1')
        inputs.append((config,'::/CHOS/TEST.CFG'))
    if os.environ.get('CHOS_DIAGNOSTIC'):
        if os.environ.get('CHOS_TEST_STORAGE'): raise ValueError('Diagnostic and destructive storage-test modes cannot be combined')
        config=BUILD/'diagnostic.cfg'
        config.write_text('CHOS INPUT TEST V1' if os.environ.get('CHOS_INPUT_TEST') else 'CHOS DIAGNOSTICS V1')
        inputs.append((config,'::/CHOS/DIAG.CFG'))
    if os.environ.get('CHOS_DISPLAY_MODE'):
        config=BUILD/'display.cfg'
        config.write_text(os.environ['CHOS_DISPLAY_MODE'])
        inputs.append((config,'::/CHOS/DISPLAY.CFG'))
    for source, target in inputs:
        run('mcopy', '-i', image, source, target)
    # FAT directory timestamps created by mmd/mformat are normalized separately.
    data = bytearray(image.read_bytes())
    bps = int.from_bytes(data[11:13], 'little')
    reserved = int.from_bytes(data[14:16], 'little')
    fats = data[16]
    fat_sectors = int.from_bytes(data[22:24], 'little')
    root_entries = int.from_bytes(data[17:19], 'little')
    # Our fixed tiny image is FAT12/16; scan root and all allocated data clusters.
    root_offset = (reserved + fats * fat_sectors) * bps
    root_bytes = root_entries * 32
    cluster_bytes = data[13] * bps
    date = ((max(1980, min(2107, datetime.datetime.fromtimestamp(epoch, datetime.timezone.utc).year))-1980)<<9) | (1<<5) | 1
    def directory(offset, length):
        for pos in range(offset, offset+length, 32):
            if data[pos] == 0:
                break
            if data[pos] == 0xe5 or data[pos+11] == 0x0f:
                continue
            data[pos+13] = 0
            for field in (14,22):
                data[pos+field:pos+field+2] = b'\0\0'
            for field in (16,18,24):
                data[pos+field:pos+field+2] = date.to_bytes(2,'little')
            if data[pos+11] & 0x10 and data[pos] != 46:
                cluster = int.from_bytes(data[pos+26:pos+28], 'little')
                directory(root_offset+root_bytes+(cluster-2)*cluster_bytes, cluster_bytes)
    directory(root_offset,root_bytes)
    image.write_bytes(data)
    output = BUILD / ('CHOS-ARM64-M7G-INPUT.iso' if os.environ.get('CHOS_INPUT_TEST') else 'CHOS-ARM64-M7C.iso' if os.environ.get('CHOS_DIAGNOSTIC') else 'CHOS-STORAGE-TEST.iso' if os.environ.get('CHOS_TEST_STORAGE') else 'CHOS-missing-kernel.iso' if os.environ.get('CHOS_TEST_MISSING_KERNEL') else 'CHOS-ARM64.iso')
    run('xorriso', '-as', 'mkisofs', '-quiet', '-V', 'CHOS_ARM64',
        '--modification-date='+stamp, '--set_all_file_dates', stamp,
        '-c', 'boot/boot.cat', '-e', 'boot/efi.img', '-no-emul-boot',
        '-o', output, stage)
    run('sha256sum', output)

if __name__ == '__main__':
    main()
