"""Low-level primitives: dump region parsing, memory map, addr2line."""
from __future__ import annotations

import base64
import re
import struct
import subprocess
from pathlib import Path
from typing import Iterable

# region:HPDMA, stack_top=4c300000, stack end=4c300140
# The serial logger prepends "[Serial-COM*-YYYYMMDD-HH:MM:SS.mmm]" timestamps
# which must be stripped before base64 decode (every line is otherwise off-
# alignment by the embedded timestamp bytes).
TS_RE = re.compile(r"^\[(?:Serial|UART)-[^\]]+\]\s*")
BEGIN_RE = re.compile(
    r">>>>stack mem dump begin,\s*region:\s*(?P<region>\S+?),"
    r"\s*stack_top=(?P<top>[0-9a-fA-F]+),\s*stack end=(?P<end>[0-9a-fA-F]+)"
)
END_RE = re.compile(r"<<<<stack mem dump end")


# ----------------------------------------------------------------------------
# Dump region decoder
# ----------------------------------------------------------------------------
def strip_ts(line: str) -> str:
    return TS_RE.sub("", line)


def parse_dump_regions(path: Path) -> list[tuple[str, int, bytes, int]]:
    """Return a list of ``(region_name, start_addr, decoded_bytes, expected_size)``.

    The CP-side dump emits base64-encoded 33-byte chunks (32 data + 1 CRC8).
    Lines whose base64 fails to validate are silently skipped (this protects
    against an embedded log line accidentally landing in the middle of a
    dump block).
    """
    text = path.read_text(encoding="utf-8", errors="ignore")
    lines = text.splitlines()
    regions: list[tuple[str, int, bytes, int]] = []
    i = 0
    while i < len(lines):
        clean = strip_ts(lines[i])
        m = BEGIN_RE.search(clean)
        if not m:
            i += 1
            continue
        region = m.group("region").strip(",")
        start = int(m.group("top"), 16)
        end = int(m.group("end"), 16)
        expected = end - start
        i += 1
        data = bytearray()
        while i < len(lines):
            clean = strip_ts(lines[i])
            if END_RE.search(clean):
                break
            chunk = clean.strip()
            if chunk:
                try:
                    raw = base64.b64decode(chunk, validate=True)
                except Exception:
                    i += 1
                    continue
                if len(raw) >= 1:
                    # Drop the trailing CRC8 byte.
                    data.extend(raw[:-1])
            i += 1
        if expected > 0 and len(data) > expected:
            data = data[:expected]
        regions.append((region, start, bytes(data), expected))
        i += 1
    return regions


def build_memory_map(regions: Iterable[tuple[str, int, bytes, int]]) -> list[tuple[int, bytes]]:
    """Sort decoded regions by address and merge contiguous fragments."""
    mp = sorted([(s, d) for _, s, d, _ in regions if d])
    merged: list[list] = []
    for start, data in mp:
        if not merged:
            merged.append([start, bytearray(data)])
            continue
        prev_start, prev_data = merged[-1]
        prev_end = prev_start + len(prev_data)
        if start == prev_end:
            prev_data.extend(data)
        elif start < prev_end:
            overlap = prev_end - start
            if len(data) > overlap:
                prev_data.extend(data[overlap:])
        else:
            merged.append([start, bytearray(data)])
    return [(s, bytes(d)) for s, d in merged]


def read_u32(memmap: list[tuple[int, bytes]], address: int) -> int | None:
    for start, data in memmap:
        if start <= address and address + 4 <= start + len(data):
            off = address - start
            return struct.unpack_from("<I", data, off)[0]
    return None


def read_bytes(memmap: list[tuple[int, bytes]], address: int, size: int) -> bytes | None:
    for start, data in memmap:
        if start <= address and address + size <= start + len(data):
            off = address - start
            return data[off:off + size]
    return None


def addr_in_range(memmap: list[tuple[int, bytes]], address: int) -> bool:
    for start, data in memmap:
        if start <= address < start + len(data):
            return True
    return False


# ----------------------------------------------------------------------------
# ELF utilities (addr2line / readelf)
# ----------------------------------------------------------------------------
def addr2line(elf: Path, addrs: list[int]) -> list[str]:
    """Resolve addresses to ``function at file:line`` strings."""
    if not addrs:
        return []
    args = ["arm-none-eabi-addr2line", "-piaf", "-e", str(elf)] + [f"0x{a:08x}" for a in addrs]
    try:
        out = subprocess.run(args, capture_output=True, text=True, check=False, timeout=120)
    except FileNotFoundError:
        args[0] = "addr2line"
        out = subprocess.run(args, capture_output=True, text=True, check=False, timeout=120)
    if out.returncode != 0:
        return [out.stderr or out.stdout]
    return out.stdout.splitlines()


