from __future__ import annotations

import csv
import re
import shutil
import subprocess
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path

from bk_project import app_info, bk_project


@dataclass
class bk_project_info:
    project_name: str
    project_path: Path
    build_dir: Path
    soc_name: str
    apps: list[str]


class bk_sdk_project(bk_project):
    instance = None

    def __init__(self, project_info: bk_project_info) -> None:
        self._project_info = project_info
        self._is_ab_project = None
        self._check_project_info()
        self.instance = self
        self._flash_crc_enable = False
        self._check_flash_crc_enable()

    def _check_flash_crc_enable(self):
        if not self.auto_partitions_table.exists():
            return
        text = self.auto_partitions_table.read_text()
        match = re.search(
            r"^[#\s]*Flash_CRC_ENABLE\s*=\s*(\w+)", text, re.MULTILINE | re.IGNORECASE
        )
        if match:
            flash_crc_enable_value = match.group(1)
            if flash_crc_enable_value.lower() == "false":
                self._flash_crc_enable = False
            elif flash_crc_enable_value == "0":
                self._flash_crc_enable = False
            elif flash_crc_enable_value.lower() == "true":
                self._flash_crc_enable = True
            elif flash_crc_enable_value == "1":
                self._flash_crc_enable = True

    def _check_project_info(self):
        if not self._project_info.soc_name:
            raise RuntimeError("get soc name error")
        if not self._project_info.project_name:
            raise RuntimeError("get project name error")
        if not self._project_info.project_path.is_dir():
            raise RuntimeError("project dir not exist")
        if not self._project_info.build_dir.is_dir():
            raise RuntimeError("project build dir not exist")
        if len(self._project_info.apps) == 0:
            raise RuntimeError(
                f"project {self._project_info.project_name} app num is 0"
            )

    @property
    def sdk_name(self) -> str:
        return "bk_avdk_smp"

    @property
    def project_name(self) -> str:
        return self._project_info.project_name

    @property
    def soc_name(self) -> str:
        return self._project_info.soc_name

    @property
    def sdk_path(self) -> Path:
        return Path(__file__).absolute().parents[4]

    @property
    def project_path(self) -> Path:
        return self._project_info.project_path

    @property
    def partitions_dir(self) -> Path:
        return (
            self._project_info.project_path / "partitions" / self._project_info.soc_name
        )

    @property
    def project_build_dir(self) -> Path:
        return self._project_info.build_dir

    @property
    def project_build_parititons_dir(self) -> Path:
        return self.project_build_dir / "partitions"

    @property
    def project_build_package_dir(self) -> Path:
        return self.project_build_dir / "package"

    @property
    def auto_partitions_table(self) -> Path:
        return self.partitions_dir / "auto_partitions.csv"

    @property
    def ram_regions_table(self) -> Path:
        return self.partitions_dir / "ram_regions.csv"

    @property
    def bootloader_archive_path(self) -> Path:
        bootloader_libs_path = (
            self.sdk_path
            / "cp/components/bk_libs"
            / self._project_info.soc_name
            / "bootloader"
        )
        if self.is_ab_project:
            dirname = "ab_bootloader"
        else:
            dirname = "normal_bootloader"

        return bootloader_libs_path / dirname / "bootloader.bin"

    @property
    def bootloader_build_path(self) -> Path:
        bootloader_propertites_path = (
            self.sdk_path / "cp/properties/modules/bootloader/aboot"
        )
        if self.is_ab_project:
            dirname = "arm_bootloader_ab"
        else:
            dirname = "arm_bootloader"

        return bootloader_propertites_path / dirname / "output/bootloader.bin"

    @property
    def bootloader_backup_dir(self) -> Path:
        return self.project_build_dir / self.app0_name / "bootloader_out"

    @property
    def app0_name(self) -> str:
        return self._project_info.apps[0]

    @property
    def app0_src_root_path(self) -> Path:
        return self.sdk_path / "cp"

    @property
    def tools_path(self) -> Path:
        return self.sdk_path / "tools"

    @property
    def is_ab_project(self) -> bool:
        if self._is_ab_project is not None:
            return bool(self._is_ab_project)
        ab_config_path = self.partitions_dir / "ab_position_independent.csv"
        if not ab_config_path.exists():
            return False

        data: dict[str, str] = {}
        with ab_config_path.open("r") as f:
            reader = csv.reader(f)
            next(reader)  # Skip header line
            for row in reader:
                data[row[0]] = row[1]
        self._is_ab_project = data.get("pos_independent", "").lower() == "true"
        return self._is_ab_project

    @property
    def flash_crc_enable(self) -> bool:
        return self._flash_crc_enable

    @property
    def ram_regions_setting(self) -> Path:
        config_dir = Path(__file__).absolute().parent
        chip_specific_config = config_dir / f"smp_ram_setting_{self.soc_name}.json"
        if chip_specific_config.exists():
            return chip_specific_config
        # Fallback to default config
        return config_dir / "smp_ram_setting.json"

    @property
    def flash_partitions_setting(self) -> Path:
        return Path(__file__).absolute().parent / "smp_flash_partitions_setting.json"

    @property
    def apps_info(self) -> list[app_info]:
        app_list: list[app_info] = []
        for app_name in self._project_info.apps:
            if app_name.endswith("_ap"):
                app_name_in_sdk = "AP"
                app_pack_bin_name = "app1.bin"
            else:
                app_name_in_sdk = "CP"
                app_pack_bin_name = "app.bin"
            app_build_bin = self.project_build_dir / app_name / "app.bin"
            app_list.append(
                app_info(app_name, app_name_in_sdk, app_build_bin, app_pack_bin_name)
            )
        return app_list

    def get_middleware_soc_config_path(self, app_name: str) -> Path:
        if app_name.endswith("_ap"):
            app_type = "ap"
        else:
            app_type = "cp"
        return self.sdk_path / app_type / "middleware/boards" / app_name

    def pre_auto_partition(self) -> None:
        pass

    def post_auto_partition(self) -> None:
        from .bk_ota_pack import gen_ota_pack_json

        gen_ota_pack_json()

    def pre_package(self) -> None:
        pass

    def post_package(self) -> None:
        from .bk_ota_pack import ota_pack

        ota_bin = ota_pack()
        self.build_summary += f"ota binary: {ota_bin}\n"
        self._write_product_id_to_all_app_bin()

    def _read_product_id_from_bootloader(self) -> tuple[str, str] | None:
        magic_map = (
            (0x100, 0x108, "NO_CRC"),
            (0x110, 0x118, "CRC"),
        )
        magic_strings = (b"BEKEN", b"BK.SB", b"BK7236", b"BK7259")

        bootloader = self.bootloader_archive_path
        if not bootloader.exists():
            return None

        data = bootloader.read_bytes()
        for magic_offset, product_offset, crc_mode in magic_map:
            for magic in magic_strings:
                if data[magic_offset:magic_offset + len(magic)] == magic:
                    product_bytes = data[product_offset:product_offset + 15]
                    product_id = product_bytes.split(b"\0", 1)[0].decode("ascii", "ignore").strip()
                    if product_id:
                        return product_id, crc_mode
                    return None

        return None

    def _write_product_id_to_all_app_bin(self) -> None:
        product_info = self._read_product_id_from_bootloader()
        if product_info is None:
            return

        product_id, crc_mode = product_info
        if product_id == "0":
            return

        all_app_bin = self.project_build_package_dir / "all-app.bin"
        write_product_id_script = self.tools_path / "build_tools/write_product_id.py"
        if not all_app_bin.exists() or not write_product_id_script.exists():
            return

        subprocess.check_call(
            ["python3", str(write_product_id_script), str(all_app_bin), product_id]
        )

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        spid_firmware = self.project_build_dir / f"{product_id}_{crc_mode}_{timestamp}.bin"
        shutil.copy(all_app_bin, spid_firmware)
        self.build_summary += f"spid firmware: {spid_firmware}\n"

    def _copy_bootloader_to_pack_dir(self, pack_dir: Path):
        """override super class method"""
        from .bk_ota_pack import handle_bootloader_bin

        handle_bootloader_bin(pack_dir)
