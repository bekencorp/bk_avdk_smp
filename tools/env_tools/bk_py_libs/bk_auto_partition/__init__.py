import logging

from .bk_partitions_table import bk_partitions_table

__version__ = "0.0.1"

__all__ = ["bk_partitions_table"]


def set_debug_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)


def set_info_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)
