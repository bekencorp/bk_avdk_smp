#!/usr/bin/env python3
from __future__ import annotations

import base64
import binascii
import re
from dataclasses import dataclass, field
from pathlib import Path


HEX_BYTE_RE = re.compile(r"\b[0-9a-fA-F]{2}\b")
HEX_RE = re.compile(r"0x[0-9a-fA-F]+|(?<![A-Za-z0-9_])[0-9a-fA-F]{8}(?![A-Za-z0-9_])")
STACK_BEGIN_RE = re.compile(
    r">>>>stack mem dump begin(?:,\s*region:\s*(?P<region>[^,]+))?.*?"
    r"stack_top=(?P<top>[0-9a-fA-F]+)\s*,\s*stack end=(?P<end>[0-9a-fA-F]+)",
    re.IGNORECASE,
)
STACK_END_RE = re.compile(r"<<<<stack mem dump end", re.IGNORECASE)
REG_RE = re.compile(r"^\s*(?:\d+\s+)?(?P<name>[A-Za-z][A-Za-z0-9_]*)\s+x\s+0x(?P<value>[0-9a-fA-F]+)\s*$")
TRACE_CMD_RE = re.compile(r"arm-none-eabi-addr2line\s+-piaf\s+-e\s+app\.elf\s+(?P<addrs>.*)")
INT_RE = re.compile(
    r"\[(?P<core>core\d+)\]\[(?P<seq>\d+)\]\s+irq=(?P<irq>\d+)\s+done=(?P<done>\d+)\s+"
    r"enter=(?P<enter>\d+)\s+exit=(?P<exit>\d+)"
)
INT_HEADER_RE = re.compile(r"interrupt recorder (?P<core>core\d+) total=(?P<total>\d+) depth=(?P<depth>\d+)")


@dataclass
class StackRegion:
    name: str
    start: int
    end: int
    data: bytes
    line_start: int
    line_end: int

    def contains(self, address: int, size: int = 1) -> bool:
        return self.start <= address and address + size <= self.start + len(self.data)

    def read_u32(self, address: int) -> int | None:
        if not self.contains(address, 4):
            return None
        off = address - self.start
        return int.from_bytes(self.data[off:off + 4], "little")

    def iter_words(self):
        limit = len(self.data) - (len(self.data) % 4)
        for off in range(0, limit, 4):
            yield self.start + off, int.from_bytes(self.data[off:off + 4], "little")


@dataclass
class RegisterBlock:
    core: int
    registers: dict[str, int] = field(default_factory=dict)
    line_start: int = 0


@dataclass
class TracebackCommand:
    line_no: int
    addresses: list[int]
    raw: str


@dataclass
class InterruptRecord:
    core: str
    seq: int
    irq: int
    done: bool
    enter: int
    exit: int
    source: str = "text"


@dataclass
class InterruptHeader:
    core: str
    total: int
    depth: int


@dataclass
class TaskRow:
    name: str
    state: str = ""
    priority: str = ""
    stack_top: int | None = None
    stack_range: tuple[int, int] | None = None
    stack_size: int | None = None
    overflow: str = ""
    addresses: list[int] = field(default_factory=list)
    raw: str = ""
    source: str = "text"


@dataclass
class DumpSession:
    index: int
    start_line: int
    end_line: int
    lines: list[str]
    build_time: str | None = None
    dump_reason: str | None = None
    registers: list[RegisterBlock] = field(default_factory=list)
    tracebacks: list[TracebackCommand] = field(default_factory=list)
    stack_regions: list[StackRegion] = field(default_factory=list)
    interrupt_headers: list[InterruptHeader] = field(default_factory=list)
    interrupt_records: list[InterruptRecord] = field(default_factory=list)
    task_rows: list[TaskRow] = field(default_factory=list)
    evidence: list[tuple[int, str]] = field(default_factory=list)


def parse_int(text: str) -> int:
    return int(text, 16) if text.lower().startswith("0x") else int(text, 16)


def split_sessions(text: str) -> list[tuple[int, int, list[str]]]:
    lines = text.splitlines()
    begins = [i for i, line in enumerate(lines) if "user except handler begin" in line.lower()]
    if not begins:
        return [(1, len(lines), lines)]

    sessions: list[tuple[int, int, list[str]]] = []
    for begin in begins:
        end = len(lines) - 1
        for idx in range(begin + 1, len(lines)):
            if "user except handler end" in lines[idx].lower():
                end = idx
                break
        sessions.append((begin + 1, end + 1, lines[begin:end + 1]))
    return sessions


def _decode_ascii_stack(lines: list[str]) -> bytes:
    data = bytearray()
    for line in lines:
        tokens = HEX_BYTE_RE.findall(line)
        if tokens:
            data.extend(int(tok, 16) for tok in tokens)
    return bytes(data)


def _decode_base64_stack(lines: list[str]) -> bytes:
    data = bytearray()
    for line in lines:
        chunk = line.strip()
        if not chunk:
            continue
        try:
            raw = base64.b64decode(chunk, validate=False)
        except binascii.Error:
            continue
        if len(raw) >= 2:
            data.extend(raw[:-1])  # last byte is the coredump CRC8
    return bytes(data)


def parse_stack_regions(lines: list[str], base_line_no: int) -> list[StackRegion]:
    regions: list[StackRegion] = []
    idx = 0
    while idx < len(lines):
        match = STACK_BEGIN_RE.search(lines[idx])
        if not match:
            idx += 1
            continue

        name = (match.group("region") or f"stack@{match.group('top')}").strip()
        start = int(match.group("top"), 16)
        end = int(match.group("end"), 16)
        body: list[str] = []
        begin_idx = idx
        idx += 1
        while idx < len(lines) and not STACK_END_RE.search(lines[idx]):
            body.append(lines[idx])
            idx += 1
        end_idx = idx if idx < len(lines) else len(lines) - 1
        ascii_data = _decode_ascii_stack(body)
        data = ascii_data if ascii_data else _decode_base64_stack(body)
        if end > start:
            data = data[: end - start]
        regions.append(
            StackRegion(
                name=name,
                start=start,
                end=end,
                data=data,
                line_start=base_line_no + begin_idx,
                line_end=base_line_no + end_idx,
            )
        )
        idx += 1
    return regions


