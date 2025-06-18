import logging
import os
import shutil
from pathlib import Path

from bk_bootloader_post import get_bootloader_archieve_dir
from bk_serialize_partitions_table import serialize_partitions_table

logger = logging.getLogger(Path(__file__).name)


class bk_project_info:
    def __init__(self) -> None:
        self.soc_name: str = os.getenv("ARMINO_SOC", "")
        self.project_name: str = os.getenv("PROJECT", "")
        self._project_dir = os.getenv("PROJECT_DIR", "")
        self._project_build_dir = os.getenv("PROJECT_BUILD_DIR", "")
        self.project_path = Path(self._project_dir)
        self.project_build_path = Path(self._project_build_dir)
        self.flash_crc_enable = True
        self.sdk_info = bk_sdk_info()

    def _check_project_info(self):
        if not self.soc_name:
            raise RuntimeError("get soc name error")
        if not self.project_name:
            raise RuntimeError("get project name error")
        if not self._project_dir:
            raise RuntimeError("get project dir error")
        if not self._project_build_dir:
            raise RuntimeError("get project build dir error")
        if not self.project_path.is_dir():
            raise RuntimeError(f"project path {self.project_path} not exist")
        if not self.project_build_path.is_dir():
            raise RuntimeError(
                f"project build path {self.project_build_path} not exist"
            )

    def get_project_name(self) -> str:
        return self.project_name

    def get_project_path(self) -> Path:
        return self.project_path

    def get_project_build_path(self) -> Path:
        return self.project_build_path

    def get_soc_name(self) -> str:
        return self.soc_name

    def get_auto_partitions_table(self) -> Path:
        table_path = self.sdk_info.get_auto_partitions_table(self)
        if not table_path.exists():
            raise RuntimeError(f"auto partitions table {table_path} not exists.")
        return table_path

    def get_ram_regions_table(self) -> Path:
        table_path = self.sdk_info.get_ram_regions_table(self)
        if not table_path.exists():
            raise RuntimeError(f"ram regions table {table_path} not exists.")
        return table_path

    def get_flash_crc_enable(self) -> bool:
        return self.flash_crc_enable

    def prepare_apps_bin_to_pack_dir(self, pack_dir: Path) -> None:
        self.sdk_info.handle_bootloader_bin(self, pack_dir)
        self.sdk_info.copy_app_bin_to_pack_dir(self, pack_dir)


class bk_sdk_info:
    def __init__(self) -> None:
        self.sdk_name = "bk_avdk_smp"

    def get_sdk_name(self) -> str:
        return self.sdk_name

    def get_auto_partitions_table(self, project_info: bk_project_info):
        soc_name = project_info.get_soc_name()
        project_path = project_info.get_project_path()
        table_path = project_path / f"partitions/{soc_name}/auto_partitions.csv"

        return table_path

    def get_ram_regions_table(self, project_info: bk_project_info):
        soc_name = project_info.get_soc_name()
        project_path = project_info.get_project_path()
        table_path = project_path / f"partitions/{soc_name}/ram_regions.csv"

        return table_path

    def handle_bootloader_bin(self, project_info: bk_project_info, pack_dir: Path):
        project_build_path = project_info.get_project_build_path()
        if not pack_dir.exists():
            pack_dir.mkdir()
        # copy bootloader cp ap binary
        bootloader_name = "bootloader.bin"
        bootloader_dir = Path(get_bootloader_archieve_dir())
        origin_bootloader_path = bootloader_dir / bootloader_name
        pack_bootloader_path = pack_dir / bootloader_name
        ota_json = project_build_path / "partitions/bk_ota_partitions.json"
        part_bytes = serialize_partitions_table(ota_json)
        shutil.copy(origin_bootloader_path, pack_bootloader_path)
        with pack_bootloader_path.open("ab") as f:
            pos = f.tell()
            if pos % 32 != 0:
                f.write(bytes(32 - pos % 32))
            f.write(part_bytes)

        logger.info("attach ota partitions to bootloader")

    def copy_app_bin_to_pack_dir(self, project_info: bk_project_info, pack_dir: Path):
        def copy_binaries_to_pack_dir(origin_path: Path, pack_path: Path):
            if not origin_path.exists():
                raise FileNotFoundError(f"{origin_path} not found.")
            shutil.copy(origin_path, pack_path)

        soc_name = project_info.get_soc_name()
        project_build_dir = project_info.get_project_build_path()
        cp_name = "app.bin"
        cp_dir = project_build_dir / soc_name
        origin_cp_path = cp_dir / cp_name
        pack_cp_path = pack_dir / cp_name
        copy_binaries_to_pack_dir(origin_cp_path, pack_cp_path)

        ap_name = "app.bin"
        ap_dir = project_build_dir / f"{soc_name}_ap"
        origin_ap_path = ap_dir / ap_name
        pack_ap_path = pack_dir / "app1.bin"
        copy_binaries_to_pack_dir(origin_ap_path, pack_ap_path)
