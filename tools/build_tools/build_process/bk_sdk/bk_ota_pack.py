from __future__ import annotations

import csv
import json
import logging
import os
import shutil
import struct
import zlib
from pathlib import Path

import bk_packager
from bk_misc import parse_format_size
from bk_ota_partition import bk_ota_partition

from .bk_curr_project import curr_project

logger = logging.getLogger(Path(__file__).name)
project_dir = curr_project.project_path
armino_path = curr_project.app0_src_root_path
ota_tool = curr_project.tools_path / "env_tools/rtt_ota/ota-rbl/ota_packager_python.py"
header_path = curr_project.tools_path / "env_tools/rtt_ota/ota-rbl"

OTA_PACK_BIN_ALIGN_LEN = 64
BL_METADATA_VERSION = 1
BL_METADATA_PARTITION_OFFSET = 0xF000
BL_METADATA_PARTITION_SIZE = 0x400
BL_METADATA_FLASH_CONFIG_OFFSET = 0xF400
BL_METADATA_FLASH_CONFIG_SIZE = 0xC00
BL_METADATA_BOOTLOADER_SIZE = 0x10000
BL_METADATA_PARTITION_MAGIC = 0x31545042  # "BPT1"
BL_METADATA_FLASH_CONFIG_MAGIC = 0x31434642  # "BFC1"
BL_OTA_SCHEME_FULL = 1
BL_OTA_SCHEME_DIFF = 2
BL_OTA_SCHEME_AB = 3
# header: magic, version, header_size, entry_size, entry_count, total_size, ota_scheme, payload_crc32
BL_METADATA_HEADER_FORMAT = "<IHHHHIII"
BL_METADATA_HEADER_SIZE = struct.calcsize(BL_METADATA_HEADER_FORMAT)
BL_PARTITION_NAME_MAX = 24
# partition entry (App semantic): partition_id, owner, name[24], start_addr, length, options, reserved
BL_PARTITION_ENTRY_FORMAT = "<II24sIIII"
BL_PARTITION_ENTRY_SIZE = struct.calcsize(BL_PARTITION_ENTRY_FORMAT)
BL_FLASH_CONFIG_ENTRY_FORMAT = "<IBBHHH"
BL_FLASH_CONFIG_ENTRY_SIZE = struct.calcsize(BL_FLASH_CONFIG_ENTRY_FORMAT)

# bk_flash_t owner
BL_FLASH_OWNER_EMBEDDED = 0
# PAR_OPT_* bit positions (see flash_partition.h)
PAR_OPT_READ_EN = 1 << 0
PAR_OPT_WRITE_EN = 1 << 1
PAR_OPT_EXECUTE_EN = 1 << 2


# region gen json
def gen_ota_pack_json():
    partitions_dir = curr_project.project_build_parititons_dir
    partitions_json = partitions_dir / "partitions.json"
    ota_partition_json = partitions_dir / "bk_ota_partitions.json"
    ota_partition = bk_ota_partition(partitions_json)
    if curr_project.is_ab_project:
        ota_partition.gen_ab_ota_json(ota_partition_json)
        ota_partition.gen_ab_configuartion_json(partitions_dir / "configurationab.json")
    else:
        ota_partition.gen_ota_json(ota_partition_json)


# end region
# region bl attach table
def format_string_to_bytes(string: str, length: int) -> bytes:
    string_bytes = string.encode()
    if len(string_bytes) < length:
        string_bytes += bytes(length - len(string_bytes))
    return string_bytes


def get_ota_scheme() -> int:
    if curr_project.is_ab_project:
        return BL_OTA_SCHEME_AB
    return BL_OTA_SCHEME_DIFF


def build_metadata(magic: int, entry_size: int, entries: bytes, ota_scheme: int) -> bytes:
    """Prepend the common header (with payload CRC32) to the entry payload."""
    if len(entries) % entry_size != 0:
        raise RuntimeError(
            f"metadata payload {len(entries)} not aligned to entry size {entry_size}"
        )
    entry_count = len(entries) // entry_size
    total_size = BL_METADATA_HEADER_SIZE + len(entries)
    payload_crc32 = zlib.crc32(entries) & 0xFFFFFFFF
    header = struct.pack(
        BL_METADATA_HEADER_FORMAT,
        magic,
        BL_METADATA_VERSION,
        BL_METADATA_HEADER_SIZE,
        entry_size,
        entry_count,
        total_size,
        ota_scheme,
        payload_crc32,
    )
    return header + entries


def _adapt_partition_name(name: str, execute: bool, app_count: int) -> tuple[str, int]:
    """Replicate bk_flash_partition._part_adapter naming so on-flash names match App."""
    if "bootloader" in name and execute:
        return "bootloader", app_count
    if execute:
        adapted = "application" + (str(app_count) if app_count else "")
        return adapted, app_count + 1
    return name, app_count


