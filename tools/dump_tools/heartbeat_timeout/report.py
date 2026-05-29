"""Auto-generated heartbeat-timeout review report.

Combines the structured outputs from :mod:`extract`, :mod:`msp_walk`, and
:mod:`peri_regs` into a single Markdown summary aimed at team review:

  * banner with build / runtime / reason
  * pre-dump CPU exception banner (when present in the firmware preamble)
  * smoking-gun panel: per-core last IRQ recorder entry + done=0 flag
  * MSP exception-frame summary highlighting EXC_RETURN -> ISR chain
  * "hang fingerprint" probes (PSRAM0 Reg 0x10, PPHS sresp, etc.)
  * raw evidence pointers back to the log/extract files
"""
from __future__ import annotations

import re
import struct
import subprocess
from dataclasses import dataclass, field
from pathlib import Path

from .core import irq_name
from .decoders import DecoderSummary
from .extract import ExtractResult, IRQRecorderSummary, IRQRecord
from .msp_walk import MspWalkResult, ExceptionFrame
from .peri_regs import PeriRegion, PeriReport


BUILD_TIME_RE = re.compile(r"build time =>\s*([^!\n]+)", re.IGNORECASE)
DUMP_REASON_RE = re.compile(r"@Dump-reason:\s*(.+)", re.IGNORECASE)
ASSERT_AT_RE = re.compile(r"Assert at:\s*([^\s].*)", re.IGNORECASE)
SESSION_FILENAME_RE = re.compile(r"(\d+)min", re.IGNORECASE)

CPU_BANNER_RE = re.compile(r"CPU(\d+) Current regs:", re.IGNORECASE)
# Match register lines like:  "0 r0 x 0x40fb948"  or "16 pc x 0x4006900"
CPU_REG_LINE_RE = re.compile(
    r"^\s*\d+\s+([A-Za-z_][A-Za-z0-9_]*)\s+x\s+(0x[0-9a-fA-F]+)"
)
TRACEBACK_RE = re.compile(r"arm-none-eabi-addr2line\s+-piaf\s+-e\s+\S+\s+(.+?)\s*$")


@dataclass
class CpuExceptionBanner:
    """The pre-dump 'CPUx Current regs: ...' banner emitted by some builds.

    Older defconfigs (e.g. the 1080PUVC night build) emit a CP-side dump
    preamble even when the AP recorders/TCBs are not yet captured; this
    structure preserves what we can extract for those cases.
    """

    cpu: int
    regs: dict[str, int] = field(default_factory=dict)
    traceback: list[int] = field(default_factory=list)


@dataclass
class HeaderInfo:
    build_time: str | None
    dump_reason: str | None
    assert_at: str | None
    session_label: str | None
    cpu_banners: list[CpuExceptionBanner] = field(default_factory=list)


