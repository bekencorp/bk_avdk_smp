from __future__ import annotations

import logging
from typing import Any

from bk_flash_partiton import bk_flash_partition_content_generator, partition_info

logger = logging.getLogger(__name__)


def get_license() -> str:
    s_license = """\
// Copyright 2022-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//This is a generated file, don't modify it!

"""

    return s_license


class bk_flash_denpendecny_generator(bk_flash_partition_content_generator):
    # region layout_hdr_content
    def get_flash_partitions_layout_hdr_content(
        self, part_info: list[partition_info], flash_crc_enable: bool
    ) -> str:
        s_hdr = get_license()

        s_hdr += "#pragma once\n\n"
        s_hdr += f"#define {'CONFIG_FLASH_CRC_ENABLE':<45} {int(flash_crc_enable)}\n"
        s_hdr += f"#define {'CONFIG_PARTITIONS_NUM':<45} {len(part_info) if part_info else 0}\n"

        partition_struct_array = "#define PARTITION_MAP { \\\n"
        if part_info:
            for part in part_info:
                partition_name = (
                    part.Name.upper().replace(" ", "_") if part.Name else ""
                )

                macro_offset = f"CONFIG_{partition_name}_PARTITION_OFFSET"
                macro_size = f"CONFIG_{partition_name}_PARTITION_SIZE"

                s_hdr += f"#define {macro_offset:<45} 0x{part.Offset:08x}\n"
                s_hdr += f"#define {macro_size:<45} 0x{part.Size:08x}\n"

                partition_struct_array += (
                    f'    {{"{part.Name}", {macro_offset}, {macro_size}}}, \\\n'
                )

        partition_struct_array += "}\n"
        return s_hdr + "\n" + partition_struct_array

    # endregion get_flash_partitions_layout_hdr_content

    # region src_content
    def get_partitions_src_content(self, part_info: list[partition_info]) -> str:
        # 构建源文件头部信息
        src_contents = get_license()
        src_contents += self._generate_header_includes()

        # 获取 header_arch 中预定义的数据结构
        header_data = self.header_arch

        # 生成具体的分区结构体定义
        src_contents += self._generate_partition_struct(
            "bk_flash_partitions",
            part_info,
            header_data["bitmap_list"],
            header_data["enum_list"],
            header_data["struct_list"],
        )

        return src_contents

    def _generate_header_includes(self) -> str:
        return """#include <common/bk_include.h>
#include <os/os.h>
#if CONFIG_FLASH_ORIGIN_API
#include \"BkDriverFlash.h\"
#else
#include <driver/flash_partition.h>
#endif
#include <common/bk_kernel_err.h>
#include <os/mem.h>

"""

    def _generate_partition_struct(
        self,
        struct_name: str,
        partitions: list[partition_info],
        bitmaps: list[dict[str, Any]],
        enums: list[dict[str, Any]],
        structs: list[dict[str, Any]],
    ) -> str:
        # 查找必要的枚举和结构定义
        partition_flags = self._find_entry(bitmaps, "partition_flags")
        flash_enum = self._find_entry(enums, "bk_flash_t")
        user_enum = self._find_entry(enums, "bk_partition_user_t")
        logic_partition = self._find_entry(structs, "bk_logic_partition_t")

        # 构建结构体内容
        result = "/* Logic partition on flash devices */\n"
        enum_macro = self._build_enum_macro(user_enum, "max").upper()
        result += f"const {logic_partition['extern_name']} {struct_name}[{enum_macro}] = \n{{\n"

        for partition in partitions:
            macro_name = self._build_enum_macro(user_enum, partition.Name).upper()
            result += f"    [{macro_name}] = \n"
            result += "    {\n"

            # 添加结构体字段
            result += self._generate_partition_fields(
                logic_partition, flash_enum, partition
            )

            # 添加权限标志
            result += self._generate_access_flags(
                logic_partition, partition, partition_flags
            )

            result += "    },\n"

        result += "};\n\n"
        return result

    def _generate_partition_fields(
        self,
        logic_partition: dict[str, Any],
        flash_enum: dict[str, Any],
        partition: partition_info,
    ) -> str:
        fields = logic_partition["inner_entries"]
        embedded_macro = self._build_enum_macro(flash_enum, "embedded").upper()

        return (
            f"        .{fields[0]} = {embedded_macro},\n"
            f'        .{fields[1][1:]} = "{partition.Name}",\n'
            f"        .{fields[2]} = 0x{partition.Offset:x},\n"
            f"        .{fields[3]} = 0x{partition.Size:x},\n"
        )

    def _generate_access_flags(
        self,
        logic_partition: dict[str, Any],
        partition: partition_info,
        partition_flags: dict[str, Any],
    ) -> str:
        access_flags: list[str] = []
        if partition.Execute:
            access_flags.append(
                f"{self._build_enum_macro(partition_flags, 'execute').upper()}_EN"
            )
        else:
            access_flags.append(
                f"{self._build_enum_macro(partition_flags, 'execute').upper()}_DIS"
            )
        if partition.Read:
            access_flags.append(
                f"{self._build_enum_macro(partition_flags, 'read').upper()}_EN"
            )
        else:
            access_flags.append(
                f"{self._build_enum_macro(partition_flags, 'read').upper()}_DIS"
            )
        if partition.Write:
            access_flags.append(
                f"{self._build_enum_macro(partition_flags, 'write').upper()}_EN"
            )
        else:
            access_flags.append(
                f"{self._build_enum_macro(partition_flags, 'write').upper()}_DIS"
            )

        fields = logic_partition["inner_entries"]
        return f"        .{fields[4]} = {' | '.join(access_flags)},\n"

    def _find_entry(
        self, collection: list[dict[str, Any]], target_name: str
    ) -> dict[str, Any]:
        for item in collection:
            if item["extern_name"] == target_name:
                return item
        msg = (
            "Please firstly in func print_part_table_inc(...) initialize and add "
            + f'"{target_name}" entry it to inc_arch_dict!'
        )
        raise RuntimeError(msg)

    def _build_enum_macro(self, enum: dict[str, Any], name: str) -> str:
        for entry in enum["inner_entries"]:
            if entry == name:
                return f"{enum['inner_prefix']}{entry}{enum['inner_suffix']}"
        raise RuntimeError(
            f"Please firstly in func print_part_table_inc(...) add '{name}' to {enum['extern_name']}['inner_entries']!"
        )

    # endregion src_content

    # region hdr_content
    def get_partitions_hdr_content(self, part_info: list[partition_info]) -> str:
        arch_dict: dict[str, Any] = {
            "bitmap_list": [],
            "enum_list": [],
            "struct_list": [],
        }

        self._init_bitmap_entry(arch_dict)
        self._init_bk_flash_enum(arch_dict)
        self._init_bk_partition_type_enum(arch_dict)
        self._init_bk_partition_user_enum(arch_dict, part_info)
        self._init_logic_partition_struct(arch_dict)

        content = get_license() + "#pragma once\n"
        content += '#ifdef __cplusplus\nextern "C" {\n#endif\n\n'

        content += self._generate_enums(arch_dict)
        content += self._generate_structs(arch_dict)

        content += "#ifdef __cplusplus\n}\n#endif\n"

        self.header_arch = arch_dict
        return content

    def _init_bitmap_entry(self, arch_dict: dict[str, Any]):
        arch_dict["bitmap_list"].append(
            {
                "extern_name": "partition_flags",
                "inner_prefix": "par_opt_",
                "inner_suffix": "",
                "inner_entries": ["read", "write", "execute"],
                "contents_generator": lambda *args: "",  # type: ignore
            }
        )

    def _init_bk_flash_enum(self, arch_dict: dict[str, Any]):
        flash_entries = ["embedded", "spi", "max", "none"]
        arch_dict["enum_list"].append(
            {
                "extern_name": "bk_flash_t",
                "inner_prefix": "bk_flash_",
                "inner_suffix": "",
                "inner_entries": flash_entries,
                "inner_values": list(range(len(flash_entries))),
                "contents_generator": lambda *args: "",  # type: ignore
            }
        )

    def _init_bk_partition_type_enum(self, arch_dict: dict[str, Any]):
        partition_entries = [
            "bootloader",
            "application",
            "ota",
            "application1",
            "matter_flash",
            "rf_firmware",
            "net_param",
            "usr_config",
            "ota_fina_executive",
            "application2",
            "easyflash",
            "easyflash_ap",
            "max",
        ]
        arch_dict["enum_list"].append(
            {
                "extern_name": "bk_partition_t",
                "inner_prefix": "bk_partition_",
                "inner_suffix": "",
                "inner_entries": partition_entries,
                "inner_values": list(range(len(partition_entries))),
                "contents_generator": self._generate_partition_enum_content,
            }
        )

    def _init_bk_partition_user_enum(
        self, arch_dict: dict[str, Any], part_info: list[partition_info]
    ) -> None:
        user_entries = [part.Name for part in part_info] + ["max"]
        arch_dict["enum_list"].append(
            {
                "extern_name": "bk_partition_user_t",
                "inner_prefix": "bk_partition_",
                "inner_suffix": "_user",
                "inner_entries": user_entries,
                "inner_values": list(range(len(user_entries))),
                "contents_generator": self._generate_user_enum_content,
            }
        )

    def _init_logic_partition_struct(self, arch_dict: dict[str, Any]):
        logic_partition_fields = {
            "partition_owner": "bk_flash_t",
            "*partition_description": "const char",
            "partition_start_addr": "uint32_t",
            "partition_length": "uint32_t",
            "partition_options": "uint32_t",
        }
        arch_dict["struct_list"].append(
            {
                "extern_name": "bk_logic_partition_t",
                "inner_types": list(logic_partition_fields.values()),
                "inner_entries": list(logic_partition_fields.keys()),
                "contents_generator": lambda *args: "",  # type: ignore
            }
        )

    def _generate_enums(self, arch_dict: dict[str, Any]) -> str:
        content = ""
        for enum in arch_dict["enum_list"]:
            if "contents_generator" in enum and enum["contents_generator"]:
                content += enum["contents_generator"](
                    enum["extern_name"],
                    enum["inner_prefix"].upper(),
                    enum["inner_suffix"].upper(),
                    enum["inner_entries"],
                    enum["inner_values"],
                )
        return content

    def _generate_structs(self, arch_dict: dict[str, Any]) -> str:
        content = ""
        for struct in arch_dict["struct_list"]:
            if "contents_generator" in struct and struct["contents_generator"]:
                content += struct["contents_generator"](
                    struct["extern_name"],
                    struct["inner_types"],
                    struct["inner_entries"],
                )
        return content

    def _generate_partition_enum_content(
        self,
        extern_name: str,
        prefix: str,
        suffix: str,
        entries: list[str],
        values: list[int],
    ) -> str:
        return (
            "".join(
                f"#define {prefix}{entry.upper()}{suffix}_TEMP {value}\n"
                for entry, value in zip(entries, values)
            )
            + "\n"
        )

    def _generate_user_enum_content(
        self,
        extern_name: str,
        prefix: str,
        suffix: str,
        entries: list[str],
        values: list[int],
    ) -> str:
        res = ""

        for entry in entries:
            if entry == "max":
                continue
            res += f"#ifdef BK_PARTITION_{entry.upper()}_TEMP\n"
            res += f"#define {prefix}{entry.upper()}{suffix} BK_PARTITION_{entry.upper()}_TEMP\n"
            res += "#endif\n"

        res += "\n"
        res += "typedef enum\n{\n"
        res += "    BK_PARTITION_START_USER = BK_PARTITION_MAX_TEMP - 1,\n"

        for entry in entries:
            if entry == "max":
                res += f"    {prefix}MAX{suffix},\n"
            else:
                res += f"#ifndef {prefix}{entry.upper()}{suffix}\n"
                res += f"    {prefix}{entry.upper()}{suffix},\n"
                res += "#endif\n"
        res += "} " + extern_name + ";\n\n"

        return res

    # endregion hdr_content
