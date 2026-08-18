import csv
import logging
import os
import re
import shutil
import sys
from pathlib import Path
from typing import Dict, Optional


logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


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


def install_configs(cfg_dir: Path, install_dir: Path):
    logger.debug(f"install configs from: {cfg_dir}")
    logger.debug(f"to: {install_dir}")
    if not cfg_dir.exists():
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
        copy_files(".pem", f"{cfg_dir}/key", str(install_dir))
    if os.path.exists(f"{cfg_dir}/csv"):
        copy_files(".csv", f"{cfg_dir}/csv", str(install_dir))
    if os.path.exists(f"{cfg_dir}/regs"):
        copy_files(".csv", f"{cfg_dir}/regs", str(install_dir))


def gpio_dev_to_ppc_device(gpio_dev: str, app_name: str) -> Optional[str]:
    """Map an active IO-matrix function to its PPC/PPHS policy device."""
    if gpio_dev in {
        "GPIO_DEV_INVALID",
        "GPIO_DEV_GPIO_INPUT",
        "GPIO_DEV_GPIO_OUTPUT",
    }:
        return None

    is_ap = app_name.endswith("_ap")
    if re.fullmatch(r"GPIO_DEV_PWM(?:[0-9]|1[01])", gpio_dev):
        # BK7259 exposes one 12-channel PWM unit through the PWM0 PPC device.
        return "PWM0"

    prefix_map = (
        ("GPIO_DEV_SDIO", "SDIO"),
        ("GPIO_DEV_QSPI0_", "QSPI0"),
        ("GPIO_DEV_QSPI1_", "QSPI1"),
        ("GPIO_DEV_USB", "USB"),
        ("GPIO_DEV_LCD_", "VIDP" if is_ap else "DISP"),
        ("GPIO_DEV_JPEG_", "ISP" if is_ap else "JPGD"),
        ("GPIO_DEV_CLK_AUXS_CIS", "ISP" if is_ap else "JPGD"),
        ("GPIO_DEV_DMIC", "AUD"),
        ("GPIO_DEV_I2S0_", "I2S0"),
        ("GPIO_DEV_I2S1_", "I2S1"),
        ("GPIO_DEV_I2S2_", "I2S2"),
        ("GPIO_DEV_UART0_", "UART0"),
        ("GPIO_DEV_UART1_", "UART1"),
        ("GPIO_DEV_UART5_", "UART5"),
    )
    for prefix, device in prefix_map:
        if gpio_dev.startswith(prefix):
            return device

    raise ValueError(f"unsupported active GPIO function in usr_gpio_cfg.h: {gpio_dev}")


def parse_usr_gpio_cfg(config_file: Path, app_name: str) -> Dict[str, str]:
    """Parse the active boot-time mappings from GPIO_DEFAULT_DEV_CONFIG."""
    if not config_file.exists():
        return {}

    text = config_file.read_text(encoding="utf-8")
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"//.*", "", text)

    mappings: Dict[str, str] = {}
    for body in re.findall(r"\{([^{}]+)\}", text):
        fields = [field.strip().rstrip("\\") for field in body.split(",")]
        if len(fields) < 10 or not re.fullmatch(r"GPIO_\d+", fields[0]):
            continue
        if fields[1] != "GPIO_SECOND_FUNC_ENABLE":
            continue
        if fields[9] != "GPIO_INIT_ENABLE":
            continue

        device = gpio_dev_to_ppc_device(fields[2], app_name)
        if device is not None:
            mappings[fields[0].replace("GPIO_", "GPIO")] = device

    return mappings


def generate_gpio_dev_csv(curr_project, app_name: str, outfile: Path):
    """Generate GPIO security-check input from the project's real pinmux."""
    mappings: Dict[str, str] = {}
    for app in curr_project.apps_info:
        config_file = (
            curr_project.project_path
            / app.app_name_in_sdk.lower()
            / "config"
            / app.app_name
            / "usr_gpio_cfg.h"
        )
        for gpio, device in parse_usr_gpio_cfg(config_file, app_name).items():
            previous = mappings.get(gpio)
            if previous is not None and previous != device:
                raise ValueError(
                    f"{gpio} is configured by multiple cores: {previous} and {device}"
                )
            mappings[gpio] = device

    if not mappings:
        logger.debug("No project usr_gpio_cfg.h mappings, keep board gpio_dev.csv")
        return

    with outfile.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(("GPIO", "Device"))
        for gpio in sorted(mappings, key=lambda name: int(name[4:])):
            writer.writerow((gpio, mappings[gpio]))
    logger.debug(f"generated {outfile} from project usr_gpio_cfg.h")


