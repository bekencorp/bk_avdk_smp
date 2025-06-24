import logging
import sys
import unittest
from pathlib import Path


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    # 设置默认打印等级
    logging.basicConfig(format=log_format, level=logging.INFO)


def TestMain():
    """通过默认加载器"""
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    currPath = Path(__file__).resolve().parent
    for item in currPath.iterdir():
        if item.is_dir() and (item / "ut_main.py").exists():
            sys.path.append(str(item))
            discovered_tests = loader.discover(str(item), top_level_dir=str(currPath))
            suite.addTests(discovered_tests)

    runner = unittest.TextTestRunner(verbosity=1)
    runner.run(suite)


if __name__ == "__main__":
    set_logging()
    currPath = Path(__file__).resolve().parent
    packagePath = currPath
    sys.path.append(str(packagePath))
    packagePath = currPath.parent
    sys.path.append(str(packagePath))
    TestMain()
