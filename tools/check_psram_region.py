import logging
import sys
from pathlib import Path

logger = logging.getLogger(Path(__file__).name)


def set_logging():
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.DEBUG)


def check_psram_overlaps(space_sections: list[tuple[int, int]]):
    intervals = [(start, start + length) for start, length in space_sections]
    intervals.sort()
    for i in range(1, len(intervals)):
        if intervals[i][0] < intervals[i - 1][1]:
            return False
    return True


def parse_build_summary(summary: str):
    def find_app_mem(memory_summary: str):
        pos = memory_summary.find("FLASH")
        if pos == -1:
            raise RuntimeError("find error")
        pos2 = memory_summary.find("<<<<<<<<<<", pos)
        if pos2 == -1:
            raise RuntimeError("find error")
        return memory_summary[pos:pos2], pos2

    memory_region = ""
    try:
        cp_mem_region, end = find_app_mem(summary)
        memory_region += cp_mem_region
    except RuntimeError:
        return memory_region
    left_summary = summary[end:]
    try:
        ap_mem_region, _ = find_app_mem(left_summary)
        memory_region += ap_mem_region
    except RuntimeError:
        pass
    return memory_region


def check_psram_region(mem_region: str):
    lines = mem_region.strip().split("\n")
    space_sections: list[tuple[int, int]] = []
    for line in lines:
        line = line.strip()
        region_info = line.split()
        if "PSRAM" in region_info[0]:
            addr = int(region_info[1], 16)
            size = int(region_info[2], 16)
            space_sections.append((addr, size))

    ret = check_psram_overlaps(space_sections)
    if not ret:
        logger.error(
            "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
        )
        logger.error(
            "PSRAM memory regions overlap, please check psram memory partitions!"
        )
        logger.error(
            "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
        )


def main():
    build_summary = sys.argv[1]
    summary_path = Path(build_summary)
    if not summary_path.exists():
        raise RuntimeError(f"{summary_path.name} not exists.")
    summary = summary_path.read_text()
    mem_region = parse_build_summary(summary)
    check_psram_region(mem_region)


if __name__ == "__main__":
    set_logging()
    main()
