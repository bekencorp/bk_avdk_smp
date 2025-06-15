import logging

__version__ = "0.0.1"
__all__ = ["bk_build_summary"]
logger = logging.getLogger(__name__)

from .bk_build_summary import bk_build_summary  # noqa: E402


def set_debug_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)


def set_info_log():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)
