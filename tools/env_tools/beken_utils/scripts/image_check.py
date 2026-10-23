#!/usr/bin/env python3
"""Re-read the packed BK7259 artifacts and re-check their structure and CRCs.

Lightweight self-check: it never touches signing keys and never decrypts, so it
finishes in well under a second on any host. It catches things the packer can
only see in memory: container CRC, sub-image checksums, sub-image offsets, the
XIP boot_param record, and the bootloader layout.

The CRC conventions MUST stay identical to the producer in scripts/partition.py:
  * download container (all-app.bin / ota.bin): Partitions.crc32(0xffffffff, buf)
    is a reflected table CRC with init 0xFFFFFFFF and NO final xor, which equals
    zlib.crc32(buf) ^ 0xFFFFFFFF  <- note the complement, see crc_beken().
  * boot_param record (partition.py process_boot_param): plain zlib.crc32(head).

Flash CRC interleaving (security.csv: crc_en=TRUE) inserts a 2-byte big-endian
crc16 after every 32 bytes of a partition buffer. Because the extra bytes and
the 0xFF padding between partitions shift every later offset, the interleave can
only be sampled where the layout is known; the sampler below is therefore
reported as advisory INFO, not as a hard gate.
"""

import argparse
import os
import struct
import sys
import zlib

from .common import *

GLOBAL_HDR_LEN = 32
IMG_HDR_LEN = 32

ALL_APP_MAGIC = b'BKDLV10.'      # big-endian download container
OTA_MAGIC = b'BK723658'          # little-endian OTA container

MANIFEST_MAGIC = 0xA1BC2FD8

# BootROM/BL2 signature-policy selector magic, plaintext at flash 0x100.
SECBOOT_MAGIC_OFFSET = 0x100
SECBOOT_MAGIC_REQUIRE_SIG = b'BK.SB\n'
SECBOOT_MAGIC_HASH_ONLY = b'BEKEN\n'

FLASH_SECTOR_SZ = 0x1000

# XIP boot_param (ab_flag_record_t) record.
AB_FLAG_MAGIC = 0x31464241
AB_FLAG_STRUCT_VER = 1
AB_FLAG_RECORD_SIZE = 32
AB_FLAG_HEAD_LEN = AB_FLAG_RECORD_SIZE - 4

GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
RESET = '\033[0m'


class Reporter:
    """Collects check results and prints a readable summary."""

    def __init__(self):
        self.failed = 0
        self.passed = 0

    def section(self, title):
        print(f'\n=== {title} ===')

    def check(self, ok, desc, detail=''):
        tag = f'{GREEN}PASS{RESET}' if ok else f'{RED}FAIL{RESET}'
        line = f'  [{tag}] {desc}'
        if detail:
            line += f' -> {detail}'
        print(line)
        if ok:
            self.passed += 1
        else:
            self.failed += 1

    def info(self, desc, detail=''):
        line = f'  [{YELLOW}INFO{RESET}] {desc}'
        if detail:
            line += f' -> {detail}'
        print(line)


def crc_beken(buf):
    """Container CRC32, identical to Partitions.crc32(0xffffffff, buf)."""
    return zlib.crc32(buf) ^ 0xFFFFFFFF


def read_file(path, rep, required=True):
    if not os.path.isfile(path):
        if required:
            rep.check(False, f'{path} exists', 'file not found')
        else:
            rep.info(f'{path} not present, skipped')
        return None
    with open(path, 'rb') as f:
        return f.read()