def serialize_partitions_table(partitions_json: Path) -> bytes:
    """Serialize the FULL App partition table (App naming/semantics) from partitions.json."""
    if not partitions_json.exists():
        raise RuntimeError(f"{partitions_json} not exists.")
    with partitions_json.open("r") as f:
        part_info = json.load(f)

    sections = sorted(part_info["section"], key=lambda x: x["Id"])
    entries = bytes()
    app_count = 0
    for part in sections:
        execute = bool(part["Execute"])
        name, app_count = _adapt_partition_name(part["Name"], execute, app_count)
        options = 0
        if part["Read"]:
            options |= PAR_OPT_READ_EN
        if part["Write"]:
            options |= PAR_OPT_WRITE_EN
        if execute:
            options |= PAR_OPT_EXECUTE_EN
        entries += struct.pack(
            BL_PARTITION_ENTRY_FORMAT,
            int(part["Id"]),
            BL_FLASH_OWNER_EMBEDDED,
            format_string_to_bytes(name, BL_PARTITION_NAME_MAX),
            int(part["Offset"]),
            int(part["Size"]),
            options,
            0,
        )
    return build_metadata(
        BL_METADATA_PARTITION_MAGIC,
        BL_PARTITION_ENTRY_SIZE,
        entries,
        get_ota_scheme(),
    )


def parse_int(value: str) -> int:
    return int(value.strip(), 0)


def get_flash_config_csv() -> Path:
    """Locate flash config csv: project override first, then project-independent common default."""
    # 1. project override (optional)
    project_csv = curr_project.partitions_dir / "flash_config.csv"
    if project_csv.exists():
        return project_csv
    # 2. common default (project independent), named by soc
    common_csv = (
        curr_project.tools_path
        / "build_tools/build_process/config/flash_config"
        / f"{curr_project.soc_name}.csv"
    )
    if common_csv.exists():
        return common_csv
    raise RuntimeError(
        f"flash config csv not found: {project_csv} or {common_csv}"
    )


def serialize_flash_config_table(csv_path: Path) -> bytes:
    if not csv_path.exists():
        raise RuntimeError(f"{csv_path} not exists.")

    entries = bytes()
    count = 0
    with csv_path.open("r", newline="") as f:
        reader = csv.DictReader(
            line for line in f if line.strip() and not line.lstrip().startswith("#")
        )
        for row in reader:
            entries += struct.pack(
                BL_FLASH_CONFIG_ENTRY_FORMAT,
                parse_int(row["flash_id"]),
                parse_int(row["sr_size"]),
                0,
                parse_int(row["protect_all"]),
                parse_int(row["protect_none"]),
                0,
            )
            count += 1

    if count == 0:
        raise RuntimeError(f"{csv_path} contains no flash config entries.")

    return build_metadata(
        BL_METADATA_FLASH_CONFIG_MAGIC,
        BL_FLASH_CONFIG_ENTRY_SIZE,
        entries,
        get_ota_scheme(),
    )


def select_bootloader_source() -> Path:
    archive_path = curr_project.bootloader_archive_path
    if archive_path.exists():
        return archive_path
    build_path = curr_project.bootloader_build_path
    if build_path.exists():
        logger.warning(f"{archive_path} not found, use build output {build_path}")
        return build_path
    raise FileNotFoundError(f"{archive_path} not found.")


def write_fixed_metadata(binary_path: Path, offset: int, region_size: int, blob: bytes):
    if len(blob) > region_size:
        raise RuntimeError(
            f"metadata blob too large: {len(blob)} > {region_size} at 0x{offset:x}"
        )
    with binary_path.open("r+b") as f:
        f.seek(offset)
        f.write(blob)
        f.write(bytes([0xFF]) * (region_size - len(blob)))


def handle_bootloader_bin(pack_dir: Path):
    if not pack_dir.exists():
        pack_dir.mkdir()
    # copy bootloader cp ap binary
    bootloader_name = "bootloader.bin"
    origin_bootloader_path = select_bootloader_source()
    pack_bootloader_path = pack_dir / bootloader_name
    partitions_json = curr_project.project_build_parititons_dir / "partitions.json"
    part_bytes = serialize_partitions_table(partitions_json)
    flash_config_bytes = serialize_flash_config_table(get_flash_config_csv())
    shutil.copy(origin_bootloader_path, pack_bootloader_path)
    bootloader_size = pack_bootloader_path.stat().st_size
    if bootloader_size > BL_METADATA_PARTITION_OFFSET:
        raise RuntimeError(
            f"bootloader binary size 0x{bootloader_size:x} exceeds metadata offset 0x{BL_METADATA_PARTITION_OFFSET:x}"
        )
    with pack_bootloader_path.open("ab") as f:
        f.write(bytes([0xFF]) * (BL_METADATA_BOOTLOADER_SIZE - bootloader_size))

    write_fixed_metadata(
        pack_bootloader_path,
        BL_METADATA_PARTITION_OFFSET,
        BL_METADATA_PARTITION_SIZE,
        part_bytes,
    )
    write_fixed_metadata(
        pack_bootloader_path,
        BL_METADATA_FLASH_CONFIG_OFFSET,
        BL_METADATA_FLASH_CONFIG_SIZE,
        flash_config_bytes,
    )

    logger.info("attach bootloader metadata")


