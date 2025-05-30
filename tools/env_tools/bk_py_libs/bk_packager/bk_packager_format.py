from .bk_packager import bk_packager
from .bk_packager_format_linker import bk_packager_format_linker
from . import logger

class bk_packager_format(bk_packager):
    def __init__(self, workdir, pack_json:str, output_file_name="all-app.bin"):
        linker = bk_packager_format_linker()
        super().__init__(workdir, pack_json, linker, output_file_name)
        logger.info(f"use {self.__class__.__name__} to pack {output_file_name}")

    def _pre_link(self):
        pass
    
    def _post_link(self):
        pass
