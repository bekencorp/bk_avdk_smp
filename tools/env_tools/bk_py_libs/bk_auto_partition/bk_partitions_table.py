from __future__ import annotations

import json
import logging
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from bk_misc import check_overlaps, parse_format_size

from .bk_partition import PARTITION_ATTR_NUM, bk_partition

logger = logging.getLogger(__package__)


@dataclass
class partition_limit:
    name: str
    index: int


class bk_partitions_table:
    def __init__(self, csv_path: Path, crc_enable: bool = False) -> None:
        if not csv_path.exists():
            msg = f"auto partition config table {csv_path} not exist."
            raise FileNotFoundError(msg)

        logger.info(f"read parititon table from {csv_path}")
        self.crc_enable = crc_enable
        self._csv_path = csv_path
        self.partitions: list[bk_partition] = []
        self.cumulative_offset = 0
        # self.default_setting: list[partition_limit] = []
        self._parse_auto_partition_table()
        self._check_partition_valid()

    def _parser_partition_line(self, part_line: str) -> bk_partition:
        part_info = part_line.split(",")
        if len(part_info) < PARTITION_ATTR_NUM:
            msg = f"auto partition config table invalid, line:\n{part_line}"
            raise RuntimeError(msg)
        part_info = [item.strip() for item in part_info]
        name = part_info[0]
        offset_str = part_info[1]
        size_str = part_info[2]
        mode = part_info[3]
        read_str = part_info[4]
        write_str = part_info[5]

        offset = self.cumulative_offset if offset_str == "" else int(offset_str, 16)

        size = parse_format_size(size_str)

        if mode.lower() == "code":
            execute = True
        elif mode.lower() == "data":
            execute = False
        else:
            msg = f"not support type: {mode}"
            raise RuntimeError(msg)

        read = read_str.lower() == "true"
        write = write_str.lower() == "true"

        part = bk_partition(name, offset, size)
        part.chmod(write, read, execute)
        self.cumulative_offset = offset + size
        return part

    def _check_default_setting(self) -> None:
        pass

    def _check_partition_valid(self) -> None:
        self._check_offset_and_size_valid()
        self._check_partition_overlaps()

    def _check_offset_and_size_valid(self) -> None:
        def check_align(name: str, num: int, align_num: int) -> None:
            if num % align_num != 0:
                msg = f"{name} partition align error"
                raise RuntimeError(msg)

        for part in self.partitions:
            part_info = part.get_info()
            offset: int = part_info.Offset
            size: int = part_info.Size
            name: str = part_info.Name
            execute: int = part_info.Execute
            logger.debug(part_info)
            check_align(name, offset, 0x1000)
            check_align(name, size, 0x1000)
            if self.crc_enable and execute:
                check_align(name, offset, 1024 * 34)
                check_align(name, size, 1024 * 34)

    def _check_partition_overlaps(self) -> None:
        space_sections: list[tuple[int, int]] = []
        for part in self.partitions:
            offset, size = part.get_partition_size()
            space_sections.append((offset, size))
        if check_overlaps(space_sections):
            raise RuntimeError("partition table config overlaps")

    def _parse_auto_partition_table(self) -> None:
        def check_auto_partition_line_valid(part_line: str) -> None:
            ret = re.match(r"(?<!\\)\$([A-Za-z_][A-Za-z0-9_]*)", part_line)
            if ret:
                msg = f"auto partition table format error, line:\n{part_line}"
                raise RuntimeError(msg)

        csv_contents = self._csv_path.read_text()
        lines = csv_contents.splitlines()
        for line in lines:
            line_content = line.strip()
            if line_content.startswith("#") or len(line_content) == 0:
                continue
            check_auto_partition_line_valid(line_content)
            part = self._parser_partition_line(line_content)
            self.partitions.append(part)

    def gen_partition_csv(self, save_path: Path) -> None:
        logger.info(f"save partition csv to {save_path}")
        with save_path.open("w", newline="\n") as f:
            f.write("Name,Offset,Size,Execute,Read,Write\n")
            for part in self.partitions:
                f.write(part.get_format_info() + "\n")

    def gen_partition_json(self, save_path: Path) -> None:
        json_content: dict[str, Any] = {}
        table = [part.get_part_dict() for part in self.partitions]

        json_content.update({"crc_enable": self.crc_enable})
        json_content.update({"section": table})
        logger.info(f"save partition json to {save_path}")
        with save_path.open("w", newline="\n") as f:
            json.dump(json_content, f, indent=4)

    def gen_pretty_format_table(self, save_path: Path) -> None:
        text_content = ""
        text_content += bk_partition.get_pretty_format_info_head()
        offset = 0
        for part in self.partitions:
            addr, size = part.get_partition_size()
            if addr > offset:
                text_content += bk_partition.get_unused_part_info(offset, addr - offset)
            text_content += part.get_pretty_format_info()
            offset = addr + size

        with save_path.open("w", newline="\n") as f:
            f.write(text_content)

    def _check_partition_exists(self, part_name: str) -> bool:
        for partition in self.partitions:
            part_info = partition.get_info()
            if part_info.Name == part_name:
                return True
        return False

    def _check_partition_index(self, part_name: str, index: int) -> bool:
        if index < 0:
            index = len(self.partitions) + index
        for part_id, partition in enumerate(self.partitions):
            part_info = partition.get_info()
            if part_info.Name == part_name:
                return index == part_id
        return False

    def set_default_setting(self, default_setting: list[partition_limit]) -> None:
        for item in default_setting:
            if not self._check_partition_exists(item.name):
                raise RuntimeError(f"{item.name} is not exists")
            if not self._check_partition_index(item.name, item.index):
                raise RuntimeError(f"{item.name} index error")
