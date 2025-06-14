import re
from dataclasses import dataclass
from pathlib import Path

from . import logger


@dataclass
class mem_region:
    name: str
    type: str
    offset: int
    size: int


def parse_size(size_str: str) -> int:
    size_str = size_str.strip()
    if size_str.endswith("K"):
        return int(size_str[:-1]) * 1024
    elif size_str.endswith("M"):
        return int(size_str[:-1]) * 1024 * 1024
    else:
        return int(size_str)


class bk_ram_region:
    def __init__(self, ram_mem_csv: Path):
        self.ram_mem_csv = ram_mem_csv
        self.sram_base = 0
        self.sram_capacity = 0
        self.psram_base = 0
        self.psram_capacity = 0
        self.total_offset = 0
        self.sram_regions_num = 0
        self.psram_regions_num = 0
        self.regions: list[mem_region] = []
        self._gen_regions()
        self._check_region_valid()
        self._check_region_overlaps()

    def _gen_regions(self):
        if not self.ram_mem_csv.exists():
            raise RuntimeError(f"{self.ram_mem_csv} not exists")
        self._parse_ram_mem_csv()

    def _check_region_valid(self):
        for region in self.regions:
            if region.type == "SRAM":
                base = self.sram_base
                capacity = self.sram_capacity
                self.sram_regions_num += 1
            elif region.type == "PSRAM":
                base = self.psram_base
                capacity = self.psram_capacity
                self.psram_regions_num += 1
            else:
                raise RuntimeError(f"{region.type} is not supported")
            if region.offset < base:
                raise RuntimeError(
                    f"{region.name} addr is not valid, base  addr: 0x{base:08x}"
                )
            limit_addr = base + capacity
            if region.offset + region.size > base + capacity:
                msg = (
                    f"{region.name} is out of range, end addr: 0x{limit_addr:08x},"
                    + f"offset: 0x{region.offset:08x}, size: 0x{region.size:06x}"
                )
                raise RuntimeError(msg)

    def _check_region_overlaps(self):
        space_sections: list[tuple[int, int]] = []
        for region in self.regions:
            space_sections.append((region.offset, region.size))
        intervals = [(start, start + length) for start, length in space_sections]
        intervals.sort()
        for i in range(1, len(intervals)):
            if intervals[i][0] < intervals[i - 1][1]:
                msg = "partition table config overlaps"
                raise RuntimeError(msg)

    def _parse_ram_mem_csv(self):
        csv_contents = self.ram_mem_csv.read_text()
        lines = csv_contents.splitlines()

        for line in lines:
            line_content = line.strip()
            if "#SRAM_BASE_ADDR=" in line_content:
                self.sram_base = int(line_content.split("=")[1].strip(), 16)
                continue
            if "#SRAM_CAPCAITY_SIZE=" in line_content:
                self.sram_capacity = parse_size(line_content.split("=")[1])
                continue
            if "#PSRAM_BASE_ADDR=" in line_content:
                self.psram_base = int(line_content.split("=")[1].strip(), 16)
                continue
            if "#PSRAM_CAPCAITY_SIZE=" in line_content:
                self.psram_capacity = parse_size(line_content.split("=")[1])
                continue
            if line_content.startswith("#") or len(line_content) == 0:
                continue
            self._check_line_valid(line_content)
            self.regions.append(self._parse_line_mem_region(line_content))

    def _parse_line_mem_region(self, line_content: str) -> mem_region:
        region_content = line_content.split(",")
        offset_str = region_content[2].strip()
        if len(offset_str) == 0:
            offset = self.total_offset
        else:
            offset = int(offset_str, 16)
        size = int(region_content[3].strip(), 16)
        self.total_offset = offset + size
        return mem_region(
            name=region_content[0].strip(),
            type=region_content[1].strip(),
            offset=offset,
            size=size,
        )

    @staticmethod
    def _check_line_valid(line_content: str) -> None:
        ret = re.match(r"(?<!\\)\$([A-Za-z_][A-Za-z0-9_]*)", line_content)
        if ret:
            msg = f"auto partition table format error, line:\n{line_content}"
            raise RuntimeError(msg)

    def gen_memory_layout_hdr(self, hdr_file: Path) -> None:
        with hdr_file.open("w", newline="\n") as f:
            f.write(self._get_region_hdr_text())
        logger.info(f"generate ram region header file: {hdr_file}")

    def _get_region_hdr_text(self) -> str:
        hdr_text = ""
        hdr_text += "#pragma once\n"
        if self.sram_regions_num:
            hdr_text += f"#define {'CONFIG_SRAM_BASE':<36} 0x{self.sram_base:08X}\n"
            hdr_text += (
                f"#define {'CONFIG_SRAM_CAPACITY':<36} 0x{self.sram_capacity:08X}\n"
            )
        if self.psram_regions_num:
            hdr_text += f"#define {'CONFIG_PSRAM_BASE':<36} 0x{self.psram_base:08X}\n"
            hdr_text += (
                f"#define {'CONFIG_PSRAM_CAPACITY':<36} 0x{self.psram_capacity:08X}\n"
            )
        for region in self.regions:
            name_addr = f"CONFIG_{region.name.upper()}_ADDR"
            name_size = f"CONFIG_{region.name.upper()}_SIZE"
            hdr_text += f"#define {name_addr:<36} 0x{region.offset:08X}\n"
            hdr_text += f"#define {name_size:<36} 0x{region.size:08X}\n"
        return hdr_text
