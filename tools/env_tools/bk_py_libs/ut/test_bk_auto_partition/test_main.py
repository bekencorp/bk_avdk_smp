import unittest
import sys
import os
import logging

def set_logging():
    log_format='[%(name)s|%(levelname)s] %(message)s'
    # 设置默认打印等级
    logging.basicConfig(format=log_format, level=logging.INFO)

def TestMain():
    """ 通过默认加载器 """
    suite = unittest.defaultTestLoader.discover(os.path.dirname(os.path.abspath(__file__)))
    runner = unittest.TextTestRunner(verbosity=1)
    runner.run(suite)

if __name__ == '__main__':
    set_logging()
    currPath = os.path.dirname(os.path.abspath(__file__))
    packagePath = os.path.dirname(currPath)
    sys.path.append(packagePath)
    packagePath = os.path.dirname(os.path.dirname(currPath))
    sys.path.append(packagePath)
    TestMain()

