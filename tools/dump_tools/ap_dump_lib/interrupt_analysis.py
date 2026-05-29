#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass, field

from .dat_parser import DumpSession, InterruptRecord, StackRegion
from .elf_symbols import ElfSymbols


EXIT_FLAG = 0xF0000000
RECORDER_STRUCT_SIZE = 24

# Firmware struct layout (interrupt_recorder_dump_t):
#   [+0]  uint32_t count            (4 B)
#   [+4]  padding for uint64        (4 B)
#   [+8]  recorder[N]               (N * 24 B)
#   [tail] optional extension fields (IRQ-silence monitor, may be absent in old fw)
RECORDER_ARRAY_OFFSET = 8
# Known tail-extension size (last_isr_exit_us..gap_threshold_us). Older builds
# don't have this; we detect which layout the dump uses by struct total size.
RECORDER_EXT_SIZE = 40


@dataclass
class CoreInterruptSummary:
    core: str
    total: int | None = None
    depth: int | None = None
    records: list[InterruptRecord] = field(default_factory=list)
    incomplete: list[InterruptRecord] = field(default_factory=list)
    source: str = "text"


def _region_for(regions: list[StackRegion], address: int, size: int) -> StackRegion | None:
    for region in regions:
        if region.contains(address, size):
            return region
    return None


def _read_u64(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset:offset + 8], "little")


def decode_recorder_from_memory(session: DumpSession, symbols: ElfSymbols) -> list[CoreInterruptSummary]:
    summaries: list[CoreInterruptSummary] = []
    for core_id, name in enumerate(("s_interrupt_core0_dump", "s_interrupt_core1_dump")):
        sym = symbols.get(name)
        if not sym or not sym.size:
            continue
        region = _region_for(session.stack_regions, sym.address, min(sym.size, 4))
        if not region:
            continue
        off = sym.address - region.start
        available = min(sym.size, len(region.data) - off)
        if available < RECORDER_ARRAY_OFFSET + RECORDER_STRUCT_SIZE:
            continue
        blob = region.data[off:off + available]
        total = int.from_bytes(blob[:4], "little")
        # Detect capacity from the ELF struct size, distinguishing old (no
        # IRQ-silence extension) from new (recorder + 40 B tail) layouts.
        # New: (sym.size - 8 - 40) / 24 is exact; Old: (sym.size - 8) / 24.
        if sym.size >= RECORDER_ARRAY_OFFSET + RECORDER_EXT_SIZE + RECORDER_STRUCT_SIZE \
                and (sym.size - RECORDER_ARRAY_OFFSET - RECORDER_EXT_SIZE) % RECORDER_STRUCT_SIZE == 0:
            capacity = (sym.size - RECORDER_ARRAY_OFFSET - RECORDER_EXT_SIZE) // RECORDER_STRUCT_SIZE
        else:
            capacity = (available - RECORDER_ARRAY_OFFSET) // RECORDER_STRUCT_SIZE
        depth = min(total, capacity)
        start = max(0, total - depth)
        records: list[InterruptRecord] = []
        for seq in range(start, total):
            rec_idx = seq % capacity
            rec_off = RECORDER_ARRAY_OFFSET + rec_idx * RECORDER_STRUCT_SIZE
            int_flag = int.from_bytes(blob[rec_off:rec_off + 4], "little")
            current_cnt = int.from_bytes(blob[rec_off + 4:rec_off + 8], "little")
            enter = _read_u64(blob, rec_off + 8)
            exit_time = _read_u64(blob, rec_off + 16)
            irq = int_flag & ~EXIT_FLAG
            done = bool(int_flag & EXIT_FLAG)
            records.append(
                InterruptRecord(
                    core=f"core{core_id}",
                    seq=current_cnt,
                    irq=irq,
                    done=done,
                    enter=enter,
                    exit=exit_time,
                    source=f"memory:{name}",
                )
            )
        summaries.append(
            CoreInterruptSummary(
                core=f"core{core_id}",
                total=total,
                depth=depth,
                records=records,
                incomplete=[r for r in records if not r.done],
                source=f"memory:{name}",
            )
        )
    return summaries


def summarize_interrupts(session: DumpSession, symbols: ElfSymbols) -> list[CoreInterruptSummary]:
    by_core: dict[str, CoreInterruptSummary] = {}
    for header in session.interrupt_headers:
        by_core[header.core] = CoreInterruptSummary(core=header.core, total=header.total, depth=header.depth, source="text")
    for record in session.interrupt_records:
        summary = by_core.setdefault(record.core, CoreInterruptSummary(core=record.core, source="text"))
        summary.records.append(record)
    for summary in by_core.values():
        summary.incomplete = [record for record in summary.records if not record.done]

    memory_summaries = decode_recorder_from_memory(session, symbols)
    for summary in memory_summaries:
        if summary.core not in by_core or not by_core[summary.core].records:
            by_core[summary.core] = summary
    return [by_core[key] for key in sorted(by_core)]

