import os
import sys
import json
import copy
import logging
import shutil
import bk_packager
import platform
from bk_bootloader_post import get_bootloader_archieve_dir
from bk_bootloader_post import check_is_ab_project

ARMINO_SOC = os.environ['ARMINO_SOC']
PROJECT_BUILD_DIR = os.environ['PROJECT_BUILD_DIR']
armino_tools_path = os.getenv("ARMINO_TOOLS_PATH")
pack_boot_tools = '%s/env_tools/beken_packager'%(armino_tools_path)
header_path = "{}/env_tools/rtt_ota/ota-rbl/".format(armino_tools_path)
ota_tool = '%s/env_tools/rtt_ota/ota-rbl/ota_packager_python.py'%(armino_tools_path)
armino_path = os.getenv("ARMINO_CP_DIR")
project_dir = os.getenv("PROJECT_DIR")

def get_pack_tool_exe():
    system = platform.system()
    pack_tool_dir = '%s/env_tools/beken_packager'%(armino_tools_path)
    if system == "Windows":
        return os.path.join(pack_tool_dir, 'cmake_Gen_image.exe')
    elif system == "Linux":
        return os.path.join(pack_tool_dir, 'cmake_Gen_image')
    elif system == "Darwin":
        raise RuntimeError("not support macos")
    else:
        raise RuntimeError("unknown system type")

logger = logging.getLogger(os.path.basename(__file__))
def set_logging():
    log_format='[%(name)s|%(levelname)s] %(message)s'
    logging.basicConfig(format=log_format, level=logging.INFO)

def copy_binaries_to_pack_dir(origin_path, pack_path):
    if not os.path.exists(origin_path):
        raise FileNotFoundError(f"{origin_path} not found.")
    shutil.copy(origin_path, pack_path)

def prepare_package_dependencies(project_build_dir, pack_dir):
    if not os.path.exists(pack_dir):
        os.mkdir(pack_dir)
    # copy bootloader cp ap binary
    bootloader_name = "bootloader.bin"
    bootloader_dir = get_bootloader_archieve_dir()
    origin_bootloader_path = f"{bootloader_dir}/{bootloader_name}"
    pack_bootloader_path = f"{pack_dir}/{bootloader_name}"
    ota_json = f"{PROJECT_BUILD_DIR}/partitions/bk_ota_partitions.json"
    ret = os.system('%s genfile -injsonfile %s/config.json -infile %s -outfile %s -genjson %s' %
              (get_pack_tool_exe(), pack_boot_tools, origin_bootloader_path, pack_bootloader_path, ota_json))
    if ret != 0:
        raise RuntimeError("attach ota partitions to bootloader fail!")
    logger.info(f"attach ota partitions to bootloader")
    
    cp_name = "app.bin"
    cp_dir = f"{project_build_dir}/{ARMINO_SOC}"
    origin_cp_path = f"{cp_dir}/{cp_name}"
    pack_cp_path = f"{pack_dir}/{cp_name}"
    copy_binaries_to_pack_dir(origin_cp_path, pack_cp_path)

    ap_name = "app.bin"
    ap_dir = f"{project_build_dir}/{ARMINO_SOC}_ap"
    origin_ap_path = f"{ap_dir}/{ap_name}"
    pack_ap_path = f"{pack_dir}/app1.bin"
    copy_binaries_to_pack_dir(origin_ap_path, pack_ap_path)

def parse_format_size(size_str:str):
    size_str = size_str.lower()
    if size_str.endswith('m'):
        return int(size_str[:-1]) * 1024 * 1024
    if size_str.endswith('k'):
        return int(size_str[:-1]) * 1024
    return int(size_str)

class bk_smp_packager:
    def __init__(self, pack_dir, pack_json):
        if not os.path.exists(pack_dir):
            raise RuntimeError(f"{pack_dir} not found")
        if not os.path.exists(pack_json):
            raise FileNotFoundError(f"{pack_json} not found")
        self.pack_dir = pack_dir
        self.pack_json = pack_json
        with open(self.pack_json, 'r') as f:
            self.part_info = json.load(f)
        self.crc_enable = self.part_info["crc_enable"]

    def pack_all_bin(self, output_bin):
        def binary_align_32_byte(bin_path):
            if not os.path.exists(bin_path):
                raise RuntimeError(f"{bin_path} no exist.")
            bin_size = os.path.getsize(output_bin)
            padding_size = (32 - bin_size % 32) % 32
            with open(bin_path, 'ab') as f:
                f.write(bytes([0xff] * padding_size))

        if self.crc_enable:
            packager = bk_packager.bk_packager_linear_crc(self.pack_dir, self.pack_json, output_bin)
        else:
            packager = bk_packager.bk_packager_linear(self.pack_dir, self.pack_json, output_bin)

        packager.pack()
        # cmake_Gen_img 32byte align, so do same here.
        binary_align_32_byte(output_bin)

    def pack_ota_app_bin(self, output_bin):
        apps_part_info = copy.deepcopy(self.part_info)
        sections:list = apps_part_info["section"]
        for index, part in enumerate(sections):
            if "bootloader" in part["partition"]:
                sections.pop(index)
                apps_part_info["count"] -= 1
                break

        if self.crc_enable:
            for part in sections:
                addr = int(int(part["start_addr"], 16) / 34 * 32)
                part["start_addr"] = f"0x{addr:08x}"
                size = parse_format_size(part["size"]) / 34 * 32
                size_format = int(size / 1024)
                part["size"] = f"{size_format}K"
        app_pack_json = f"{self.pack_dir}/ota_apps_pack.json"
        with open(app_pack_json, 'w') as f:
            json.dump(apps_part_info, f, indent=4)

        ota_app_bin = output_bin
        packager = bk_packager.bk_packager_linear(self.pack_dir, app_pack_json, ota_app_bin)
        packager.pack()

