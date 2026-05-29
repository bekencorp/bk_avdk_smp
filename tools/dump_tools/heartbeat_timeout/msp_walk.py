"""Per-core MSP stack walk.

Heartbeat-timeout dumps do not contain saved CPU registers (the CP initiates
the dump without halting the AP). We recover the IRQ context by scanning the
MSP region for code-like words and EXC_RETURN tokens, then anchor exception
stack frames around each EXC_RETURN.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field
from pathlib import Path

from .core import (
    EXC_RETURN_VALUES,
    addr2line,
    get_text_ranges,
    in_code,
    read_bytes,
)
from .symbols import SymbolTable


@dataclass
class StackSlot:
    sp: int
    value: int
    kind: str           # 'EXC_RETURN' | 'lr/ret' | 'code?'
    resolved: str = ""


@dataclass
class FrameProbe:
    delta: int
    sp: int
    value: int
    resolved: str


@dataclass
class ExceptionFrame:
    exc_return_sp: int
    exc_return_value: int
    probes: list[FrameProbe] = field(default_factory=list)


@dataclass
class MspWalkResult:
    core: int
    msp_lo: int
    msp_hi: int
    captured: bool
    slots: list[StackSlot] = field(default_factory=list)
    frames: list[ExceptionFrame] = field(default_factory=list)


def _resolve_batch(elf: Path, addrs: list[int]) -> dict[int, str]:
    if not addrs:
        return {}
    out_lines = addr2line(elf, addrs)
    res: dict[int, str] = {}
    cur_idx = 0
    for a in addrs:
        line = out_lines[cur_idx] if cur_idx < len(out_lines) else ""
        cur_idx += 1
        chain = [line]
        while cur_idx < len(out_lines) and out_lines[cur_idx].startswith(" (inlined by"):
            chain.append(out_lines[cur_idx])
            cur_idx += 1
        res[a] = "\n        ".join(chain)
    return res


def walk_core(memmap, elf: Path, sym: SymbolTable, core: int) -> MspWalkResult:
    msp = sym.core0_msp_range if core == 0 else sym.core1_msp_range
    if not msp:
        return MspWalkResult(core=core, msp_lo=0, msp_hi=0, captured=False)
    msp_lo, msp_hi = msp
    data = read_bytes(memmap, msp_lo, msp_hi - msp_lo)
    if data is None:
        return MspWalkResult(core=core, msp_lo=msp_lo, msp_hi=msp_hi, captured=False)

    code_ranges = get_text_ranges(elf)
    slots: list[StackSlot] = []
    for off in range(0, len(data), 4):
        v = struct.unpack_from("<I", data, off)[0]
        sp = msp_lo + off
        if v in EXC_RETURN_VALUES:
            slots.append(StackSlot(sp, v, "EXC_RETURN"))
        elif (v & 1) and in_code(code_ranges, v):
            slots.append(StackSlot(sp, v, "lr/ret"))
        elif (v & ~1) != 0 and in_code(code_ranges, v):
            slots.append(StackSlot(sp, v, "code?"))

    sym_table = _resolve_batch(elf, [s.value for s in slots if s.kind != "EXC_RETURN"])
    for s in slots:
        s.resolved = sym_table.get(s.value, "")

    frames: list[ExceptionFrame] = []
    for s in slots:
        if s.kind != "EXC_RETURN":
            continue
        frame = ExceptionFrame(exc_return_sp=s.sp, exc_return_value=s.value)
        for delta in (-32, -28, -24, -20, -16, -12, -8, -4, 4, 8, 12, 16, 20, 24, 28, 32):
            probe = s.sp + delta
            if probe < msp_lo or probe + 4 > msp_hi:
                continue
            pv = struct.unpack_from("<I", data, probe - msp_lo)[0]
            if (pv & 1) and in_code(code_ranges, pv):
                resolved = _resolve_batch(elf, [pv]).get(pv, "")
                frame.probes.append(FrameProbe(delta=delta, sp=probe, value=pv, resolved=resolved))
        if frame.probes:
            frames.append(frame)

    return MspWalkResult(
        core=core,
        msp_lo=msp_lo,
        msp_hi=msp_hi,
        captured=True,
        slots=slots,
        frames=frames,
    )


def render_msp_text(result: MspWalkResult) -> str:
    if not result.captured:
        return (
            f"# AP core{result.core} MSP region "
            f"{result.msp_lo:#x}..{result.msp_hi:#x} not in dump\n"
        )

    lines: list[str] = []
    size = result.msp_hi - result.msp_lo
    lines.append(
        f"# AP core{result.core} MSP region "
        f"{result.msp_lo:#x}..{result.msp_hi:#x} ({size} B) recovered from dump"
    )
    lines.append(f"# {len(result.slots)} candidate stack slots referencing code / EXC_RETURN")
    lines.append("")
    for s in result.slots:
        lines.append(
            f"  [SP={s.sp:#010x}] = {s.value:#010x}  {s.kind:<10}  {s.resolved}"
        )

    if result.frames:
        lines.append("")
        lines.append("# Possible ARMv8-M exception stack frames anchored at EXC_RETURN")
        for frame in result.frames:
            lines.append(
                f"  EXC_RETURN @ {frame.exc_return_sp:#010x}  ({frame.exc_return_value:#010x})"
            )
            for p in frame.probes:
                lines.append(
                    f"    SP{p.delta:+4d} [{p.sp:#010x}] = {p.value:#010x}  {p.resolved}"
                )
    return "\n".join(lines) + "\n"