def parse_registers(lines: list[str], base_line_no: int) -> list[RegisterBlock]:
    blocks: list[RegisterBlock] = []
    current: RegisterBlock | None = None
    for offset, line in enumerate(lines):
        m = re.search(r"CPU(?P<core>\d+)\s+Current regs", line)
        if m:
            current = RegisterBlock(core=int(m.group("core")), line_start=base_line_no + offset)
            blocks.append(current)
            continue
        if current is None:
            continue
        reg = REG_RE.match(line)
        if reg:
            current.registers[reg.group("name").lower()] = int(reg.group("value"), 16)
        elif line.strip() == "" or line.startswith("Traceback:"):
            current = None
    return blocks


def parse_tracebacks(lines: list[str], base_line_no: int) -> list[TracebackCommand]:
    tracebacks: list[TracebackCommand] = []
    for offset, line in enumerate(lines):
        m = TRACE_CMD_RE.search(line)
        if not m:
            continue
        addrs = [int(tok, 16) for tok in HEX_RE.findall(m.group("addrs"))]
        tracebacks.append(TracebackCommand(line_no=base_line_no + offset, addresses=addrs, raw=line.strip()))
    return tracebacks


def parse_interrupts(lines: list[str]) -> tuple[list[InterruptHeader], list[InterruptRecord]]:
    headers: list[InterruptHeader] = []
    records: list[InterruptRecord] = []
    for line in lines:
        h = INT_HEADER_RE.search(line)
        if h:
            headers.append(InterruptHeader(h.group("core"), int(h.group("total")), int(h.group("depth"))))
        m = INT_RE.search(line)
        if m:
            records.append(
                InterruptRecord(
                    core=m.group("core"),
                    seq=int(m.group("seq")),
                    irq=int(m.group("irq")),
                    done=bool(int(m.group("done"))),
                    enter=int(m.group("enter")),
                    exit=int(m.group("exit")),
                )
            )
    return headers, records


def parse_task_rows(lines: list[str]) -> list[TaskRow]:
    rows: list[TaskRow] = []
    stack_line_re = re.compile(
        r"^\s*(?P<name>\S+)\s+\[(?P<low>(?:0x)?[0-9a-fA-F]+)\s*~\s*(?P<high>(?:0x)?[0-9a-fA-F]+)\]\s+"
        r"(?P<top>(?:0x)?[0-9a-fA-F]+)\s+(?P<size>\d+)\s+(?P<overflow>\d+)\s*(?P<rest>.*)$"
    )
    for line in lines:
        m = stack_line_re.match(line)
        if m:
            rows.append(
                TaskRow(
                    name=m.group("name"),
                    stack_top=parse_int(m.group("top")),
                    stack_range=(parse_int(m.group("low")), parse_int(m.group("high"))),
                    stack_size=int(m.group("size")),
                    overflow=m.group("overflow"),
                    addresses=[int(tok, 16) for tok in HEX_RE.findall(m.group("rest"))],
                    raw=line.strip(),
                    source="stack-backtrace-table",
                )
            )
            continue
        stripped = line.strip()
        if not stripped or stripped.startswith(("-", "=")):
            continue
        if re.search(r"\b(Ready|Blocked|Suspended|Deleted|Running|Run|B|R|S|D)\b", stripped) and len(stripped.split()) >= 3:
            parts = stripped.split()
            if not parts[0].lower().startswith(("task", "name")):
                rows.append(TaskRow(name=parts[0], state=parts[1], priority=parts[2], raw=stripped, source="task-list"))
    return rows


def collect_evidence(lines: list[str], base_line_no: int) -> list[tuple[int, str]]:
    needles = (
        "build time =>",
        "@Dump-reason:",
        "Current regs",
        "Traceback:",
        "arm-none-eabi-addr2line",
        "interrupt recorder",
        "flexa port",
        "decode timeout",
        "user except handler",
    )
    evidence = []
    for offset, line in enumerate(lines):
        if any(n.lower() in line.lower() for n in needles):
            evidence.append((base_line_no + offset, line.strip()))
    return evidence[:80]


def parse_dat(path: Path, session_index: int | None = None) -> tuple[list[DumpSession], DumpSession]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    raw_sessions = split_sessions(text)
    sessions: list[DumpSession] = []
    for idx, (start_line, end_line, lines) in enumerate(raw_sessions):
        session = DumpSession(index=idx, start_line=start_line, end_line=end_line, lines=lines)
        for line in lines:
            if "build time =>" in line:
                session.build_time = line.split("=>", 1)[1].strip().strip("!")
            if "@Dump-reason:" in line:
                session.dump_reason = line.split(":", 1)[1].strip()
        session.registers = parse_registers(lines, start_line)
        session.tracebacks = parse_tracebacks(lines, start_line)
        session.stack_regions = parse_stack_regions(lines, start_line)
        session.interrupt_headers, session.interrupt_records = parse_interrupts(lines)
        session.task_rows = parse_task_rows(lines)
        session.evidence = collect_evidence(lines, start_line)
        sessions.append(session)

    if not sessions:
        raise RuntimeError(f"No dump sessions found in {path}")
    selected = sessions[session_index] if session_index is not None else sessions[-1]
    return sessions, selected

