__version__ = "0.0.1"
import logging

logger = logging.getLogger(__name__)

from .bk_ram_region import bk_ram_region


def set_debug_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)


def set_info_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)


__all__ = ["logger", "bk_ram_region"]
