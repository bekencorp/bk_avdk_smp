from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from pathlib import Path


@dataclass
class app_info:
    app_name: str  # bk7258 bk7258_ap
    app_name_in_sdk: str  # cp     ap
    build_bin: Path
    pack_bin_name: str


class bk_project(ABC):
    @property
    @abstractmethod
    def project_name(self) -> str: ...

    @property
    @abstractmethod
    def soc_name(self) -> str: ...

    @property
    @abstractmethod
    def sdk_path(self) -> Path: ...

    @property
    @abstractmethod
    def project_path(self) -> Path: ...

    @property
    @abstractmethod
    def partitions_dir(self) -> Path: ...

    @property
    @abstractmethod
    def project_build_dir(self) -> Path: ...

    @property
    @abstractmethod
    def project_build_parititons_dir(self) -> Path: ...

    @property
    @abstractmethod
    def project_build_package_dir(self) -> Path: ...

    @property
    @abstractmethod
    def auto_partitions_table(self) -> Path: ...

    @property
    @abstractmethod
    def ram_regions_table(self) -> Path: ...

    @property
    @abstractmethod
    def bootloader_archive_path(self) -> Path: ...

    @property
    @abstractmethod
    def bootloader_build_path(self) -> Path: ...

    @property
    @abstractmethod
    def bootloader_backup_dir(self) -> Path: ...

    @property
    @abstractmethod
    def app0_name(self) -> str: ...

    @property
    @abstractmethod
    def app0_src_root_path(self) -> Path: ...

    @property
    @abstractmethod
    def tools_path(self) -> Path: ...

    @property
    @abstractmethod
    def is_ab_project(self) -> bool: ...

    @property
    @abstractmethod
    def flash_crc_enable(self) -> bool: ...

    @property
    @abstractmethod
    def ram_regions_setting(self) -> Path: ...

    @property
    @abstractmethod
    def flash_partitions_setting(self) -> Path: ...

    @property
    @abstractmethod
    def apps_info(self) -> list[app_info]: ...

    @abstractmethod
    def get_middleware_soc_config_path(self, app_name: str) -> Path: ...
