#!/usr/bin/env python3
"""Verify a secure-boot firmware set (bootloader.bin + all-app.bin).

The tool re-checks, offline, that a delivered firmware was produced with a
known key set:

  bootloader.bin
    - BootROM secure-boot magic present.
    - Manifest signature verified with the root public key.
    - Manifest-embedded public key matches the provided root public key.
    - BL2 image digest in the manifest matches the (decrypted) BL2 image.

  all-app.bin
    - Application image signature verified with the root public key.
    - Application-embedded public key matches the provided root public key.
    - Image hash consistent with its signed payload.

When no paths are given the tool uses the keys under config/ and the firmware
under output_dir/ by default.

Flash encryption can be turned off at build time, so encryption is a separate
switch (--encrypted / --plaintext) instead of being auto-assumed.

Flash layout of this platform:
  - Application/boot code is stored with hardware CRC interleaving: every 32
    data bytes are followed by a 2-byte CRC (34-byte physical unit). The tool
    strips the CRC to recover the virtual (CPU-visible) image before checking.
  - Flash encryption is AES-128-XTS with a 32-byte key (k1||k2). Each 32-byte
    virtual data unit is byte-swapped per 32-bit word, XTS-processed with the
    unit's virtual byte address as the tweak, and byte-swapped back.
"""

import os
import sys
import json
import struct
import hashlib
import argparse

from cryptography.hazmat.primitives.serialization import (
    load_pem_public_key, Encoding, PublicFormat)
from cryptography.hazmat.primitives.asymmetric import ec, utils
from cryptography.hazmat.primitives.hashes import SHA256
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.exceptions import InvalidSignature

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_CONFIG_DIR = os.path.join(SCRIPT_DIR, 'config')
DEFAULT_OUTPUT_DIR = os.path.join(SCRIPT_DIR, 'output_dir')

DEFAULT_ROOT_PUBKEY = os.path.join(DEFAULT_CONFIG_DIR, 'key', 'root_ec256_pubkey.pem')
DEFAULT_FLASH_KEY = os.path.join(DEFAULT_CONFIG_DIR, 'flash_aes_key.txt')
DEFAULT_BOOTLOADER = os.path.join(DEFAULT_OUTPUT_DIR, 'bootloader.bin')
DEFAULT_ALL_APP = os.path.join(DEFAULT_OUTPUT_DIR, 'all-app.bin')
DEFAULT_OTP = os.path.join(DEFAULT_OUTPUT_DIR, 'otp_efuse_config.json')

# Flash XIP base: virtual code addresses are (flash_base + virtual_offset).
FLASH_BASE_ADDR = 0x02000000

# CRC interleaving: 32 data bytes + 2 CRC bytes per physical unit.
CRC_DATA_SZ = 32
CRC_UNIT_SZ = 34

# XTS data unit size (bytes).
XTS_UNIT_SZ = 32

# Download container ("all-app.bin") header sizes.
GLOBAL_HDR_LEN = 32
IMG_HDR_LEN = 32
DOWNLOAD_MAGIC = b'BKDLV10.'

# BootROM secure-boot magic, written in plaintext into the first code unit and
# ending up (after CRC interleaving) at physical offset 0x110.
SECBOOT_MAGIC_OFFSET = 0x110
SECBOOT_MAGIC = b'BK7236'

# Manifest layout.
MANIFEST_MAGIC = 0xA1BC2FD8
MANIFEST_SIG_LEN = 64          # r || s
MANIFEST_PUBKEY_LEN = 65       # 0x04 || X || Y (uncompressed EC-P256 point)

# MCUboot image constants.
IMAGE_MAGIC = 0x96F3B83D
IMAGE_MAGIC_LE = struct.pack('<I', IMAGE_MAGIC)
TLV_INFO_MAGIC = 0x6907
TLV_PROT_INFO_MAGIC = 0x6908
TLV_INFO_SIZE = 4
TLV_HDR_SIZE = 4
TLV_SHA256 = 0x10
TLV_PUBKEY = 0x02
TLV_ECDSA256 = 0x22

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


