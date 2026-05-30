#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate spid.h for BK7259 closed-source library builds."""

import argparse
import os
import re


def parse_single_id(id_str):
    """Parse one SPID string as hex or ASCII."""
    id_str = id_str.strip()
    if not id_str or id_str == "0":
        return 0

    hex_match = re.search(r"0x([0-9A-Fa-f]+)", id_str)
    if hex_match:
        hex_str = hex_match.group(1)
        if len(hex_str) > 32:
            raise ValueError(f"ID length exceeds 16 bytes: {len(hex_str) // 2} bytes")
        return int(hex_str, 16)

    if len(id_str) > 15:
        raise ValueError(f"ASCII string length exceeds 15 characters: {len(id_str)} characters")

    hex_value = 0
    for char in id_str:
        hex_value = (hex_value << 8) | ord(char)
    return hex_value


def parse_single_spid(file_path):
    """Parse a single SPID from input file."""
    with open(file_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    id_value = None
    id_string = None
    for line_num, line in enumerate(lines, 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parsed_value = parse_single_id(line)
        if id_value is not None:
            raise ValueError(
                f"Multiple IDs found in file. First ID: {id_string}, "
                f"Second ID: {line} (line {line_num})"
            )
        id_value = parsed_value
        id_string = line

    if id_value is None:
        id_value = 0
        id_string = "0"

    return id_value, id_string


def extract_bytes_big_endian(spid):
    """Extract up to 15 ID bytes and pad to the 16-byte OTP_PRODUCT_ID size."""
    if spid == 0:
        return [0x00] * 16

    temp_id = spid
    id_bytes_count = 0
    while temp_id > 0:
        id_bytes_count += 1
        temp_id >>= 8

    spid_bytes = []
    for i in range(min(id_bytes_count, 15)):
        byte_val = (spid >> ((id_bytes_count - 1 - i) * 8)) & 0xFF
        spid_bytes.append(byte_val)

    while len(spid_bytes) < 16:
        spid_bytes.append(0x00)

    return spid_bytes


def generate_spid_header(spid, output_path, id_string=None, soc_name=None):
    """Generate spid.h for runtime SPID verification."""
    spid_bytes = extract_bytes_big_endian(spid)
    bytes_line1 = ", ".join([f"0x{b:02X}" for b in spid_bytes[0:8]])
    bytes_line2 = ", ".join([f"0x{b:02X}" for b in spid_bytes[8:16]])
    bytes_comment = "{" + ", ".join([f"0x{b:02X}" for b in spid_bytes]) + "}"

    if id_string in (None, "", "0"):
        spid_len = 0
    elif id_string.strip().lower().startswith("0x"):
        spid_len = min(len(id_string.strip()[2:]) // 2, 15)
    else:
        spid_len = len(id_string.strip())

    spid_default_section = ""
    if soc_name:
        soc_name = soc_name.strip().upper()
        soc_bytes = [ord(c) for c in soc_name]
        soc_bytes_str = ", ".join([f"0x{b:02X}" for b in soc_bytes])
        soc_bytes_comment = "{" + ", ".join([f"0x{b:02X}" for b in soc_bytes]) + "}"
        spid_default_section = f"""
// Default product ID: "{soc_name}" {soc_bytes_comment}
const uint8_t SPID_DEFAULT[{len(soc_bytes)}] = {{
    {soc_bytes_str}
}};
#define SPID_DEFAULT_LEN  {len(soc_bytes)}
"""

    header_content = f"""#ifndef __SPID_H__
#define __SPID_H__

#include <stdint.h>

// Product ID (15 bytes ID + 1 byte CRC/unused)
// Byte order (OTP_PRODUCT_ID and SPID_IN_SDK): {bytes_comment}
const uint8_t SPID_IN_SDK[16] = {{
    {bytes_line1},
    {bytes_line2}
}};
#define SPID_IN_SDK_LEN  {spid_len}
{spid_default_section}
#endif //__SPID_H__
"""

    output_dir = os.path.dirname(output_path)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(header_content)

    print(f"Successfully generated spid.h file: {output_path}")
    print(f"Byte array: {bytes_comment}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate spid.h for a single BK7259 Product ID",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s cp/properties/soc/bk7259/spid.txt -o build/properties_libs/bk7259/generated/spid.h --soc BK7259
        """,
    )
    parser.add_argument("input_file", help="Input file containing a single SPID/Product ID")
    parser.add_argument("-o", "--output", default=None, help="Output file or directory path")
    parser.add_argument("--soc", default=None, help="SOC name for SPID_DEFAULT, e.g. BK7259")
    args = parser.parse_args()

    input_file = os.path.abspath(args.input_file)
    if not os.path.exists(input_file):
        print(f"Error: Input file does not exist: {input_file}")
        return 1

    if args.output:
        output_path = os.path.abspath(args.output)
        if os.path.isdir(output_path) or not os.path.splitext(output_path)[1]:
            output_file = os.path.join(output_path, "spid.h")
        else:
            output_file = output_path
    else:
        output_file = os.path.join(os.path.dirname(input_file), "spid.h")

    try:
        spid, id_string = parse_single_spid(input_file)
        soc_name = args.soc or os.path.basename(os.path.dirname(input_file)).upper()
        print(f"Parsed SPID: {id_string}")
        generate_spid_header(spid, output_file, id_string, soc_name)
    except Exception as exc:
        print(f"Error: {exc}")
        return 1

    return 0


if __name__ == "__main__":
    exit(main())
