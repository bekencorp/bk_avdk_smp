import os
from pathlib import Path

from bk_sdk.bk_sdk_project import bk_project_info, bk_sdk_project

if not os.getenv("PROJECT_DIR"):
    raise RuntimeError("get PROJECT_DIR error")
project_dir = Path(os.getenv("PROJECT_DIR", ""))
soc_name = os.getenv("ARMINO_SOC_NAME", "")
project_name = os.getenv("PROJECT_NAME", "")

project_info = bk_project_info(
    project_name, project_dir, soc_name, True, [soc_name, f"{soc_name}_ap"]
)
curr_project = bk_sdk_project(project_info)
