import json
import logging
import os
from pathlib import Path

from bk_crc import bk_crc16
from bk_curr_project import curr_project
from bk_misc import parse_format_size

logger = logging.getLogger(Path(__file__).name)
project_dir = curr_project.project_path
armino_path = curr_project.app0_src_root_path
ota_tool = curr_project.tools_path / "env_tools/rtt_ota/ota-rbl/ota_packager_python.py"
header_path = curr_project.tools_path / "env_tools/rtt_ota/ota-rbl"


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
    ota_app_temp_bin = pack_dir / "ota_app_temp.bin"
    soc_name = curr_project.soc_name
    cmd = (
        f"python3 {ota_tool} -i {origin_ota_app_bin} -o {ota_app_temp_bin} "
        + f"-g {header_path} -ap {armino_path} -soc {soc_name} -pjd {project_dir} packager"
    )
    # raise RuntimeError(cmd)
    ret = os.system(cmd)
    if ret != 0:
        raise RuntimeError("generate ota rbl file fail.")

    crc_handler = bk_crc16()
    crc_handler.crc_file(ota_app_temp_bin, ota_bin)
    logger.info(f"generate ota firmware {ota_bin}")
    ota_app_temp_bin.unlink()

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


if __name__ == "__main__":
    project_build_dir = curr_project.project_build_dir
    build_pack_dir = curr_project.project_build_package_dir
    build_partitions_dir = curr_project.project_build_parititons_dir
    sumary_file = build_pack_dir / "build_summary.txt"
    pack_dir_temp = build_pack_dir / "tmp"
    pack_json = build_partitions_dir / "bk_package.json"
    all_app_bin = build_pack_dir / "all-app.bin"
    origin_ota_app_bin = pack_dir_temp / "origin_ota_app.bin"
    ota_bin = pack_ota_rbl(pack_dir_temp, pack_json, origin_ota_app_bin, all_app_bin)
