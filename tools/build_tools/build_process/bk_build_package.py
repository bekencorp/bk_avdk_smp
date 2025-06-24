from __future__ import annotations

import copy
import json
import logging
import os
import shutil
from pathlib import Path

import bk_packager
from bk_bootloader_post import backup_bootloader_path
from bk_build_summary import bk_build_summary
from bk_misc import parse_format_size
from bk_sdk.bk_curr_project import curr_project
from bk_sdk.bk_ota_pack import pack_ota_rbl
from bk_serialize_partitions_table import serialize_partitions_table

logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


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


def gen_build_summary(output_info: str):
    build_pack_dir = curr_project.project_build_package_dir
    build_dir = curr_project.project_build_dir
    sumary_file = build_pack_dir / "build_summary.txt"
    summary = bk_build_summary()
    partitions_info = build_dir / "partitions" / "partitions.txt"
    summary.set_partitions_info(partitions_info)

    for app in curr_project.apps_info:
        app_build_dir = build_dir / app.app_name
        summary.set_app_folder(app.app_name_in_sdk, app_build_dir)

    summary.set_output_file_info(output_info)
    summary.gen_summary(sumary_file)


def handle_bootloader_bin(pack_dir: Path):
    if not pack_dir.exists():
        pack_dir.mkdir()
    # copy bootloader cp ap binary
    bootloader_name = "bootloader.bin"
    origin_bootloader_path = curr_project.bootloader_archive_path
    pack_bootloader_path = pack_dir / bootloader_name
    ota_json = curr_project.project_build_parititons_dir / "bk_ota_partitions.json"
    part_bytes = serialize_partitions_table(ota_json)
    shutil.copy(origin_bootloader_path, pack_bootloader_path)
    with pack_bootloader_path.open("ab") as f:
        pos = f.tell()
        if pos % 32 != 0:
            f.write(bytes(32 - pos % 32))
        f.write(part_bytes)

    logger.info("attach ota partitions to bootloader")


def copy_app_bin_to_pack_dir(pack_dir: Path):
    def copy_binaries_to_pack_dir(origin_path: Path, pack_path: Path):
        if not origin_path.exists():
            raise FileNotFoundError(f"{origin_path} not found.")
        shutil.copy(origin_path, pack_path)

    app_list = curr_project.apps_info
    for app in app_list:
        app_build_bin = app.build_bin
        app_pack_bin = pack_dir / app.pack_bin_name
        copy_binaries_to_pack_dir(app_build_bin, app_pack_bin)


def prepare_apps_bin_to_pack_dir(pack_dir: Path):
    handle_bootloader_bin(pack_dir)
    copy_app_bin_to_pack_dir(pack_dir)


def firmware_package():
    build_pack_dir = curr_project.project_build_package_dir
    all_app_bin = build_pack_dir / "all-app.bin"
    build_partitions_dir = curr_project.project_build_parititons_dir
    pack_dir_temp = build_pack_dir / "tmp"
    pack_json = build_partitions_dir / "bk_package.json"
    origin_ota_app_bin = pack_dir_temp / "origin_ota_app.bin"
    if not pack_dir_temp.exists():
        pack_dir_temp.mkdir()
    prepare_apps_bin_to_pack_dir(pack_dir_temp)
    os.chdir(build_pack_dir)
    packager = bk_smp_packager(pack_dir_temp, pack_json)
    packager.pack_all_bin(all_app_bin)
    packager.pack_ota_app_bin(origin_ota_app_bin)
    return all_app_bin


def ota_pack():
    build_pack_dir = curr_project.project_build_package_dir
    pack_dir_temp = build_pack_dir / "tmp"
    build_partitions_dir = curr_project.project_build_parititons_dir
    pack_json = build_partitions_dir / "bk_package.json"
    origin_ota_app_bin = pack_dir_temp / "origin_ota_app.bin"
    all_app_bin = build_pack_dir / "all-app.bin"
    ota_bin = pack_ota_rbl(pack_dir_temp, pack_json, origin_ota_app_bin, all_app_bin)
    return ota_bin.absolute()


def main():
    logger.info("Enter Armino Package")
    backup_bootloader_path()
    all_app_bin = firmware_package()
    ota_bin = ota_pack()
    output_info = f"firmware: {all_app_bin}\n" + f"ota binary: {ota_bin}\n"
    gen_build_summary(output_info)


if __name__ == "__main__":
    set_logging()
    main()
