#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from .dat_parser import DumpSession
from .elf_symbols import ElfSymbols
from .interrupt_analysis import CoreInterruptSummary
from .stack_analysis import AnalysisResult, StackChain


def _hex(value: int | None) -> str:
    return "unknown" if value is None else f"0x{value:08x}"


def _table(headers: list[str], rows: list[list[str]]) -> list[str]:
    if not rows:
        return ["_No data found._", ""]
    out = ["| " + " | ".join(headers) + " |", "| " + " | ".join(["---"] * len(headers)) + " |"]
    out.extend("| " + " | ".join(cell.replace("\n", "<br>") for cell in row) + " |" for row in rows)
    out.append("")
    return out


def _chain_lines(chain: StackChain, limit: int = 16) -> list[str]:
    if not chain.resolved:
        return ["  - No code-looking return addresses found."]
    lines = []
    for item in chain.resolved[:limit]:
        lines.append(f"  - `{item.display}`")
    if len(chain.resolved) > limit:
        lines.append(f"  - ... {len(chain.resolved) - limit} more")
    return lines


def render_report(
    elf_path: Path,
    dat_path: Path,
    sessions_count: int,
    session: DumpSession,
    symbols: ElfSymbols,
    analysis: AnalysisResult,
    interrupts: list[CoreInterruptSummary],
) -> str:
    px = symbols.get("pxCurrentTCBs")
    lines: list[str] = []
    lines.append("# AP Dump Analysis Report")
    lines.append("")
    lines.append("## Summary")
    lines.extend(
        _table(
            ["Item", "Value"],
            [
                ["ELF", str(elf_path)],
                ["DAT", str(dat_path)],
                ["Selected session", f"{session.index} of {sessions_count} (lines {session.start_line}-{session.end_line})"],
                ["Build time", session.build_time or "not found"],
                ["Dump reason", session.dump_reason or "not found"],
                ["Likely stuck point", analysis.likely_stuck],
                ["pxCurrentTCBs", _hex(px.address if px else None)],
                ["Stack regions", str(len(session.stack_regions))],
                ["Traceback commands", str(len(session.tracebacks))],
            ],
        )
    )

    lines.append("## Current Core State")
    rows = []
    for core in analysis.current_cores:
        task = core.task.name if core.task else ""
        rows.append([f"core{core.core}", _hex(core.tcb_address), task, core.source or "", core.note or ""])
    lines.extend(_table(["Core", "TCB", "Task", "Source", "Note"], rows))
    for core in analysis.current_cores:
        if not core.chain:
            continue
        lines.append(f"### core{core.core} Current Stack Chain")
        for item in core.chain[:16]:
            lines.append(f"- `{item.display}`")
        lines.append("")

    lines.append("## Task Overview")
    task_rows = []
    for row in session.task_rows[:80]:
        rng = ""
        if row.stack_range:
            rng = f"0x{row.stack_range[0]:08x}..0x{row.stack_range[1]:08x}"
        task_rows.append([row.name, row.state, row.priority, rng, _hex(row.stack_top), row.source])
    lines.extend(_table(["Task", "State", "Priority", "Stack Range", "Stack Top", "Source"], task_rows))

    lines.append("## Task Call Chains")
    if not analysis.stack_chains:
        lines.append("_No task backtrace table or stack-scan chains were reconstructed._")
        lines.append("")
    for chain in analysis.stack_chains:
        rng = ""
        if chain.stack_range:
            rng = f" stack=0x{chain.stack_range[0]:08x}..0x{chain.stack_range[1]:08x}"
        stuck = f" likely_stuck=`{chain.likely_stuck}`" if chain.likely_stuck else ""
        lines.append(f"### {chain.name}")
        lines.append(f"- Source: `{chain.source}`{rng}{stuck}")
        lines.extend(_chain_lines(chain))
        lines.append("")

    lines.append("## Interrupt Analysis")
    int_rows = []
    for summary in interrupts:
        incomplete = ", ".join(f"irq{r.irq}@{r.seq}" for r in summary.incomplete) or "none"
        last = summary.records[-1] if summary.records else None
        last_text = "" if not last else f"irq={last.irq} done={int(last.done)} seq={last.seq}"
        int_rows.append([summary.core, str(summary.total or ""), str(summary.depth or len(summary.records)), summary.source, last_text, incomplete])
    lines.extend(_table(["Core", "Total", "Depth", "Source", "Last Record", "Incomplete"], int_rows))

    lines.append("## Exception Stack Analysis")
    if not analysis.exception_stacks:
        lines.append("_No CPU register blocks were found._")
        lines.append("")
    for stack in analysis.exception_stacks:
        pc = stack.registers.get("pc")
        lr = stack.registers.get("lr")
        msp = stack.registers.get("msp")
        psp = stack.registers.get("psp")
        lines.append(f"### CPU{stack.core}")
        lines.append(f"- PC: `{_hex(pc)}`, LR: `{_hex(lr)}`, MSP: `{_hex(msp)}`, PSP: `{_hex(psp)}`")
        for item in stack.chain:
            lines.append(f"  - `{item.display}`")
        lines.append("")

    lines.append("## Raw Evidence")
    if session.evidence:
        for line_no, text in session.evidence:
            lines.append(f"- Line {line_no}: `{text}`")
    else:
        lines.append("_No high-signal raw evidence lines matched known patterns._")
    lines.append("")

    if analysis.unresolved:
        lines.append("## Unresolved Addresses")
        lines.append(", ".join(f"`0x{addr:08x}`" for addr in analysis.unresolved[:80]))
        lines.append("")

    return "\n".join(lines)

