from __future__ import annotations

import logging
import re
from dataclasses import dataclass
from pathlib import Path

from bk_misc import check_overlaps, parse_format_size

logger = logging.getLogger(__package__)

# BK7259 PSRAM interleaved view: PSRAM0 at 0x80000000, PSRAM1 at 0x81000000
PSRAM_INTERLEAVE_OFFSET_PSRAM0 = 0x20000000  # 0x60000000 -> 0x80000000
PSRAM_INTERLEAVE_OFFSET_PSRAM1 = 0x1D000000  # 0x64000000 -> 0x81000000
PSRAM_INTERLEAVE_BASE = 0x80000000
PSRAM_NON_INTERLEAVE_PSRAM1_BASE = 0x64000000
PSRAM_NON_INTERLEAVE_PSRAM0_BASE = 0x60000000
SECURE_REGION_ALIGNMENT = 0x1000
PSRAM_NS_ADDR_DIFF = 0x10000000
MPU_MIN_REGION_ALIGNMENT = 32


@dataclass
class mem_region:
    name: str
    type: str
    offset: int
    size: int


class bk_ram_region:
    def __init__(self, ram_mem_csv: Path, soc_name: str | None = None) -> None:
        self.ram_mem_csv = ram_mem_csv
        self.soc_name = soc_name
        self.sram_base = 0
        self.sram_capacity = 0
        self.psram_base = 0
        self.psram_capacity = 0
        self.psram_interleave = False
        self.total_offset = 0
        self.sram_regions_num = 0
        self.psram_regions_num = 0
        self.regions: list[mem_region] = []
        self.default_setting: list[mem_region] = []

    def set_default_setting(self, default_setting: list[mem_region]) -> None:
        self.default_setting = default_setting

    def set_sram_setting(self, sram_base: int, sram_capacity: int) -> None:
        self.sram_base = sram_base
        self.sram_capacity = sram_capacity

    def set_psram_setting(self, psram_base: int, psram_capacity: int) -> None:
        self.psram_base = psram_base
        self.psram_capacity = psram_capacity

    def set_psram_interleave(self, enable: bool) -> None:
        """When True and soc is bk7259, PSRAM region addrs are remapped to interleaved view (0x80/0x81)."""
        self.psram_interleave = enable

    def _apply_psram_interleave_remap(self) -> None:
        """Remap PSRAM region offsets to BK7259 interleaved address space."""
        if not self.psram_interleave or self.soc_name != "bk7259":
            return
        self.psram_base = PSRAM_INTERLEAVE_BASE
        for r in self.regions:
            if r.type != "PSRAM":
                continue
            offset = r.offset
            ns_alias = False
            for base in (
                PSRAM_NON_INTERLEAVE_PSRAM0_BASE,
                PSRAM_NON_INTERLEAVE_PSRAM1_BASE,
            ):
                ns_base = base + PSRAM_NS_ADDR_DIFF
                if ns_base <= offset < ns_base + self.psram_capacity:
                    offset -= PSRAM_NS_ADDR_DIFF
                    ns_alias = True
                    break
            if offset >= PSRAM_NON_INTERLEAVE_PSRAM1_BASE:
                offset += PSRAM_INTERLEAVE_OFFSET_PSRAM1
            elif offset >= PSRAM_NON_INTERLEAVE_PSRAM0_BASE:
                offset += PSRAM_INTERLEAVE_OFFSET_PSRAM0
            if ns_alias:
                offset += PSRAM_NS_ADDR_DIFF
            r.offset = offset

    def _gen_regions(self) -> None:
        if not self.ram_mem_csv.exists():
            raise RuntimeError(f"{self.ram_mem_csv} not exists")
        self._parse_ram_mem_csv()

    def _check_default_setting(self) -> None:
        # Only check default setting for bk7259
        if self.soc_name not in (None, "bk7259"):
            return

        def find_region(def_region_name: str) -> int:
            for id, region in enumerate(self.regions):
                if region.name == def_region_name:
                    return id
            return -1

        for def_item in self.default_setting:
            region_id = find_region(def_item.name)
            if region_id == -1:
                msg = f"default region {def_item.name} not found"
                raise RuntimeError(msg)
            region = self.regions[region_id]
            if def_item.type != region.type:
                msg = f"{region.name} type is not valid, default type: {def_item.type}"
                raise RuntimeError(msg)
            if def_item.offset != region.offset:
                msg = f"{region.name} addr is not valid, default addr: 0x{def_item.offset:08x}"
                raise RuntimeError(msg)
            if def_item.size != region.size:
                msg = f"{region.name} size is not valid, default size: 0x{def_item.size:08x}"
                raise RuntimeError(msg)

    def _check_region_valid(self) -> None:
        for region in self.regions:
            if region.type == "SRAM":
                base = self.sram_base
                capacity = self.sram_capacity
                self.sram_regions_num += 1
            elif region.type == "PSRAM":
                base = self.psram_base
                capacity = self.psram_capacity
                self.psram_regions_num += 1
            elif region.type == "SECURE":
                base = self.sram_base
                capacity = self.sram_capacity
                if (region.offset % SECURE_REGION_ALIGNMENT) != 0:
                    raise RuntimeError(
                        f"{region.name} addr is not {SECURE_REGION_ALIGNMENT:#x} aligned"
                    )
                if (region.size == 0) or (
                    (region.size % SECURE_REGION_ALIGNMENT) != 0
                ):
                    raise RuntimeError(
                        f"{region.name} size is not {SECURE_REGION_ALIGNMENT:#x} aligned"
                    )
            else:
                raise RuntimeError(f"{region.type} is not supported")
            if region.offset < base:
                raise RuntimeError(
                    f"{region.name} addr is not valid, base  addr: 0x{base:08x}"
                )
            limit_addr = base + capacity
            # if region.offset + region.size > base + capacity:
            #     msg = (
            #         f"{region.name} is out of range, end addr: 0x{limit_addr:08x},"
            #         + f"offset: 0x{region.offset:08x}, size: 0x{region.size:06x}, {capacity=}"
            #     )
            #     raise RuntimeError(msg)

    def _check_region_overlaps(self):
        space_sections: list[tuple[int, int]] = []
        for region in self.regions:
            space_sections.append((region.offset, region.size))
        # if check_overlaps(space_sections):
        #     raise RuntimeError("RAM regions config overlaps")

    def _parse_ram_mem_csv(self):
        csv_contents = self.ram_mem_csv.read_text(encoding="utf-8")
        lines = csv_contents.splitlines()

        for line in lines:
            line_content = line.strip()
            if line_content.startswith("#") or len(line_content) == 0:
                continue
            # Single PSRAM chip size (e.g. 8M, 16M). Header CONFIG_PSRAM_CAPACITY uses this value.
            if "SINGLE_PSRAM_CAPACITY_SIZE=" in line_content:
                self.psram_capacity = parse_format_size(line_content.split("=")[1])
                continue
            if "PSRAM_CAPACITY_SIZE=" in line_content:
                self.psram_capacity = parse_format_size(line_content.split("=")[1])
                continue
            self._check_line_valid(line_content)
            self.regions.append(self._parse_line_mem_region(line_content))

    def _parse_line_mem_region(self, line_content: str) -> mem_region:
        region_content = line_content.split(",")
        region_type = region_content[1].strip()
        offset_str = region_content[2].strip()
        if len(offset_str) == 0:
            if region_type == "SECURE":
                raise RuntimeError("SECURE region requires an explicit offset")
            offset = self.total_offset
        else:
            offset = int(offset_str, 16)
        size = int(region_content[3].strip(), 16)
        if region_type != "SECURE":
            self.total_offset = offset + size
        return mem_region(
            name=region_content[0].strip(),
            type=region_type,
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
        self._gen_regions()
        self._apply_psram_interleave_remap()
        self._check_default_setting()
        self._check_region_valid()
        self._check_region_overlaps()
        with hdr_file.open("w", newline="\n") as f:
            f.write(self._get_region_hdr_text())
        logger.info(f"generate ram region header file: {hdr_file}")

    def _psram_bank_ranges(self) -> tuple[tuple[int, int], tuple[int, int]]:
        if self.soc_name != "bk7259":
            raise RuntimeError("PSRAM MPU region generation is only supported for bk7259")
        if self.psram_interleave:
            return (
                (PSRAM_INTERLEAVE_BASE, 0x81000000),
                (0x81000000, 0x82000000),
            )
        return (
            (PSRAM_NON_INTERLEAVE_PSRAM0_BASE, PSRAM_NON_INTERLEAVE_PSRAM1_BASE),
            (PSRAM_NON_INTERLEAVE_PSRAM1_BASE, 0x68000000),
        )

    def _canonical_psram_addr(self, addr: int, size: int) -> int:
        """Return the secure/canonical alias for a BK7259 PSRAM interval."""
        for base, bank_end in self._psram_bank_ranges():
            if base <= addr and addr + size <= bank_end:
                return addr
            ns_base = base + PSRAM_NS_ADDR_DIFF
            if ns_base <= addr and addr + size <= bank_end + PSRAM_NS_ADDR_DIFF:
                return addr - PSRAM_NS_ADDR_DIFF
        raise RuntimeError(
            f"PSRAM region [0x{addr:08x}, 0x{addr + size:08x}) is outside BK7259 PSRAM banks"
        )

    def _build_psram_mpu_segments(
        self, policies: dict[str, int], default_attr: int
    ) -> list[tuple[int, int, int, list[str]]]:
        if not self.regions:
            raise RuntimeError("RAM regions must be generated before PSRAM MPU regions")
        valid_attrs = {1, 3, 5}
        if default_attr not in valid_attrs:
            raise RuntimeError(f"invalid default MPU attribute index: {default_attr}")

        explicit: list[tuple[int, int, int, str]] = []
        for region in self.regions:
            if region.type != "PSRAM" or region.size == 0:
                continue
            attr = policies.get(region.name, default_attr)
            if not isinstance(attr, int) or attr not in valid_attrs:
                raise RuntimeError(f"{region.name} has invalid MPU attribute index: {attr}")
            start = self._canonical_psram_addr(region.offset, region.size)
            explicit.append((start, start + region.size, attr, region.name))

        explicit.sort(key=lambda item: item[0])
        for previous, current in zip(explicit, explicit[1:]):
            if current[0] < previous[1]:
                raise RuntimeError(
                    f"PSRAM regions overlap after alias normalization: "
                    f"{previous[3]} and {current[3]}"
                )

        segments: list[tuple[int, int, int, list[str]]] = []
        for bank_base, bank_aperture_end in self._psram_bank_ranges():
            cursor = bank_base
            bank_regions = [
                item for item in explicit if bank_base <= item[0] < bank_aperture_end
            ]
            configured_end = bank_base + self.psram_capacity
            bank_end = max(
                configured_end,
                max((item[1] for item in bank_regions), default=configured_end),
            )
            bank_end = (
                bank_end + MPU_MIN_REGION_ALIGNMENT - 1
            ) & ~(MPU_MIN_REGION_ALIGNMENT - 1)
            if bank_end > bank_aperture_end:
                raise RuntimeError(
                    f"configured PSRAM bank end 0x{bank_end:08x} exceeds "
                    f"aperture end 0x{bank_aperture_end:08x}"
                )
            for start, end, attr, name in bank_regions:
                if cursor < start:
                    segments.append((cursor, start, default_attr, ["reserved"]))
                segments.append((start, end, attr, [name]))
                cursor = end
            if cursor < bank_end:
                tail_attr = (
                    bank_regions[-1][2]
                    if bank_regions
                    and bank_end - cursor < MPU_MIN_REGION_ALIGNMENT
                    else default_attr
                )
                segments.append((cursor, bank_end, tail_attr, ["reserved"]))

        merged: list[tuple[int, int, int, list[str]]] = []
        for start, end, attr, names in segments:
            if merged and merged[-1][1] == start and merged[-1][2] == attr:
                prev_start, _, prev_attr, prev_names = merged[-1]
                merged[-1] = (prev_start, end, prev_attr, prev_names + names)
            else:
                merged.append((start, end, attr, names))
        for start, end, _, names in merged:
            if start % MPU_MIN_REGION_ALIGNMENT or end % MPU_MIN_REGION_ALIGNMENT:
                logger.warning(
                    "MPU rounds %s [0x%08x, 0x%08x) to 32-byte boundaries",
                    ", ".join(names),
                    start,
                    end,
                )
        return merged

    def append_psram_mpu_regions_hdr(
        self,
        hdr_file: Path,
        policies: dict[str, int],
        default_attr: int = 1,
    ) -> None:
        """Append named PSRAM MPU region macros to ram_regions.h."""
        segments = self._build_psram_mpu_segments(policies, default_attr)
        if len(segments) > 4:
            raise RuntimeError(
                f"PSRAM MPU requires {len(segments)} regions, but BK7259 has only "
                "four MPU slots available for PSRAM"
            )
        lines = [
            "",
            "/* Auto-generated PSRAM MPU configuration. Do not edit. */",
            f"#define CONFIG_PSRAM_MPU_REGION_COUNT {len(segments)}",
        ]
        for index, (start, end, attr, names) in enumerate(segments):
            lines.append(f"/* {', '.join(names)} */")
            limit = end - MPU_MIN_REGION_ALIGNMENT
            prefix = f"CONFIG_PSRAM_MPU_REGION_{index}"
            ns_prefix = f"CONFIG_PSRAM_MPU_NS_REGION_{index}"
            lines.extend(
                [
                    f"#define {prefix + '_BASE':<40} 0x{start:08X}UL",
                    f"#define {prefix + '_LIMIT':<40} 0x{limit:08X}UL",
                    f"#define {prefix + '_ATTR':<40} {attr}",
                    f"#define {ns_prefix + '_BASE':<40} "
                    f"0x{start + PSRAM_NS_ADDR_DIFF:08X}UL",
                    f"#define {ns_prefix + '_LIMIT':<40} "
                    f"0x{limit + PSRAM_NS_ADDR_DIFF:08X}UL",
                ]
            )
        with hdr_file.open("a", newline="\n") as f:
            f.write("\n".join(lines) + "\n")
        logger.info(f"append PSRAM MPU region macros to: {hdr_file}")

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
