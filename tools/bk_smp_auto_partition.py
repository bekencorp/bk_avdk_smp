import logging
import sys
from pathlib import Path

from bk_auto_partition import bk_partitions_table
from bk_bootloader_post import check_is_ab_project
from bk_flash_partiton import bk_flash_partition
from bk_ota_partition import bk_ota_partition

logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


class bk_part:
    def __init__(self, partitions_dir: Path, auto_part_table: Path, crc_enable: bool):
        if not partitions_dir.exists():
            raise RuntimeError(f"partitions directory not found: {partitions_dir}")
        if not auto_part_table.exists():
            raise FileNotFoundError(f"partitions_table not found: {auto_part_table}")

        self.partitions_dir = partitions_dir
        self.auto_part_table = auto_part_table
        self.partitions_json = Path()
        self.crc_enable = crc_enable

    def auto_partition(self, partitions_json: Path, partitions_txt: Path):
        partitions_csv = self.partitions_dir / "partitions.csv"
        part_table = bk_partitions_table(self.auto_part_table, self.crc_enable)
        part_table.gen_partition_csv(partitions_csv)
        part_table.gen_partition_json(partitions_json)
        part_table.gen_pretty_format_table(partitions_txt)
        self.partitions_json = partitions_json

    def gen_partition_header(self):
        flash_part = bk_flash_partition(self.partitions_json)
        header_path = self.partitions_dir / "partitions.h"
        flash_part.gen_partitions_layout_hdr(header_path)

    def gen_flash_partition_src(self):
        flash_part = bk_flash_partition(self.partitions_json)
        header_path = self.partitions_dir / "vendor_flash_partition.h"
        src_path = self.partitions_dir / "vendor_flash.c"
        flash_part.gen_flash_partitions_src(header_path, src_path)

    def gen_pack_config_json(self):
        flash_part = bk_flash_partition(self.partitions_json)
        pack_json = self.partitions_dir / "bk_package.json"
        flash_part.gen_pack_json(pack_json)


def main():
    logger.info("Enter SMP Auto Partition")
    partitions_dir = Path(sys.argv[1])
    auto_part_table = Path(sys.argv[2])
    partitions_json = partitions_dir / "partitions.json"
    partitions_txt = partitions_dir / "partitions.txt"
    crc_enable = True
    partitioner = bk_part(partitions_dir, auto_part_table, crc_enable)
    partitioner.auto_partition(partitions_json, partitions_txt)
    partitioner.gen_partition_header()
    partitioner.gen_flash_partition_src()
    partitioner.gen_pack_config_json()

    ota_partition_json = partitions_dir / "bk_ota_partitions.json"
    ota_partition = bk_ota_partition(partitions_json)
    if check_is_ab_project() == "True":
        ota_partition.gen_ab_ota_json(ota_partition_json)
        ota_partition.gen_ab_configuartion_json(partitions_dir / "configurationab.json")
    else:
        ota_partition.gen_ota_json(ota_partition_json)


if __name__ == "__main__":
    set_logging()
    main()