def crc_en_units_ok(data, count=8):
    """Count how many of the first `count` 34-byte units carry a valid trailer.

    Advisory only: a partition buffer starts with a 0xFF header pad and file
    level 0xFF padding sits between partitions, so a whole-file stride check is
    not sound.
    """
    unit_len = CRC_PAKEIT + CRC_SIZE
    max_units = min(count, len(data) // unit_len)
    ok = 0
    for i in range(max_units):
        unit = data[i * unit_len:(i + 1) * unit_len]
        stored = struct.unpack('>H', unit[CRC_PAKEIT:])[0]
        if stored == crc16(unit, 0, CRC_PAKEIT):
            ok += 1
    return ok, max_units


def check_boot_param(seg, rep, label):
    """Check the ABF1 ping-pong record pre-provisioned into boot_param."""
    magic, struct_ver, size, seq, exec_slot, upd_slot, bstate, dl_state, try_max, _, _ = \
        struct.unpack('<IHHIBBBBB3s8s', seg[:AB_FLAG_HEAD_LEN])
    rep.check(magic == AB_FLAG_MAGIC, f'{label} boot_param magic', f'0x{magic:08x}')
    rep.check(struct_ver == AB_FLAG_STRUCT_VER, f'{label} boot_param struct_ver', f'{struct_ver}')
    rep.check(size == AB_FLAG_RECORD_SIZE, f'{label} boot_param record size', f'{size}')
    stored = struct.unpack('<I', seg[AB_FLAG_HEAD_LEN:AB_FLAG_RECORD_SIZE])[0]
    calc = zlib.crc32(seg[:AB_FLAG_HEAD_LEN]) & 0xFFFFFFFF
    rep.check(stored == calc, f'{label} boot_param crc32',
              f'stored 0x{stored:08x} calc 0x{calc:08x}')
    rep.info(f'{label} boot_param state',
             f'seq={seq} exec_slot={exec_slot} update_slot={upd_slot} '
             f'boot_state={bstate} dl_state={dl_state} try_max={try_max}')


def check_all_app(path, rep, crc_en):
    rep.section(f'all-app container: {path}')
    data = read_file(path, rep)
    if data is None:
        return

    if len(data) < GLOBAL_HDR_LEN or data[:8] != ALL_APP_MAGIC:
        rep.check(False, 'container magic', f'{data[:8]!r} != {ALL_APP_MAGIC!r}')
        return

    rep.check(True, 'container magic', ALL_APP_MAGIC.decode())
    crc_field = struct.unpack('>I', data[8:12])[0]
    version, hdr_len, img_num, _, _, _ = struct.unpack('>IHHIII', data[12:GLOBAL_HDR_LEN])
    rep.check(hdr_len == GLOBAL_HDR_LEN, 'global header hdr_len', f'{hdr_len}')
    rep.check(img_num > 0, 'sub-image count', f'{img_num}')
    rep.info('container version', f'{version}')

    if hdr_len != GLOBAL_HDR_LEN or img_num <= 0 \
            or GLOBAL_HDR_LEN + IMG_HDR_LEN * img_num > len(data):
        rep.check(False, 'global header is usable', 'hdr_len/img_num out of range')
        return

    body = data[12:GLOBAL_HDR_LEN] + b''.join(
        data[GLOBAL_HDR_LEN + IMG_HDR_LEN * i:GLOBAL_HDR_LEN + IMG_HDR_LEN * (i + 1)]
        for i in range(img_num))
    calc = crc_beken(body)
    rep.check(calc == crc_field, 'global header crc32',
              f'stored 0x{crc_field:08x} calc 0x{calc:08x}')

    expect_off = GLOBAL_HDR_LEN + IMG_HDR_LEN * img_num
    has_ff_filler = False
    has_boot_param = False
    for i in range(img_num):
        po, ps, fsa, io, il, cks, ver, typ, _ = struct.unpack(
            '>IIIIIIIHH', data[GLOBAL_HDR_LEN + IMG_HDR_LEN * i:GLOBAL_HDR_LEN + IMG_HDR_LEN * (i + 1)])
        rep.check(io == expect_off, f'sub-image[{i}] img_offset',
                  f'0x{io:x} (expected 0x{expect_off:x})')
        rep.check(io + il <= len(data), f'sub-image[{i}] fits in file',
                  f'partition_offset=0x{po:x} partition_size=0x{ps:x} img_len=0x{il:x} type={typ} version={ver}')
        if typ == 0:
            rep.check(fsa == po, f'sub-image[{i}] flash_start_addr',
                      f'0x{fsa:x} (partition_offset 0x{po:x})')

        seg = data[io:io + il]
        calc = crc_beken(seg)
        rep.check(calc == cks, f'sub-image[{i}] checksum',
                  f'stored 0x{cks:08x} calc 0x{calc:08x}')

        if seg[:4] == b'\xff\xff\xff\xff':
            has_ff_filler = True
        if seg[:4] == b'ABF1':
            has_boot_param = True
            check_boot_param(seg, rep, f'sub-image[{i}]')

        expect_off = io + il

    rep.check(has_ff_filler, '0xFF pre-provisioned data partition present')
    if not has_boot_param:
        # Detection is magic driven: without partitions.csv the checker cannot
        # tell boot_param (XIP) from ota_control (OVERWRITE), so a missing
        # record is reported instead of failed.
        rep.info('no ABF1 boot_param record found',
                 'expected for XIP; OVERWRITE packs ota_control instead')

    if crc_en:
        ok, total = crc_en_units_ok(data)
        rep.info('crc_en interleave sample (advisory)',
                 f'{ok}/{total} leading 34-byte units carry a valid crc16 trailer')


def check_ota(path, rep, crc_en):
    rep.section(f'ota container: {path}')
    if path is None:
        rep.info('ota.bin was not produced by this configuration, skipped')
        return
    data = read_file(path, rep, required=False)
    if data is None:
        return

    if len(data) < GLOBAL_HDR_LEN or data[:8] != OTA_MAGIC:
        rep.check(False, 'container magic', f'{data[:8]!r} != {OTA_MAGIC!r}')
        return

    rep.check(True, 'container magic', OTA_MAGIC.decode())
    crc_field = struct.unpack('<I', data[8:12])[0]
    sec_counter, hdr_len, img_num, _, _, _ = struct.unpack('<IHHIII', data[12:GLOBAL_HDR_LEN])
    rep.check(hdr_len == GLOBAL_HDR_LEN, 'global header hdr_len', f'{hdr_len}')
    rep.check(img_num > 0, 'sub-image count', f'{img_num}')
    rep.info('global security counter', f'{sec_counter}')

    if hdr_len != GLOBAL_HDR_LEN or img_num <= 0 \
            or GLOBAL_HDR_LEN + IMG_HDR_LEN * img_num > len(data):
        rep.check(False, 'global header is usable', 'hdr_len/img_num out of range')
        return

    body = data[12:GLOBAL_HDR_LEN] + b''.join(
        data[GLOBAL_HDR_LEN + IMG_HDR_LEN * i:GLOBAL_HDR_LEN + IMG_HDR_LEN * (i + 1)]
        for i in range(img_num))
    calc = crc_beken(body)
    rep.check(calc == crc_field, 'global header crc32',
              f'stored 0x{crc_field:08x} calc 0x{calc:08x}')

    payload_start = GLOBAL_HDR_LEN + IMG_HDR_LEN * img_num
    for i in range(img_num):
        il, io, fo, cks, sc, flg, _, _ = struct.unpack(
            '<IIIIIIII', data[GLOBAL_HDR_LEN + IMG_HDR_LEN * i:GLOBAL_HDR_LEN + IMG_HDR_LEN * (i + 1)])
        rep.check(payload_start + il <= len(data), f'sub-image[{i}] fits in file',
                  f'img_len=0x{il:x} img_offset=0x{io:x} flash_offset=0x{fo:x} '
                  f'security_counter={sc} flags={flg}')
        if img_num == 1:
            rep.check(il == len(data) - payload_start, f'sub-image[{i}] img_len matches file size',
                      f'0x{il:x} (file {len(data)} - header {payload_start})')

        seg = data[payload_start:payload_start + il]
        calc = crc_beken(seg)
        rep.check(calc == cks, f'sub-image[{i}] checksum',
                  f'stored 0x{cks:08x} calc 0x{calc:08x}')

    rep.info('payload is checked as stored',
             'XIP encrypts the OTA payload with the flash AES key, so only the '
             'ciphertext length and checksum can be verified without the key')

    if crc_en:
        ok, total = crc_en_units_ok(data[payload_start:])
        rep.info('crc_en interleave sample (advisory)',
                 f'{ok}/{total} leading 34-byte units carry a valid crc16 trailer')


def check_bootloader(path, rep, crc_en):
    rep.section(f'bootloader image: {path}')
    data = read_file(path, rep)
    if data is None:
        return

    rep.check(len(data) > 0, 'image is not empty', f'{len(data)} bytes')
    rep.check(len(data) % FLASH_SECTOR_SZ == 0, 'size is sector aligned',
              f'0x{len(data):x}')

    manifest_off = data.find(struct.pack('>I', MANIFEST_MAGIC))
    if manifest_off < 0:
        manifest_off = data.find(struct.pack('<I', MANIFEST_MAGIC))
    rep.check(manifest_off >= 0, 'manifest magic present',
              f'0x{MANIFEST_MAGIC:08x}' + (f' @0x{manifest_off:x}' if manifest_off >= 0 else ''))

    if crc_en:
        rep.info('crc_en enabled: the policy magic offset is shifted by the '
                 'interleaving, 0x100 check skipped')
        ok, total = crc_en_units_ok(data)
        rep.info('crc_en interleave sample (advisory)',
                 f'{ok}/{total} leading 34-byte units carry a valid crc16 trailer')
    else:
        magic = data[SECBOOT_MAGIC_OFFSET:SECBOOT_MAGIC_OFFSET + 6]
        rep.check(magic in (SECBOOT_MAGIC_REQUIRE_SIG, SECBOOT_MAGIC_HASH_ONLY),
                  f'BootROM policy magic @0x{SECBOOT_MAGIC_OFFSET:x}', f'{magic!r}')
        if magic == SECBOOT_MAGIC_REQUIRE_SIG:
            rep.info('signature policy', 'BK.SB: signature required')
        elif magic == SECBOOT_MAGIC_HASH_ONLY:
            rep.info('signature policy', 'BEKEN: hash only, no signature')


def check_images(all_app, ota, bootloader, rep, crc_en=None):
    """Check the produced artifacts; returns True when no check failed."""
    check_all_app(all_app, rep, crc_en)
    check_ota(ota, rep, crc_en)
    check_bootloader(bootloader, rep, crc_en)
    return rep.failed == 0


def build_arg_parser():
    p = argparse.ArgumentParser(
        description='Check packed BK7259 artifacts (container structure + CRCs).')
    p.add_argument('--all-app', dest='all_app', default='all-app.bin',
                   help='download container to check (default: all-app.bin)')
    p.add_argument('--ota', dest='ota', default='ota.bin',
                   help='OTA container to check (default: ota.bin)')
    p.add_argument('--bootloader', dest='bootloader', default='bootloader.bin',
                   help='bootloader image to check (default: bootloader.bin)')
    p.add_argument('--crc_en', '--crc-en', dest='crc_en', action='store_true',
                   help='flash CRC interleaving is enabled (security.csv: crc_en)')
    return p


def main(argv=None):
    args = build_arg_parser().parse_args(argv)
    rep = Reporter()
    check_images(args.all_app, args.ota, args.bootloader, rep, crc_en=args.crc_en)
    print(f'\nResult: {rep.passed} passed, {rep.failed} failed')
    return 1 if rep.failed else 0


if __name__ == '__main__':
    sys.exit(main())
