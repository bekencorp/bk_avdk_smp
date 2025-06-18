import logging
import os
import shutil
import subprocess
from pathlib import Path

logger = logging.getLogger(Path(__file__).name)
armino_path = os.getenv("ARMINO_PATH")
armino_tools_path = os.getenv("ARMINO_TOOLS_PATH")
project_dir = os.getenv("PROJECT_DIR")
project_name = os.getenv("PROJECT")
armino_soc = os.getenv("ARMINO_SOC", "")
build_path = os.path.realpath(".")


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def run_cmd(cmd: str):
    p = subprocess.Popen(cmd, shell=True)
    ret = p.wait()
    if ret:
        logger.error(f'failed to run "{cmd}"')
        exit(1)


def copy_file(src: str, dst: str):
    logger.debug(f"copy {src} to {dst}")
    logger.debug(f"to {dst}")
    if os.path.exists(f"{src}"):
        shutil.copy(f"{src}", f"{dst}")


def copy_files(postfix: str, src_dir: str, dst_dir: str):
    logger.debug(f"copy *{postfix} from {src_dir}")
    logger.debug(f"to {dst_dir}")
    for f in os.listdir(src_dir):
        if f.endswith(postfix):
            if os.path.isfile(f"{src_dir}/{f}"):
                shutil.copy(f"{src_dir}/{f}", f"{dst_dir}/{f}")


def install_configs(cfg_dir: str, install_dir: str):
    logger.debug(f"install configs from: {cfg_dir}")
    logger.debug(f"to: {install_dir}")
    if not os.path.exists(f"{cfg_dir}"):
        return

    if os.path.exists(f"{cfg_dir}/partitions.csv") and os.path.exists(
        f"{install_dir}/partitions.csv"
    ):
        logger.debug(f"exists {cfg_dir}/partitions.csv")
        logger.debug(f"remove {install_dir}/partitions.csv")
        os.remove(f"{install_dir}/partitions.csv")

    if os.path.exists(f"{cfg_dir}/csv/partitions.csv") and os.path.exists(
        f"{install_dir}/csv/partitions.csv"
    ):
        logger.debug(f"exists {cfg_dir}/csv/partitions.csv")
        logger.debug(f"remove {install_dir}/partitions.csv")
        os.remove(f"{install_dir}/csv/partitions.csv")

    for f in os.listdir(f"{cfg_dir}"):
        if (
            f.endswith(".csv")
            or f.endswith(".bin")
            or f.endswith(".json")
            or f.endswith(".pem")
        ):
            if f == "partitions.csv":
                continue

            if os.path.isfile(f"{cfg_dir}/{f}"):
                shutil.copy(f"{cfg_dir}/{f}", f"{install_dir}/{f}")

    if os.path.exists(f"{cfg_dir}/key"):
        copy_files(".pem", f"{cfg_dir}/key", install_dir)
    if os.path.exists(f"{cfg_dir}/csv"):
        copy_files(".csv", f"{cfg_dir}/csv", install_dir)
    if os.path.exists(f"{cfg_dir}/regs"):
        copy_files(".csv", f"{cfg_dir}/regs", install_dir)


def prebuild():
    soc: str = armino_soc
    tools_dir = f"{armino_tools_path}/env_tools"
    bin_dir = build_path
    cpu0_armino_soc = armino_soc.replace("_ap", "")
    base_cfg_dir = f"{armino_path}/middleware/boards/{armino_soc}"  # CPU1/CPU2 share the same config with CPU0
    prefered_cfg_dir = f"{project_dir}/partitions"

    logger.debug(f"tools_dir={tools_dir}")
    logger.debug(f"base_cfg_dir={base_cfg_dir}")
    logger.debug(f"prefered_cfg_dir={prefered_cfg_dir}")
    logger.debug(f"soc={soc}")
    BK_UTILS_TOOL = f"{tools_dir}/beken_utils/main.py"
    BASE_CFG_DIR = base_cfg_dir
    _BUILD_DIR = f"{bin_dir}/_build"
    logger.debug("Create temporary _build")
    os.makedirs(_BUILD_DIR, exist_ok=True)
    os.chdir(_BUILD_DIR)
    logger.debug(f"cd {_BUILD_DIR}")
    copy_file(
        f"{base_cfg_dir}/partitions/bl1_control.json", f"{_BUILD_DIR}/bl1_control.json"
    )

    install_configs(BASE_CFG_DIR, _BUILD_DIR)
    install_configs(prefered_cfg_dir, _BUILD_DIR)
    install_configs(f"{prefered_cfg_dir}/common", _BUILD_DIR)
    install_configs(f"{prefered_cfg_dir}/{cpu0_armino_soc}", _BUILD_DIR)
    logger.debug("partition pre-processing")
    run_cmd(f"python3 {BK_UTILS_TOOL} gen all --debug")


if __name__ == "__main__":
    set_logging()
    prebuild()
