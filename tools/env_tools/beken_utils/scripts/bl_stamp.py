#!/usr/bin/env python3
"""Single definition of the bootloader version stamp read back by the application.

16-byte record, little-endian:
    magic    4  see BL2_VERSION_STAMP_MAGIC / BL2_INTERNAL_VERSION_STAMP_MAGIC
    version  4  major << 16 | minor << 8 | patch
    crc32    4  zlib CRC32 over magic + version
    reserved 4  0

Written by both the secure signing flow (BL2 manifest slot) and the non-secure
packager (aboot vector table slot); a dependency-free module importable as both
a package submodule and a top-level module through sys.path.
"""
import logging
import struct
import zlib

# Customer version, from ota.csv (bootloader_ota_version).
BL2_VERSION_STAMP_MAGIC = b'BL2V'
# Internal SDK version, from RECORD_BL2_INTERNAL_VERSION. Same layout, distinct
# magic since both share the manifest slot tail.
BL2_INTERNAL_VERSION_STAMP_MAGIC = b'BL2I'
BL2_VERSION_STAMP_SIZE = 16


def encode_version(version_str):
    """Encode "x.y.z" into major<<16 | minor<<8 | patch.

    The 4-byte stamp field carries three 8-bit fields, so every field is
    range-checked against 0..255 instead of silently wrapping on overflow.
    """
    if not version_str or not version_str.strip():
        logging.error('missing bootloader_version, expect x.y.z')
        exit(1)
    parts = version_str.strip().lstrip('vV').split('.')
    values = []
    for i, name in enumerate(('major', 'minor', 'patch')):
        # Missing trailing fields default to 0; extra fields are ignored.
        text = parts[i] if i < len(parts) else '0'
        try:
            value = int(text)
        except ValueError:
            logging.error(f'invalid bootloader_version "{version_str}", expect x.y.z')
            exit(1)
        if not 0 <= value <= 0xFF:
            logging.error(f'invalid bootloader_version "{version_str}": '
                          f'{name}={value} out of range (0-255)')
            exit(1)
        values.append(value)
    return (values[0] << 16) | (values[1] << 8) | values[2]


def crc32_zlib(buf, crc=0):
    """zlib CRC32 (poly 0xEDB88320, init 0xFFFFFFFF, final xor)."""
    return zlib.crc32(buf, crc) & 0xFFFFFFFF


def build_stamp(version_str, magic=BL2_VERSION_STAMP_MAGIC):
    """Build the 16-byte stamp record for version "x.y.z"."""
    version = struct.pack('<I', encode_version(version_str))
    crc = struct.pack('<I', crc32_zlib(magic + version))
    return magic + version + crc + struct.pack('<I', 0)


def is_stamp(record, magic=BL2_VERSION_STAMP_MAGIC):
    """True when the 16 bytes hold a well formed stamp, whatever its version."""
    if len(record) != BL2_VERSION_STAMP_SIZE or record[:4] != magic:
        return False
    crc = struct.unpack('<I', record[8:12])[0]
    return crc == crc32_zlib(record[:8])


def verify_stamp_slot(bin_path, offset, magic=BL2_VERSION_STAMP_MAGIC):
    """Check that offset really is the reserved slot before writing there.

    The slot is blank (zeros or 0xFF) before the first stamp, and already holds
    a valid stamp when a rebuild reuses the image. Anything else means the
    offset and the image disagree, so fail instead of overwriting live data.
    """
    with open(bin_path, 'rb') as f:
        f.seek(offset)
        slot = f.read(BL2_VERSION_STAMP_SIZE)
    # A file shorter than the slot gets 0xFF padded by write_stamp.
    slot = slot.ljust(BL2_VERSION_STAMP_SIZE, b'\xff')
    if (slot == b'\x00' * BL2_VERSION_STAMP_SIZE
            or slot == b'\xff' * BL2_VERSION_STAMP_SIZE
            or is_stamp(slot, magic)):
        return
    logging.error(f'no version stamp slot at 0x{offset:x} of {bin_path}, '
                  f'found {slot!r}; the offset and the image disagree')
    exit(1)


def write_stamp(bin_path, offset, version_str, magic=BL2_VERSION_STAMP_MAGIC):
    """Splice the stamp into bin_path at offset, growing the file if needed."""
    stamp = build_stamp(version_str, magic)
    verify_stamp_slot(bin_path, offset, magic)
    end = offset + len(stamp)
    with open(bin_path, 'rb') as f:
        data = f.read()
    if len(data) < end:
        data = data.ljust(end, b'\xff')
    data = data[:offset] + stamp + data[end:]
    with open(bin_path, 'wb') as f:
        f.write(data)
