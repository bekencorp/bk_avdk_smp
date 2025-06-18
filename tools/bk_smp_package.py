from __future__ import annotations

import copy
import json
import logging
import os
import shutil
from pathlib import Path

import bk_packager
from bk_build_summary import bk_build_summary
from bk_misc import parse_format_size
from bk_ota_pack import pack_ota_rbl
from bk_project import bk_project_info

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


def gen_build_summary(build_dir: Path, sumary_file: Path, output_info: str):
    summary = bk_build_summary()
    partitions_info = build_dir / "partitions" / "partitions.txt"
    # TODO optimize it
    ap_build_dir = build_dir / "bk7258_ap"
    cp_build_dir = build_dir / "bk7258"
    summary.set_partitions_info(partitions_info)
    summary.set_app_folder("CP", cp_build_dir)
    summary.set_app_folder("AP", ap_build_dir)
    summary.set_output_file_info(output_info)
    summary.gen_summary(sumary_file)


def main():
    def firmware_package():
        if not pack_dir_temp.exists():
            pack_dir_temp.mkdir()
        project_info.prepare_apps_bin_to_pack_dir(pack_dir_temp)
        os.chdir(build_pack_dir)
        packager = bk_smp_packager(pack_dir_temp, pack_json)
        all_app_bin = build_pack_dir / "all-app.bin"
        packager.pack_all_bin(all_app_bin)
        packager.pack_ota_app_bin(origin_ota_app_bin)

    logger.info("Enter Armino Package")
    project_info = bk_project_info()
    project_build_dir = project_info.get_project_build_path()
    build_pack_dir = project_build_dir / "package"
    build_partitions_dir = project_build_dir / "partitions"
    raw_pack_json = build_partitions_dir / "bk_package.json"
    sumary_file = build_pack_dir / "build_summary.txt"
    pack_dir_temp = build_pack_dir / "tmp"
    pack_json = build_pack_dir / "bk_package.json"
    all_app_bin = build_pack_dir / "all-app.bin"
    origin_ota_app_bin = pack_dir_temp / "origin_ota_app.bin"
    shutil.copy(raw_pack_json, pack_json)
    firmware_package()

    ota_bin = pack_ota_rbl(pack_dir_temp, pack_json, origin_ota_app_bin, all_app_bin)
    pack_json.unlink()
    output_info = f"firmware: {all_app_bin}\n" + f"ota binary: {ota_bin.absolute()}\n"
    gen_build_summary(project_build_dir, sumary_file, output_info)


if __name__ == "__main__":
    set_logging()
    main()
