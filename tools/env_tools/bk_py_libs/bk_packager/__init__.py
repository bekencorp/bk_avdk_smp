
__version__ = '0.0.3'

import logging

logger = logging.getLogger(__name__)

def set_debug_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

def set_info_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)

from .bk_packager_linear import bk_packager_linear
from .bk_packager_format import bk_packager_format
from .bk_packager_linear_crc import bk_packager_linear_crc
from .bk_packager_format_crc import bk_packager_format_crc
