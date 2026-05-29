"""Auto-generated heartbeat-timeout review report.

Combines the structured outputs from :mod:`extract`, :mod:`msp_walk`, and
:mod:`peri_regs` into a single Markdown summary aimed at team review:

  * banner with build / runtime / reason
  * smoking-gun panel: per-core last IRQ recorder entry + done=0 flag
  * MSP exception-frame summary highlighting EXC_RETURN -> ISR chain
  * "hang fingerprint" probes (PSRAM0 Reg 0x10, PPHS sresp, etc.)
  * raw evidence pointers back to the log/extract files
"""
from __future__ import annotations

import re
import struct
from dataclasses import dataclass
from pathlib import Path

from .core import irq_name
from .extract import ExtractResult, IRQRecorderSummary, IRQRecord
from .msp_walk import MspWalkResult, ExceptionFrame
from .peri_regs import PeriRegion, PeriReport


BUILD_TIME_RE = re.compile(r"build time =>\s*([^!\n]+)", re.IGNORECASE)
DUMP_REASON_RE = re.compile(r"@Dump-reason:\s*(.+)", re.IGNORECASE)
ASSERT_AT_RE = re.compile(r"Assert at:\s*([^\s].*)", re.IGNORECASE)
SESSION_FILENAME_RE = re.compile(r"(\d+)min", re.IGNORECASE)


@dataclass
class HeaderInfo:
    build_time: str | None
    dump_reason: str | None
    assert_at: str | None
    session_label: str | None


def _scan_header(log_path: Path) -> HeaderInfo:
    build = reason = assert_at = None
    try:
        with log_path.open("r", encoding="utf-8", errors="ignore") as fh:
            for _, line in zip(range(15000), fh):
                if build and reason and assert_at:
                    break
                if not build:
                    m = BUILD_TIME_RE.search(line)
                    if m:
                        build = m.group(1).strip()
                if not reason:
                    m = DUMP_REASON_RE.search(line)
                    if m:
                        reason = m.group(1).strip()
                if not assert_at:
                    m = ASSERT_AT_RE.search(line)
                    if m:
                        assert_at = m.group(1).strip()
    except OSError:
        pass
    session_label = None
    m = SESSION_FILENAME_RE.search(log_path.name)
    if m:
        session_label = f"{m.group(1)} min session"
    return HeaderInfo(build, reason, assert_at, session_label)


def _summarize_last_irq(summary: IRQRecorderSummary | None) -> str:
    if summary is None:
        return "no recorder data"
    rec = summary.last_record
    if rec is None:
        return f"count={summary.count}, no entries decoded"
    suffix = ""
    if not rec.done:
        suffix = "  **<-- done=0, ISR did not exit**"
    return (
        f"count={summary.count}, last ring[{rec.idx}] cnt={rec.current_cnt} "
        f"irq={rec.irq}({irq_name(rec.irq)}) done={int(rec.done)} "
        f"enter={rec.enter} exit={rec.exit}{suffix}"
    )


def _format_record_row(rec: IRQRecord) -> str:
    flag = "✱" if not rec.done else " "
    gap = f", gap={rec.gap_from_prev_us}us" if rec.gap_from_prev_us else ""
    return (
        f"  {flag} ring[{rec.idx:3d}] cnt={rec.current_cnt:>10} "
        f"irq={rec.irq:3d}({irq_name(rec.irq)}) done={int(rec.done)} "
        f"enter={rec.enter} exit={rec.exit} cost={rec.cost}{gap}"
    )


def _frame_chain_line(frame: ExceptionFrame) -> str:
    bits: list[str] = [f"EXC_RETURN @ {frame.exc_return_sp:#x} = {frame.exc_return_value:#x}"]
    for p in frame.probes:
        if not p.resolved:
            continue
        head = p.resolved.split("\n", 1)[0].strip()
        bits.append(f"SP{p.delta:+d} → {head}")
    return " · ".join(bits)


def _psram_reg_check(peri: PeriReport) -> tuple[str, str] | None:
    """Check PSRAM0 Reg 0x10 hang fingerprint.

    Baseline (idle): 0x00000000.  All hang dumps observed so far: 0x03xxxxxx.
    Inspects the region with start 0x48060000 (called ``PSRAM`` in 0516
    builds and ``PSRAM0`` in 0518+ builds).
    """
    for region in peri.regions:
        if region.start != 0x48060000:
            continue
        buf = region.raw_dump_block or region.data
        if buf is None or len(buf) < 0x44:
            return None
        reg10 = struct.unpack_from("<I", buf, 0x40)[0]
        if (reg10 >> 24) == 0x03:
            return ("HIT", f"PSRAM0 Reg 0x10 = 0x{reg10:08x} — matches hang fingerprint 0x03xxxxxx")
        if reg10 == 0:
            return ("CLEAR", "PSRAM0 Reg 0x10 = 0x00000000 — baseline / idle value")
        return ("UNKNOWN", f"PSRAM0 Reg 0x10 = 0x{reg10:08x} — value outside known patterns")
    return None


