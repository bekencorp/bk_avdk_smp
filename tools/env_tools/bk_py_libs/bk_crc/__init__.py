
__version__ = '0.0.2'

import logging

logger = logging.getLogger(__name__)

def set_debug_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

def set_info_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)

from .bk_crc16 import bk_crc16
from .bk_crc32 import bk_crc32