def prebuild():
    from bk_curr_project import curr_project

    soc_name: str = curr_project.soc_name
    tools_dir = curr_project.tools_path
    cpu0_armino_soc = curr_project.app0_name

    app_name = sys.argv[1]
    cmake_partition_bin_dir = (
        curr_project.project_build_dir / app_name / "armino/partitions/_build"
    )

    middleware_soc_cfg_dir = curr_project.get_middleware_soc_config_path(app_name)
    project_partitions_root = curr_project.project_path / "partitions"
    project_partitions_dir = curr_project.partitions_dir

    logger.debug(f"tools_dir={tools_dir}")
    logger.debug(f"base_cfg_dir={middleware_soc_cfg_dir}")
    logger.debug(f"prefered_cfg_dir={project_partitions_dir}")
    logger.debug(f"soc={soc_name}")

    bk_utils_script = tools_dir / "env_tools/beken_utils/main.py"
    logger.debug("Create temporary _build")
    os.makedirs(cmake_partition_bin_dir, exist_ok=True)
    os.chdir(cmake_partition_bin_dir)
    logger.debug(f"cd {cmake_partition_bin_dir}")
    copy_file(
        f"{middleware_soc_cfg_dir}/partitions/bl1_control.json",
        f"{cmake_partition_bin_dir}/bl1_control.json",
    )

    install_configs(middleware_soc_cfg_dir, cmake_partition_bin_dir)
    install_configs(project_partitions_dir, cmake_partition_bin_dir)
    install_configs(project_partitions_root / "common", cmake_partition_bin_dir)
    if app_name != cpu0_armino_soc:
        install_configs(project_partitions_root / app_name, cmake_partition_bin_dir)
    # Secure-boot projects derive the GPIO security map from their actual
    # AP/CP pinmux instead of relying on the board's legacy gpio_dev.csv.
    if curr_project.project_path.name in {"secureboot_xip", "secureboot_ai"}:
        generate_gpio_dev_csv(
            curr_project,
            app_name,
            cmake_partition_bin_dir / "gpio_dev.csv",
        )

    generated_partitions_csv = (
        curr_project.project_build_parititons_dir / "partitions.csv"
    )
    if not generated_partitions_csv.exists():
        raise FileNotFoundError(
            f"generated partition table not found: {generated_partitions_csv}"
        )
    copy_file(
        str(generated_partitions_csv),
        f"{cmake_partition_bin_dir}/partitions.csv",
    )

    logger.debug("partition pre-processing")
    # `gen all` (scripts/gen_code.py) already generates partitions_gen.h and
    # partitions_partition.h in one shot (matching the BK7234N flow), so a
    # separate `gen partition` call would just regenerate identical files.
    ret = os.system(f"python3 {bk_utils_script} gen all --debug")
    if ret != 0:
        raise RuntimeError(f"run {bk_utils_script} fail")

    # NOTE: do NOT sync the generated partitions_gen.h into the TF-M platform stub
    # here. prebuild() runs for the CP side of EVERY project (both the non-secure
    # `app` and the secure `secureboot_xip`), so syncing here blindly copied the
    # project-specific PHY header into the shared committed TF-M stub - the app
    # build would overwrite the secure stub with its own (primary_bootloader/
    # primary_cp_app) layout even though app has CONFIG_TFM=n and never builds TF-M.
    # The stub is now re-synced only by the CMake target sync_tfm_stub_partitions in
    # cp/components/tfm/CMakeLists.txt, which is gated by `if (CONFIG_TFM ...)` and
    # therefore fires solely for the TF-M (secureboot_xip) build.


if __name__ == "__main__":
    set_logging()
    sys.path.append(str(Path(__file__).parent.parent))
    prebuild()