_LOAD_RE = re.compile(
    r"^\s*LOAD\s+"
    r"(0x[0-9a-fA-F]+)\s+"            # Offset
    r"(0x[0-9a-fA-F]+)\s+"            # VirtAddr
    r"(0x[0-9a-fA-F]+)\s+"            # PhysAddr
    r"(0x[0-9a-fA-F]+)\s+"            # FileSiz
    r"(0x[0-9a-fA-F]+)\s+"            # MemSiz
    r"([RWE ]+)\s+"                    # Flg
    r"(0x[0-9a-fA-F]+)"                # Align
)


def get_text_ranges(elf: Path) -> list[tuple[int, int, str]]:
    """Return ``[(vma_start, vma_end, label)]`` for every executable LOAD."""
    out = subprocess.run(
        ["arm-none-eabi-readelf", "-lW", str(elf)],
        capture_output=True, text=True, check=True
    ).stdout
    ranges: list[tuple[int, int, str]] = []
    for line in out.splitlines():
        m = _LOAD_RE.match(line)
        if not m:
            continue
        vaddr = int(m.group(2), 16)
        memsz = int(m.group(5), 16)
        flags = m.group(6)
        if "E" in flags and memsz > 0:
            ranges.append((vaddr, vaddr + memsz, "LOAD"))
    return ranges


def in_code(ranges: list[tuple[int, int, str]], addr: int) -> bool:
    addr &= ~1
    for s, e, _ in ranges:
        if s <= addr < e:
            return True
    return False


def looks_like_code(value: int | None, ranges: list[tuple[int, int, str]] | None = None) -> bool:
    """True if ``value`` looks like a Thumb return address.

    When ``ranges`` is provided the address must fall inside an executable
    segment of the ELF; otherwise we fall back to BK7259-specific Flash /
    PSRAM-code / IRAM windows (used by older debug tools).
    """
    if value is None or (value & 1) == 0:
        return False
    if ranges is not None:
        return in_code(ranges, value)
    masked = value & ~1
    if 0x04132000 <= masked < 0x041c0000:
        return True
    if 0x64c00000 <= masked < 0x65000000:
        return True
    if 0x28180000 <= masked < 0x281c0000:
        return True
    return False


# ARMv8-M EXC_RETURN tokens (basic / extended, secure / non-secure, MSP / PSP).
EXC_RETURN_VALUES = {
    0xFFFFFFE1, 0xFFFFFFE9, 0xFFFFFFED,
    0xFFFFFFF1, 0xFFFFFFF9, 0xFFFFFFFD,
    0xFFFFFFA0, 0xFFFFFFA8, 0xFFFFFFAC,
    0xFFFFFFB0, 0xFFFFFFB8, 0xFFFFFFBC,
}


# AP-side IRQ source table (mirrors `icu_int_src_t` in
# ap/include/soc/bk7259/int_types_impl.h).
AP_IRQ_NAMES: dict[int, str] = {
    0:  "INT_SRC_M52S",       1:  "INT_SRC_HPDMA",      2:  "INT_SRC_MAILBOX",
    3:  "INT_SRC_IPI",        4:  "INT_SRC_GDMA0",      5:  "INT_SRC_CPU0_FPU",
    6:  "INT_SRC_NPU",        7:  "INT_SRC_USB_FS",     8:  "INT_SRC_USB_HS",
    9:  "INT_SRC_USB_PLUG",   10: "INT_SRC_UART5",      11: "INT_SRC_WWDT",
    12: "INT_SRC_SDIO0",      13: "INT_SRC_SDIO1",      14: "INT_SRC_INET0",
    16: "INT_SRC_QSPI0",      17: "INT_SRC_QSPI1",      18: "INT_SRC_HSPL",
    19: "INT_SRC_ISP_MI",     20: "INT_SRC_ISP_FE",     21: "INT_SRC_ISP_ISP",
    22: "INT_SRC_CSI",        23: "INT_SRC_H26E",       24: "INT_SRC_GPU",
    25: "INT_SRC_H264D",      26: "INT_SRC_DPU",        27: "INT_SRC_DSI",
    28: "INT_SRC_H264D_PP",   29: "INT_SRC_PSRAM0_ERR", 30: "INT_SRC_PSRAM1_ERR",
    31: "INT_SRC_MPC",        32: "INT_SRC_TIMER4",     33: "INT_SRC_TIMER5",
    34: "INT_SRC_GPIO_NS",    35: "INT_SRC_GPIO",       36: "INT_SRC_AUDIO",
    37: "INT_SRC_I2S0",       38: "INT_SRC_I2S1",       39: "INT_SRC_I2S2",
    40: "INT_SRC_I2S3",       41: "INT_SRC_I2S4",       42: "INT_SRC_SPDIF0",
    43: "INT_SRC_SPDIF1",     44: "INT_SRC_CEC",        45: "INT_SRC_I2C0",
    46: "INT_SRC_I2C1",       47: "INT_SRC_I3C",        48: "INT_SRC_UART0",
    49: "INT_SRC_UART1",      50: "INT_SRC_UART2",      51: "INT_SRC_UART3",
    52: "INT_SRC_UART4",      53: "INT_SRC_L2CACHE_ERR",
}


def irq_name(irq: int) -> str:
    return AP_IRQ_NAMES.get(irq, f"IRQ{irq}")
