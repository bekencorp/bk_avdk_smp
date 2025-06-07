from __future__ import annotations

import json
from pathlib import Path
from typing import NamedTuple

from . import logger


class PartitionInfo(NamedTuple):
    addr: int
    size: int
    name: str
    data: bytes


def check_overlaps(spaces: list[tuple[int, int]]):
    intervals = [(start, start + length) for start, length in spaces]
    intervals.sort()
    for i in range(1, len(intervals)):
        if intervals[i][0] < intervals[i - 1][1]:
            return True

    return False


def parse_size(size_str: str) -> int:
    for letter, multiplier in [("k", 1024), ("m", 1024 * 1024)]:
        if size_str.lower().endswith(letter):
            return parse_size(size_str[:-1]) * multiplier
    return int(size_str, 0)


class bk_packager_json:
    def __init__(self, workdir: Path, pack_json: Path):
        self.workdir = workdir.absolute()
        self.pack_json = pack_json.absolute()
        logger.debug(f"pack_json:{self.pack_json}")
        # check workdir
        if (not self.workdir.exists()) or (not self.workdir.is_dir()):
            raise RuntimeError(f"work directory {self.workdir} not exist.")

        # check json exist
        if (not self.pack_json.exists()) or (not self.pack_json.is_file()):
            raise RuntimeError(f"config json {self.pack_json} not exist.")

        # check json valid
        with self.pack_json.open("r", encoding="utf-8") as f:
            config_json = json.load(f)

        self.section = config_json["section"]
        self.partitions: list[PartitionInfo] = []
        self._parse_and_check_partitions()

    def get_partitions(self) -> list[PartitionInfo]:
        return self.partitions

    def _parse_and_check_partitions(self):
        space_sections: list[tuple[int, int]] = []
        for part in self.section:
            bin_name: str = part["firmware"]
            part_addr = int(part["start_addr"], 16)
            part_size = parse_size(part["size"])
            part_name = part["partition"]
            bin_path = self.workdir / bin_name
            if (not bin_path.exists()) or (not bin_path.is_file()):
                raise RuntimeError(f"{bin_path} not exist!")
            self._check_binary_size_valid(bin_path, part_size)
            bin_content = bin_path.read_bytes()
            part_info = PartitionInfo(part_addr, part_size, part_name, bin_content)
            self.partitions.append(part_info)
            space_sections.append((part_addr, part_size))

        if check_overlaps(space_sections):
            raise RuntimeError("partition exist overlaps!")

        self.partitions.sort()

    def _check_binary_size_valid(self, bin_path: Path, part_size: int):
        bin_size = bin_path.stat().st_size
        if bin_size > part_size:
            raise RuntimeError(f"{bin_path} size is over partitions size.")


def parse_packager_json(workdir: Path, pack_json: Path) -> list[PartitionInfo]:
    pack_json_parser = bk_packager_json(workdir, pack_json)
    return pack_json_parser.get_partitions()
