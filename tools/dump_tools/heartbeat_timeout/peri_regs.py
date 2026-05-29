"""Peripheral register snapshot decoder.

The dump emits each peripheral with a 16-byte header (magic + count + idx)
followed by the captured register words. Here we just present the raw 32-bit
words; downstream :mod:`report` runs the bit-level heuristics (PSRAM Reg
0x10 fingerprint, PPHS sresp check, etc.).

Region table tracks the union of:
  - 0516 night build  (HSPL/PSRAM/AON/SYS only)
  - 0518 14:30 build  (+ ISP, H26E, HPDMA, GENER_DMA, PSRAM0/1)
  - 0518 17:56 build  (v3 patch: + ISP_MI, DPU, GPU, PPHS, PPRO, SYS_AHBP)

Regions missing from a particular dump are reported as ``not in dump``.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

from .core import read_bytes

# (name, start, end). Sized to match the patched memory.c entries.
DEFAULT_REGIONS: tuple[tuple[str, int, int], ...] = (
    ("AON_PMU",   0x44000000, 0x440001fc),
    ("AON_RTC",   0x44000200, 0x44000228),
    ("AON_GPIO",  0x44000408, 0x440004c8),
    ("SYS",       0x44010000, 0x44010170),
    ("FLASH",     0x44030000, 0x44030080),
    ("PPRO",      0x44050000, 0x44050090),
    ("MEM_CHECK", 0x44890000, 0x44890204),
    ("MBOX0",     0x45000000, 0x450000e0),
    ("HSPL0_CFG", 0x45010000, 0x45010040),
    ("HSPL0_STA", 0x45010080, 0x450100c0),
    ("GENER_DMA", 0x45020000, 0x45020110),
    ("SYS_AHBP",  0x48000000, 0x48000180),
    ("PSRAM0",    0x48060000, 0x48060060),
    ("PSRAM1",    0x48070000, 0x48070060),
    ("HSPL1_CFG", 0x480c0000, 0x480c0040),
    ("HSPL1_STA", 0x480c0080, 0x480c00c0),
    ("PPHS",      0x480d0000, 0x480d0040),
    ("ISP",       0x4c040000, 0x4c040200),
    # ISP_MI window covers TIMEOUT_CFG / STREAM_STATUS read inside vsi_isp_isr.
    ("ISP_MI",    0x4c0441c0, 0x4c044200),
    ("H26E",      0x4c100000, 0x4c100200),
    ("GPU",       0x4c280000, 0x4c280400),
    ("DPU",       0x4c2c0000, 0x4c2c0400),
    ("HPDMA",     0x4c300000, 0x4c300400),
)


@dataclass
class PeriRegion:
    name: str
    start: int
    end: int
    data: bytes | None = None       # decoded payload (None = not in dump)
    raw_dump_block: bytes | None = None  # full 'stack mem' block as captured

    @property
    def size(self) -> int:
        return self.end - self.start

    @property
    def captured(self) -> bool:
        return self.data is not None or self.raw_dump_block is not None


@dataclass
class PeriReport:
    regions: list[PeriRegion] = field(default_factory=list)


def _raw_region_lookup(
    regions: Iterable[tuple[str, int, bytes, int]],
) -> tuple[dict[str, bytes], dict[int, bytes]]:
    """Return ``(by_name, by_start_addr)`` raw-byte maps.

    Some builds capture peripherals under different names (e.g. 0516 dumps
    the PSRAM block as ``PSRAM``, 0518+ as ``PSRAM0`` / ``PSRAM1``). Looking
    up by start address sidesteps the rename issue.
    Sizes also differ across builds (HPDMA 0x140 B vs 0x400 B). We use the
    actual on-the-wire bytes so we can still show useful data even when the
    region table over-reads.
    """
    by_name: dict[str, bytes] = {}
    by_start: dict[int, bytes] = {}
    for name, start, data, _expected in regions:
        if not data:
            continue
        by_name.setdefault(name.upper(), data)
        by_start.setdefault(start, data)
    return by_name, by_start


def _partial_read(memmap, lo: int, hi: int) -> bytes | None:
    """Return as many bytes of ``[lo, hi)`` as the memory map can provide.

    The standard :func:`read_bytes` is all-or-nothing; for peripheral
    snapshots we prefer to display whatever was actually captured.
    """
    for start, data in memmap:
        if start <= lo < start + len(data):
            off = lo - start
            avail = min(len(data) - off, hi - lo)
            if avail <= 0:
                return None
            return data[off:off + avail]
    return None


def decode_peripherals(
    memmap,
    regions: Iterable[tuple[str, int, bytes, int]],
    table: tuple[tuple[str, int, int], ...] = DEFAULT_REGIONS,
) -> PeriReport:
    by_name, by_start = _raw_region_lookup(regions)
    out = PeriReport()
    for name, lo, hi in table:
        data = read_bytes(memmap, lo, hi - lo)
        if data is None:
            data = _partial_read(memmap, lo, hi)
        raw_block = by_start.get(lo) or by_name.get(name.upper())
        out.regions.append(PeriRegion(name=name, start=lo, end=hi, data=data, raw_dump_block=raw_block))
    return out


def _hex_rows(start: int, data: bytes) -> Iterable[str]:
    for off in range(0, len(data), 16):
        words = []
        for i in range(0, min(16, len(data) - off), 4):
            if off + i + 4 <= len(data):
                words.append(f"{struct.unpack_from('<I', data, off+i)[0]:08x}")
            else:
                rem = data[off + i:]
                words.append(rem.hex())
        addr = start + off
        yield f"  {addr:08x}  " + " ".join(words)


def render_peri_text(report: PeriReport) -> str:
    lines: list[str] = []
    for region in report.regions:
        lines.append("")
        lines.append(
            f"=== {region.name}  [{region.start:#010x}..{region.end:#010x})  size={region.size} ==="
        )
        # Prefer the actual block as captured (which may be larger than the
        # table window in newer builds, e.g. HPDMA 0x140 vs 0x400). Fall back
        # to the partial-read slice when the dump matches the table.
        block = region.raw_dump_block
        partial = region.data
        if block is not None and (partial is None or len(block) >= len(partial)):
            if partial is None or len(block) != region.size:
                lines.append(
                    f"  captured size = {len(block)} (region table size = {region.size})"
                )
            for row in _hex_rows(region.start, block):
                lines.append(row)
            continue
        if partial is not None:
            if len(partial) != region.size:
                lines.append(f"  partial capture: {len(partial)} of {region.size} bytes")
            for row in _hex_rows(region.start, partial):
                lines.append(row)
            continue
        lines.append("  not in dump")
    return "\n".join(lines) + "\n"
