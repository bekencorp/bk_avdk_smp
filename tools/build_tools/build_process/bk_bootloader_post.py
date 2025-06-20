import logging
import shutil
from pathlib import Path

from bk_curr_project import curr_project

logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def backup_bootloader_path():
    # copy bootloader elf-map-asm to build directory
    bootloader_build_dir = curr_project.bootloader_build_path.parent
    if not bootloader_build_dir.exists():
        logger.debug("bootloader not build, not backup.")
        return

    bootloader_backup_path = curr_project.bootloader_backup_dir
    bootloader_backup_path.parent.mkdir(parents=True, exist_ok=True)
    if bootloader_backup_path.exists():
        shutil.rmtree(bootloader_backup_path)

    shutil.copytree(bootloader_build_dir, bootloader_backup_path)
    logger.info(f"backup bootloader to {bootloader_backup_path}")


def copy_bootloader_to_relevant_dir():
    if curr_project.project_name == "ate_mini_code":
        return
    bootloader_build_bin_path = curr_project.bootloader_build_path
    if not bootloader_build_bin_path.exists():
        logger.info("bootloader not build")
        return

    bootloader_archive_bin_path = curr_project.bootloader_archive_path
    # copy bootloader bin to components/bk_libs
    bootloader_archive_bin_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy(bootloader_build_bin_path, bootloader_archive_bin_path)
    logger.info("copy bootloader.bin")
    logger.info(f"bootloader build path: {bootloader_build_bin_path}")
    logger.info(f"bootloader archive path: {bootloader_archive_bin_path}")


if __name__ == "__main__":
    set_logging()
    copy_bootloader_to_relevant_dir()
