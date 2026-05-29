"""FreeRTOS globals + task/IRQ recorder extraction.

This is the heartbeat-timeout-flavoured `ap_dump_extract.py`, but with
symbol addresses sourced from :mod:`symbols` rather than hard-coded for one
build.

Public functions return structured records so the renderer in
:mod:`report` can run smoking-gun heuristics without re-parsing text.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field
from pathlib import Path

from .core import addr2line, irq_name, looks_like_code, read_bytes, read_u32
from .symbols import SymbolTable

TASK_REC_ENTRY_SIZE = 24
TASK_REC_ENTRIES = 10

# Recorder array layout:
#   uint32 count
#   uint32 _pad
#   struct entry { uint32 int_flag; uint32 current_cnt; uint64 enter; uint64 exit; } entries[N];
#   /* extension footer */
#   uint64 last_isr_exit_us;
#   uint64 max_gap_us;
#   uint64 last_warn_print_us;
#   uint32 max_gap_irq;
#   uint32 max_gap_cnt;
#   uint32 gap_event_total;
#   uint32 gap_threshold_us;     /* probe sentinel: 50000 or 100000 */
INT_REC_ENTRY_SIZE = 24
INT_REC_PROBE_SIZES = (512, 256, 192, 160, 128, 96, 64, 40, 32, 16)
INT_REC_FOOTER_SIZE = 40
INT_REC_FOOTER_SENTINELS = (50000, 100000)
INT_REC_TAIL_VIEW = 64


@dataclass
class GlobalSnapshot:
    name: str
    addr: int
    value: int | None


@dataclass
class TaskRecorderEntry:
    idx: int
    tick: int
    aon_us: int
    tcb: int
    top: int
    bot: int
    size: int
    is_latest: bool = False


@dataclass
class TaskInfo:
    core: int
    tcb: int
    top_of_stack: int
    name: str
    name_kind: str | None
    name_offset: int | None
    stack_chain: list[tuple[int, str]] = field(default_factory=list)


@dataclass
class IRQRecord:
    idx: int
    int_flag: int
    current_cnt: int
    enter: int
    exit: int
    gap_from_prev_us: int | None = None

    @property
    def done(self) -> bool:
        return (self.int_flag & 0xF0000000) == 0xF0000000

    @property
    def irq(self) -> int:
        return self.int_flag & 0x0FFFFFFF

    @property
    def cost(self) -> int:
        if self.done and self.exit and self.enter:
            return self.exit - self.enter
        return 0


@dataclass
class IRQRecorderSummary:
    core: int
    base: int
    count: int
    n_entries: int | None
    tail: list[IRQRecord]
    last_record: IRQRecord | None
    stuck_record: IRQRecord | None
    last_isr_exit_us: int | None = None
    max_gap_us: int | None = None
    last_warn_us: int | None = None
    max_gap_irq: int | None = None
    max_gap_cnt: int | None = None
    gap_event_total: int | None = None
    gap_threshold_us: int | None = None


@dataclass
class ExtractResult:
    memmap: list[tuple[int, bytes]]
    globals: list[GlobalSnapshot]
    current_tasks: list[TaskInfo]
    referenced_tasks: list[TaskInfo]
    task_recorder_core0: list[TaskRecorderEntry]
    task_recorder_core1: list[TaskRecorderEntry]
    irq_summary_core0: IRQRecorderSummary | None
    irq_summary_core1: IRQRecorderSummary | None
    name_kind: str | None
    name_offset: int | None


# ----------------------------------------------------------------------------
# Globals + tasks
# ----------------------------------------------------------------------------
def _read_globals(memmap, sym: SymbolTable) -> list[GlobalSnapshot]:
    names = (
        "s_bk_exception_magic",
        "s_core_id",
        "s_bk_assert_info",
        "s_hb_paused",
        "uxCurrentNumberOfTasks",
        "xSchedulerRunning",
        "xTickCount",
        "s_task_cnt_core0",
        "s_task_cnt_core1",
    )
    out: list[GlobalSnapshot] = []
    for name in names:
        addr = sym.get(name)
        if addr is None:
            continue
        out.append(GlobalSnapshot(name=name, addr=addr, value=read_u32(memmap, addr)))
    # pxCurrentTCBs[0..1]
    base = sym.get("pxCurrentTCBs")
    if base is not None:
        for i in range(2):
            out.append(
                GlobalSnapshot(
                    name=f"pxCurrentTCBs[{i}]",
                    addr=base + i * 4,
                    value=read_u32(memmap, base + i * 4),
                )
            )
    return out


def _ascii_or_empty(buf: bytes) -> str:
    end = buf.find(b"\x00")
    if end == -1:
        end = len(buf)
    s = buf[:end]
    if not s or not all(0x20 <= ch < 0x7f for ch in s):
        return ""
    return s.decode("ascii", errors="replace")


def _read_inline_name(memmap, tcb: int, off: int) -> str:
    nb = read_bytes(memmap, tcb + off, 16)
    return _ascii_or_empty(nb) if nb else ""


def _read_ptr_name(memmap, tcb: int, off: int) -> str:
    ptr = read_u32(memmap, tcb + off)
    if not ptr:
        return ""
    if not (0x28000000 <= ptr < 0x282c0000 or 0x64c00000 <= ptr < 0x65000000):
        return ""
    nb = read_bytes(memmap, ptr, 32)
    return _ascii_or_empty(nb) if nb else ""


_NAME_OFF_CANDIDATES = (52, 56, 60, 64, 48, 44, 68, 72, 76, 80, 84, 88, 92)


def find_name_offset(memmap, tcb: int) -> tuple[str | None, int | None, str]:
    for off in _NAME_OFF_CANDIDATES:
        n = _read_inline_name(memmap, tcb, off)
        if len(n) >= 3:
            return ("inline", off, n)
    for off in _NAME_OFF_CANDIDATES:
        n = _read_ptr_name(memmap, tcb, off)
        if len(n) >= 2:
            return ("ptr", off, n)
    return (None, None, "")


def _walk_stack(elf: Path, memmap, top: int, limit_bytes: int = 0x1000, max_hits: int = 32):
    chain: list[int] = []
    for off in range(0, limit_bytes, 4):
        v = read_u32(memmap, top + off)
        if v is None:
            break
        if looks_like_code(v):
            chain.append(v & ~1)
            if len(chain) >= max_hits:
                break
    if not chain:
        return []
    resolved = addr2line(elf, chain)
    return list(zip(chain, resolved))


def _read_current_tasks(memmap, sym: SymbolTable, elf: Path) -> tuple[list[TaskInfo], str | None, int | None]:
    out: list[TaskInfo] = []
    base = sym.get("pxCurrentTCBs")
    if base is None:
        return out, None, None

    name_kind_used: str | None = None
    name_off_used: int | None = None
    for core in (0, 1):
        tcb = read_u32(memmap, base + core * 4)
        if not tcb:
            continue
        if read_bytes(memmap, tcb, 0x80) is None:
            continue
        top_of_stack = read_u32(memmap, tcb) or 0
        kind, off, name = find_name_offset(memmap, tcb)
        if off is not None and name_off_used is None:
            name_kind_used, name_off_used = kind, off
        info = TaskInfo(
            core=core,
            tcb=tcb,
            top_of_stack=top_of_stack,
            name=name,
            name_kind=kind,
            name_offset=off,
            stack_chain=_walk_stack(elf, memmap, top_of_stack),
        )
        out.append(info)
    return out, name_kind_used, name_off_used


def _lookup_task_name(memmap, tcb: int, name_kind: str | None, name_offset: int | None) -> str:
    if not name_kind or name_offset is None:
        return ""
    if name_kind == "ptr":
        return _read_ptr_name(memmap, tcb, name_offset)
    return _read_inline_name(memmap, tcb, name_offset)


# ----------------------------------------------------------------------------
# Task switch recorder
# ----------------------------------------------------------------------------
def _read_task_recorder(memmap, base: int, cnt_addr: int | None) -> list[TaskRecorderEntry]:
    cnt = read_u32(memmap, cnt_addr) if cnt_addr is not None else None
    out: list[TaskRecorderEntry] = []
    for i in range(TASK_REC_ENTRIES):
        data = read_bytes(memmap, base + i * TASK_REC_ENTRY_SIZE, TASK_REC_ENTRY_SIZE)
        if data is None:
            continue
        tick, aon, tcb, top, bot, sz = struct.unpack_from("<IIIIII", data)
        is_latest = (cnt is not None) and (i == (cnt - 1) % TASK_REC_ENTRIES)
        out.append(TaskRecorderEntry(i, tick, aon, tcb, top, bot, sz, is_latest))
    return out


def _collect_referenced_tcbs(t0: list[TaskRecorderEntry], t1: list[TaskRecorderEntry]) -> list[int]:
    seen: set[int] = set()
    ordered: list[int] = []
    for entry in (*t0, *t1):
        if not entry.tcb or entry.tcb in seen:
            continue
        if not (0x28000000 <= entry.tcb < 0x282c0000 or 0x64c00000 <= entry.tcb < 0x65000000):
            continue
        seen.add(entry.tcb)
        ordered.append(entry.tcb)
    return ordered


def _build_referenced_tasks(
    memmap, elf: Path,
    tcbs: list[int],
    referenced_stacks: dict[int, tuple[int, int]],
    name_kind: str | None,
    name_offset: int | None,
) -> list[TaskInfo]:
    out: list[TaskInfo] = []
    for tcb in tcbs:
        top, bot = referenced_stacks.get(tcb, (0, 0))
        name = _lookup_task_name(memmap, tcb, name_kind, name_offset)
        if top:
            chain = _walk_stack(elf, memmap, top, limit_bytes=min(0x800, max(0, bot - top)), max_hits=16)
        else:
            chain = []
        out.append(
            TaskInfo(
                core=-1,
                tcb=tcb,
                top_of_stack=top,
                name=name,
                name_kind=name_kind,
                name_offset=name_offset,
                stack_chain=chain,
            )
        )
    return out


# ----------------------------------------------------------------------------
# Interrupt recorder
# ----------------------------------------------------------------------------
def _summarize_irq_recorder(memmap, base: int, core: int, symbol_size: int | None = None) -> IRQRecorderSummary | None:
    count = read_u32(memmap, base)
    if count is None:
        return None
    rec_base = base + 8

    # Detect array length. Prefer the exact size we read from the ELF symbol
    # table (nm -S), since that's authoritative; fall back to probing the
    # gap_threshold_us sentinel for builds that linked without footer/extension.
    n_entries: int | None = None
    footer_addr: int | None = None
    last_exit = max_gap = last_warn = None
    gap_irq = gap_cnt = gap_total = gap_thr = None
    candidates_with_footer: list[int] = []
    if symbol_size is not None:
        # Layout = 8 (count + pad) + N * 24 (entries) + 40 (footer)
        for candidate in INT_REC_PROBE_SIZES:
            if 8 + candidate * INT_REC_ENTRY_SIZE + INT_REC_FOOTER_SIZE == symbol_size:
                candidates_with_footer = [candidate]
                break
        if not candidates_with_footer:
            # No footer: arr_size = 8 + N * 24
            for candidate in INT_REC_PROBE_SIZES:
                if 8 + candidate * INT_REC_ENTRY_SIZE == symbol_size:
                    n_entries = candidate
                    break

    if n_entries is None:
        probe_list = candidates_with_footer or list(INT_REC_PROBE_SIZES)
        for candidate in probe_list:
            tail_addr = rec_base + candidate * INT_REC_ENTRY_SIZE
            footer = read_bytes(memmap, tail_addr, INT_REC_FOOTER_SIZE)
            if footer is None:
                continue
            le, mg, lw, gi, gc, gt, gth = struct.unpack_from("<QQQIIII", footer)
            if gth in INT_REC_FOOTER_SENTINELS:
                n_entries = candidate
                footer_addr = tail_addr
                last_exit, max_gap, last_warn = le, mg, lw
                gap_irq, gap_cnt, gap_total, gap_thr = gi, gc, gt, gth
                break
    elif candidates_with_footer:
        # Authoritative n_entries from symbol size, but still read footer.
        n_entries = candidates_with_footer[0]
        tail_addr = rec_base + n_entries * INT_REC_ENTRY_SIZE
        footer = read_bytes(memmap, tail_addr, INT_REC_FOOTER_SIZE)
        if footer is not None:
            le, mg, lw, gi, gc, gt, gth = struct.unpack_from("<QQQIIII", footer)
            last_exit, max_gap, last_warn = le, mg, lw
            gap_irq, gap_cnt, gap_total, gap_thr = gi, gc, gt, gth
            footer_addr = tail_addr

    probe_count = n_entries or INT_REC_PROBE_SIZES[0]
    records: list[IRQRecord] = []
    for i in range(probe_count):
        data = read_bytes(memmap, rec_base + i * INT_REC_ENTRY_SIZE, INT_REC_ENTRY_SIZE)
        if data is None:
            break
        int_flag, current_cnt, enter, exit_t = struct.unpack_from("<IIQQ", data)
        records.append(IRQRecord(i, int_flag, current_cnt, enter, exit_t))

    if not records:
        return None

    chrono = sorted(records, key=lambda r: r.current_cnt)
    tail = chrono[-INT_REC_TAIL_VIEW:]
    last_exit_time = None
    for rec in tail:
        if last_exit_time is not None and last_exit_time > 0 and rec.enter > last_exit_time:
            rec.gap_from_prev_us = rec.enter - last_exit_time
        if rec.done and rec.exit:
            last_exit_time = rec.exit

    last_record = chrono[-1] if chrono else None
    # Smoking gun: most recent record where done==0 AND counter is the largest.
    stuck = None
    if last_record and not last_record.done:
        stuck = last_record

    return IRQRecorderSummary(
        core=core,
        base=base,
        count=count,
        n_entries=n_entries,
        tail=tail,
        last_record=last_record,
        stuck_record=stuck,
        last_isr_exit_us=last_exit,
        max_gap_us=max_gap,
        last_warn_us=last_warn,
        max_gap_irq=gap_irq,
        max_gap_cnt=gap_cnt,
        gap_event_total=gap_total,
        gap_threshold_us=gap_thr,
    )


# ----------------------------------------------------------------------------
# Public entry
# ----------------------------------------------------------------------------
def build_extract(memmap: list[tuple[int, bytes]], sym: SymbolTable, elf: Path) -> ExtractResult:
    globals_ = _read_globals(memmap, sym)
    current_tasks, name_kind, name_offset = _read_current_tasks(memmap, sym, elf)

    rec0_base = sym.get("s_task_recorder_core0")
    rec1_base = sym.get("s_task_recorder_core1")
    cnt0_addr = sym.get("s_task_cnt_core0")
    cnt1_addr = sym.get("s_task_cnt_core1")
    t0 = _read_task_recorder(memmap, rec0_base, cnt0_addr) if rec0_base else []
    t1 = _read_task_recorder(memmap, rec1_base, cnt1_addr) if rec1_base else []

    # Build a tcb -> (top, bot) map from the recorder.
    tcb_stacks: dict[int, tuple[int, int]] = {}
    for entry in (*t0, *t1):
        if entry.tcb and entry.top:
            tcb_stacks.setdefault(entry.tcb, (entry.top, entry.bot))

    referenced_tcbs = _collect_referenced_tcbs(t0, t1)
    referenced = _build_referenced_tasks(
        memmap, elf, referenced_tcbs, tcb_stacks, name_kind, name_offset,
    )

    irq0_base = sym.get("s_interrupt_core0_dump")
    irq1_base = sym.get("s_interrupt_core1_dump")
    irq0_size = sym.size_of("s_interrupt_core0_dump")
    irq1_size = sym.size_of("s_interrupt_core1_dump")
    irq0 = _summarize_irq_recorder(memmap, irq0_base, 0, irq0_size) if irq0_base else None
    irq1 = _summarize_irq_recorder(memmap, irq1_base, 1, irq1_size) if irq1_base else None

    return ExtractResult(
        memmap=memmap,
        globals=globals_,
        current_tasks=current_tasks,
        referenced_tasks=referenced,
        task_recorder_core0=t0,
        task_recorder_core1=t1,
        irq_summary_core0=irq0,
        irq_summary_core1=irq1,
        name_kind=name_kind,
        name_offset=name_offset,
    )


# ----------------------------------------------------------------------------
# Text formatter (matches the original ap_dump_extract.py layout closely)
# ----------------------------------------------------------------------------
def render_extract_text(dump_path: Path, regions: list, memmap, result: ExtractResult) -> str:
    lines: list[str] = []
    lines.append(f"# Parsing {dump_path}")
    total = sum(len(d) for _, d in memmap)
    lines.append(f"# Parsed {len(regions)} dump regions, merged to {len(memmap)} blocks")
    lines.append(f"# Total decoded bytes: {total} (~{total/1024:.1f} KiB)")
    for start, data in memmap:
        lines.append(f"#   block 0x{start:08x} - 0x{start+len(data):08x}  size={len(data)}")

    lines.append("")
    lines.append("## AP critical globals")
    for g in result.globals:
        if g.value is None:
            lines.append(f"  {g.name:32s} @ 0x{g.addr:08x}: <not in dump>")
        else:
            lines.append(f"  {g.name:32s} @ 0x{g.addr:08x}: 0x{g.value:08x} ({g.value})")

    lines.append("")
    lines.append("## Current Running Tasks (per core)")
    for t in result.current_tasks:
        lines.append(
            f"core{t.core}: TCB=0x{t.tcb:08x}  pxTopOfStack=0x{t.top_of_stack:08x}  "
            f"task='{t.name}'  (kind={t.name_kind} off={t.name_offset})"
        )
        if t.stack_chain:
            lines.append(f"  Stack scan ({len(t.stack_chain)} candidates):")
            for _, s in t.stack_chain:
                lines.append(f"    {s}")
        else:
            lines.append("  no code-like words in stack window")

    lines.append("")
    lines.append(f"# Using name kind={result.name_kind} offset={result.name_offset} for subsequent task lookups")

    lines.append("")
    lines.append("## Tasks referenced in switch recorders (recent execution)")
    for t in result.referenced_tasks:
        lines.append(
            f"\n  TCB=0x{t.tcb:08x} name='{t.name}' top=0x{t.top_of_stack:08x}"
        )
        if t.stack_chain:
            lines.append(f"  Stack scan ({len(t.stack_chain)} candidates):")
            for _, s in t.stack_chain:
                lines.append(f"    {s}")

    for core, recorder, base in (
        (0, result.task_recorder_core0, "s_task_recorder_core0"),
        (1, result.task_recorder_core1, "s_task_recorder_core1"),
    ):
        lines.append("")
        lines.append(f"## Task switch recorder core{core} ({base})")
        for entry in recorder:
            label = "*" if entry.is_latest else " "
            lines.append(
                f"  {label}[{entry.idx:2d}] tick={entry.tick:10d} aon={entry.aon_us:10d} "
                f"tcb=0x{entry.tcb:08x} top=0x{entry.top:08x} bot=0x{entry.bot:08x} sz={entry.size}"
            )

    lines.append("")
    lines.append("## IRQ -> AP source mapping in use (icu_int_src_t):")
    lines.append("   1=HPDMA  2=MAILBOX  4=GDMA0  18=HSPL  19=ISP_MI  20=ISP_FE  21=ISP_ISP")
    lines.append("   22=CSI  23=H26E  24=GPU  25=H264D  26=DPU  27=DSI  28=H264D_PP")

    for summary in (result.irq_summary_core0, result.irq_summary_core1):
        if summary is None:
            continue
        lines.append("")
        lines.append(
            f"## Interrupt recorder core{summary.core} @ 0x{summary.base:08x} "
            f"count={summary.count} n_entries={summary.n_entries}"
        )
        lines.append(f"  -- chronological (last {len(summary.tail)}) --")
        for rec in summary.tail:
            gap = f" gap_from_prev_exit={rec.gap_from_prev_us}us" if rec.gap_from_prev_us else ""
            lines.append(
                f"  ring[{rec.idx:3d}] cnt={rec.current_cnt:10d} "
                f"irq={rec.irq:3d}({irq_name(rec.irq)}) done={int(rec.done)} "
                f"enter={rec.enter} exit={rec.exit} cost={rec.cost}{gap}"
            )
        if summary.gap_threshold_us is not None:
            lines.append("")
            lines.append("  ## footer")
            lines.append(f"    last_isr_exit_us = {summary.last_isr_exit_us}")
            lines.append(f"    max_gap_us       = {summary.max_gap_us}")
            lines.append(f"    last_warn_us     = {summary.last_warn_us}")
            lines.append(f"    max_gap_irq      = {summary.max_gap_irq}")
            lines.append(f"    max_gap_cnt      = {summary.max_gap_cnt}")
            lines.append(f"    gap_event_total  = {summary.gap_event_total}")
            lines.append(f"    gap_threshold_us = {summary.gap_threshold_us}")
        if summary.stuck_record is not None:
            r = summary.stuck_record
            lines.append("")
            lines.append(
                f"  >>> SMOKING GUN <<< core{summary.core} ring[{r.idx}] "
                f"irq={r.irq}({irq_name(r.irq)}) cnt={r.current_cnt} "
                f"done=0 enter={r.enter} exit=0 — ISR entered, never exited"
            )

    return "\n".join(lines) + "\n"
