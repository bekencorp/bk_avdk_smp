#!/usr/bin/env python3
from __future__ import annotations

import re
from dataclasses import dataclass, field

from .dat_parser import DumpSession
from .interrupt_analysis import CoreInterruptSummary
from .stack_analysis import AnalysisResult


ISR_SYMBOL_HINTS = (
    "timer_group_isr",
    "bk_ipi_isr_dispatch",
    "bk_int_dispatch_registered_isr",
    "_isr",
)


@dataclass
class ContextAnalysis:
    verdict: str
    confidence: str
    core: int | None = None
    evidence: list[str] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)


def _dump_test_mode(session: DumpSession) -> str:
    for line in session.lines:
        match = re.search(r"\bDUMP_TEST_BEGIN\b.*\bmode=([A-Za-z0-9_]+)", line)
        if match:
            return match.group(1)
    return ""


def _symbol_names(analysis: AnalysisResult) -> list[str]:
    names: list[str] = []
    for stack in analysis.exception_stacks:
        for item in stack.chain:
            if item.symbol:
                names.append(item.symbol.name)
    for chain in analysis.stack_chains:
        if chain.confidence == "low":
            continue
        for item in chain.resolved:
            if item.symbol:
                names.append(item.symbol.name)
    return names


def _isr_symbol_hits(names: list[str]) -> list[str]:
    hits: list[str] = []
    for name in names:
        lower = name.lower()
        if any(hint in lower for hint in ISR_SYMBOL_HINTS):
            if name not in hits:
                hits.append(name)
    return hits


def _exception_regs(analysis: AnalysisResult) -> tuple[int | None, dict[str, int]]:
    if not analysis.exception_stacks:
        return None, {}
    stack = analysis.exception_stacks[0]
    return stack.core, stack.registers


def _interrupt_incomplete(interrupts: list[CoreInterruptSummary]) -> list[str]:
    out: list[str] = []
    for summary in interrupts:
        for rec in summary.incomplete:
            if rec.irq == 0 and rec.seq == 0 and rec.enter == 0 and rec.exit == 0:
                continue
            out.append(f"{summary.core}: irq{rec.irq}@{rec.seq} entered but not exited")
    return out


def analyze_context(
    session: DumpSession,
    analysis: AnalysisResult,
    interrupts: list[CoreInterruptSummary],
) -> ContextAnalysis:
    mode = _dump_test_mode(session)
    core, regs = _exception_regs(analysis)
    names = _symbol_names(analysis)
    isr_hits = _isr_symbol_hits(names)
    incomplete = _interrupt_incomplete(interrupts)

    xpsr = regs.get("xpsr")
    ipsr = (xpsr & 0x1FF) if xpsr is not None else 0
    primask = regs.get("primask", 0)
    basepri = regs.get("basepri", 0)
    faultmask = regs.get("faultmask", 0)

    evidence: list[str] = []
    notes: list[str] = []

    if mode:
        evidence.append(f"DUMP_TEST mode={mode}")

    if xpsr is not None:
        evidence.append(f"xPSR=0x{xpsr:08x}, exception={ipsr}")
    if isr_hits:
        evidence.append("ISR frames: " + ", ".join(isr_hits[:6]))
    if incomplete:
        evidence.extend(incomplete[:4])

    if primask or faultmask:
        evidence.append(
            f"interrupt mask register set: PRIMASK=0x{primask:x} FAULTMASK=0x{faultmask:x}"
        )
    if basepri:
        notes.append(
            f"BASEPRI=0x{basepri:x} observed; use as weak evidence only because dump/assert paths may also raise BASEPRI"
        )

    # Test mode is a strong, explicit label for dump_test validation logs.
    if mode.startswith("critical_"):
        return ContextAnalysis(
            verdict="critical section / interrupts masked",
            confidence="high",
            core=core,
            evidence=evidence,
            notes=notes,
        )
    if mode.startswith("isr_"):
        return ContextAnalysis("ISR / handler", "high", core, evidence, notes)
    if mode.startswith("task_"):
        return ContextAnalysis("task / thread", "high", core, evidence, notes)

    # Production dumps: prefer architectural and call-chain evidence.
    if ipsr:
        return ContextAnalysis("ISR / handler", "high", core, evidence, notes)
    if isr_hits:
        return ContextAnalysis("ISR / handler", "medium", core, evidence, notes)
    if incomplete:
        return ContextAnalysis("ISR possibly active", "medium", core, evidence, notes)
    if primask or faultmask:
        return ContextAnalysis(
            "thread with interrupts masked",
            "medium",
            core,
            evidence,
            notes,
        )
    if basepri:
        return ContextAnalysis(
            "thread / task (BASEPRI mask observed)",
            "low",
            core,
            evidence,
            notes,
        )
    if regs:
        return ContextAnalysis("task / thread", "medium", core, evidence, notes)
    return ContextAnalysis("unknown", "low", core, evidence, notes)
