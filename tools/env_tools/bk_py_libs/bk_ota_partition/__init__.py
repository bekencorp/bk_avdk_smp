
__version__ = '0.0.3'

import logging

logger = logging.getLogger(__name__)

def set_debug_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

def set_info_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)


from .bk_ota_partition import bk_ota_partition