def _msp_smoking_gun(walk: MspWalkResult) -> ExceptionFrame | None:
    """Pick the EXC_RETURN frame that looks like the innermost-active IRQ.

    We prefer frames whose probes resolve to ``*_isr`` / ``bk_int_dispatch``
    / ``hpdma_isr`` etc, since those are the dispatch entrypoints.
    """
    if not walk.captured:
        return None
    best = None
    best_score = -1
    for frame in walk.frames:
        score = 0
        for probe in frame.probes:
            text = probe.resolved.lower()
            if "_isr" in text or "interrupt" in text or "dispatch" in text:
                score += 3
            if "hpdma" in text or "isp" in text or "dpu" in text or "gpu" in text:
                score += 4
            if "pendsv" in text or "systick" in text:
                score += 1
        if score > best_score:
            best = frame
            best_score = score
    return best if best_score > 0 else None


def render_report(
    log_path: Path,
    elf_path: Path,
    header: HeaderInfo,
    extract: ExtractResult,
    msp0: MspWalkResult,
    msp1: MspWalkResult,
    peri: PeriReport,
) -> str:
    lines: list[str] = []
    lines.append(f"# Heartbeat-timeout RCA — {log_path.name}")
    lines.append("")
    lines.append("## 0. Inputs & banner")
    lines.append("")
    lines.append("| Field | Value |")
    lines.append("|---|---|")
    lines.append(f"| Log | `{log_path}` |")
    lines.append(f"| AP ELF | `{elf_path}` |")
    lines.append(f"| Build time | {header.build_time or 'n/a'} |")
    lines.append(f"| Dump reason | {header.dump_reason or 'n/a'} |")
    if header.assert_at:
        lines.append(f"| Assert at | `{header.assert_at}` |")
    if header.session_label:
        lines.append(f"| Session | {header.session_label} |")

    # ------------------------------------------------------------------
    # Smoking gun — IRQ recorder
    # ------------------------------------------------------------------
    lines.append("")
    lines.append("## 1. Smoking-gun — interrupt recorder")
    lines.append("")
    lines.append(f"- **core0**: {_summarize_last_irq(extract.irq_summary_core0)}")
    lines.append(f"- **core1**: {_summarize_last_irq(extract.irq_summary_core1)}")
    lines.append("")
    for summary in (extract.irq_summary_core0, extract.irq_summary_core1):
        if summary is None or summary.stuck_record is None:
            continue
        r = summary.stuck_record
        lines.append(
            f"> 🔥 core{summary.core} IRQ {r.irq} ({irq_name(r.irq)}) "
            f"entered @ AON-RTC {r.enter} µs and never exited "
            f"(ring[{r.idx}], cnt={r.current_cnt}). "
            f"Last successful ISR exit @ {summary.last_isr_exit_us}, "
            f"max_gap_us={summary.max_gap_us}, gap_event_total={summary.gap_event_total}."
        )

    # ------------------------------------------------------------------
    # Last 16 IRQs per core
    # ------------------------------------------------------------------
    lines.append("")
    lines.append("## 2. Last interrupts (chronological tail)")
    for summary in (extract.irq_summary_core0, extract.irq_summary_core1):
        if summary is None:
            continue
        lines.append("")
        lines.append(f"### core{summary.core} (last {min(16, len(summary.tail))} of {summary.count})")
        lines.append("")
        lines.append("```")
        for rec in summary.tail[-16:]:
            lines.append(_format_record_row(rec))
        lines.append("```")

    # ------------------------------------------------------------------
    # Current tasks
    # ------------------------------------------------------------------
    lines.append("")
    lines.append("## 3. Current tasks per core")
    if not extract.current_tasks:
        lines.append("- pxCurrentTCBs not in dump")
    for t in extract.current_tasks:
        lines.append("")
        lines.append(
            f"### core{t.core}: TCB=0x{t.tcb:08x}  top=0x{t.top_of_stack:08x}  name='{t.name}'"
        )
        if t.stack_chain:
            lines.append("")
            lines.append("```")
            for _, s in t.stack_chain[:12]:
                lines.append(f"  {s}")
            lines.append("```")

    # ------------------------------------------------------------------
    # Task switch recorder (just last entry per core)
    # ------------------------------------------------------------------
    lines.append("")
    lines.append("## 4. Most recent task switch per core")
    for core, recorder in ((0, extract.task_recorder_core0), (1, extract.task_recorder_core1)):
        latest = next((e for e in recorder if e.is_latest), None)
        if latest is None and recorder:
            latest = recorder[-1]
        if latest is None:
            lines.append(f"- core{core}: no recorder data")
            continue
        lines.append(
            f"- core{core}: tick={latest.tick} aon={latest.aon_us} "
            f"TCB=0x{latest.tcb:08x} stack=[0x{latest.top:08x}..0x{latest.bot:08x}] sz={latest.size}"
        )

    # ------------------------------------------------------------------
    # MSP analysis
    # ------------------------------------------------------------------
    lines.append("")
    lines.append("## 5. MSP exception-frame call chains")
    for walk in (msp0, msp1):
        lines.append("")
        lines.append(f"### core{walk.core} MSP {walk.msp_lo:#x}..{walk.msp_hi:#x}")
        if not walk.captured:
            lines.append("- not in dump")
            continue
        gun = _msp_smoking_gun(walk)
        if gun is not None:
            lines.append("")
            lines.append(f"- innermost active frame: `{_frame_chain_line(gun)}`")
        if walk.frames:
            lines.append("")
            lines.append("```")
            for frame in walk.frames:
                lines.append(
                    f"  EXC_RETURN @ {frame.exc_return_sp:#010x}  ({frame.exc_return_value:#010x})"
                )
                for p in frame.probes:
                    if not p.resolved:
                        continue
                    short = p.resolved.split("\n", 1)[0]
                    lines.append(
                        f"    SP{p.delta:+4d} [{p.sp:#010x}] = {p.value:#010x}  {short}"
                    )
            lines.append("```")

    # ------------------------------------------------------------------
    # Peripheral fingerprint checks
    # ------------------------------------------------------------------
    lines.append("")
    lines.append("## 6. Peripheral fingerprint")
    psram = _psram_reg_check(peri)
    if psram is not None:
        verdict, msg = psram
        emoji = {"HIT": "🔥", "CLEAR": "✅", "UNKNOWN": "⚠️"}.get(verdict, "")
        lines.append(f"- {emoji} {msg}")
    captured = [r for r in peri.regions if r.captured]
    missing = [r.name for r in peri.regions if not r.captured]
    lines.append(f"- regions captured: {len(captured)} / {len(peri.regions)}")
    if missing:
        lines.append(f"- regions absent from dump: {', '.join(missing)}")
    # Regions added by the v2/v3 coredump patches (PPHS/PPRO/SYS_AHBP/DPU/
    # GPU/ISP_MI). HPDMA / ISP / H26E predate v2 so they aren't listed here.
    v3_only = ("PPHS", "PPRO", "SYS_AHBP", "DPU", "GPU", "ISP_MI")
    have_v3 = [n for n in v3_only if any(r.name == n and r.captured for r in peri.regions)]
    miss_v3 = [n for n in v3_only if n not in have_v3]
    if have_v3 and not miss_v3:
        lines.append(f"- v2/v3-patch regions present: {', '.join(have_v3)}")
    elif have_v3 and miss_v3:
        lines.append(f"- v2/v3-patch regions partially present (have: {', '.join(have_v3)}; missing: {', '.join(miss_v3)})")
    else:
        lines.append(
            "- v2/v3-patch regions (PPHS/PPRO/SYS_AHBP/DPU/GPU/ISP_MI) "
            "**not present** — build predates the patches"
        )

    # ------------------------------------------------------------------
    # Next steps
    # ------------------------------------------------------------------
    lines.append("")
    lines.append("## 7. Suggested next steps")
    if extract.irq_summary_core0 and extract.irq_summary_core0.stuck_record:
        v = extract.irq_summary_core0.stuck_record.irq
        lines.append(
            f"- Investigate **IRQ {v} ({irq_name(v)})** ISR on core0 — fits the "
            f"AHB-hang pattern observed across previous heartbeat-timeout cases."
        )
    if miss_v3:
        lines.append(
            "- Re-run night testing with the v3 coredump patch so "
            f"{', '.join(miss_v3)} are captured."
        )
    lines.append("- Compare PSRAM0 Reg 0x10 against active-load baseline once available to confirm fingerprint.")
    lines.append("- See companion files `extract.txt`, `msp_core0.txt`, `msp_core1.txt`, `peri.txt` for raw data.")
    return "\n".join(lines) + "\n"
