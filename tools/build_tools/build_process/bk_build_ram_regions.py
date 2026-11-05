from __future__ import annotations

import json
import logging
import os
from pathlib import Path

from bk_misc import parse_format_size
from bk_ram_region import bk_ram_region, mem_region
from bk_sdk.bk_curr_project import curr_project

logger = logging.getLogger(Path(__file__).name)

PSRAM_MPU_ATTRS = {
    "NON_CACHE": 1,
    "L1_L2_WRITE_BACK": 3,
    "L2_WRITE_BACK": 5,
}


def _parse_psram_mpu_attr(region_name: str, value: object) -> int:
    if isinstance(value, int):
        return value
    if isinstance(value, str) and value in PSRAM_MPU_ATTRS:
        return PSRAM_MPU_ATTRS[value]
    raise RuntimeError(
        f"{region_name} has invalid PSRAM MPU policy {value!r}; "
        f"expected one of {', '.join(PSRAM_MPU_ATTRS)}"
    )


def _config_has_psram_interleave(config_dir: Path) -> bool:
    for name in ("config", "defconfig"):
        cfg = config_dir / name
        if cfg.exists():
            try:
                text = cfg.read_text()
                if "CONFIG_PSRAM_INTERLEAVE=y" in text:
                    return True
            except (OSError, ValueError):
                pass
    return False


def _is_psram_interleave_configured() -> bool:
    """Return True if either core enables CONFIG_PSRAM_INTERLEAVE (for bk7259).

    Interleave needs both PSRAM dies, so it conflicts with the single-die
    layout whichever core enables it. Check AP and CP, both the project
    defconfig and the generated build config, so a stale project defconfig or
    a board-level default cannot hide the conflict.
    """
    if curr_project.soc_name != "bk7259":
        return False
    soc = curr_project.soc_name
    project_path = curr_project.project_path
    build_dir = curr_project.project_build_dir
    config_dirs = [
        project_path / "ap" / "config" / f"{soc}_ap",
        project_path / "cp" / "config" / f"{soc}",
        build_dir / f"{soc}_ap" / "config",
        build_dir / f"{soc}" / "config",
    ]
    return any(_config_has_psram_interleave(d) for d in config_dirs)


def _is_single_psram_layout() -> bool:
    """True when the build selects the single-die layout (PSRAM_LAYOUT=single)."""
    return os.getenv("PSRAM_LAYOUT", "dual").strip().lower() == "single"


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
    single_layout = _is_single_psram_layout()
    interleave = _is_psram_interleave_configured()
    if single_layout and interleave:
        raise RuntimeError(
            "PSRAM_LAYOUT=single conflicts with CONFIG_PSRAM_INTERLEAVE=y: "
            "interleave needs two PSRAM dies, but the single-die layout leaves "
            "the PSRAM0 window unmapped. Disable CONFIG_PSRAM_INTERLEAVE, or "
            "build with PSRAM_LAYOUT=dual."
        )
    ram_regions.set_single_psram_window(single_layout)
    if interleave:
        ram_regions.set_psram_interleave(True)
        logger.info("PSRAM interleave enabled: ram_regions.h will use 0x80/0x81 address space")
    if soc_name != "bk7259":
        ram_regions.gen_memory_layout_hdr(ram_regions_hdr_file)
        return

    policies = {
        name: _parse_psram_mpu_attr(name, attr)
        for name, attr in def_config.get("PSRAM_MPU_POLICIES", {}).items()
    }
    overrides: dict[str, object] = {}
    policy_override = ram_regions_table.with_name(
        f"{ram_regions_table.stem}_mpu.json"
    )
    if policy_override.exists():
        with policy_override.open("r") as f:
            overrides = json.load(f)
        if not isinstance(overrides, dict):
            raise RuntimeError(f"{policy_override} must contain a JSON object")
        policies.update(
            {
                name: _parse_psram_mpu_attr(name, attr)
                for name, attr in overrides.items()
            }
        )
        logger.info(f"apply project PSRAM MPU policies: {policy_override}")
    default_attr = _parse_psram_mpu_attr(
        "PSRAM_MPU_DEFAULT_ATTR",
        def_config.get("PSRAM_MPU_DEFAULT_ATTR", 1),
    )

    tmp_hdr = ram_regions_hdr_file.with_suffix(".h.tmp")
    try:
        ram_regions.gen_memory_layout_hdr(tmp_hdr)
        psram_names = {
            region.name for region in ram_regions.regions if region.type == "PSRAM"
        }
        unknown_overrides = sorted(set(overrides) - psram_names)
        if unknown_overrides:
            raise RuntimeError(
                f"unknown PSRAM MPU policy regions: {', '.join(unknown_overrides)}"
            )
        ram_regions.append_psram_mpu_regions_hdr(
            tmp_hdr,
            policies,
            default_attr,
        )
        tmp_hdr.replace(ram_regions_hdr_file)
    finally:
        tmp_hdr.unlink(missing_ok=True)


def main():
    logger.info("Enter Armino Ram Regions Parititons")
    build_partitions_dir = curr_project.project_build_parititons_dir
    ram_regions_table = curr_project.ram_regions_table
    ram_region_partition(build_partitions_dir, ram_regions_table)


if __name__ == "__main__":
    set_logging()
    main()
