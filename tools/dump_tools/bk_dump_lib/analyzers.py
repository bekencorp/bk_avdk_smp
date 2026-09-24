#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from ap_dump_lib.context_analysis import analyze_context
from ap_dump_lib.dat_parser import DumpSession, parse_dat
from ap_dump_lib.elf_symbols import ElfSymbols, ResolvedAddress
from ap_dump_lib.heap_analysis import analyze_heap, render_heap_console
from ap_dump_lib.interrupt_analysis import summarize_interrupts
from ap_dump_lib.report import render_report as render_exception_markdown
from ap_dump_lib.stack_analysis import analyze as analyze_exception_session
from ap_dump_lib.stack_analysis import primary_chains
from heartbeat_timeout import core as hb_core
from heartbeat_timeout import decoders, extract, msp_walk, peri_regs
from heartbeat_timeout import report as hb_report
from heartbeat_timeout.symbols import load_symbols


@dataclass
class AnalysisOutput:
    mode: str
    console: str
    markdown: str
    exit_code: int = 0


def _hex(value: int | None) -> str:
    return "unknown" if value is None else f"0x{value:08x}"


def _short_resolved(items: list[ResolvedAddress], limit: int = 8) -> list[str]:
    if not items:
        return ["  no call-chain candidates"]
    lines = []
    for item in items[:limit]:
        lines.append(f"  {item.display}")
    if len(items) > limit:
        lines.append(f"  ... {len(items) - limit} more")
    return lines


def _render_exception_console(
    elf: Path,
    log: Path,
    sessions_count: int,
    session: DumpSession,
    analysis,
    interrupts,
    context,
    mode: str,
    side: str,
    heap=None,
) -> str:
    lines: list[str] = []
    lines.append("BK7259 Dump Analyzer")
    lines.append("")
    lines.append("Input")
    lines.append(f"  Log : {log}")
    lines.append(f"  ELF : {elf}")
    lines.append(f"  Mode: {mode}")
    lines.append(f"  Side: {side}")
    lines.append(f"  Session: {session.index}, lines {session.start_line}-{session.end_line}")
    lines.append("")

    lines.append("Summary")
    lines.append(f"  Dump reason : {session.dump_reason or 'not found'}")
    lines.append(f"  Build time  : {session.build_time or 'not found'}")
    if analysis.elf_build_match is True:
        lines.append(f"  ELF match   : yes ({analysis.elf_build_note})")
    elif analysis.elf_build_match is False:
        lines.append(f"  ELF match   : NO ({analysis.elf_build_note})")
    else:
        lines.append(f"  ELF match   : unknown ({analysis.elf_build_note or 'n/a'})")
    lines.append(f"  Stack regions : {len(session.stack_regions)}")
    lines.append(f"  Tracebacks    : {len(session.tracebacks)}")
    lines.append(f"  Likely point  : {analysis.likely_stuck}")
    lines.append(f"  Context       : {context.verdict} [{context.confidence}]")
    lines.append("")

    lines.append("Crash Point")
    if analysis.exception_stacks:
        for stack in analysis.exception_stacks:
            regs = stack.registers
            lines.append(
                f"  CPU{stack.core}: PC={_hex(regs.get('pc'))} "
                f"LR={_hex(regs.get('lr'))} MSP={_hex(regs.get('msp'))} PSP={_hex(regs.get('psp'))}"
            )
            fault = []
            for key in ("mmfar", "bfar", "cfsr", "hfsr"):
                if key in regs:
                    fault.append(f"{key.upper()}={_hex(regs.get(key))}")
            if fault:
                lines.append("  " + " ".join(fault))
            lines.extend(_short_resolved(stack.chain, limit=4))
    else:
        lines.append("  no CPU register block found")
    lines.append("")

    lines.append("Execution Context")
    lines.append(f"  Verdict : {context.verdict} [{context.confidence}]")
    if context.core is not None:
        lines.append(f"  Core    : CPU{context.core}")
    if context.evidence:
        lines.append("  Evidence:")
        for item in context.evidence[:8]:
            lines.append(f"    - {item}")
    if context.notes:
        lines.append("  Notes:")
        for item in context.notes[:4]:
            lines.append(f"    - {item}")
    lines.append("")

    lines.append("Current Core State")
    for core_state in analysis.current_cores:
        lines.append(
            f"  core{core_state.core}: TCB={_hex(core_state.tcb_address)} "
            f"{core_state.note or core_state.source or ''}"
        )
        if core_state.chain:
            lines.extend(_short_resolved(core_state.chain, limit=4))
    lines.append("")

    lines.append("Primary Call Chain Candidates")
    shown = primary_chains(analysis.stack_chains, limit=4)
    if shown:
        for chain in shown:
            suffix = f" likely={chain.likely_stuck}" if chain.likely_stuck else ""
            lines.append(f"  {chain.name}: {chain.source} [{chain.confidence}]{suffix}")
            lines.extend(_short_resolved(chain.resolved, limit=8))
        low_count = sum(1 for c in analysis.stack_chains if c.confidence == "low")
        if low_count:
            lines.append(f"  ({low_count} low-confidence stack-scan chains omitted; see -o report.md)")
    else:
        lines.append("  no task stack chains reconstructed")
    lines.append("")

    lines.append("Interrupt State")
    if interrupts:
        for summary in interrupts:
            last = summary.records[-1] if summary.records else None
            last_text = "none" if last is None else (
                f"irq={last.irq} done={int(last.done)} seq={last.seq} "
                f"enter={last.enter} exit={last.exit}"
            )
            incomplete = ", ".join(f"irq{r.irq}@{r.seq}" for r in summary.incomplete) or "none"
            lines.append(f"  {summary.core}: last={last_text}; incomplete={incomplete}")
    else:
        lines.append("  no interrupt recorder data")
    lines.append("")

    lines.append("Dump Integrity")
    lines.append(f"  Sessions       : {sessions_count}")
    lines.append(f"  Memory regions : {len(session.stack_regions)} parsed")
    lines.append("  CRC failures   : not checked in first-stage parser")
    lines.append("")

    if heap is not None:
        lines.extend(render_heap_console(heap))
        lines.append("")

    lines.append("Next Steps")
    if analysis.elf_build_match is False:
        lines.append("  WARNING: ELF build time does not match dump; symbols may be wrong.")
    if heap is not None and heap.overflow_count:
        lines.append("  Review Heap Integrity canary overflows (alloc site) before assuming a pure crash-point bug.")
    lines.append("  Review high-confidence traceback / crash-point frames first.")
    lines.append("  Use -o report.md for the full Markdown report (includes low-confidence scans).")
    return "\n".join(lines)


