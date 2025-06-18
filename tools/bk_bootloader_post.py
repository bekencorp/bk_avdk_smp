import csv
import logging
import os
import shutil
from pathlib import Path

logger = logging.getLogger(Path(__file__).name)

armino_path = os.getenv("ARMINO_CP_DIR")
project_dir = os.getenv("PROJECT_DIR")
armino_tools_path = os.getenv("ARMINO_TOOLS_PATH")
armino_soc = os.getenv("ARMINO_SOC")
project_name = os.getenv("PROJECT")
project_build_dir = os.getenv("PROJECT_BUILD_DIR")


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


# Read the csv file
def read_csv_data(file_path: str):
    data: dict[str, str] = {}
    with open(file_path, "r") as file:
        reader = csv.reader(file)
        next(reader)  # Skip header line
        for row in reader:
            data[row[0]] = row[1]
    return data


def get_ab_pos_independent_value(ab_input_file: str) -> bool:
    data = read_csv_data(ab_input_file)
    if data.get("pos_independent").lower() == "true":  # type: ignore
        output_value = True
    else:
        output_value = False

    return output_value


# copy bootloader.bin to components/bk_libs
def copy_bootloader_to_component_bklibs_dir(sour_boot_dir: str, dest_boot_dir: str):
    if not os.path.exists(dest_boot_dir):
        os.makedirs("%s/" % (dest_boot_dir))
    else:
        print("exsit", dest_boot_dir)
    shutil.copy(
        "%s/%s/bootloader.bin" % (armino_path, sour_boot_dir),
        "%s/bootloader.bin" % (dest_boot_dir),
    )


def check_is_ab_project() -> bool:
    if os.path.exists(
        "%s/partitions/%s/ab_position_independent.csv" % (project_dir, armino_soc)
    ):
        ab_pos_independent_file = "%s/partitions/%s/ab_position_independent.csv" % (
            project_dir,
            armino_soc,
        )
        pos_independent_status = get_ab_pos_independent_value(ab_pos_independent_file)
    else:
        pos_independent_status = False
    return pos_independent_status


def get_bootloader_archieve_dir():
    archieve_dir = f"{armino_path}/components/bk_libs/{armino_soc}/bootloader"
    if check_is_ab_project():
        return f"{archieve_dir}/ab_bootloader"
    else:
        return f"{archieve_dir}/normal_bootloader"


def copy_bootloader_to_relevant_dir():
    if project_name == "ate_mini_code":
        return
    ab_project_bl_path = "properties/modules/bootloader/aboot/arm_bootloader_ab/output"
    non_ab_project_bl_path = "properties/modules/bootloader/aboot/arm_bootloader/output"
    if check_is_ab_project():
        bl_project_path = ab_project_bl_path
        dest_boot_dir = (
            f"{armino_path}/components/bk_libs/{armino_soc}/bootloader/ab_bootloader"
        )
    else:
        bl_project_path = non_ab_project_bl_path
        dest_boot_dir = f"{armino_path}/components/bk_libs/{armino_soc}/bootloader/normal_bootloader"

    bootloader_bin_path = f"{armino_path}/{bl_project_path}/bootloader.bin"
    if not os.path.exists(bootloader_bin_path):
        return

    # copy bootloader bin to components/bk_libs
    copy_bootloader_to_component_bklibs_dir(bl_project_path, dest_boot_dir)

    # copy bootloader elf-map-asm to build directory
    bootloader_backup_path = f"{project_build_dir}/{armino_soc}/bootloader_out"
    if not os.path.exists(bootloader_backup_path):
        os.makedirs(bootloader_backup_path)
    if os.path.exists(bootloader_backup_path):
        shutil.rmtree(bootloader_backup_path)

    shutil.copytree("%s/%s/" % (armino_path, bl_project_path), bootloader_backup_path)


if __name__ == "__main__":
    logger.info("Enter bootloader post")
    set_logging()
    copy_bootloader_to_relevant_dir()