# end region
# region pack ota
def pack_ota_rbl_non_ab(origin_ota_app_bin: Path):
    ota_bin = Path("app_pack.rbl")
    cmd = (
        f"python3 {ota_tool} -i {origin_ota_app_bin} -o {ota_bin} "
        + f"-g {header_path} -ap {armino_path} -pjd {project_dir} packager"
    )
    ret = os.system(cmd)
    if ret != 0:
        raise RuntimeError("generate ota rbl file fail.")
    logger.info(f"generate ota firmware {ota_bin}")
    return ota_bin


def pack_ota_rbl_ab(
    pack_dir: Path, bootloader_size: int, origin_ota_app_bin: Path, all_app_bin: Path
):
    ota_bin = Path("app_ab_crc.rbl")
    soc_name = curr_project.soc_name
    cmd = (
        f"python3 {ota_tool} -i {origin_ota_app_bin} -o {ota_bin} "
        + f"-g {header_path} -ap {armino_path} -soc {soc_name} -pjd {project_dir} packager"
    )
    ret = os.system(cmd)
    if ret != 0:
        raise RuntimeError("generate ota rbl file fail.")
    logger.info(f"generate ota firmware {ota_bin}")

    with all_app_bin.open("rb+") as dest_f, ota_bin.open("rb") as src_f:
        dest_f.seek(bootloader_size)
        write_data = src_f.read()
        dest_f.write(write_data)
    logger.info(f"overwrite all_app.bin with {ota_bin}")
    return ota_bin


def pack_ota_rbl(
    pack_dir: Path, pack_json: Path, origin_ota_app_bin: Path, all_app_bin: Path
) -> Path:
    if not curr_project.is_ab_project:
        return pack_ota_rbl_non_ab(origin_ota_app_bin)

    with open(pack_json, "r") as f:
        pack_info = json.load(f)
    bootloader_size_fmt = "0"
    for part in pack_info["section"]:
        if part["partition"] == "bootloader":
            bootloader_size_fmt = part["size"]
            break
    bootloader_size = parse_format_size(bootloader_size_fmt)
    if bootloader_size == 0:
        raise RuntimeError("bootloader parse error")
    return pack_ota_rbl_ab(pack_dir, bootloader_size, origin_ota_app_bin, all_app_bin)


def pack_ota_app_bin(pack_dir: Path, output_bin: Path):
    pack_json = curr_project.project_build_parititons_dir / "bk_package.json"
    with pack_json.open("r") as f:
        apps_part_info = json.load(f)
    sections: list[dict[str, str]] = apps_part_info["section"]
    for index, part in enumerate(sections):
        if "bootloader" in part["partition"]:
            sections.pop(index)
            apps_part_info["count"] -= 1
            break

    if curr_project.flash_crc_enable:
        for part in sections:
            addr = int(int(part["start_addr"], 16) / 34 * 32)
            part["start_addr"] = f"0x{addr:08x}"
            size = parse_format_size(part["size"]) / 34 * 32
            size_format = int(size / 1024)
            part["size"] = f"{size_format}K"

    app_pack_json = pack_dir / "ota_apps_pack.json"
    with app_pack_json.open("w") as f:
        json.dump(apps_part_info, f, indent=4)

    ota_app_bin = output_bin
    packager = bk_packager.bk_packager_linear(pack_dir, app_pack_json, ota_app_bin)
    packager.pack()
    ota_app_size = ota_app_bin.stat().st_size
    padding_len = (
        OTA_PACK_BIN_ALIGN_LEN - (ota_app_size % OTA_PACK_BIN_ALIGN_LEN)
    ) % OTA_PACK_BIN_ALIGN_LEN
    logger.info(f"ota app size {ota_app_size}, padding len {padding_len}")
    if padding_len > 0:
        with ota_app_bin.open("ab") as f:
            f.write(bytes([0xFF]) * padding_len)


def ota_pack():
    build_pack_dir = curr_project.project_build_package_dir
    pack_dir_temp = build_pack_dir / "tmp"
    origin_ota_app_bin = pack_dir_temp / "app_pack.bin"
    pack_ota_app_bin(pack_dir_temp, origin_ota_app_bin)

    build_partitions_dir = curr_project.project_build_parititons_dir
    pack_json = build_partitions_dir / "bk_package.json"
    all_app_bin = build_pack_dir / "all-app.bin"
    ota_bin = pack_ota_rbl(pack_dir_temp, pack_json, origin_ota_app_bin, all_app_bin)
    return ota_bin.absolute()


# end region