def _scan_header(log_path: Path) -> HeaderInfo:
    """Scan the log for build / dump-reason / assert lines.

    These markers appear inside the dump preamble.  In short tests they sit
    near the top of the file (<200 lines in idle reproductions), but for a
    multi-hour stress test the preceding telemetry can push them tens of
    thousands of lines down.  We therefore keep scanning until either all
    three are found, the first ``>>>>stack mem dump begin`` marker is hit
    (after which only memory blocks follow), or 200k lines have been
    examined.
    """
    build = reason = assert_at = None
    DUMP_BEGIN = ">>>>stack mem dump begin"
    cpu_banners: list[CpuExceptionBanner] = []
    current_banner: CpuExceptionBanner | None = None
    in_traceback = False
    # Strip per-line timestamp prefixes (see ``core.TS_RE`` for the same set
    # of supported formats); copied here to avoid an import-cycle / so this
    # module's banner scanner works even if ``core`` is refactored.
    strip_ts = re.compile(
        r"^\["
        r"(?:"
        r"(?:Serial|UART)[^\]]+"
        r"|\d{8}-\d{2}:\d{2}:\d{2}(?:\.\d+)?"
        r"|\d{2}:\d{2}:\d{2}[-.]\d{1,4}"
        r")"
        r"\]\s*"
    )
    try:
        with log_path.open("r", encoding="utf-8", errors="ignore") as fh:
            for idx, raw_line in enumerate(fh):
                if idx >= 200_000:
                    break
                if DUMP_BEGIN in raw_line:
                    # Base64 dump body starts here; stop scanning preamble.
                    break
                line = strip_ts.sub("", raw_line)
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

                m = CPU_BANNER_RE.search(line)
                if m:
                    current_banner = CpuExceptionBanner(cpu=int(m.group(1)))
                    cpu_banners.append(current_banner)
                    in_traceback = False
                    continue
                if current_banner is not None:
                    if line.strip().lower().startswith("traceback"):
                        in_traceback = True
                        continue
                    if in_traceback:
                        m = TRACEBACK_RE.search(line)
                        if m:
                            for tok in m.group(1).split():
                                try:
                                    current_banner.traceback.append(int(tok, 16))
                                except ValueError:
                                    pass
                            current_banner = None
                            in_traceback = False
                            continue
                    rm = CPU_REG_LINE_RE.match(line)
                    if rm:
                        try:
                            current_banner.regs[rm.group(1).lower()] = int(rm.group(2), 16)
                        except ValueError:
                            pass
                    elif line.strip().startswith("***"):
                        # End-of-banner sentinel (e.g. "***user except handler begin***")
                        current_banner = None
                        in_traceback = False
    except OSError:
        pass
    session_label = None
    m = SESSION_FILENAME_RE.search(log_path.name)
    if m:
        session_label = f"{m.group(1)} min session"
    return HeaderInfo(build, reason, assert_at, session_label, cpu_banners)


def _decode_cfsr(cfsr: int) -> str:
    if cfsr == 0:
        return "0x0 (no fault recorded)"
    mmfsr = cfsr & 0xFF
    bfsr = (cfsr >> 8) & 0xFF
    ufsr = (cfsr >> 16) & 0xFFFF
    bits: list[str] = []
    mm_names = [
        (0x01, "IACCVIOL"), (0x02, "DACCVIOL"), (0x08, "MUNSTKERR"),
        (0x10, "MSTKERR"), (0x20, "MLSPERR"), (0x80, "MMARVALID"),
    ]
    for mask, n in mm_names:
        if mmfsr & mask:
            bits.append(f"MM:{n}")
    bf_names = [
        (0x01, "IBUSERR"), (0x02, "PRECISERR"), (0x04, "IMPRECISERR"),
        (0x08, "UNSTKERR"), (0x10, "STKERR"), (0x20, "LSPERR"),
        (0x80, "BFARVALID"),
    ]
    for mask, n in bf_names:
        if bfsr & mask:
            bits.append(f"BF:{n}")
    uf_names = [
        (0x0001, "UNDEFINSTR"), (0x0002, "INVSTATE"), (0x0004, "INVPC"),
        (0x0008, "NOCP"), (0x0010, "STKOF"), (0x0100, "UNALIGNED"),
        (0x0200, "DIVBYZERO"),
    ]
    for mask, n in uf_names:
        if ufsr & mask:
            bits.append(f"UF:{n}")
    return f"0x{cfsr:08x} (" + ", ".join(bits) + ")" if bits else f"0x{cfsr:08x}"


def _decode_hfsr(hfsr: int) -> str:
    if hfsr == 0:
        return "0x0 (no hard fault)"
    bits: list[str] = []
    if hfsr & (1 << 1):
        bits.append("VECTTBL")
    if hfsr & (1 << 30):
        bits.append("FORCED")
    if hfsr & (1 << 31):
        bits.append("DEBUGEVT")
    return f"0x{hfsr:08x} (" + ", ".join(bits) + ")" if bits else f"0x{hfsr:08x}"