def pack_ota_rbl_non_ab(pack_dir, origin_ota_app_bin):
    ota_bin = 'app_pack.rbl'
    ret = os.system('python3 %s -i %s -o %s -g %s -ap %s -pjd %s packager'%(ota_tool,
                origin_ota_app_bin, ota_bin, header_path, armino_path, project_dir))
    if ret != 0:
        raise RuntimeError("generate ota rbl file fail.")
    logger.info(f"generate ota firmware {ota_bin}")

def get_crc_tool_exe():
    system = platform.system()
    crc_tool_dir = '%s/env_tools/beken_packager'%(armino_tools_path)
    if system == "Windows":
        return os.path.join(crc_tool_dir, 'cmake_encrypt_crc.exe')
    elif system == "Linux":
        return os.path.join(crc_tool_dir, 'cmake_encrypt_crc')
    elif system == "Darwin":
        raise RuntimeError("not support macos")
    else:
        raise RuntimeError("unknown system type")

def crc_from_config_json(origin_file):
    crc_tool = get_crc_tool_exe()
    if os.path.exists(crc_tool.strip()) and os.path.isfile(crc_tool.strip()):
        os.system("%s -crc %s"%(crc_tool, origin_file))
    else:
        raise RuntimeError('crc_tool path error!')

def pack_ota_rbl_ab(pack_dir, bootloader_size, origin_ota_app_bin, all_app_bin):
    ota_bin = 'app_ab_crc.rbl'
    ota_app_temp_bin = f"{pack_dir}/ota_app_temp.bin"
    ret = os.system('python3 %s -i %s -o %s -g %s -ap %s -soc %s -pjd %s packager'%(ota_tool,
        origin_ota_app_bin, ota_app_temp_bin, header_path, armino_path, ARMINO_SOC, project_dir))
    if ret != 0:
        raise RuntimeError("generate ota rbl file fail.")

    crc_from_config_json(ota_app_temp_bin)
    logger.info(f"generate ota firmware {ota_bin}")
    ota_app_temp_crc_bin = f"{pack_dir}/ota_app_temp_crc.bin"
    shutil.copy(ota_app_temp_crc_bin, ota_bin)
    os.remove(ota_app_temp_bin)
    os.remove(ota_app_temp_crc_bin)
    
    with open(all_app_bin , 'r+b') as dest_f, \
        open(ota_bin, 'rb') as src_f:
        dest_f.seek(bootloader_size)
        write_data = src_f.read()
        dest_f.write(write_data)
    logger.info(f"overwrite all_app.bin with {ota_bin}")

def pack_ota_rbl(pack_dir, pack_json, origin_ota_app_bin, all_app_bin):
    if check_is_ab_project() == "False":
        pack_ota_rbl_non_ab(pack_dir, origin_ota_app_bin)
        return
    
    with open(pack_json, 'r') as f:
        pack_info = json.load(f)
    bootloader_size_fmt = "0"
    for part in pack_info["section"]:
        if part["partition"] == "bootloader":
            bootloader_size_fmt = part["size"]
            break
    bootloader_size = parse_format_size(bootloader_size_fmt)
    if bootloader_size == 0:
        raise RuntimeError(f"bootloader parse error")
    pack_ota_rbl_ab(pack_dir, bootloader_size, origin_ota_app_bin, all_app_bin)

if __name__ == '__main__':
    set_logging()
    project_build_dir = sys.argv[1]
    raw_pack_json = sys.argv[2]
    pack_dir = os.path.join(project_build_dir, "package")
    pack_json = os.path.join(pack_dir, "bk_package.json")
    shutil.copy(raw_pack_json, pack_json)
    logger.info(f"Enter SMP Package")
    pack_dir_temp = f"{pack_dir}/tmp"
    prepare_package_dependencies(project_build_dir, pack_dir_temp)
    os.chdir(pack_dir)
    packager = bk_smp_packager(pack_dir_temp, pack_json)
    all_app_bin = f"{pack_dir}/all-app.bin"
    packager.pack_all_bin(all_app_bin)
    origin_ota_app_bin = f"{pack_dir_temp}/origin_ota_app.bin"
    packager.pack_ota_app_bin(origin_ota_app_bin)
    pack_ota_rbl(pack_dir_temp, pack_json, origin_ota_app_bin, all_app_bin)
