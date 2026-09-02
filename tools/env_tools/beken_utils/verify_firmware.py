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
    - Application (primary_all) image signature verified with the root
      public key.
    - Application-embedded public key matches the provided root public key.
    - Image hash consistent with its signed payload.

When no paths are given the tool uses the keys under config/ and the firmware
under output_dir/ by default.

Flash encryption can be turned off at build time, so encryption is a separate
switch (--encrypted / --plaintext) instead of being auto-assumed.

Note: flash CRC interleaving is assumed disabled (identity flash mapping).
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

# XTS data unit size (bytes) used by the flash controller.
XTS_DATA_UNIT = 32

# Download container ("all-app.bin") header sizes.
GLOBAL_HDR_LEN = 32
IMG_HDR_LEN = 32
DOWNLOAD_MAGIC = b'BKDLV10.'

# BootROM signature-policy selector magic, written in plaintext at flash 0x100.
#   "BK.SB\n" -> require signature verification
#   "BEKEN\n" -> skip signature (BL2 still checks the image hash)
SECBOOT_MAGIC_OFFSET = 0x100
SECBOOT_MAGIC_LEN = 6
SECBOOT_MAGIC_REQUIRE_SIG = b'BK.SB\n'
SECBOOT_MAGIC_HASH_ONLY = b'BEKEN\n'

# Manifest layout.
MANIFEST_MAGIC = 0xA1BC2FD8
MANIFEST_SIG_LEN = 64          # r || s
MANIFEST_PUBKEY_LEN = 65       # 0x04 || X || Y (uncompressed EC-P256 point)

# MCUboot image constants.
IMAGE_MAGIC = 0x96F3B83D
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


def load_flash_key(source, use_aes256):
    """Return the effective XTS key bytes.

    `source` is either a path to a key file or a raw hex string. A 128-hex
    key is trimmed to its last 64 hex chars for AES-128-XTS (the default flash
    mode); pass use_aes256=True to keep all 128 hex chars for AES-256-XTS.
    """
    if os.path.isfile(source):
        with open(source, 'r') as f:
            key_hex = f.read().strip()
    else:
        key_hex = source.strip()

    key_hex = ''.join(key_hex.split())
    if len(key_hex) == 128 and not use_aes256:
        key_hex = key_hex[64:]

    if len(key_hex) not in (64, 128):
        raise ValueError(f'invalid flash AES key length: {len(key_hex)} hex chars')

    return bytes.fromhex(key_hex)


def _xts_tweak(unit_num):
    tweak = bytearray(16)
    for k in range(16):
        if 8 * k >= 32:
            break
        tweak[15 - k] = (unit_num >> (8 * k)) & 0xFF
    for j in range(8):
        tweak[j], tweak[15 - j] = tweak[15 - j], tweak[j]
    return bytes(tweak)


def xts_decrypt(data, key, start_address):
    """Decrypt a flash region encrypted with per-data-unit XTS tweaks."""
    if len(data) % XTS_DATA_UNIT:
        data = data + b'\xff' * (XTS_DATA_UNIT - len(data) % XTS_DATA_UNIT)

    backend = default_backend()
    out = bytearray()
    unit_num = start_address // XTS_DATA_UNIT
    for i in range(len(data) // XTS_DATA_UNIT):
        block = data[i * XTS_DATA_UNIT:(i + 1) * XTS_DATA_UNIT]
        cipher = Cipher(algorithms.AES(key), modes.XTS(_xts_tweak(unit_num + i)),
                        backend=backend)
        dec = cipher.decryptor()
        out += dec.update(block) + dec.finalize()
    return bytes(out)


def maybe_decrypt(data, key, start_address, encrypted):
    return xts_decrypt(data, key, start_address) if encrypted else data


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


def _to_der_sig(sig):
    """Accept either a raw r||s (64 byte) or a DER-encoded ECDSA signature."""
    if len(sig) == 64:
        r = int.from_bytes(sig[:32], 'big')
        s = int.from_bytes(sig[32:], 'big')
        return utils.encode_dss_signature(r, s)
    return strip_der_sig_padding(sig)


def verify_sig_prehashed(pubkey, sig, digest):
    """Verify an ECDSA signature computed directly over `digest`.

    Used by the BL2 manifest, whose signature is produced over the manifest
    digest itself (RFC-6979 style, no extra hashing).
    """
    try:
        pubkey.verify(_to_der_sig(sig), digest,
                      ec.ECDSA(utils.Prehashed(SHA256())))
        return True
    except InvalidSignature:
        return False


def verify_sig_over_message(pubkey, sig, message):
    """Verify an ECDSA signature whose input message is hashed with SHA-256.

    Used by the application image: the signer feeds the MCUboot image digest
    as the message and signs SHA256(digest).
    """
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


def strip_der_sig_padding(sig):
    """Drop the legacy zero padding some signers append after the DER blob."""
    if len(sig) >= 2 and sig[0] == 0x30:
        return sig[:sig[1] + 2]
    return sig


def verify_bootloader(path, root_pubkey, flash_key, encrypted, otp, rep):
    rep.section(f'bootloader: {path}')
    with open(path, 'rb') as f:
        data = f.read()

    magic = data[SECBOOT_MAGIC_OFFSET:SECBOOT_MAGIC_OFFSET + SECBOOT_MAGIC_LEN]
    if magic == SECBOOT_MAGIC_REQUIRE_SIG:
        rep.info('signature policy', 'require-signature (BK.SB)')
    elif magic == SECBOOT_MAGIC_HASH_ONLY:
        rep.info('signature policy', 'hash-only (BEKEN)')
    else:
        rep.check(False, 'BootROM signature-policy magic recognized',
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
    code_off = static_addr & 0x00FFFFFF
    bl2_region = data[code_off:code_off + hashed_size]
    if len(bl2_region) < hashed_size:
        rep.check(False, 'BL2 image digest matches manifest',
                  'BL2 region truncated')
    else:
        bl2_plain = maybe_decrypt(bl2_region, flash_key, code_off, encrypted)
        bl2_digest = hashlib.sha256(bl2_plain[:hashed_size]).digest()
        rep.check(bl2_digest == stored_digest,
                  'BL2 image digest matches manifest',
                  f'code@0x{code_off:x} len=0x{hashed_size:x}')

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

    app = None
    for part_off, img_off, img_len in subs:
        blob = data[img_off:img_off + img_len]
        head = maybe_decrypt(blob[:XTS_DATA_UNIT], flash_key, part_off, encrypted)
        if struct.unpack('<I', head[:4])[0] == IMAGE_MAGIC:
            app = (part_off, blob)
            break

    if app is None:
        rep.check(False, 'application image located',
                  'no MCUboot image found in container')
        return

    part_off, blob = app
    rep.info('application image', f'flash@0x{part_off:x} size=0x{len(blob):x}')
    image = maybe_decrypt(blob, flash_key, part_off, encrypted)

    parsed = parse_mcuboot_tlvs(image)
    if parsed is None:
        rep.check(False, 'application image parsed', 'invalid MCUboot TLVs')
        return
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

    p.add_argument('--aes256', action='store_true',
                   help='use a 256-bit XTS key (keep all 128 hex chars)')
    return p


def main(argv=None):
    args = build_arg_parser().parse_args(argv)
    rep = Reporter()

    root_pubkey = load_root_pubkey(args.root_pubkey)
    flash_key = None
    if args.encrypted:
        flash_key = load_flash_key(args.flash_key, args.aes256)

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