def list_ap_sessions(log: Path) -> str:
    sessions, _ = parse_dat(log, None)
    lines = []
    for item in sessions:
        lines.append(
            f"[{item.index}] lines={item.start_line}-{item.end_line} "
            f"build={item.build_time or '-'} reason={item.dump_reason or '-'} "
            f"regions={len(item.stack_regions)} tracebacks={len(item.tracebacks)}"
        )
    return "\n".join(lines)


def analyze_exception_dump(
    log: Path,
    elf: Path,
    session_index: int | None = None,
    *,
    mode: str,
    side: str,
) -> AnalysisOutput:
    sessions, session = parse_dat(log, session_index)
    symbols = ElfSymbols(elf)
    analysis = analyze_exception_session(session, symbols)
    interrupts = summarize_interrupts(session, symbols)
    context = analyze_context(session, analysis, interrupts)
    heap = analyze_heap(session, symbols)
    markdown = render_exception_markdown(
        elf, log, len(sessions), session, symbols, analysis, interrupts, side=side, heap=heap,
        context=context,
    )
    console = _render_exception_console(
        elf, log, len(sessions), session, analysis, interrupts, context, mode=mode,
        side=side, heap=heap,
    )
    exit_code = 0
    if analysis.elf_build_match is False:
        exit_code = 1
    if heap.overflow_count:
        exit_code = 1
    return AnalysisOutput(mode=mode, console=console, markdown=markdown, exit_code=exit_code)


def analyze_ap_dump(log: Path, elf: Path, session_index: int | None = None) -> AnalysisOutput:
    return analyze_exception_dump(log, elf, session_index, mode="ap-exception", side="AP")


def analyze_cp_dump(log: Path, elf: Path, session_index: int | None = None) -> AnalysisOutput:
    return analyze_exception_dump(log, elf, session_index, mode="cp-exception", side="CP")


def analyze_heartbeat_dump(log: Path, elf: Path) -> AnalysisOutput:
    sym = load_symbols(elf)
    regions = hb_core.parse_dump_regions(log)
    memmap = hb_core.build_memory_map(regions)
    extract_result = extract.build_extract(memmap, sym, elf)
    msp0 = msp_walk.walk_core(memmap, elf, sym, 0)
    msp1 = msp_walk.walk_core(memmap, elf, sym, 1)
    peri_report = peri_regs.decode_peripherals(memmap, regions)
    decoder_summary = decoders.summarise_for_report(elf, peri_report, memmap)
    header = hb_report._scan_header(log)
    markdown = hb_report.render_report(
        log, elf, header, extract_result, msp0, msp1, peri_report,
        decoder_summary=decoder_summary,
    )

    smoking = any(
        s and s.stuck_record is not None
        for s in (extract_result.irq_summary_core0, extract_result.irq_summary_core1)
    )
    console = "\n".join([
        "BK7259 Dump Analyzer",
        "",
        "Input",
        f"  Log : {log}",
        f"  ELF : {elf}",
        "  Mode: heartbeat",
        "",
        "Summary",
        f"  Dump reason : {header.dump_reason or 'not found'}",
        f"  Build time  : {header.build_time or 'not found'}",
        f"  Regions     : {len(regions)}",
        f"  Smoking gun : {'yes' if smoking else 'no'}",
        "",
        "Next Steps",
        "  Use -o report.md for the full Markdown report.",
    ])
    return AnalysisOutput(mode="heartbeat", console=console, markdown=markdown, exit_code=1 if smoking else 0)