# ---------------------------------------------------------------------------
# Flash address mapping and CRC interleaving
# ---------------------------------------------------------------------------
def virtual2phy(addr):
    """Virtual (CPU) offset -> physical (flash) offset with CRC interleaving."""
    return (addr % CRC_DATA_SZ) + ((addr // CRC_DATA_SZ) * CRC_UNIT_SZ)


def phy2virtual(addr):
    """Physical (flash) offset -> virtual (CPU) offset with CRC interleaving."""
    return (addr % CRC_UNIT_SZ) + ((addr // CRC_UNIT_SZ) * CRC_DATA_SZ)


def crc_phys_size(virtual_size):
    """Physical byte count needed to hold `virtual_size` interleaved bytes."""
    return ((virtual_size + CRC_DATA_SZ - 1) // CRC_DATA_SZ) * CRC_UNIT_SZ


def ceil_align(value, alignment):
    return ((value + alignment - 1) // alignment) * alignment


def crc16_beken(data):
    """CRC16 used by the flash interleaving (poly 0x8005)."""
    crc = 0xFFFFFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x8005
            else:
                crc = crc << 1
    return crc & 0xFFFF


def find_crc_offset(blob):
    """Return the offset at which the 34-byte CRC interleaving is aligned."""
    for off in range(CRC_UNIT_SZ):
        units = (len(blob) - off) // CRC_UNIT_SZ
        if units < 4:
            break
        if all(crc16_beken(blob[off + i * CRC_UNIT_SZ: off + i * CRC_UNIT_SZ + CRC_DATA_SZ])
               == int.from_bytes(blob[off + i * CRC_UNIT_SZ + CRC_DATA_SZ:
                                      off + i * CRC_UNIT_SZ + CRC_UNIT_SZ], 'big')
               for i in range(4)):
            return off
    return None


def deinterleave(phys):
    """Strip the 2-byte CRC from every 34-byte unit, returning virtual bytes."""
    out = bytearray()
    full = len(phys) // CRC_UNIT_SZ
    for i in range(full):
        out += phys[i * CRC_UNIT_SZ: i * CRC_UNIT_SZ + CRC_DATA_SZ]
    rem = len(phys) - full * CRC_UNIT_SZ
    if rem:
        out += phys[full * CRC_UNIT_SZ: full * CRC_UNIT_SZ + min(rem, CRC_DATA_SZ)]
    return bytes(out)


# ---------------------------------------------------------------------------
# Flash AES-128-XTS (platform-specific byte ordering)
# ---------------------------------------------------------------------------
def _word_reverse(data):
    """Reverse the byte order inside every 32-bit word."""
    out = bytearray(len(data))
    for i in range(0, len(data) - len(data) % 4, 4):
        out[i:i + 4] = data[i:i + 4][::-1]
    return bytes(out)


def _xor(a, b):
    return bytes(x ^ y for x, y in zip(a, b))


def _mul_alpha_be(tweak):
    """Multiply the 16-byte tweak by the XTS primitive element (big-endian)."""
    t = bytearray(tweak)
    carry = 0
    for i in range(15, -1, -1):
        nb = ((t[i] << 1) & 0xFF) | carry
        carry = (t[i] >> 7) & 1
        t[i] = nb
    if carry:
        t[15] ^= 0x87
    return bytes(t)


def _aes_ecb(key, data, encrypt):
    cipher = Cipher(algorithms.AES(key), modes.ECB(), backend=default_backend())
    op = cipher.encryptor() if encrypt else cipher.decryptor()
    return op.update(data) + op.finalize()


def xts_decrypt(data, key, start_vaddr):
    """Decrypt CRC-stripped flash data with the platform's AES-128-XTS scheme.

    `key` is the 32-byte flash key (k1 || k2); `start_vaddr` is the virtual
    byte address of the first data unit.
    """
    k1, k2 = key[:16], key[16:]
    if len(data) % XTS_UNIT_SZ:
        data = data + b'\xff' * (XTS_UNIT_SZ - len(data) % XTS_UNIT_SZ)
    out = bytearray()
    for i in range(0, len(data), XTS_UNIT_SZ):
        tweak = _aes_ecb(k2, (start_vaddr + i).to_bytes(16, 'big'), True)
        c0 = _word_reverse(data[i:i + 16])
        out += _word_reverse(_xor(_aes_ecb(k1, _xor(c0, tweak), False), tweak))
        tweak1 = _mul_alpha_be(tweak)
        c1 = _word_reverse(data[i + 16:i + 32])
        out += _word_reverse(_xor(_aes_ecb(k1, _xor(c1, tweak1), False), tweak1))
    return bytes(out)


def load_flash_key(source):
    """Return the 32-byte flash key from a file path or a hex string."""
    if os.path.isfile(source):
        with open(source, 'r') as f:
            key_hex = f.read().strip()
    else:
        key_hex = source.strip()

    key_hex = ''.join(key_hex.split())
    if len(key_hex) != 64:
        raise ValueError(
            f'invalid flash AES key length: {len(key_hex)} hex chars (expected 64)')
    return bytes.fromhex(key_hex)


# ---------------------------------------------------------------------------
# Public key / signature helpers
# ---------------------------------------------------------------------------
def load_root_pubkey(path):
    with open(path, 'rb') as f:
        return load_pem_public_key(f.read())


def pubkey_uncompressed(pubkey):
    return pubkey.public_bytes(Encoding.X962, PublicFormat.UncompressedPoint)


def pubkey_der(pubkey):
    return pubkey.public_bytes(Encoding.DER, PublicFormat.SubjectPublicKeyInfo)


def reverse_words(data):
    """Byte-swap every 4-byte word (matches the packer's ROTPK word order)."""
    out = bytearray()
    for i in range(0, len(data) - len(data) % 4, 4):
        out += data[i:i + 4][::-1]
    return bytes(out)


def strip_der_sig_padding(sig):
    """Drop the legacy zero padding some signers append after the DER blob."""
    if len(sig) >= 2 and sig[0] == 0x30:
        return sig[:sig[1] + 2]
    return sig


def _to_der_sig(sig):
    """Accept either a raw r||s (64 byte) or a DER-encoded ECDSA signature."""
    if len(sig) == 64:
        r = int.from_bytes(sig[:32], 'big')
        s = int.from_bytes(sig[32:], 'big')
        return utils.encode_dss_signature(r, s)
    return strip_der_sig_padding(sig)


def verify_sig_prehashed(pubkey, sig, digest):
    """Verify an ECDSA signature computed directly over `digest`."""
    try:
        pubkey.verify(_to_der_sig(sig), digest,
                      ec.ECDSA(utils.Prehashed(SHA256())))
        return True
    except InvalidSignature:
        return False


def verify_sig_over_message(pubkey, sig, message):
    """Verify an ECDSA signature whose input message is hashed with SHA-256."""
    try:
        pubkey.verify(_to_der_sig(sig), message, ec.ECDSA(SHA256()))
        return True
    except InvalidSignature:
        return False


def load_otp(path):
    """Return {name: hex_data} for Security_Data entries, or None."""
    if not path or not os.path.isfile(path):
        return None
    try:
        with open(path, 'r') as f:
            lines = [ln for ln in f if not ln.lstrip().startswith('#')]
        data = json.loads(''.join(lines))
    except (ValueError, OSError):
        return None
    result = {}
    for entry in data.get('Security_Data', []):
        name = entry.get('name')
        if name:
            result[name] = entry.get('data', '').lower()
    return result


def parse_mcuboot_tlvs(image):
    """Return (payload, digest, pubkey_der, signature) from an MCUboot image."""
    if len(image) < 16:
        return None
    magic, _, header_size, _, img_size = struct.unpack('<IIHHI', image[:16])
    if magic != IMAGE_MAGIC:
        return None

    tlv_off = header_size + img_size
    info = image[tlv_off:tlv_off + TLV_INFO_SIZE]
    if len(info) < TLV_INFO_SIZE:
        return None
    tlv_magic, _ = struct.unpack('<HH', info)
    if tlv_magic == TLV_PROT_INFO_MAGIC:
        _, prot_tot = struct.unpack('<HH', info)
        tlv_off += prot_tot
        info = image[tlv_off:tlv_off + TLV_INFO_SIZE]
        if len(info) < TLV_INFO_SIZE:
            return None
        tlv_magic, _ = struct.unpack('<HH', info)

    if tlv_magic != TLV_INFO_MAGIC:
        return None

    _, tlv_tot = struct.unpack('<HH', info)
    prot_tlv_size = tlv_off
    payload = image[:prot_tlv_size]

    stored_digest = pub = sig = None
    end = tlv_off + tlv_tot
    pos = tlv_off + TLV_INFO_SIZE
    while pos < end:
        if pos + TLV_HDR_SIZE > len(image):
            break
        tlv_type, _, tlv_len = struct.unpack('<BBH', image[pos:pos + TLV_HDR_SIZE])
        val = image[pos + TLV_HDR_SIZE:pos + TLV_HDR_SIZE + tlv_len]
        if tlv_type == TLV_SHA256:
            stored_digest = val
        elif tlv_type == TLV_PUBKEY:
            pub = val
        elif tlv_type == TLV_ECDSA256:
            sig = val
        pos += TLV_HDR_SIZE + tlv_len

    return payload, stored_digest, pub, sig


def _image_hash_consistent(image):
    parsed = parse_mcuboot_tlvs(image)
    if parsed is None:
        return False
    payload, stored_digest, _, _ = parsed
    return stored_digest is not None and \
        hashlib.sha256(payload).digest() == stored_digest


# ---------------------------------------------------------------------------
# bootloader.bin
# ---------------------------------------------------------------------------
def verify_bootloader(path, root_pubkey, flash_key, encrypted, otp, rep):
    rep.section(f'bootloader: {path}')
    with open(path, 'rb') as f:
        data = f.read()

    magic = data[SECBOOT_MAGIC_OFFSET:SECBOOT_MAGIC_OFFSET + len(SECBOOT_MAGIC)]
    if magic == SECBOOT_MAGIC:
        rep.info('BootROM secure-boot magic', f'{SECBOOT_MAGIC.decode()} @ 0x{SECBOOT_MAGIC_OFFSET:x}')
    else:
        rep.check(False, 'BootROM secure-boot magic present',
                  f'unexpected value at 0x{SECBOOT_MAGIC_OFFSET:x}: {magic!r}')

    man_off = data.find(struct.pack('<I', MANIFEST_MAGIC))
    if man_off < 0:
        rep.check(False, 'manifest located', 'magic not found')
        return
    rep.info('manifest located', f'offset 0x{man_off:x}')

    (_, _, sec_counter, total_len, sign_size, num_img) = struct.unpack(
        '<IIIIII', data[man_off:man_off + 24])
    static_addr, load_addr, hashed_size = struct.unpack(
        '<III', data[man_off + 0x20:man_off + 0x2c])

    manifest = data[man_off:man_off + total_len]
    pub_off = total_len - MANIFEST_SIG_LEN - MANIFEST_PUBKEY_LEN
    sig_off = pub_off + MANIFEST_PUBKEY_LEN
    embedded_pub = manifest[pub_off:sig_off]
    signature = manifest[sig_off:sig_off + MANIFEST_SIG_LEN]
    stored_digest = manifest[0x30:0x50]

    rep.info('security counter', str(sec_counter))
    rep.info('BL2 static/load addr',
             f'0x{static_addr:08x} / 0x{load_addr:08x}')

    # 1) manifest authenticity: signature over sha256(header + embedded pubkey)
    signed_region = manifest[:sig_off]
    man_digest = hashlib.sha256(signed_region).digest()
    rep.check(verify_sig_prehashed(root_pubkey, signature, man_digest),
              'manifest signature verified with root public key')

    # 2) the manifest signing key is the provided root key
    rep.check(embedded_pub == pubkey_uncompressed(root_pubkey),
              'manifest public key matches root public key')

    # 3) the manifest describes the BL2 image actually present in the image
    vir_code = static_addr - FLASH_BASE_ADDR
    phy_code = virtual2phy(vir_code)
    region = data[phy_code:phy_code + crc_phys_size(hashed_size)]
    if len(region) < crc_phys_size(hashed_size):
        rep.check(False, 'BL2 image digest matches manifest', 'BL2 region truncated')
    else:
        flat = deinterleave(region)
        bl2_plain = xts_decrypt(flat, flash_key, vir_code) if encrypted else flat
        bl2_digest = hashlib.sha256(bl2_plain[:hashed_size]).digest()
        rep.check(bl2_digest == stored_digest,
                  'BL2 image digest matches manifest',
                  f'vcode@0x{vir_code:x} len=0x{hashed_size:x}')

    _cross_check_rotpk(root_pubkey, otp, rep)


def _cross_check_rotpk(root_pubkey, otp, rep):
    if not otp:
        return
    # BL2 ROTPK: sha256 of the DER SubjectPublicKeyInfo, stored word-reversed.
    bl2_hash = hashlib.sha256(pubkey_der(root_pubkey)).digest()
    otp_bl2 = otp.get('bl2_rotpk_hash')
    if otp_bl2:
        rep.check(reverse_words(bl2_hash).hex() == otp_bl2,
                  'OTP bl2_rotpk_hash matches root public key')


# ---------------------------------------------------------------------------
# all-app.bin
# ---------------------------------------------------------------------------
def _locate_app_image(blob, flash_key, encrypted, p_off):
    """Recover the MCUboot application image from a download sub-image blob.

    The code is CRC-interleaved and (optionally) flash-encrypted. Returns the
    decrypted MCUboot image (starting at its magic) or None.
    """
    crc_off = find_crc_offset(blob)
    if crc_off is None:
        # Not interleaved (e.g. plaintext data partition): scan directly.
        idx = blob.find(IMAGE_MAGIC_LE)
        if idx >= 0 and _image_hash_consistent(blob[idx:]):
            return blob[idx:]
        return None

    flat = deinterleave(blob[crc_off:])

    if not encrypted:
        idx = flat.find(IMAGE_MAGIC_LE)
        if idx >= 0 and _image_hash_consistent(flat[idx:]):
            return flat[idx:]
        return None

    base = phy2virtual(p_off)
    head = flat[:64]
    candidates = [ceil_align(base, XTS_UNIT_SZ)]
    candidates += list(range(max(0, base - 64), base + 768, XTS_UNIT_SZ))
    tried = set()
    for start in candidates:
        if start in tried or start < 0:
            continue
        tried.add(start)
        if struct.unpack('<I', xts_decrypt(head, flash_key, start)[:4])[0] != IMAGE_MAGIC:
            continue
        image = xts_decrypt(flat, flash_key, start)
        if _image_hash_consistent(image):
            return image
    return None


def verify_all_app(path, root_pubkey, flash_key, encrypted, otp, rep):
    rep.section(f'all-app: {path}')
    with open(path, 'rb') as f:
        data = f.read()

    if data[:len(DOWNLOAD_MAGIC)] != DOWNLOAD_MAGIC:
        rep.check(False, 'download container magic', 'BKDLV10. not found')
        return

    num_img = struct.unpack('>H', data[18:20])[0]
    subs = []
    off = GLOBAL_HDR_LEN
    for _ in range(num_img):
        hdr = data[off:off + IMG_HDR_LEN]
        part_off, part_size, flash_start, img_off, img_len = struct.unpack(
            '>IIIII', hdr[:20])
        subs.append((part_off, img_off, img_len))
        off += IMG_HDR_LEN
    rep.info('sub-images', str(num_img))

    image = None
    part_off_found = None
    for part_off, img_off, img_len in subs:
        blob = data[img_off:img_off + img_len]
        image = _locate_app_image(blob, flash_key, encrypted, part_off)
        if image is not None:
            part_off_found = part_off
            break

    if image is None:
        rep.check(False, 'application image located',
                  'no MCUboot image found in container')
        return

    rep.info('application image', f'partition@0x{part_off_found:x}')

    parsed = parse_mcuboot_tlvs(image)
    payload, stored_digest, embedded_pub, sig = parsed

    digest = hashlib.sha256(payload).digest()
    rep.check(stored_digest is not None and digest == stored_digest,
              'application image hash consistent with payload')

    if embedded_pub is None or sig is None:
        rep.check(False, 'application signature verified',
                  'missing PUBKEY/ECDSA TLV')
        return

    rep.check(verify_sig_over_message(root_pubkey, sig, digest),
              'application signature verified with root public key')
    rep.check(embedded_pub == pubkey_der(root_pubkey),
              'application public key matches root public key')

    if otp and otp.get('bl2_rotpk_hash'):
        emb_hash = hashlib.sha256(embedded_pub).digest()
        rep.check(reverse_words(emb_hash).hex() == otp['bl2_rotpk_hash'],
                  'OTP bl2_rotpk_hash matches application public key')


def build_arg_parser():
    p = argparse.ArgumentParser(
        description='Verify secure-boot bootloader.bin and all-app.bin.')
    p.add_argument('--bootloader', default=DEFAULT_BOOTLOADER,
                   help='path to bootloader.bin (default: output_dir/)')
    p.add_argument('--all-app', dest='all_app', default=DEFAULT_ALL_APP,
                   help='path to all-app.bin (default: output_dir/)')
    p.add_argument('--root-pubkey', default=DEFAULT_ROOT_PUBKEY,
                   help='root EC public key PEM (default: config/key/)')
    p.add_argument('--flash-key', default=DEFAULT_FLASH_KEY,
                   help='flash AES key file or hex string (default: config/)')
    p.add_argument('--otp', default=DEFAULT_OTP,
                   help='otp_efuse_config.json for ROTPK cross-check (optional)')

    enc = p.add_mutually_exclusive_group()
    enc.add_argument('--encrypted', dest='encrypted', action='store_true',
                     help='firmware is flash-encrypted (default)')
    enc.add_argument('--plaintext', dest='encrypted', action='store_false',
                     help='firmware is not flash-encrypted')
    p.set_defaults(encrypted=True)
    return p


def main(argv=None):
    args = build_arg_parser().parse_args(argv)
    rep = Reporter()

    root_pubkey = load_root_pubkey(args.root_pubkey)
    flash_key = None
    if args.encrypted:
        flash_key = load_flash_key(args.flash_key)

    print('Secure firmware verification')
    print(f'  root public key : {args.root_pubkey}')
    print(f'  encryption      : {"ON" if args.encrypted else "OFF"}')
    if args.encrypted:
        print(f'  flash AES key   : {args.flash_key} ({len(flash_key) * 8}-bit)')

    otp = load_otp(args.otp)
    if otp is None and args.otp:
        rep.info('OTP cross-check skipped', f'{args.otp} not usable')

    if os.path.isfile(args.bootloader):
        verify_bootloader(args.bootloader, root_pubkey, flash_key,
                          args.encrypted, otp, rep)
    else:
        rep.check(False, 'bootloader present', f'{args.bootloader} missing')

    if os.path.isfile(args.all_app):
        verify_all_app(args.all_app, root_pubkey, flash_key,
                       args.encrypted, otp, rep)
    else:
        rep.check(False, 'all-app present', f'{args.all_app} missing')

    print(f'\nResult: {rep.passed} passed, {rep.failed} failed')
    return 1 if rep.failed else 0


if __name__ == '__main__':
    sys.exit(main())
