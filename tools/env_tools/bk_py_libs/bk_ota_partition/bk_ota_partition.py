from __future__ import annotations

import copy
import json
import re
from pathlib import Path
from typing import Any

from . import logger


class bk_ota_partition:
    def __init__(self, part_json: Path) -> None:
        logger.info(f"read parititons from {part_json}")
        with part_json.open("r") as f:
            json_content = json.load(f)
        part_info: list[dict[str, Any]] = json_content["section"]
        self.crc_enable = json_content["crc_enable"]
        self.raw_part_info = copy.deepcopy(part_info)
        self._part_adapter(part_info)
        self.part_info = part_info
        self.header_path = "flash_partition.h"
        self.header_arch = None

    def _part_adapter(self, part_sections: list[dict[str, Any]]):
        app_count = 0
        for item in part_sections:
            if "bootloader" in item["Name"] and item["Execute"]:
                item["Name"] = "bootloader"
                continue
            if item["Execute"]:
                item["Name"] = "application" + (str(app_count) if app_count else "")
                app_count += 1
                continue
            if item["Name"] == "sys_rf":
                item["Name"] = "rf_firmware"
                continue
            if item["Name"] == "sys_net":
                item["Name"] = "net_param"

    def gen_ab_ota_json(self, ota_json: Path):
        KEYWORDS = {
            "application": "appa",
            "application1": "appa",
            "application2": "appa",
        }
        part_table_dict: dict[str, Any] = {
            "part_table": [],
        }
        part_dict_temp: dict[str, Any] = {
            "magic": "0x45503130",
            "name": None,
            "flash_name": None,
            "offset": None,
            "len": None,
        }
        app_total_size = 0x0
        appa_offset = 0x0
        part_table_bark = copy.deepcopy(self.part_info)
        part_table_temp: list[dict[str, Any]] = list()
        for p in part_table_bark:
            if re.match(r"^bootloader(\d)*$", p["Name"]):
                part_table_temp.append(p)
                appa_offset = p["Offset"] + p["Size"]
            if p["Name"] in KEYWORDS:
                if p["Name"] == "application":
                    app_total_size = p["Size"]
                    part_table_temp.append(p)
                elif p["Name"] == "application1" or p["Name"] == "application2":
                    app_total_size += p["Size"]
                else:
                    raise RuntimeError("error:not deal this situation!")
            if re.match(r"^s_app(\d)*$", p["Name"]):
                part_table_temp.append(p)
            if re.match(r"^ota_fina_executive(\d)*$", p["Name"]):
                part_table_temp.append(p)
        for p in part_table_temp:
            part_name = KEYWORDS[p["Name"]] if KEYWORDS.get(p["Name"]) else p["Name"]
            p["Name"] = part_name

        for p in sorted(part_table_temp, key=lambda x: x["Offset"]):
            part_dict: dict[str, Any] = dict()
            part_dict.update(part_dict_temp)
            part_dict["name"] = p["Name"]
            part_dict["flash_name"] = (
                "beken_onchip_crc" if p["Execute"] else "beken_onchip"
            )
            part_dict["offset"] = f"0x{p['Offset']:08x}"
            part_dict["len"] = self._size_format(p["Size"])
            part_table_dict["part_table"].append(part_dict)
            if p["Name"] in KEYWORDS.values():
                part_dict["len"] = self._size_format(app_total_size)
            if p["Name"] == "appa":
                part_dict["offset"] = f"0x{appa_offset:08x}"
            if p["Name"] == "s_app":
                part_dict["flash_name"] = "beken_onchip_crc"

        logger.info(f"gen package json: {ota_json}")
        with ota_json.open("w", newline="\n") as f:
            json.dump(part_table_dict, f, sort_keys=False, indent=4)

    def gen_ota_json(self, ota_json: Path):
        KEYWORDS = {
            "ota": "download",
            "application": "app",
            "application1": "app1",
            "application2": "app2",
        }
        part_table_dict: dict[str, Any] = {
            "part_table": [],
        }
        part_dict_temp: dict[str, Any] = {
            "magic": "0x45503130",
            "name": None,
            "flash_name": None,
            "offset": None,
            "len": None,
        }

        part_table_bark = copy.deepcopy(self.part_info)
        part_table_temp: list[dict[str, Any]] = list()
        for p in part_table_bark:
            if re.match(r"^bootloader(\d)*$", p["Name"]):
                part_table_temp.append(p)
            if re.match(r"^application(\d)*$", p["Name"]):
                part_table_temp.append(p)
            if re.match(r"^ota(\d)*$", p["Name"]):
                part_table_temp.append(p)

        for p in part_table_temp:
            part_name = KEYWORDS[p["Name"]] if KEYWORDS.get(p["Name"]) else p["Name"]
            p["Name"] = part_name

        for p in sorted(part_table_temp, key=lambda x: x["Offset"]):
            part_dict: dict[str, Any] = dict()
            part_dict.update(part_dict_temp)
            part_dict["name"] = p["Name"]
            part_dict["flash_name"] = (
                "beken_onchip_crc" if p["Execute"] else "beken_onchip"
            )
            part_dict["offset"] = f"0x{p['Offset']:08x}"
            part_dict["len"] = self._size_format(p["Size"])
            part_table_dict["part_table"].append(part_dict)
        logger.info(f"gen package json: {ota_json}")
        with ota_json.open("w", newline="\n") as f:
            json.dump(part_table_dict, f, sort_keys=False, indent=4)

    def gen_ab_configuartion_json(self, config_json: Path):
        json_content: dict[str, Any] = {}
        json_content.update({"magic": "beken"})
        sections: list[dict[str, str]] = []
        bootloader_start = 0
        bootloader_size = 0
        app_size = 0
        app_start = 0xFFFFFFFF
        for part in self.part_info:
            if part["Name"] == "bootloader":
                bootloader_start = part["Offset"]
                bootloader_size = part["Size"]
                continue
            if part["Execute"]:
                app_start = min(app_start, part["Offset"])
                app_size += part["Size"]

        if self.crc_enable:
            bootloader_start = int(bootloader_start / 34 * 32)
            bootloader_size = int(bootloader_size / 34 * 32 / 1024)
            app_start = int(app_start / 34 * 32)
            app_size = int(app_size / 34 * 32 / 1024)

        bootloader_sect = {
            "firmware": "bootloader.bin",
            "partition": "bootloader",
            "start_addr": f"0x{bootloader_start:08x}",
            "size": f"{bootloader_size}K",
        }
        app_sect = {
            "firmware": "app_ab.bin",
            "partition": "app",
            "start_addr": f"0x{app_start:08x}",
            "size": f"{app_size}K",
        }
        sections.append(bootloader_sect)
        sections.append(app_sect)
        json_content.update({"count": len(sections)})
        json_content.update({"section": sections})
        logger.info(f"gen ab configuartion json: {config_json}")
        with config_json.open("w", newline="\n") as f:
            json.dump(json_content, f, indent=4)

    @staticmethod
    def _size_format(size: int) -> str:
        for val, suffix in [(0x400, "K"), (0x100000, "M")]:
            if size % val == 0:
                return f"{size // val}{suffix}"
        return f"{size}"
