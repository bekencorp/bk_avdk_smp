from __future__ import annotations

import copy
import json
import logging
import os
import platform
import shutil
import sys
from pathlib import Path

import bk_packager
from bk_bootloader_post import check_is_ab_project, get_bootloader_archieve_dir
from bk_build_summary import bk_build_summary

ARMINO_SOC = os.environ["ARMINO_SOC"]
PROJECT_BUILD_DIR = os.environ["PROJECT_BUILD_DIR"]
armino_tools_path = os.getenv("ARMINO_TOOLS_PATH")
pack_boot_tools = "%s/env_tools/beken_packager" % (armino_tools_path)
header_path = "{}/env_tools/rtt_ota/ota-rbl/".format(armino_tools_path)
ota_tool = "%s/env_tools/rtt_ota/ota-rbl/ota_packager_python.py" % (armino_tools_path)
armino_path = os.getenv("ARMINO_CP_DIR")
project_dir = os.getenv("PROJECT_DIR")
g_output_info = ""


def get_pack_tool_exe():
    system = platform.system()
    pack_tool_dir = "%s/env_tools/beken_packager" % (armino_tools_path)
    if system == "Windows":
        return os.path.join(pack_tool_dir, "cmake_Gen_image.exe")
    elif system == "Linux":
        return os.path.join(pack_tool_dir, "cmake_Gen_image")
    elif system == "Darwin":
        raise RuntimeError("not support macos")
    else:
        raise RuntimeError("unknown system type")


logger = logging.getLogger(os.path.basename(__file__))


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def copy_binaries_to_pack_dir(origin_path: Path, pack_path: Path):
    if not os.path.exists(origin_path):
        raise FileNotFoundError(f"{origin_path} not found.")
    shutil.copy(origin_path, pack_path)


def prepare_package_dependencies(project_build_dir: Path, pack_dir: Path):
    if not os.path.exists(pack_dir):
        os.mkdir(pack_dir)
    # copy bootloader cp ap binary
    bootloader_name = "bootloader.bin"
    bootloader_dir = Path(get_bootloader_archieve_dir())
    origin_bootloader_path = bootloader_dir / bootloader_name
    pack_bootloader_path = pack_dir / bootloader_name
    ota_json = f"{PROJECT_BUILD_DIR}/partitions/bk_ota_partitions.json"
    ret = os.system(
        "%s genfile -injsonfile %s/config.json -infile %s -outfile %s -genjson %s"
        % (
            get_pack_tool_exe(),
            pack_boot_tools,
            origin_bootloader_path,
            pack_bootloader_path,
            ota_json,
        )
    )
    if ret != 0:
        raise RuntimeError("attach ota partitions to bootloader fail!")
    logger.info("attach ota partitions to bootloader")

    cp_name = "app.bin"
    cp_dir = project_build_dir / ARMINO_SOC
    origin_cp_path = cp_dir / cp_name
    pack_cp_path = pack_dir / cp_name
    copy_binaries_to_pack_dir(origin_cp_path, pack_cp_path)

    ap_name = "app.bin"
    ap_dir = project_build_dir / f"{ARMINO_SOC}_ap"
    origin_ap_path = ap_dir / ap_name
    pack_ap_path = pack_dir / "app1.bin"
    copy_binaries_to_pack_dir(origin_ap_path, pack_ap_path)


def parse_format_size(size_str: str):
    size_str = size_str.lower()
    if size_str.endswith("m"):
        return int(size_str[:-1]) * 1024 * 1024
    if size_str.endswith("k"):
        return int(size_str[:-1]) * 1024
    return int(size_str)


class bk_smp_packager:
    def __init__(self, pack_dir: Path, pack_json: Path):
        if not os.path.exists(pack_dir):
            raise RuntimeError(f"{pack_dir} not found")
        if not os.path.exists(pack_json):
            raise FileNotFoundError(f"{pack_json} not found")
        self.pack_dir = pack_dir
        self.pack_json = pack_json
        with self.pack_json.open("r") as f:
            self.part_info = json.load(f)
        self.crc_enable = self.part_info["crc_enable"]

    def pack_all_bin(self, output_bin: Path):
        def binary_align_32_byte(bin_path: Path):
            if not bin_path.exists():
                raise RuntimeError(f"{bin_path} no exist.")
            bin_size = output_bin.stat().st_size
            padding_size = (32 - bin_size % 32) % 32
            with bin_path.open("ab") as f:
                f.write(bytes([0xFF] * padding_size))

        if self.crc_enable:
            packager = bk_packager.bk_packager_linear_crc(
                self.pack_dir, self.pack_json, output_bin
            )
        else:
            packager = bk_packager.bk_packager_linear(
                self.pack_dir, self.pack_json, output_bin
            )

        packager.pack()
        # cmake_Gen_img 32byte align, so do same here.
        binary_align_32_byte(output_bin)

    def pack_ota_app_bin(self, output_bin: Path):
        apps_part_info = copy.deepcopy(self.part_info)
        sections: list[dict[str, str]] = apps_part_info["section"]
        for index, part in enumerate(sections):
            if "bootloader" in part["partition"]:
                sections.pop(index)
                apps_part_info["count"] -= 1
                break

        if self.crc_enable:
            for part in sections:
                addr = int(int(part["start_addr"], 16) / 34 * 32)
                part["start_addr"] = f"0x{addr:08x}"
                size = parse_format_size(part["size"]) / 34 * 32
                size_format = int(size / 1024)
                part["size"] = f"{size_format}K"
        app_pack_json = self.pack_dir / "ota_apps_pack.json"
        with app_pack_json.open("w") as f:
            json.dump(apps_part_info, f, indent=4)

        ota_app_bin = output_bin
        packager = bk_packager.bk_packager_linear(
            self.pack_dir, app_pack_json, ota_app_bin
        )
        packager.pack()


