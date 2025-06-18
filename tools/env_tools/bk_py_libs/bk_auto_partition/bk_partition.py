from __future__ import annotations

from dataclasses import asdict, dataclass, fields
from typing import Any

OUTPUT_LINE_MAX_LEN = 60


@dataclass
class PartInfo:
    Name: str
    Offset: int
    Size: int
    Execute: bool
    Read: bool
    Write: bool


PARTITION_ATTR_NUM = len(fields(PartInfo))


class bk_partition:
    def __init__(self, name: str, offset: int, size: int) -> None:
        if size == 0 or size % 1024 != 0:
            msg = f"size vale = {size}, not valid."
            raise ValueError(msg)
        self._part_info = PartInfo(name, offset, size, False, True, True)

    def chmod(self, write: bool, read: bool, execute: bool) -> None:
        self._part_info.Execute = execute
        self._part_info.Read = read
        self._part_info.Write = write

    def get_info(self) -> PartInfo:
        return self._part_info

    def get_part_dict(self) -> dict[str, Any]:
        return asdict(self._part_info)

    def get_format_info(self) -> str:
        info = ""
        info += self._part_info.Name + ","
        info += f"0x{self._part_info.Offset:08x},"  # offfset
        size_kb = int(self._part_info.Size / 1024)
        info += f"{size_kb}K,"  # size
        info += str(self._part_info.Execute) + ","
        info += str(self._part_info.Read) + ","
        info += str(self._part_info.Write)
        return info

    def get_partition_size(self) -> tuple[int, int]:
        return self._part_info.Offset, self._part_info.Size

    @classmethod
    def get_pretty_format_info_head(cls):
        name = "Name"
        offset = "Offset"
        size = "Size"
        head = f"{name:<26}" + f"{offset:^22}" + f"{size:^12}" + "\n"
        head += f"{'':-^{OUTPUT_LINE_MAX_LEN}}\n"
        return head

    @classmethod
    def get_partitions_pretty_info(
        cls,
        part_name: str,
        part_offset: int,
        part_size: int,
    ):
        info = ""
        info += f"{part_name:<26}"  # 26
        offset = f"0x{part_offset:08x}"
        info += f"{offset:^22}"  # offfset # 22
        size_kb = f"{int(part_size / 1024)}K"
        info += f"{size_kb:^12}"  # size # 12
        info += "\n"
        return info

    def get_pretty_format_info(self) -> str:
        return self.get_partitions_pretty_info(
            self._part_info.Name, self._part_info.Offset, self._part_info.Size
        )

    @classmethod
    def get_unused_part_info(cls, offset: int, size: int):
        name = "(unused)"
        return bk_partition.get_partitions_pretty_info(name, offset, size)
