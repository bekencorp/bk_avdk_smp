import os
import re
import json
from typing import List
from .bk_partition import bk_partition
from . import logger

def parse_size(size_str:str):
    for letter, multiplier in [("k", 1024), ("m", 1024 * 1024)]:
        if size_str.lower().endswith(letter):
            return parse_size(size_str[:-1]) * multiplier
    return int(size_str, 0)

class bk_partitions_table:
    def __init__(self, csv_path, crc_enable=False):
        if not os.path.exists(csv_path):
            raise FileNotFoundError(f"auto partition config table {csv_path} not exist.")
        
        logger.info(f"read parititon table from {csv_path}")
        self.crc_enable = crc_enable
        self._csv_path = csv_path
        self.partitions:List[bk_partition] = []
        self.cumulative_offset = 0
        self._parse_auto_partition_table()
        self._check_partition_valid()

    def _parser_partition_line(self, part_line:str):
        part_info = part_line.split(',')
        if len(part_info) < 6:
            raise RuntimeError(f"auto partition config table invalid, line:\n{part_line}")
        part_info = [item.strip() for item in part_info]
        name = part_info[0]
        offset_str = part_info[1]
        size_str = part_info[2]
        mode = part_info[3]
        read_str = part_info[4]
        write_str = part_info[5]

        if offset_str == '':
            offset = self.cumulative_offset
        else:
            offset = int(offset_str, 16)
        
        size = parse_size(size_str)

        if mode.lower() == 'code':
            execute = True
        elif mode.lower() == 'data':
            execute = False
        else:
            raise RuntimeError(f"not support type: {mode}")

        read = True if read_str.lower() == 'true' else False
        write = True if write_str.lower() == 'true' else False

        part = bk_partition(name, offset, size)
        part.chmod(write, read, execute)
        self.cumulative_offset = offset + size
        return part

    def _check_partition_valid(self):
        self._check_offset_and_size_valid()
        self._check_partition_overlaps()
    
    def _check_offset_and_size_valid(self):
        def check_align(name, num, align_num):
            if num % align_num != 0:
                raise RuntimeError(f"{name} partition align error")

        for part in self.partitions:
            part_info = part.get_info()
            offset = part_info['Offset']
            size = part_info['Size']
            name = part_info['Name']
            execute = part_info['Execute']
            logger.debug(part_info)
            check_align(name, offset, 0x1000)
            check_align(name, size, 0x1000)
            if self.crc_enable and execute:
                check_align(name, offset, 1024 * 34)
                check_align(name, size, 1024 * 34)

    def _check_partition_overlaps(self):          
        space_sections = []
        for part in self.partitions:
            offset, size = part.get_partition_size()
            space_sections.append((offset, size))
        intervals = [(start, start + length) for start, length in space_sections]
        intervals.sort()
        for i in range(1, len(intervals)):
            if intervals[i][0] < intervals[i - 1][1]:
                raise RuntimeError("partition table config overlaps")

    def _parse_auto_partition_table(self):
        def check_auto_partition_line_valid(part_line):
            ret = re.match(r'(?<!\\)\$([A-Za-z_][A-Za-z0-9_]*)', part_line)
            if ret:
                raise RuntimeError(f"auto partition table format error, line:\n{part_line}")
        
        with open(self._csv_path, 'r') as f:
            csv_contents = f.read()
        lines = csv_contents.splitlines()
        for line in lines:
            line = line.strip()
            if line.startswith("#") or len(line) == 0:
                continue
            check_auto_partition_line_valid(line)
            part = self._parser_partition_line(line)
            self.partitions.append(part)

    def gen_partition_csv(self, save_path):
        logger.info(f"save partition csv to {save_path}")
        with open(save_path, 'w', newline='\n') as f:
            f.write("Name,Offset,Size,Execute,Read,Write\n")
            for part in self.partitions:
                f.write(part.get_format_info() + '\n')

    def gen_partition_json(self, save_path):
        json_content = {}
        table = []
        for part in self.partitions:
            table.append(part.get_info())

        json_content.update({"crc_enable": self.crc_enable})
        json_content.update({"section": table})
        logger.info(f"save partition json to {save_path}")
        with open(save_path, 'w', newline='\n') as f:
            json.dump(json_content, f, indent=4)

    def gen_pretty_format_table(self, save_path):
        text_content = ""
        text_content += bk_partition.get_pretty_format_info_head()
        offset = 0
        for part in self.partitions:
            addr, size = part.get_partition_size()
            if addr > offset:
                text_content += bk_partition.get_unused_part_info(offset, addr - offset)
            text_content += part.get_pretty_format_info()
            offset = addr + size
        with open(save_path, 'w') as f:
            f.write(text_content)