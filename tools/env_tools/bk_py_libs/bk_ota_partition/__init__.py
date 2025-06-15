__version__ = "0.0.3"

import logging

logger = logging.getLogger(__name__)

from .bk_ota_partition import bk_ota_partition  # noqa: E402


def set_debug_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)


def set_info_log() -> None:
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)


__all__ = ["bk_ota_partition"]
