from typing import Callable

from bk_crc import bk_crc16

from .bk_packager import bk_packager
from .bk_packager_json import PartitionInfo


def pre_link(func: Callable[[bk_packager], None]):  # type: ignore
    def wrapper(self: bk_packager, *args, **kwargs):  # type: ignore
        for index, part_item in enumerate(self.part_info):
            crced_data = crc_handler.crc16_data(part_item.data)
            if len(crced_data) > part_item.size:
                raise RuntimeError(
                    f"partition {part_item.name} binary length over partition size after crc"
                )
            crced_part_item = PartitionInfo(
                part_item.addr, part_item.size, part_item.name, crced_data
            )
            self.part_info[index] = crced_part_item
        return func(self, *args, **kwargs)  # type: ignore

    crc_handler = bk_crc16()
    return wrapper  # type: ignore


def post_link(func: Callable[[bk_packager], None]):  # type: ignore
    def wrapper(self: bk_packager, *args, **kwargs):  # type: ignore
        # for ensure pre-read code length have 34 bytes at least.
        ret = func(self, *args, **kwargs)
        with self.output_file.open("ab") as f:
            f.write(bytes([0xFF] * 34))
        return ret

    return wrapper  # type: ignore
