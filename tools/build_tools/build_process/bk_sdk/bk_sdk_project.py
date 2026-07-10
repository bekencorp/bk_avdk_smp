from __future__ import annotations

import csv
import importlib
import json
import re
from dataclasses import dataclass
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
        # Force AB projects to package the normal arm_bootloader binary while
        # keeping AB partition/OTA logic (driven by is_ab_project) untouched.
        dirname = "normal_bootloader"

        return bootloader_libs_path / dirname / "bootloader.bin"

    @property
    def bootloader_build_path(self) -> Path:
        bootloader_propertites_path = (
            self.sdk_path / "cp/properties/modules/bootloader/aboot"
        )
        # Force AB projects to build/backup the normal arm_bootloader binary
        # while keeping AB partition/OTA logic (driven by is_ab_project) untouched.
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
    def use_format_packager(self) -> bool:
        """Whether all-app.bin is generated as a format (head-described) package.

        A format package starts with a global header + image table (instead of a
        raw flash image), so download tools can dispatch each image to its
        partition. It also means the OTA rbl must NOT be written back into
        all-app.bin.
        """
        return self.project_name == "app_ab"

    @property
    def extra_pack_partitions(self) -> list[str]:
        """Non-executable partitions to additionally embed into the download package.

        These partitions are not part of the app/OTA payload; they are only
        pre-provisioned into all-app.bin (e.g. AB state flags). Add project
        specific partitions here instead of scattering per-project checks.
        """
        if self.project_name == "app_ab":
            return ["ota_fina_executive"]
        return []

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

    @staticmethod
    def _extra_pack_bin_name(partition_name: str) -> str:
        return f"{partition_name}.bin"

    def _get_partition_info(self, partition_name: str) -> dict:
        partitions_json = self.project_build_parititons_dir / "partitions.json"
        if not partitions_json.exists():
            raise FileNotFoundError(f"{partitions_json} not found.")

        with partitions_json.open("r") as f:
            partitions_info = json.load(f)

        for part in partitions_info["section"]:
            if part["Name"] == partition_name:
                return part
        raise RuntimeError(f"partition {partition_name} not found.")

    def _append_extra_pack_partitions(self) -> None:
        partitions = self.extra_pack_partitions
        if not partitions:
            return

        from bk_misc import format_size

        pack_json = self.project_build_parititons_dir / "bk_package.json"
        if not pack_json.exists():
            raise FileNotFoundError(f"{pack_json} not found.")

        with pack_json.open("r") as f:
            pack_info = json.load(f)

        sections = pack_info["section"]
        existing = {item["partition"] for item in sections}
        for name in partitions:
            if name in existing:
                continue
            part = self._get_partition_info(name)
            sections.append(
                {
                    "firmware": self._extra_pack_bin_name(name),
                    "partition": name,
                    "start_addr": f"0x{part['Offset']:08x}",
                    "size": format_size(part["Size"]),
                }
            )
        pack_info["count"] = len(sections)

        with pack_json.open("w") as f:
            json.dump(pack_info, f, indent=4)

    def _write_extra_pack_bins(self, pack_dir: Path) -> None:
        for name in self.extra_pack_partitions:
            part = self._get_partition_info(name)
            bin_path = pack_dir / self._extra_pack_bin_name(name)
            bin_path.write_bytes(bytes([0xFF]) * part["Size"])

    def pre_auto_partition(self) -> None:
        pass

    def post_auto_partition(self) -> None:
        from .bk_ota_pack import gen_ota_pack_json

        gen_ota_pack_json()
        self._append_extra_pack_partitions()

    def pre_package(self) -> None:
        pass

    def get_packager(self, pack_dir: Path, pack_json: Path, output_bin: Path):
        if self.use_format_packager:
            bk_packager = importlib.import_module("bk_packager")
            if self.flash_crc_enable:
                return bk_packager.bk_packager_format_crc(
                    pack_dir, pack_json, output_bin
                )
            return bk_packager.bk_packager_format(pack_dir, pack_json, output_bin)
        return super().get_packager(pack_dir, pack_json, output_bin)

    def post_package(self) -> None:
        from .bk_ota_pack import ota_pack

        ota_bin = ota_pack()
        self.build_summary += f"ota binary: {ota_bin}\n"

    def _copy_bootloader_to_pack_dir(self, pack_dir: Path):
        """override super class method"""
        from .bk_ota_pack import handle_bootloader_bin

        handle_bootloader_bin(pack_dir)

    def copy_binaries_to_pack_dir(self, pack_dir: Path) -> None:
        super().copy_binaries_to_pack_dir(pack_dir)
        self._write_extra_pack_bins(pack_dir)
