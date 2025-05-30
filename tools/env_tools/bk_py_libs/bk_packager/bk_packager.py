import os
from abc import ABC,abstractmethod
from .bk_packager_json import parse_packager_json
from .bk_packager_linker import bk_packager_linker
from . import logger

class bk_packager(ABC):
    """ beken packager """
    def __init__(self, workdir, pack_json:str, linker:bk_packager_linker,
                       output_file_name="all-app.bin"):
        logger.info(f"bk_partition workdir: {workdir}")
        logger.info(f"load pack json: {pack_json}")
        self.workdir = os.path.abspath(workdir)

        # check workdir
        if (not os.path.exists(self.workdir)) or (not os.path.isdir(self.workdir)):
            raise RuntimeError(f"work directory {self.workdir} not exist.")

        self.part_info = parse_packager_json(workdir, pack_json)
        self.output_file = output_file_name
        if not os.path.isabs(output_file_name):
            self.output_file = os.path.join(self.workdir, output_file_name)

        self.linker = linker

    def pack(self):
        self._pre_link()
        self.linker.link(self.part_info, self.output_file)
        self._post_link()

    @abstractmethod
    def _pre_link(self):
        pass

    @abstractmethod
    def _post_link(self):
        pass
