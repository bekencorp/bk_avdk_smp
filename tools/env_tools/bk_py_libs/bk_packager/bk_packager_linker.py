from abc import ABC,abstractmethod
from typing import List
from .bk_packager_json import PartitionInfo

class bk_packager_linker(ABC):
    @abstractmethod
    def link(self, part_info:List[PartitionInfo], output_file_name):
        pass
