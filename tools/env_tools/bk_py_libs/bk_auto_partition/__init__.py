__version__ = "0.0.1"
import logging

logger = logging.getLogger(__name__)


from .bk_partitions_table import bk_partitions_table  # noqa: E402


def set_debug_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)


def set_info_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)


__all__ = ["bk_partitions_table", "logger"]