def pack_ota_rbl_non_ab(pack_dir: Path, origin_ota_app_bin: Path):
    global g_output_info
    ota_bin = Path("app_pack.rbl")
    ret = os.system(
        "python3 %s -i %s -o %s -g %s -ap %s -pjd %s packager"
        % (ota_tool, origin_ota_app_bin, ota_bin, header_path, armino_path, project_dir)
    )
    if ret != 0:
        raise RuntimeError("generate ota rbl file fail.")
    logger.info(f"generate ota firmware {ota_bin}")
    g_output_info += f"ota binary: {ota_bin.absolute()}\n"


def get_crc_tool_exe():
    system = platform.system()
    crc_tool_dir = "%s/env_tools/beken_packager" % (armino_tools_path)
    if system == "Windows":
        return os.path.join(crc_tool_dir, "cmake_encrypt_crc.exe")
    elif system == "Linux":
        return os.path.join(crc_tool_dir, "cmake_encrypt_crc")
    elif system == "Darwin":
        raise RuntimeError("not support macos")
    else:
        raise RuntimeError("unknown system type")


def crc_from_config_json(origin_file: Path):
    crc_tool = get_crc_tool_exe()
    if os.path.exists(crc_tool.strip()) and os.path.isfile(crc_tool.strip()):
        os.system("%s -crc %s" % (crc_tool, origin_file))
    else:
        raise RuntimeError("crc_tool path error!")


def pack_ota_rbl_ab(
    pack_dir: Path, bootloader_size: int, origin_ota_app_bin: Path, all_app_bin: Path
):
    global g_output_info
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

    crc_from_config_json(ota_app_temp_bin)
    logger.info(f"generate ota firmware {ota_bin}")
    g_output_info += f"ota binary: {ota_bin.absolute()}\n"
    ota_app_temp_crc_bin = f"{pack_dir}/ota_app_temp_crc.bin"
    shutil.copy(ota_app_temp_crc_bin, ota_bin)
    os.remove(ota_app_temp_bin)
    os.remove(ota_app_temp_crc_bin)

    with all_app_bin.open("r+b") as dest_f, ota_bin.open("rb") as src_f:
        dest_f.seek(bootloader_size)
        write_data = src_f.read()
        dest_f.write(write_data)
    logger.info(f"overwrite all_app.bin with {ota_bin}")


def pack_ota_rbl(
    pack_dir: Path, pack_json: Path, origin_ota_app_bin: Path, all_app_bin: Path
):
    if check_is_ab_project() == "False":
        pack_ota_rbl_non_ab(pack_dir, origin_ota_app_bin)
        return

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
    pack_ota_rbl_ab(pack_dir, bootloader_size, origin_ota_app_bin, all_app_bin)


def gen_build_summary(build_dir: Path, sumary_file: Path):
    summary = bk_build_summary()
    partitions_info = build_dir / "partitions" / "partitions.txt"
    ap_build_dir = build_dir / "bk7258_ap"
    cp_build_dir = build_dir / "bk7258"
    summary.set_partitions_info(partitions_info)
    summary.set_app_folder("CP", cp_build_dir)
    summary.set_app_folder("AP", ap_build_dir)
    summary.set_output_file_info(g_output_info)
    summary.gen_summary(sumary_file)


if __name__ == "__main__":
    set_logging()
    project_build_dir = Path(sys.argv[1])
    raw_pack_json = Path(sys.argv[2])
    sumary_file = Path(sys.argv[3])
    pack_dir = project_build_dir / "package"
    pack_json = pack_dir / "bk_package.json"
    shutil.copy(raw_pack_json, pack_json)
    logger.info("Enter SMP Package")
    pack_dir_temp = pack_dir / "tmp"
    prepare_package_dependencies(project_build_dir, pack_dir_temp)
    os.chdir(pack_dir)
    packager = bk_smp_packager(pack_dir_temp, pack_json)
    all_app_bin = pack_dir / "all-app.bin"
    packager.pack_all_bin(all_app_bin)
    g_output_info += f"firmware: {all_app_bin}\n"
    origin_ota_app_bin = pack_dir_temp / "origin_ota_app.bin"
    packager.pack_ota_app_bin(origin_ota_app_bin)
    pack_ota_rbl(pack_dir_temp, pack_json, origin_ota_app_bin, all_app_bin)
    pack_json.unlink()
    gen_build_summary(Path(PROJECT_BUILD_DIR), sumary_file)
