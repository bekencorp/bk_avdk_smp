import binascii
import json
from pathlib import Path

from bk_misc import parse_format_size


def format_string_to_bytes(string: str, length: int) -> bytes:
    string_bytes = string.encode()
    if len(string_bytes) < length:
        string_bytes += bytes(length - len(string_bytes))
    return string_bytes


def serialize_partitions_table(partitions_json: Path) -> bytes:
    def int_to_bytes(value: int):
        return value.to_bytes(4, "little")

    serialize_bytes = bytes()
    if not partitions_json.exists():
        raise RuntimeError(f"{partitions_json} not exists.")
    with partitions_json.open("r") as f:
        part_info = json.load(f)
    for part in part_info["part_table"]:
        part_bytes = bytes()
        magic_number = int(part["magic"], 16)
        part_bytes += int_to_bytes(magic_number)
        name = part["name"]
        part_bytes += format_string_to_bytes(name, 24)
        flash_name = part["flash_name"]
        part_bytes += format_string_to_bytes(flash_name, 24)
        offset = int(part["offset"], 16)
        part_bytes += int_to_bytes(offset)
        length = parse_format_size(part["len"])
        part_bytes += int_to_bytes(length)
        crc32_result = binascii.crc32(part_bytes) & 0xFFFFFFFF
        part_bytes += int_to_bytes(crc32_result)
        serialize_bytes += part_bytes
    return serialize_bytes
