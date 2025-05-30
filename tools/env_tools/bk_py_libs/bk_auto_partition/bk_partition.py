
class bk_partition:
    def __init__(self, name:str, offset:int, size:int):
        if size == 0 or size % 1024 != 0:
            raise ValueError(f"size vale = {size}, not valid.")
        self._name = name
        self._offset = offset
        self._size = size
        self._execute = False
        self._read = True
        self._write = True
    
    def chmod(self, write:bool, read:bool, execute:bool):
        self._execute = execute
        self._read = read
        self._write = write

    def get_info(self):
        part_info = {}
        part_info.update({"Name": self._name})
        part_info.update({"Offset": self._offset})
        part_info.update({"Size": self._size})
        part_info.update({"Execute": self._execute})
        part_info.update({"Read": self._read})
        part_info.update({"Write": self._write})
        return part_info
    
    def get_format_info(self):
        info = ""
        info += self._name + ","
        info += f"0x{self._offset:08x},"  # offfset
        size_kb = int(self._size/1024)
        info += f"{size_kb}K,"            # size
        info += str(self._execute) + ","
        info += str(self._read) + ","
        info += str(self._write)
        return info
    
    def get_partition_size(self):
        return self._offset, self._size
    
    @classmethod
    def get_pretty_format_info_head(cls):
        name =   "Name"
        offset = "Offset"
        size =   "Size"
        head = f"{name:<24}" + f"{offset:^16}" + f"{size:^12}" + "\n"
        head += '-' * (24 + 16 + 12) + "\n"
        return head

    @classmethod
    def get_partitions_pretty_info(cls, part_name, part_offset, part_size):
        info = ""
        info += f"{part_name:<24}"
        offset = f"0x{part_offset:08x}"
        info += f"{offset:^16}"             # offfset
        size_kb = f"{int(part_size/1024)}K"
        info += f"{size_kb:^12}"            # size
        info += '\n'
        return info

    def get_pretty_format_info(self):
        return self.get_partitions_pretty_info(self._name, self._offset, self._size)
    
    @classmethod
    def get_unused_part_info(cls, offset, size):
        name = "(unused)"
        return bk_partition.get_partitions_pretty_info(name, offset, size)
