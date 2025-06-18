import json
import logging
import os
from pathlib import Path

from bk_bootloader_post import check_is_ab_project
from bk_crc import bk_crc16
from bk_misc import parse_format_size

logger = logging.getLogger(Path(__file__).name)

ARMINO_SOC = os.environ["ARMINO_SOC"]
armino_tools_path = os.getenv("ARMINO_TOOLS_PATH")


header_path = "{}/env_tools/rtt_ota/ota-rbl/".format(armino_tools_path)
ota_tool = "%s/env_tools/rtt_ota/ota-rbl/ota_packager_python.py" % (armino_tools_path)
armino_path = os.getenv("ARMINO_CP_DIR")
project_dir = os.getenv("PROJECT_DIR")


def pack_ota_rbl_non_ab(origin_ota_app_bin: Path):
    ota_bin = Path("app_pack.rbl")
    ret = os.system(
        "python3 %s -i %s -o %s -g %s -ap %s -pjd %s packager"
        % (ota_tool, origin_ota_app_bin, ota_bin, header_path, armino_path, project_dir)
    )
    if ret != 0:
        raise RuntimeError("generate ota rbl file fail.")
    logger.info(f"generate ota firmware {ota_bin}")
    return ota_bin


def pack_ota_rbl_ab(
    pack_dir: Path, bootloader_size: int, origin_ota_app_bin: Path, all_app_bin: Path
):
    ota_bin = Path("app_ab_crc.rbl")
    ota_app_temp_bin = pack_dir / "ota_app_temp.bin"
    ret = os.system(
        "python3 %s -i %s -o %s -g %s -ap %s -soc %s -pjd %s packager"
        % (
            ota_tool,
            origin_ota_app_bin,
            ota_app_temp_bin,
            header_path,
            armino_path,
            ARMINO_SOC,
            project_dir,
        )
    )
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
    if not check_is_ab_project():
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