def _decode_exc_return(value: int) -> str:
    if (value & 0xFFFFFFF0) != 0xFFFFFFF0:
        return f"0x{value:08x} (not an EXC_RETURN sentinel)"
    bits: list[str] = []
    bits.append("Thread" if (value & 0x8) else "Handler")
    bits.append("PSP" if (value & 0x4) else "MSP")
    bits.append("noFP" if (value & 0x10) else "FP-stacked")
    if (value & 0x40):
        bits.append("S")
    else:
        bits.append("NS")
    return f"0x{value:08x} (" + ", ".join(bits) + ")"


def _addr2line_resolve(elf: Path, addrs: list[int]) -> dict[int, str]:
    if not addrs or not elf.exists():
        return {}
    try:
        out = subprocess.run(
            ["arm-none-eabi-addr2line", "-piaf", "-e", str(elf), *[f"0x{a:x}" for a in addrs]],
            check=False, capture_output=True, text=True, timeout=10,
        )
    except (OSError, subprocess.SubprocessError):
        return {}
    lines = [l for l in out.stdout.splitlines() if l.strip()]
    result: dict[int, str] = {}
    for addr, ln in zip(addrs, lines):
        result[addr] = ln.strip()
    return result


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
    decoder_summary: DecoderSummary | None = None,
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
    # Pre-dump CPU exception banner (older builds emit this even when
    # the v3 peripheral coverage is missing)
    # ------------------------------------------------------------------
    if header.cpu_banners:
        lines.append("")
        lines.append("## 0a. Pre-dump CPU exception banner")
        for banner in header.cpu_banners:
            r = banner.regs
            lines.append("")
            lines.append(f"### CPU{banner.cpu}")
            interesting = [
                ("pc", "PC"), ("lr", "LR"), ("sp", "SP"), ("xpsr", "xPSR"),
                ("msp", "MSP"), ("psp", "PSP"), ("primask", "PRIMASK"),
                ("basepri", "BASEPRI"), ("control", "CONTROL"),
                ("er", "EXC_RETURN"), ("cfsr", "CFSR"), ("hfsr", "HFSR"),
                ("mmfar", "MMFAR"), ("bfar", "BFAR"),
            ]
            lines.append("")
            lines.append("| Reg | Value | Decoded |")
            lines.append("|---|---|---|")
            for key, label in interesting:
                if key not in r:
                    continue
                val = r[key]
                decoded = ""
                if key == "cfsr":
                    decoded = _decode_cfsr(val)
                elif key == "hfsr":
                    decoded = _decode_hfsr(val)
                elif key == "er":
                    decoded = _decode_exc_return(val)
                lines.append(f"| {label} | 0x{val:08x} | {decoded} |")
            if banner.traceback:
                lines.append("")
                lines.append("**Traceback (per pre-dump banner):**")
                resolved = _addr2line_resolve(elf_path, banner.traceback)
                lines.append("")
                lines.append("```")
                unresolved_count = 0
                for a in banner.traceback:
                    sym = resolved.get(a, "").strip()
                    # addr2line `-piaf` emits `0xADDR: file:line\n  (inlined ...)`;
                    # strip the leading address token if present so we don't
                    # print it twice.
                    if sym.lower().startswith("0x"):
                        sym = sym.split(":", 1)[1].strip() if ":" in sym else sym
                    if sym in ("", "?? ??:0"):
                        sym = "<not in supplied ELF>"
                        unresolved_count += 1
                    lines.append(f"  0x{a:08x}: {sym}")
                lines.append("```")
                if unresolved_count == len(banner.traceback):
                    lines.append(
                        "> _Note: traceback addresses do not resolve with the supplied AP "
                        "ELF — these are CP-side addresses (the assert is fired by CP's "
                        "`mb_ipc_task`). Provide the matching CP ELF to decode them._"
                    )

    # ------------------------------------------------------------------
    # Dump coverage — flag mismatches between dumped ranges and ELF
    # data sections so users can tell at a glance when the supplied
    # ELF doesn't match the build (or when the dump truncated before
    # AP user-data was emitted).
    # ------------------------------------------------------------------
    if extract.memmap:
        lines.append("")
        lines.append("## 0b. Dump coverage")
        lines.append("")
        # Compress contiguous blocks for readable display.
        blocks = sorted(extract.memmap, key=lambda b: b[0])
        merged: list[tuple[int, int]] = []
        for start, data in blocks:
            end = start + len(data)
            if merged and start == merged[-1][1]:
                merged[-1] = (merged[-1][0], end)
            else:
                merged.append((start, end))
        lines.append("**Dumped memory ranges:**")
        lines.append("")
        lines.append("```")
        for s, e in merged:
            lines.append(f"  0x{s:08x} .. 0x{e:08x}  ({e - s} B)")
        lines.append("```")
        # Compare to AP ELF symbol clusters: if every probed FreeRTOS
        # global address sits outside the merged ranges, this means the
        # dump (or the ELF) doesn't match the AP user-data area.
        probed_addrs = [g.addr for g in extract.globals]
        in_dump = sum(
            1 for a in probed_addrs
            if any(s <= a < e for s, e in merged)
        )
        if probed_addrs and in_dump == 0:
            sym_lo = min(probed_addrs)
            sym_hi = max(probed_addrs)
            lines.append("")
            lines.append(
                f"> ⚠️ The supplied AP ELF places its FreeRTOS globals at "
                f"0x{sym_lo:08x}..0x{sym_hi:08x}, which is **not** covered by "
                "any dumped memory range. Either the build does not match "
                "this log or the dump was truncated before AP user-data was "
                "emitted; recorder/TCB analysis is therefore unavailable."
            )

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
    # HPDMA / HSPL decoder summary (v3 patch builds only)
    # ------------------------------------------------------------------
    if decoder_summary is not None and (
        decoder_summary.hpdma_block_present
        or decoder_summary.hspl_locked
    ):
        lines.append("")
        lines.append("## 6b. HPDMA / HSPL decoded snapshot")
        if decoder_summary.hpdma_block_present:
            if decoder_summary.hpdma_active_channels:
                ch_list = ", ".join(f"ch{c}" for c in decoder_summary.hpdma_active_channels)
                lines.append(f"- HPDMA active channels: {ch_list}")
                for ch in decoder_summary.hpdma_active_channels:
                    lines.append(
                        f"  - ch{ch}: {decoder_summary.hpdma_channel_summary.get(ch, '?')}"
                    )
                    cb = decoder_summary.hpdma_finish_callback.get(ch)
                    if cb:
                        lines.append(f"      finish_cb -> {cb}")
            else:
                lines.append("- HPDMA captured but no channel programmed")
            pending = decoder_summary.hpdma_finish_int_channels
            errs = decoder_summary.hpdma_busy_int_channels
            if pending:
                lines.append(
                    "- HPDMA channels with **finish_int pending** at hang: "
                    + ", ".join(f"ch{c}" for c in pending)
                )
            if errs:
                lines.append(
                    "- HPDMA channels with **bus/fifo error** at hang: "
                    + ", ".join(f"ch{c}" for c in errs)
                )
        else:
            lines.append("- HPDMA region not in dump (older build)")
        if decoder_summary.hspl_locked:
            lines.append("")
            lines.append("**HSPL channels currently locked:**")
            for hspl_id, ch, res, owner in decoder_summary.hspl_locked:
                lines.append(
                    f"- HSPL{hspl_id} ch{ch} ({res}): owner_id={owner}"
                )
            if any(hspl_id == 1 for hspl_id, *_ in decoder_summary.hspl_locked):
                lines.append(
                    "  - _HSPL1 is AP SMP-internal only; CP cannot lock HSPL1 "
                    "channels (see `ap/middleware/driver/hspl/README.md`). The "
                    "owner_id is an AP-side master-ID, never CP._"
                )
        else:
            lines.append("- HSPL: no channel has owner_valid=1 in dump")

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
