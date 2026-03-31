from __future__ import annotations

import json
import logging
from pathlib import Path

from bk_misc import parse_format_size
from bk_ram_region import bk_ram_region, mem_region
from bk_sdk.bk_curr_project import curr_project

logger = logging.getLogger(Path(__file__).name)


def _is_psram_interleave_enabled() -> bool:
    """Read project AP config and return True if CONFIG_PSRAM_INTERLEAVE=y (for bk7259)."""
    if curr_project.soc_name != "bk7259":
        return False
    ap_config_dir = curr_project.project_path / "ap" / "config" / f"{curr_project.soc_name}_ap"
    for name in ("config", "defconfig"):
        cfg = ap_config_dir / name
        if cfg.exists():
            try:
                text = cfg.read_text()
                if "CONFIG_PSRAM_INTERLEAVE=y" in text:
                    return True
            except (OSError, ValueError):
                pass
    return False


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def ram_region_partition(partitions_dir: Path, ram_regions_table: Path):
    soc_name = curr_project.soc_name
    ram_regions = bk_ram_region(ram_regions_table, soc_name)
    ram_regions_hdr_file = partitions_dir / "ram_regions.h"
    bk_default_config = curr_project.ram_regions_setting
    with bk_default_config.open("r") as f:
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
    if _is_psram_interleave_enabled():
        ram_regions.set_psram_interleave(True)
        logger.info("PSRAM interleave enabled: ram_regions.h will use 0x80/0x81 address space")
    ram_regions.gen_memory_layout_hdr(ram_regions_hdr_file)


def main():
    logger.info("Enter Armino Ram Regions Parititons")
    build_partitions_dir = curr_project.project_build_parititons_dir
    ram_regions_table = curr_project.ram_regions_table
    ram_region_partition(build_partitions_dir, ram_regions_table)


if __name__ == "__main__":
    set_logging()
    main()
