from __future__ import annotations

import json
import logging
from pathlib import Path

from bk_auto_partition import bk_partitions_table
from bk_bootloader_post import check_is_ab_project
from bk_flash_partitions_generator import bk_flash_denpendecny_generator
from bk_flash_partiton import bk_flash_partition
from bk_misc import parse_format_size
from bk_ota_partition import bk_ota_partition
from bk_project import bk_project_info
from bk_ram_region import bk_ram_region, mem_region

logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


class bk_part:
    def __init__(
        self,
        partitions_dir: Path,
        auto_part_table: Path,
        partitions_json: Path,
        crc_enable: bool,
    ):
        if not partitions_dir.exists():
            raise RuntimeError(f"partitions directory not found: {partitions_dir}")
        if not auto_part_table.exists():
            raise FileNotFoundError(f"partitions_table not found: {auto_part_table}")

        self.partitions_dir = partitions_dir
        self.auto_part_table = auto_part_table
        self.partitions_json = partitions_json
        self.crc_enable = crc_enable

    def auto_partition(self, partitions_txt: Path):
        partitions_csv = self.partitions_dir / "partitions.csv"
        part_table = bk_partitions_table(self.auto_part_table, self.crc_enable)
        part_table.gen_partition_csv(partitions_csv)
        part_table.gen_partition_json(self.partitions_json)
        part_table.gen_pretty_format_table(partitions_txt)

        generator = bk_flash_denpendecny_generator()
        self.flash_part = bk_flash_partition(self.partitions_json, generator)

    def gen_partition_header(self):
        header_path = self.partitions_dir / "partitions_gen.h"
        self.flash_part.gen_partitions_layout_hdr(header_path)

    def gen_flash_partition_src(self):
        header_path = self.partitions_dir / "flash_partitions_index.h"
        src_path = self.partitions_dir / "flash_partitions_table.c"
        self.flash_part.gen_flash_partitions_src(header_path, src_path)

    def gen_pack_config_json(self):
        pack_json = self.partitions_dir / "bk_package.json"
        self.flash_part.gen_pack_json(pack_json)


def auto_patitions(partitions_dir: Path, auto_part_table: Path, flash_crc_enable: bool):
    partitions_json = partitions_dir / "partitions.json"
    partitions_txt = partitions_dir / "partitions.txt"
    partitioner = bk_part(
        partitions_dir, auto_part_table, partitions_json, flash_crc_enable
    )
    partitioner.auto_partition(partitions_txt)
    partitioner.gen_partition_header()
    partitioner.gen_flash_partition_src()
    partitioner.gen_pack_config_json()

    ota_partition_json = partitions_dir / "bk_ota_partitions.json"
    ota_partition = bk_ota_partition(partitions_json)
    if check_is_ab_project():
        ota_partition.gen_ab_ota_json(ota_partition_json)
        ota_partition.gen_ab_configuartion_json(partitions_dir / "configurationab.json")
    else:
        ota_partition.gen_ota_json(ota_partition_json)


def ram_region_partition(partitions_dir: Path, ram_regions_table: Path):
    ram_regions = bk_ram_region(ram_regions_table)
    ram_regions_hdr_file = partitions_dir / "ram_regions.h"
    smp_default_config = Path(__file__).parent / "smp_ram_setting.json"
    with smp_default_config.open("r") as f:
        def_config = json.load(f)
    sram_addr = int(def_config["SRAM_BASE_ADDR"], 16)
    sram_size = parse_format_size(def_config["SRAM_CAPACITY"])
    psram_addr = int(def_config["PSRAM_BASE_ADDR"], 16)
    psram_size = parse_format_size(def_config["PSRAM_CAPACITY"])
    defconfig: list[mem_region] = []
    for item in def_config["Default_Regions"]:
        region = mem_region(
            item["name"], item["type"], int(item["addr"], 16), int(item["size"], 16)
        )
        defconfig.append(region)
    ram_regions.set_sram_setting(sram_addr, sram_size)
    ram_regions.set_psram_setting(psram_addr, psram_size)
    ram_regions.set_default_setting(defconfig)
    ram_regions.gen_memory_layout_hdr(ram_regions_hdr_file)


def main():
    logger.info("Enter Armino Auto Partition")
    project_info = bk_project_info()
    partitions_dir = project_info.get_project_build_path() / "partitions"
    auto_part_table = project_info.get_auto_partitions_table()
    ram_regions_table = project_info.get_ram_regions_table()
    flash_crc_enable = project_info.get_flash_crc_enable()
    auto_patitions(partitions_dir, auto_part_table, flash_crc_enable)
    ram_region_partition(partitions_dir, ram_regions_table)


if __name__ == "__main__":
    set_logging()
    main()
