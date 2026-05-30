#!/usr/bin/env python3
"""Write a Product ID into bootloader/all-app binaries.

The layout matches the BK7239 anti-channel-conflict format:
  - no-CRC mode: magic at 0x100, Product ID at 0x108, 15 bytes
  - CRC mode:    magic at 0x110, Product ID at 0x118, 15 bytes + CRC8
"""

import argparse
import os
import sys


MAGIC_STRINGS = (b"BEKEN", b"BK.SB", b"BK7236", b"BK7259")
PRODUCT_ID_LEN = 15
NO_CRC_MAGIC_OFFSET = 0x100
CRC_MAGIC_OFFSET = 0x110
NO_CRC_PRODUCT_OFFSET = 0x108
CRC_PRODUCT_OFFSET = 0x118

_verbose = False


def log(*args, **kwargs):
    if _verbose:
        print(*args, **kwargs)


def log_error(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def read_product_id_from_file(product_id_file):
    if not os.path.exists(product_id_file):
        raise FileNotFoundError(f"product_id.txt not found at {product_id_file}")

    with open(product_id_file, "r", encoding="utf-8") as f:
        product_id = f.read().strip()

    return product_id or "0"


def calculate_crc8(data):
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x01:
                crc = (crc >> 1) ^ 0x8C
            else:
                crc >>= 1
    return crc & 0xFF


def _has_magic(bin_data, offset):
    if len(bin_data) < offset + max(len(magic) for magic in MAGIC_STRINGS):
        return False

    for magic in MAGIC_STRINGS:
        if bin_data[offset:offset + len(magic)] == magic:
            return True
    return False


def detect_crc_mode(bin_data):
    if _has_magic(bin_data, NO_CRC_MAGIC_OFFSET):
        return False
    if _has_magic(bin_data, CRC_MAGIC_OFFSET):
        return True
    return None


def ascii_to_bytes(ascii_str, length=PRODUCT_ID_LEN):
    if ascii_str == "0" or ascii_str == "":
        return bytes(length)

    try:
        encoded = ascii_str.encode("ascii")
    except UnicodeEncodeError as exc:
        raise ValueError("Product ID must be printable ASCII") from exc

    if len(encoded) > length:
        raise ValueError(
            f"ASCII string length exceeds {length} characters: {len(encoded)} characters"
        )

    return encoded + bytes(length - len(encoded))


def write_product_id_to_bin(bin_file, product_id, offset=None, backup=False, use_crc=None):
    if not os.path.exists(bin_file):
        raise FileNotFoundError(f"Binary file not found: {bin_file}")

    with open(bin_file, "rb") as f:
        bin_data = bytearray(f.read())

    if use_crc is None:
        detected_mode = detect_crc_mode(bin_data)
        if detected_mode is None:
            log("No supported magic string detected at 0x100 or 0x110; skipping write.")
            return
        use_crc = detected_mode

    if offset is None:
        offset = CRC_PRODUCT_OFFSET if use_crc else NO_CRC_PRODUCT_OFFSET

    id_bytes = ascii_to_bytes(product_id)
    if use_crc:
        data_to_write = id_bytes + bytes([calculate_crc8(id_bytes)])
    else:
        data_to_write = id_bytes

    if offset + len(data_to_write) > len(bin_data):
        raise ValueError(
            f"Offset 0x{offset:X} + {len(data_to_write)} bytes exceeds file size 0x{len(bin_data):X}"
        )

    if backup:
        with open(bin_file + ".backup", "wb") as f:
            f.write(bin_data)

    bin_data[offset:offset + len(data_to_write)] = data_to_write

    with open(bin_file, "wb") as f:
        f.write(bin_data)

    with open(bin_file, "rb") as f:
        f.seek(offset)
        written_data = f.read(len(data_to_write))

    if written_data != data_to_write:
        raise ValueError("Verification failed: written data does not match expected data")

    log(
        f"Product ID '{product_id}' written to {bin_file} at 0x{offset:X} "
        f"({'CRC' if use_crc else 'NO_CRC'})"
    )


def parse_offset(offset):
    if offset is None:
        return None
    return int(offset, 16) if offset.lower().startswith("0x") else int(offset)


def main():
    parser = argparse.ArgumentParser(
        description="Write Product ID to a binary with CRC mode auto-detection."
    )
    parser.add_argument("bin_file", help="Path to binary file")
    parser.add_argument("product_id", nargs="?", default=None, help="Product ID string")
    parser.add_argument(
        "--product-id-file",
        type=str,
        default=None,
        help="Path to product ID text file",
    )
    parser.add_argument("--offset", type=str, default=None, help="Override write offset")

    crc_group = parser.add_mutually_exclusive_group()
    crc_group.add_argument("--crc", action="store_true", help="Force CRC mode")
    crc_group.add_argument("--no-crc", action="store_true", help="Force no-CRC mode")

    parser.add_argument("--backup", action="store_true", help="Create a .backup file")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print details")

    args = parser.parse_args()

    global _verbose
    _verbose = args.verbose

    product_id = args.product_id
    if product_id is None:
        if args.product_id_file is None:
            log_error("Error: Either provide product_id or use --product-id-file")
            return 1
        try:
            product_id = read_product_id_from_file(args.product_id_file)
        except Exception as exc:
            log_error(f"Error reading product ID: {exc}")
            return 1

    if args.crc:
        use_crc = True
    elif args.no_crc:
        use_crc = False
    else:
        use_crc = None

    try:
        write_product_id_to_bin(
            args.bin_file,
            product_id,
            offset=parse_offset(args.offset),
            backup=args.backup,
            use_crc=use_crc,
        )
        return 0
    except Exception as exc:
        log_error(f"Error: {exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
