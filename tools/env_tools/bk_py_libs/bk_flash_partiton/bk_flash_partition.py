
import os
import copy
import json
from . import logger

def size_format(size, include_size):
    if include_size:
        for (val, suffix) in [ (0x400, "K"), (0x100000, "M")]:
            if size % val == 0:
                return "%d%s" % (size // val, suffix)
    size_str = '%x'%size
    lead_zero = '0'*(8 - len(size_str))
    return "0x%s%s" % (lead_zero, size_str)

class bk_flash_partition:
    def __init__(self, part_json:str):
        logger.info(f"read parititons from {part_json}")
        with open(part_json, 'r') as f:
            json_content = json.load(f)
        part_info = json_content['section']
        self.crc_enable = json_content['crc_enable']
        self.raw_part_info = copy.deepcopy(part_info)
        self._part_adapter(part_info)
        self.part_info = part_info
        self.header_path = "flash_partition.h"
        self.header_arch = None

    def _part_adapter(self, part_sections):
        app_count = 0
        for item in part_sections:
            if "bootloader" in item['Name'] and item['Execute']:
                item["Name"] = "bootloader"
                continue
            if item['Execute']:
                item["Name"] = "application" + (str(app_count) if app_count else "")
                app_count += 1
                continue
            if item["Name"] == "sys_rf":
                item["Name"] = "rf_firmware"
                continue
            if item["Name"] == "sys_net":
                item["Name"] = "net_param"

    def _gen_part_table_src(self, src_path):

        def search_list_from_extern_name(ls,nm):
            found = False
            for l in ls:
                if nm == l['extern_name']:
                    found = True
                    return l
            if not found:
                raise RuntimeError('Please firstly in func print_part_table_inc(...) initialize and add \"%s\" entry it to inc_arch_dict!'%(nm))

        def macro_inner_entry_from_enum(enum, nm):
            inner_prefix = enum['inner_prefix']
            inner_suffix = enum['inner_suffix']
            found = False
            for e in enum['inner_entries']:
                if nm == e:
                    found = True
                    return '%s%s%s'%(inner_prefix, e, inner_suffix)
            if not found:
                raise RuntimeError('Please firstly in func print_part_table_inc(...) add \'%s\' to %s[\'inner_entries\']!'%(nm, enum['extern_name']))

        def gen_partitions_struct_contents(name, part_table, bitmap_list, enum_list, struct_list):
            # create bk_logic_partition_t struct type instance, 
            # the contents of this instance associate with 
            # part_table, partition_flags bitmap, 
            # bk_flash_t enum and bk_partition_t enum 
            extern_name = 'partition_flags'
            partition_flags_bitmap = search_list_from_extern_name(bitmap_list, extern_name)
            extern_name = 'bk_flash_t'
            flash_enum = search_list_from_extern_name(enum_list, extern_name)
            extern_name = 'bk_partition_t'
            partition_enum = search_list_from_extern_name(enum_list, extern_name)
            extern_name = 'bk_partition_user_t'
            user_enum = search_list_from_extern_name(enum_list, extern_name)
            extern_name = 'bk_logic_partition_t'
            logic_partition_struct = search_list_from_extern_name(struct_list, extern_name)

            res = ""
            """
            res += "void bk_user_macro_printf(void)\n"
            res += "{\n"
            for p in part_table:
                res += "    os_printf(\"bk_user_%s = %%d\\r\\n\",%s);\n"%(p.name, macro_inner_entry_from_enum(user_enum, p.name).upper())
            res += "    os_printf(\"bk_user_%s = %%d\\r\\n\",%s);\n"%('max', macro_inner_entry_from_enum(user_enum, 'max').upper())
            res += "}\n"
            """
            res += "/* Logic partition on flash devices */\n"
            res += "const %s %s[%s] = {\n"%(logic_partition_struct['extern_name'], name, macro_inner_entry_from_enum(user_enum, 'max').upper())
            for part in part_table:
                space = ' '*4
                res += "%s[%s] = \n"%(space, macro_inner_entry_from_enum(user_enum, part["Name"]).upper())
                #res += "%s[BK_USER_%s] = \n"%(space, p.name.upper())
                res += "%s{\n"%(space)
                space = ' '*8
                res += "%s.%s = %s,\n"%(space, logic_partition_struct['inner_entries'][0], macro_inner_entry_from_enum(flash_enum, 'embedded').upper())
                res += "%s.%s = \"%s\",\n"%(space, logic_partition_struct['inner_entries'][1][1:], part["Name"])
                res += "%s.%s = 0x%x,\n"%(space, logic_partition_struct['inner_entries'][2], part["Offset"])
                res += "%s.%s = 0x%x,\n"%(space, logic_partition_struct['inner_entries'][3], part["Size"])
                res += "%s.%s = %s%s%s,\n"%(space, logic_partition_struct['inner_entries'][4],
                                                        macro_inner_entry_from_enum(partition_flags_bitmap, 'execute').upper() + ('_EN' if part["Execute"] else '_DIS'),
                                                        ' | ' + macro_inner_entry_from_enum(partition_flags_bitmap, 'read').upper() + ('_EN' if part["Read"] else '_DIS'),
                                                        ' | ' + macro_inner_entry_from_enum(partition_flags_bitmap, 'write').upper() + ('_EN' if part["Write"] else '_DIS'))
                space = ' '*4
                res += "%s},\n"%(space)
            res += "};\n"

            return res


        # Generate all struct instance with uinque struct func in gen_struct_contents_funcs,
        # if you want create one new struct instance, firstly register it's func to gen_struct_contents_funcs
        # struct func template has the format: gen_bk_temp_struct_contents(name, part_table, inc_arc)
        src_contents = ""
        if 1:
            src_contents += "/**\n"
            src_contents += " *********************************************************************************\n"
            src_contents += " * @file %s\n"%(os.path.basename(src_path))
            src_contents += " * @brief This file provides all the headers of Flash operation functions..\n"
            src_contents += " *********************************************************************************\n"
            src_contents += " *\n"
            src_contents += " * The MIT License\n"
            src_contents += " * Copyright (c) 2017 BEKEN Inc.\n"
            src_contents += " *\n"
            src_contents += " * Permission is hereby granted, free of charge, to any person obtaining a copy\n"
            src_contents += " * of this software and associated documentation files (the \"Software\"), to deal\n"
            src_contents += " * in the Software without restriction, including without limitation the rights\n"
            src_contents += " * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
            src_contents += " * copies of the Software, and to permit persons to whom the Software is furnished\n"
            src_contents += " * to do so, subject to the following conditions:\n"
            src_contents += " *\n"
            src_contents += " * The above copyright notice and this permission notice shall be included in\n"
            src_contents += " * all copies or substantial portions of the Software.\n"
            src_contents += " *\n"
            src_contents += " * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
            src_contents += " * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
            src_contents += " * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
            src_contents += " * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
            src_contents += " * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR\n"
            src_contents += " * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE\n"
            src_contents += " *********************************************************************************\n"
            src_contents += "*/\n"

        src_contents += "#include <common/bk_include.h>\n"
        src_contents += "#include <os/os.h>\n"
        src_contents += "#if CONFIG_FLASH_ORIGIN_API\n"
        src_contents += "#include \"BkDriverFlash.h\"\n"
        src_contents += "#else\n"
        src_contents += "#include <driver/flash_partition.h>\n"
        src_contents += "#endif\n"
        src_contents += "#include <common/bk_kernel_err.h>\n"
        src_contents += "#include <os/mem.h>\n"
        src_contents += "#include <%s>\n"%(os.path.basename(self.header_path))
        src_contents += "\n"

        header_arch = self.header_arch
        bitmap_list = header_arch['bitmap_list']
        enum_list = header_arch['enum_list']
        struct_list = header_arch['struct_list']
        # for func,name in gen_struct_contents_funcs:
        src_contents += gen_partitions_struct_contents('bk_flash_partitions', self.part_info, bitmap_list, enum_list, struct_list)

        with open(src_path, 'w') as f:
            # status("Gen %s source file using remapped part table, which depends on %s source file..." % (os.path.basename(self.to_src), os.path.basename(self.to_inc)))
            f.write(src_contents)
        
        logger.info(f"gen flash partition src to {src_path}")

    def _gen_part_table_hdr(self, header_path):
        def gen_bitmap_null_contents(extern_name, inner_prefix, inner_suffix, inner_entries):
            res = ""
            pass
            return res

        def gen_enum_null_contents(extern_name, inner_prefix, inner_suffix, inner_entries, inner_values):
            res = ""
            pass
            return res

        def gen_enum_bk_partition_t_contents(extern_name, inner_prefix, inner_suffix, inner_entries, inner_values):
            res = ""
            """
            res += "typedef enum\n"
            res += "{\n"
            for index in range(len(inner_entries)):
                res += "    %s%s%s = %d,\n" % (inner_prefix, inner_entries[index].upper(), inner_suffix, inner_values[index])
            res += "}%s;\n" % (extern_name)
            res += "\n"
            """
            for index in range(len(inner_entries)):
                res += "#define %s%s%s_TEMP %d\n"%(inner_prefix, inner_entries[index].upper(), inner_suffix, inner_values[index])
            res += "\n"
            return res

        def gen_enum_bk_user_t_contents(extern_name, inner_prefix, inner_suffix, inner_entries, inner_values):
            res = ""
            for index in range(len(inner_entries)):
                if inner_entries[index] == 'max':
                    continue
                res += "#ifdef BK_PARTITION_%s_TEMP\n"%(inner_entries[index].upper())
                res += "#define %s%s%s BK_PARTITION_%s_TEMP\n"%(inner_prefix, inner_entries[index].upper(), inner_suffix, inner_entries[index].upper())
                res += "#endif\n"
            res += "\n"

            res += "typedef enum\n"
            res += "{\n"
            res += "    BK_PARTITION_START_USER = BK_PARTITION_%s_TEMP - 1,\n"%('max'.upper())
            for index in range(len(inner_entries)):
                if inner_entries[index] == 'max':
                    continue
                res += "#ifndef %s%s%s\n"%(inner_prefix, inner_entries[index].upper(), inner_suffix)
                res += "    %s%s%s,\n"%(inner_prefix, inner_entries[index].upper(), inner_suffix)
                res += "#endif\n"
            res += "    %s%s%s,\n"%(inner_prefix, 'max'.upper(), inner_suffix)
            res += "}%s;\n" % (extern_name)
            res += "\n"
            return res

        def gen_struct_null_contents(extern_name, inner_types, inner_entries):
            res = ""
            pass
            return res

        self.header_path = header_path
        # assume header file just include bitmap, enum, struct definition
        inc_arch_dict = {
            "bitmap_list": [],
            "enum_list": [],
            "struct_list": [],
        }

        # define the bitmap template
        bitmap_entry_temp = {
            "extern_name": None,
            "inner_prefix": None,
            "inner_suffix": None,
            "inner_entries": [],
            "contents_genenator": None,
        }
        # define the enum template
        enum_entry_temp = {
            "extern_name": None,
            "inner_prefix": None,
            "inner_suffix": None,
            "inner_entries": [],
            "inner_values": [],
            "contents_genenator": None,
        }
        # define the struct template
        struct_entry_temp = {
            "extern_name": None,
            "inner_types": [],
            "inner_entries": [],
            "contents_genenator": None,
        }

        # use bitmap template initialize one bitmap entry, add it to inc_arch_dict
        keys = ["read", "write", "execute"]
        bitmap_flags_entry = copy.deepcopy(bitmap_entry_temp)
        bitmap_flags_entry['extern_name'] = 'partition_flags'
        bitmap_flags_entry['inner_prefix'] = 'par_opt_'
        bitmap_flags_entry['inner_suffix'] = ''
        bitmap_flags_entry['inner_entries'].extend(keys)
        bitmap_flags_entry['contents_genenator'] = gen_bitmap_null_contents
        inc_arch_dict['bitmap_list'].append(bitmap_flags_entry)

        # use enum template initialize one enum entry, add it to inc_arch_dict
        keys = ["embedded", "spi",]
        keys.extend(['max'])
        keys.extend(['none'])
        values = range(len(keys))
        enum_flash_entry = copy.deepcopy(enum_entry_temp)
        enum_flash_entry['extern_name'] = 'bk_flash_t'
        enum_flash_entry['inner_prefix'] = 'bk_flash_'
        enum_flash_entry['inner_suffix'] = ''
        enum_flash_entry['inner_entries'].extend(keys)
        enum_flash_entry['inner_values'].extend(values)
        enum_flash_entry['contents_genenator'] = gen_enum_null_contents
        inc_arch_dict['enum_list'].append(enum_flash_entry)

        # use enum template initialize one enum entry, add it to inc_arch_dict
        keys = ["bootloader","application","ota","application1","matter_flash","rf_firmware","net_param","usr_config","ota_fina_executive","application2","easyflash","easyflash_ap"]
        keys.extend(['max'])
        values = range(len(keys))
        enum_partition_entry = copy.deepcopy(enum_entry_temp)
        enum_partition_entry['extern_name'] = 'bk_partition_t'
        enum_partition_entry['inner_prefix'] = 'bk_partition_'
        enum_partition_entry['inner_suffix'] = ''
        enum_partition_entry['inner_entries'].extend(keys)
        enum_partition_entry['inner_values'].extend(values)
        enum_partition_entry['contents_genenator'] = gen_enum_bk_partition_t_contents
        inc_arch_dict['enum_list'].append(enum_partition_entry)

        # use enum template initialize one enum entry, add it to inc_arch_dict
        part_info = self.part_info
        keys = [part["Name"] for part in part_info]
        keys.extend(['max'])
        values = range(len(keys))
        enum_partition_entry = copy.deepcopy(enum_entry_temp)
        enum_partition_entry['extern_name'] = 'bk_partition_user_t'
        enum_partition_entry['inner_prefix'] = 'bk_partition_'
        enum_partition_entry['inner_suffix'] = '_user'
        enum_partition_entry['inner_entries'].extend(keys)
        enum_partition_entry['inner_values'].extend(values)
        enum_partition_entry['contents_genenator'] = gen_enum_bk_user_t_contents
        inc_arch_dict['enum_list'].append(enum_partition_entry)

        # use struct template initialize one struct entry, add it to inc_arch_dict
        logic_partition_t = {
            "partition_owner": "bk_flash_t",
            "*partition_description": "const char",
            "partition_start_addr": "uint32_t",
            "partition_length": "uint32_t",
            "partition_options": "uint32_t",
        }
        keys = logic_partition_t.keys()
        values = logic_partition_t.values()
        struct_logic_partition_entry = copy.deepcopy(struct_entry_temp)
        struct_logic_partition_entry['extern_name'] = 'bk_logic_partition_t'
        struct_logic_partition_entry['inner_types'].extend(values)
        struct_logic_partition_entry['inner_entries'].extend(keys)
        struct_logic_partition_entry['contents_genenator'] = gen_struct_null_contents
        inc_arch_dict['struct_list'].append(struct_logic_partition_entry)

        # record the inc arch to PartTableGenerator instance, then it is useful for print_part_table_src func
        

        inc_contents = ""
        if 1:
            inc_contents += "/**\n"
            inc_contents += " *********************************************************************************\n"
            inc_contents += " * @file %s\n"%(os.path.basename(header_path))
            inc_contents += " * @brief This file provides all the headers of Flash operation functions..\n"
            inc_contents += " *********************************************************************************\n"
            inc_contents += " *\n"
            inc_contents += " *Copyright 2020-2021 Beken\n"
            inc_contents += " *\n"
            inc_contents += " *Licensed under the Apache License, Version 2.0 (the \"License\");\n"
            inc_contents += " *you may not use this file except in compliance with the License.\n"
            inc_contents += " *You may obtain a copy of the License at\n"
            inc_contents += " *\n"
            inc_contents += " *     http://www.apache.org/licenses/LICENSE-2.0\n"
            inc_contents += " *\n"
            inc_contents += " *Unless required by applicable law or agreed to in writing, software\n"
            inc_contents += " *distributed under the License is distributed on an \"AS IS\" BASIS,\n"
            inc_contents += " *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
            inc_contents += " *See the License for the specific language governing permissions and\n"
            inc_contents += " *limitations under the License.\n"
            inc_contents += " *********************************************************************************\n"
            inc_contents += "*/\n"

        inc_contents += "#pragma once\n"
        #inc_contents += "#include <driver/flash_partition.h>\n"
        inc_contents += "#ifdef __cplusplus\n"
        inc_contents += "extern \"C\" {\n"
        inc_contents += "#endif\n"
        inc_contents += "\n"

        # Generate all bitmap contents
        for bitmap in inc_arch_dict['bitmap_list']:
            extern_name = bitmap['extern_name']
            inner_prefix = bitmap['inner_prefix'].upper()
            inner_suffix = bitmap['inner_suffix'].upper()
            inner_entries = bitmap['inner_entries']
            contents_genenator = bitmap['contents_genenator']
            inc_contents += contents_genenator(extern_name, inner_prefix, inner_suffix, inner_entries)

        # Generate all enum contents
        for enum in inc_arch_dict['enum_list']:
            extern_name = enum['extern_name']
            inner_prefix = enum['inner_prefix'].upper()
            inner_suffix = enum['inner_suffix'].upper()
            inner_entries = enum['inner_entries']
            inner_values = enum['inner_values']
            contents_genenator = enum['contents_genenator']
            inc_contents += contents_genenator(extern_name, inner_prefix, inner_suffix, inner_entries, inner_values)

        # Generate all struct contents
        for struct in inc_arch_dict['struct_list']:
            extern_name = struct['extern_name']
            inner_types = struct['inner_types']
            inner_entries = struct['inner_entries']
            contents_genenator = struct['contents_genenator']
            inc_contents += contents_genenator(extern_name, inner_types, inner_entries)
        self.header_arch = inc_arch_dict

        inc_contents += "#ifdef __cplusplus\n"
        inc_contents += "}\n"
        inc_contents += "#endif\n"

        # to_inc_dir = os.path.abspath(os.path.dirname(self.to_inc))
        # ensure_directory(to_inc_dir)
        with open(header_path, 'w') as f:
            # status("Gen %s source file using remapped part table..." % (os.path.basename(self.to_inc)))
            f.write(inc_contents)
        logger.info(f"gen flash partition header to {header_path}")

    def gen_partitions_layout_hdr(self, partition_hdr_file):
        def get_license():
            s_license = "\
// Copyright 2022-2024 Beken\n\
//\n\
// Licensed under the Apache License, Version 2.0 (the \"License\");\n\
// you may not use this file except in compliance with the License.\n\
// You may obtain a copy of the License at\n\
//\n\
//     http://www.apache.org/licenses/LICENSE-2.0\n\
//\n\
// Unless required by applicable law or agreed to in writing, software\n\
// distributed under the License is distributed on an \"AS IS\" BASIS,\n\
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n\
// See the License for the specific language governing permissions and\n\
// limitations under the License.\n\
\n\
//This is a generated file, don't modify it!\n\
\n\
#pragma once\n\
\n\
\n\
"
            return s_license

        f = open(partition_hdr_file, 'w+')

        logger.debug(f'Create partition hdr file: {partition_hdr_file}')
        f.write(get_license())

        # line = f'#include "security.h"\n'
        # f.write(line)
        # TODO security
    
        line = f'#define %-45s %s' %("KB(size)", "((size) << 10)\n")
        f.write(line)
        line = f'#define %-45s %s' %("MB(size)", "((size) << 20)\n\n")
        f.write(line)

        # macro_name = f'CONFIG_TFM_S_JUMP_TO_TFM_NS'
        # if partition.tfm_ns_exists == True:
        #     line = f'#define %-45s %d\n' %(macro_name, 1)
        # else:
        #     line = f'#define %-45s %d\n' %(macro_name, 0)
        # f.write(line)

        # macro_name = f'CONFIG_TFM_S_JUMP_TO_CPU0_APP'
        # if partition.primary_cpu0_app_exists == True:
        #     line = f'#define %-45s %d\n' %(macro_name, 1)
        # else:
        #     line = f'#define %-45s %d\n' %(macro_name, 0)
        # f.write(line)

        macro_name = f'CPU_VECTOR_ALIGN_SZ'
        cpu_vector_align_bytes = 512
        line = f'#define %-45s %d\n' %(macro_name, cpu_vector_align_bytes)
        f.write(line)

        partition_struct_array = f"#define PARTITION_MAP {{ \\\n"
        for partition in self.raw_part_info:

            partition_name:str = partition["Name"]
            partition_struct_array +=  f"    {{\"{partition_name}\""
            partition_name = partition_name.upper()
            partition_name = partition_name.replace(' ', '_')

            logger.debug(f'generate constants for partition {partition_name}')

            macro_name = f'CONFIG_{partition_name}_PHY_PARTITION_OFFSET'
            line = f'#define %-45s 0x%x\n' %(macro_name, partition["Offset"])
            f.write(line)
            macro_name = f'CONFIG_{partition_name}_PHY_PARTITION_SIZE'
            line = f'#define %-45s 0x%x\n' %(macro_name, partition["Size"])
            f.write(line)
            partition_struct_array += f""", CONFIG_{partition_name}_PHY_PARTITION_OFFSET, CONFIG_{partition_name}_PHY_PARTITION_SIZE}}, \\\n"""
            if partition["Execute"]:

                if self.crc_enable:
                    vir_partition_size = int(partition["Size"]/34*32)
                    vir_code_offset = int(partition["Offset"]/34*32)
                else:
                    vir_partition_size = partition["Size"]
                    vir_code_offset = partition["Offset"]

                vir_code_size = vir_partition_size

                macro_name = f'CONFIG_{partition_name}_PHY_CODE_START'
                line = f'#define %-45s 0x%x\n' %(macro_name, partition["Offset"])
                f.write(line)


                # if (part_phy_size > 0):
                #     macro_name = f'CONFIG_{partition_name}_PHY_HDR_SIZE'
                #     line = f'#define %-45s 0x%x\n' %(macro_name, part_phy_size)
                #     f.write(line)

                # if (partition.bin_tail_size > 0):
                #     macro_name = f'CONFIG_{partition_name}_PHY_TAIL_SIZE'
                #     line = f'#define %-45s 0x%x\n' %(macro_name, partition.bin_tail_size)
                #     f.write(line)

                if (vir_partition_size > 0):
                    macro_name = f'CONFIG_{partition_name}_VIRTUAL_PARTITION_SIZE'
                    line = f'#define %-45s 0x%x\n' %(macro_name, vir_partition_size)
                    f.write(line)

                    macro_name = f'CONFIG_{partition_name}_VIRTUAL_CODE_START'
                    line = f'#define %-45s 0x%x\n' %(macro_name, vir_code_offset)
                    f.write(line)

                    macro_name = f'CONFIG_{partition_name}_VIRTUAL_CODE_SIZE'
                    line = f'#define %-45s 0x%x\n' %(macro_name, vir_code_size)
                    f.write(line)

            f.write('\n')
        partition_struct_array += f"}}\n"
        f.write(partition_struct_array)
        f.flush()
        f.close()
        logger.info(f"gen partition layout header to {partition_hdr_file}")

    def gen_flash_partitions_src(self, hdr_path, src_path):
        self._gen_part_table_hdr(hdr_path)
        self._gen_part_table_src(src_path)
    
    def gen_pack_json(self, pack_json):
        KEYWORDS = {
            "application": "app",
            "application1": "app1",
            "application2": "app2",
        }
        config_dict = {
            "magic": "beken",
            "crc_enable": self.crc_enable,
            "count": 0,
            "section": [],
        }
        sec_dict_temp = {
            "firmware": None,
            "partition": None,
            "start_addr": None,
            "size": None
        }
        # just executed partition need output to config_json
        exectue_partitions = [p for p in self.part_info if p["Execute"]]
        config_dict['count'] = len(exectue_partitions)
        for p in sorted(exectue_partitions, key=lambda x:x["Offset"]):
            sec_dict = dict()
            sec_dict.update(sec_dict_temp)
            part_name = KEYWORDS[p["Name"]] if KEYWORDS.get(p["Name"]) else p["Name"]
            sec_dict['firmware'] = "%s.bin" % part_name
            sec_dict['partition'] = "%s" % part_name
            sec_dict['start_addr'] = size_format(p["Offset"], False)
            sec_dict['size'] = size_format(p["Size"], True)
            config_dict['section'].append(sec_dict)
        
        logger.info(f"gen package json: {pack_json}")
        with open(pack_json, 'w') as f:
            json.dump(config_dict, f, sort_keys=False, indent=4)
