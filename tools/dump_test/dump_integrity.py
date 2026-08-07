#!/usr/bin/env python3
"""Strict BK7259 Base64 dump integrity analyzer (standard library only)."""

import argparse
import base64
import binascii
import json
import re
import sys
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple

CRC8_TABLE: Tuple[int, ...] = (
    0x00, 0xF7, 0xB9, 0x4E, 0x25, 0xD2, 0x9C, 0x6B, 0x4A, 0xBD, 0xF3, 0x04, 0x6F, 0x98, 0xD6, 0x21,
    0x94, 0x63, 0x2D, 0xDA, 0xB1, 0x46, 0x08, 0xFF, 0xDE, 0x29, 0x67, 0x90, 0xFB, 0x0C, 0x42, 0xB5,
    0x7F, 0x88, 0xC6, 0x31, 0x5A, 0xAD, 0xE3, 0x14, 0x35, 0xC2, 0x8C, 0x7B, 0x10, 0xE7, 0xA9, 0x5E,
    0xEB, 0x1C, 0x52, 0xA5, 0xCE, 0x39, 0x77, 0x80, 0xA1, 0x56, 0x18, 0xEF, 0x84, 0x73, 0x3D, 0xCA,
    0xFE, 0x09, 0x47, 0xB0, 0xDB, 0x2C, 0x62, 0x95, 0xB4, 0x43, 0x0D, 0xFA, 0x91, 0x66, 0x28, 0xDF,
    0x6A, 0x9D, 0xD3, 0x24, 0x4F, 0xB8, 0xF6, 0x01, 0x20, 0xD7, 0x99, 0x6E, 0x05, 0xF2, 0xBC, 0x4B,
    0x81, 0x76, 0x38, 0xCF, 0xA4, 0x53, 0x1D, 0xEA, 0xCB, 0x3C, 0x72, 0x85, 0xEE, 0x19, 0x57, 0xA0,
    0x15, 0xE2, 0xAC, 0x5B, 0x30, 0xC7, 0x89, 0x7E, 0x5F, 0xA8, 0xE6, 0x11, 0x7A, 0x8D, 0xC3, 0x34,
    0xAB, 0x5C, 0x12, 0xE5, 0x8E, 0x79, 0x37, 0xC0, 0xE1, 0x16, 0x58, 0xAF, 0xC4, 0x33, 0x7D, 0x8A,
    0x3F, 0xC8, 0x86, 0x71, 0x1A, 0xED, 0xA3, 0x54, 0x75, 0x82, 0xCC, 0x3B, 0x50, 0xA7, 0xE9, 0x1E,
    0xD4, 0x23, 0x6D, 0x9A, 0xF1, 0x06, 0x48, 0xBF, 0x9E, 0x69, 0x27, 0xD0, 0xBB, 0x4C, 0x02, 0xF5,
    0x40, 0xB7, 0xF9, 0x0E, 0x65, 0x92, 0xDC, 0x2B, 0x0A, 0xFD, 0xB3, 0x44, 0x2F, 0xD8, 0x96, 0x61,
    0x55, 0xA2, 0xEC, 0x1B, 0x70, 0x87, 0xC9, 0x3E, 0x1F, 0xE8, 0xA6, 0x51, 0x3A, 0xCD, 0x83, 0x74,
    0xC1, 0x36, 0x78, 0x8F, 0xE4, 0x13, 0x5D, 0xAA, 0x8B, 0x7C, 0x32, 0xC5, 0xAE, 0x59, 0x17, 0xE0,
    0x2A, 0xDD, 0x93, 0x64, 0x0F, 0xF8, 0xB6, 0x41, 0x60, 0x97, 0xD9, 0x2E, 0x45, 0xB2, 0xFC, 0x0B,
    0xBE, 0x49, 0x07, 0xF0, 0x9B, 0x6C, 0x22, 0xD5, 0xF4, 0x03, 0x4D, 0xBA, 0xD1, 0x26, 0x68, 0x9F,
)

PREFIX_RE = re.compile(r"^\[[^\]]+\]\s+(RX|TX|SYS)\s*\|\s?(.*)$")
RAW_TIMESTAMP_RE = re.compile(
    r"^\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}(?:\.\d+)?\s+(.*)$"
)
BEGIN_RE = re.compile(
    r"^>{4}stack mem dump begin,\s*region:\s*(.+?),\s*stack_top=(?:0x)?([0-9a-fA-F]+),\s*stack end=(?:0x)?([0-9a-fA-F]+)\s*$"
)
END_RE = re.compile(
    r"^<{4}stack mem dump end\.\s*region:\s*(.+?),\s*stack_top=(?:0x)?([0-9a-fA-F]+),\s*stack end=(?:0x)?([0-9a-fA-F]+)\s*$"
)
META_RE = re.compile(r"DUMP_TEST_BEGIN\s+case_id=(\S+)\s+target=(AP|CP)\s+core=(\d+)\s+mode=(\S+)")
REJECT_RE = re.compile(r"DUMP_TEST_REJECT\s+case_id=(\S+)\s+reason=(\S+)")
DUMP_REASON_RE = re.compile(r"@dump-reason\s*:\s*(.*)", re.IGNORECASE)
CORE_RE = re.compile(r"SMP-Core-id\s*(?:=>|=|:)\s*(\d+)", re.IGNORECASE)
REGISTERS_RE = re.compile(r"(?:CPU\s*registers|CPU\d+\s+Current\s+regs\s*:)", re.IGNORECASE)
PHYSICAL_CPU_RE = re.compile(r"CPU(\d+)\s+Current\s+regs\s*:", re.IGNORECASE)
SKIP_ADDR_RE = re.compile(r"skip mem dump.*?(?:0x)?([0-9a-fA-F]{6,8}).*?(?:0x)?([0-9a-fA-F]{6,8})", re.IGNORECASE)
SKIP_RE = re.compile(
    r"^>{4}skip mem dump,\s*region:\s*(.+?),\s*"
    r"stack_top=(?:0x)?([0-9a-fA-F]+),\s*"
    r"stack end=(?:0x)?([0-9a-fA-F]+),\s*"
    r"reason=(ap_bus_untrusted|ap_powered_down)\s*$",
    re.IGNORECASE,
)
RESULT_PRIORITY = (
    "HOST_ERROR", "NOT_TRIGGERED", "DUMP_INCOMPLETE", "DATA_CORRUPT",
    "REGION_MISMATCH", "PASS",
)


def hnd_crc8(data: bytes) -> int:
    crc = 0xFF
    for value in data:
        crc = CRC8_TABLE[(crc ^ value) & 0xFF]
    return crc


@dataclass
class SourceLine:
    number: int
    text: str


@dataclass
class RegionResult:
    name: str
    start: int
    end: int
    begin_line: int
    end_line: Optional[int] = None
    payload_lines: int = 0
    decoded_bytes: int = 0
    crc_lines: int = 0
    crc_failures: int = 0
    complete: bool = False
    errors: List[str] = field(default_factory=list)


@dataclass
class SkipResult:
    name: str
    start: int
    end: int
    reason: str
    line: int


@dataclass
class AnalysisResult:
    result: str
    dump_pass: bool
    boot_pass: bool
    recovery_pass: bool
    target: Optional[str]
    core: Optional[int]
    case_id: Optional[str]
    mode: Optional[str]
    fault_reason: Optional[str]
    session_start_line: Optional[int]
    session_end_line: Optional[int]
    crc_lines: int
    crc_failures: int
    regions: int
    decoded_bytes: int
    build_version: str
    power_state: str
    errors: List[str]
    region_details: List[RegionResult]
    skip_details: List[SkipResult]
    sessions_found: int = 0
    dump_present: bool = False
    expected_target: Optional[str] = None
    expected_core: Optional[int] = None
    expected_mode: Optional[str] = None
    requested_target: Optional[str] = None
    requested_core: Optional[int] = None
    requested_mode: Optional[str] = None
    actual_target: Optional[str] = None
    actual_core: Optional[int] = None
    actual_fault_reason: Optional[str] = None
    target_match: Optional[bool] = None
    mode_reason_match: Optional[bool] = None
    trigger_reject_reason: Optional[str] = None
    warnings: List[str] = field(default_factory=list)
    scenario: Optional[str] = None
    actual_dumper: Optional[str] = None
    heartbeat_timeout_seen: bool = False
    heartbeat_takeover_pass: Optional[bool] = None
    complete_dumps: int = 0

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


def _device_lines(text: str) -> Tuple[List[SourceLine], List[SourceLine]]:
    rx: List[SourceLine] = []
    all_lines: List[SourceLine] = []
    for number, raw in enumerate(text.splitlines(), 1):
        match = PREFIX_RE.match(raw)
        if match:
            direction, body = match.groups()
            all_lines.append(SourceLine(number, body))
            if direction == "RX":
                rx.append(SourceLine(number, body))
        else:
            # Raw firmware captures have no view_log direction prefix.
            raw_match = RAW_TIMESTAMP_RE.match(raw)
            body = raw_match.group(1) if raw_match else raw
            rx.append(SourceLine(number, body))
            all_lines.append(SourceLine(number, body))
    return rx, all_lines


def _split_sessions(lines: Sequence[SourceLine]) -> List[List[SourceLine]]:
    sessions: List[List[SourceLine]] = []
    current: List[SourceLine] = []
    has_reason = False
    has_time = False
    has_meta = False
    has_ap_begin = False
    has_ap_end = False
    has_cp_end = False
    has_handler_end = False
    for line in lines:
        text = line.text
        is_meta = "DUMP_TEST_BEGIN case_id=" in text
        is_reason = DUMP_REASON_RE.search(text) is not None
        is_time = "@Dump-time(AON-RTC):" in text
        new_fallback = not current and (is_reason or is_time)
        complete = has_ap_end or has_cp_end or has_handler_end
        # Cross-core output can put metadata after the reason/core/handler
        # header. Attach that late metadata to the open dump, but never merge
        # it into an already completed dump or a session that already has meta.
        if is_meta and current and (has_meta or complete):
            sessions.append(current)
            current = []
            has_reason = has_time = False
            has_meta = False
            has_ap_begin = has_ap_end = has_cp_end = has_handler_end = False
        elif current and is_reason and has_reason:
            sessions.append(current)
            current = []
            has_reason = has_time = False
            has_meta = False
            has_ap_begin = has_ap_end = has_cp_end = has_handler_end = False
        if is_meta or new_fallback or current:
            current.append(line)
            has_reason = has_reason or is_reason
            has_time = has_time or is_time
            has_meta = has_meta or is_meta
            lowered = text.lower()
            has_ap_begin = has_ap_begin or "ap memory dump begin" in lowered
            has_ap_end = has_ap_end or "ap memory dump end" in lowered
            has_cp_end = has_cp_end or "cp memory dump end" in lowered
            has_handler_end = (
                has_handler_end or "user except handler end" in lowered
            )
    if current:
        sessions.append(current)
    return sessions


def _contains(lines: Sequence[SourceLine], needle: str) -> bool:
    low = needle.lower()
    return any(low in line.text.lower() for line in lines)


def _session_complete(lines: Sequence[SourceLine]) -> bool:
    if _contains(lines, "CP memory dump begin"):
        return _contains(lines, "CP memory dump end")
    if _contains(lines, "AP memory dump begin"):
        return _contains(lines, "AP memory dump end")
    return _contains(lines, "user except handler end")


def _matches_any(lines: Sequence[SourceLine], patterns: Sequence[str], start_after: int = 0) -> bool:
    if not patterns:
        return False
    for line in lines:
        if line.number <= start_after:
            continue
        for pattern in patterns:
            if re.search(pattern, line.text, re.IGNORECASE):
                return True
    return False


def _extract_build(lines: Sequence[SourceLine]) -> str:
    for line in lines:
        match = re.search(r"build time\s*=>\s*(.+)", line.text, re.IGNORECASE)
        if match:
            return match.group(1).strip()
    return ""


def _expected_reason_kinds(mode: Optional[str]) -> Optional[Tuple[str, ...]]:
    if not mode:
        return None
    fault = mode.rsplit("_", 1)[-1].lower()
    if fault == "assert":
        return ("Assert",)
    if fault in ("udf", "divzero"):
        return ("UsageFault",)
    if fault in ("badpc", "crash"):
        return ("MemFault", "BusFault", "HardFault")
    return None


def _reason_mismatch(mode: Optional[str], reason: Optional[str]) -> Optional[str]:
    expected = _expected_reason_kinds(mode)
    if expected is None or reason is None:
        return None
    normalized_reason = re.sub(r"[^a-z0-9]", "", reason.lower())
    if any(re.sub(r"[^a-z0-9]", "", item.lower()) in normalized_reason
           for item in expected):
        return None
    return "mode {} expected dump reason {} but saw {!r}".format(
        mode, "/".join(expected), reason
    )


def _gap_has_skip(lines: Sequence[SourceLine], name: str, gap_start: int, gap_end: int) -> bool:
    for line in lines:
        if "skip mem dump" not in line.text.lower():
            continue
        match = SKIP_ADDR_RE.search(line.text)
        if match:
            first, second = int(match.group(1), 16), int(match.group(2), 16)
            low, high = min(first, second), max(first, second)
            if low <= gap_start and high >= gap_end:
                return True
        elif name.lower() in line.text.lower():
            return True
    return False


def _parse_regions(lines: Sequence[SourceLine]) -> Tuple[List[RegionResult], List[str], List[str], List[str]]:
    regions: List[RegionResult] = []
    incomplete: List[str] = []
    corrupt: List[str] = []
    mismatch: List[str] = []
    active: Optional[Tuple[RegionResult, List[SourceLine]]] = None
    for line in lines:
        begin = BEGIN_RE.match(line.text.strip())
        end = END_RE.match(line.text.strip())
        if begin:
            if active is not None:
                active[0].errors.append("nested region begin")
                incomplete.append("region {} at line {} has no end before nested begin".format(active[0].name, active[0].begin_line))
                regions.append(active[0])
            name, start, finish = begin.groups()
            region = RegionResult(name.strip(), int(start, 16), int(finish, 16), line.number)
            if region.end <= region.start:
                region.errors.append("invalid address range")
                mismatch.append("region {} has invalid range 0x{:x}-0x{:x}".format(region.name, region.start, region.end))
            active = (region, [])
            continue
        if end:
            name, _top, finish = end.groups()
            if active is None:
                incomplete.append("orphan region end at line {}".format(line.number))
                continue
            region, payload = active
            if name.strip() != region.name or int(finish, 16) != region.end:
                region.errors.append("end marker name/address mismatch")
                incomplete.append("region end mismatch at line {}".format(line.number))
                regions.append(region)
                active = None
                continue
            region.end_line = line.number
            region.complete = True
            expected = max(0, region.end - region.start)
            consumed = 0
            for index, payload_line in enumerate(payload):
                encoded = payload_line.text.strip()
                if not encoded:
                    continue
                region.payload_lines += 1
                try:
                    decoded = base64.b64decode(encoded.encode("ascii"), validate=True)
                except (UnicodeEncodeError, binascii.Error, ValueError) as exc:
                    region.errors.append("line {} invalid base64: {}".format(payload_line.number, exc))
                    corrupt.append("invalid base64 at line {}".format(payload_line.number))
                    continue
                remaining = expected - consumed
                wanted_data = min(32, max(0, remaining))
                wanted_length = wanted_data + 1
                if len(decoded) != wanted_length:
                    region.errors.append("line {} decoded length {} expected {}".format(payload_line.number, len(decoded), wanted_length))
                    corrupt.append("decoded line length mismatch at line {}".format(payload_line.number))
                    continue
                data, crc = decoded[:-1], decoded[-1]
                region.crc_lines += 1
                if hnd_crc8(data) != crc:
                    region.crc_failures += 1
                    region.errors.append("line {} CRC8 mismatch".format(payload_line.number))
                    corrupt.append("CRC8 mismatch at line {}".format(payload_line.number))
                consumed += len(data)
                region.decoded_bytes += len(data)
            if consumed != expected:
                region.errors.append("decoded bytes {} expected {}".format(consumed, expected))
                mismatch.append("region {} decoded length {} expected {}".format(region.name, consumed, expected))
            regions.append(region)
            active = None
            continue
        if active is not None:
            active[1].append(line)
    if active is not None:
        active[0].errors.append("missing region end")
        incomplete.append("region {} at line {} missing end".format(active[0].name, active[0].begin_line))
        regions.append(active[0])

    by_name: Dict[str, List[RegionResult]] = {}
    for region in regions:
        if region.complete:
            by_name.setdefault(region.name, []).append(region)
    for name, items in by_name.items():
        ordered = sorted(items, key=lambda item: (item.start, item.end))
        for previous, current in zip(ordered, ordered[1:]):
            if current.start < previous.end:
                mismatch.append("region {} segments overlap at 0x{:x}".format(name, current.start))
            elif current.start > previous.end and not _gap_has_skip(lines, name, previous.end, current.start):
                mismatch.append("region {} gap 0x{:x}-0x{:x} lacks skip evidence".format(name, previous.end, current.start))
    return regions, incomplete, corrupt, mismatch


def _parse_skips(lines: Sequence[SourceLine]) -> List[SkipResult]:
    skips: List[SkipResult] = []
    for line in lines:
        match = SKIP_RE.match(line.text.strip())
        if not match:
            continue
        name, start, end, reason = match.groups()
        skips.append(SkipResult(
            name=name.strip(), start=int(start, 16), end=int(end, 16),
            reason=reason.lower(), line=line.number,
        ))
    return skips


def _manifest_errors(
    regions: Sequence[RegionResult], skips: Sequence[SkipResult],
    manifest: Optional[Dict[str, Any]], target: Optional[str],
    build_version: str, power_state: str,
) -> List[str]:
    if not manifest:
        return []
    candidates = manifest.get("regions", [])
    expected = []
    for item in candidates:
        if item.get("target") != target:
            continue
        if item.get("build_version") and item.get("build_version") != build_version:
            continue
        if item.get("power_state", "default") != power_state:
            continue
        expected.append((
            item.get("region_name"), int(item.get("start")), int(item.get("end")),
        ))
    had_expected = bool(expected)
    expected = [
        item for item in expected
        if not any(
            skip.name == item[0] and skip.start <= item[1] and skip.end >= item[2]
            for skip in skips
        )
    ]
    actual = []
    for region in regions:
        actual.append((region.name, region.start, region.end))
    if had_expected and sorted(expected) != sorted(actual):
        return ["regions differ from golden manifest"]
    return []


def analyze_text(
    text: str,
    expected_target: Optional[str] = None,
    expected_core: Optional[int] = None,
    expected_mode: Optional[str] = None,
    boot_expect: Sequence[str] = (),
    recovery_expect: Sequence[str] = (),
    golden_manifest: Optional[Dict[str, Any]] = None,
    power_state: str = "default",
    scenario: Optional[str] = None,
) -> AnalysisResult:
    rx, all_lines = _device_lines(text)
    sessions = _split_sessions(rx)
    reject = next(
        (REJECT_RE.search(line.text) for line in reversed(rx)
         if REJECT_RE.search(line.text)),
        None,
    )
    reject_case_id = reject.group(1) if reject else None
    reject_reason = reject.group(2) if reject else None
    if not sessions:
        host_errors = [line.text for line in all_lines if re.search(r"PHYS GAP|BUFFER OVERFLOW|CONN_LOST|connection (?:failed|error)", line.text, re.IGNORECASE)]
        errors = ["no dump trigger metadata or dump header"]
        if host_errors:
            errors = host_errors + errors
        result = "HOST_ERROR" if host_errors else "NOT_TRIGGERED"
        warnings = (
            ["dump trigger rejected: {}".format(reject_reason)]
            if reject_reason else []
        )
        return AnalysisResult(
            result=result, dump_pass=False, boot_pass=False,
            recovery_pass=False, target=None, core=None,
            case_id=reject_case_id, mode=expected_mode, fault_reason=None,
            session_start_line=None, session_end_line=None, crc_lines=0,
            crc_failures=0, regions=0, decoded_bytes=0, build_version="",
            power_state=power_state, errors=errors, region_details=[],
            skip_details=[],
            sessions_found=0, dump_present=False,
            expected_target=expected_target, expected_core=expected_core,
            expected_mode=expected_mode, actual_target=None, actual_core=None,
            actual_fault_reason=None, target_match=None,
            mode_reason_match=None, trigger_reject_reason=reject_reason,
            warnings=warnings, scenario=scenario,
            heartbeat_takeover_pass=(
                False if scenario in (
                    "ap_heartbeat_timeout", "cp_heartbeat_timeout",
                ) else None
            ),
        )

    complete_sessions = [item for item in sessions if _session_complete(item)]
    session = complete_sessions[-1] if complete_sessions else sessions[-1]
    complete_dumps = len(complete_sessions)
    session_start = session[0].number
    tx_before_session = any(
        PREFIX_RE.match(raw)
        and PREFIX_RE.match(raw).group(1) == "TX"
        for raw in text.splitlines()[:max(0, session_start - 1)]
    )
    host_errors = [
        line.text for line in all_lines
        if (line.number >= session_start or not tx_before_session)
        and re.search(r"PHYS GAP|BUFFER OVERFLOW|CONN_LOST|connection (?:failed|error)", line.text, re.IGNORECASE)
    ]
    errors: List[str] = list(host_errors)
    warnings: List[str] = []
    meta = next((META_RE.search(line.text) for line in session if META_RE.search(line.text)), None)
    case_id = meta.group(1) if meta else None
    requested_target = meta.group(2) if meta else None
    requested_core = int(meta.group(3)) if meta else None
    requested_mode = meta.group(4) if meta else None
    mode = requested_mode or expected_mode
    reason_match = next(
        (DUMP_REASON_RE.search(line.text) for line in session
         if DUMP_REASON_RE.search(line.text)),
        None,
    )
    fault_reason = reason_match.group(1).strip() if reason_match else None
    core_match = next((CORE_RE.search(line.text) for line in session if CORE_RE.search(line.text)), None)
    physical_cpu_match = next(
        (PHYSICAL_CPU_RE.search(line.text) for line in session
         if PHYSICAL_CPU_RE.search(line.text)),
        None,
    )
    has_ap_markers = (
        _contains(session, "AP memory dump begin")
        or _contains(session, "AP memory dump end")
    )
    cp_takeover_ap_hang = scenario == "ap_heartbeat_timeout" or (
        _contains(session, "IPC[2]heartbeat timeout")
        and _contains(session, "AP memory dump begin")
    )
    ap_observer_cp_hang = scenario == "cp_heartbeat_timeout" or (
        _contains(session, "observer=AP target=CP reason=cp_heartbeat_timeout")
        or _contains(session, "CP heartbeat timeout observed by AP")
    )
    heartbeat_timeout_seen = (
        _contains(session, "IPC[2]heartbeat timeout")
        or (
            cp_takeover_ap_hang
            and _contains(rx, "[hb_test] heartbeat STOPPED")
        )
        or _contains(
            session, "observer=AP target=CP reason=cp_heartbeat_timeout",
        )
    )
    has_handler_dump = (
        _contains(session, "user except handler begin")
        or _contains(session, "user except handler end")
        or any(BEGIN_RE.match(line.text.strip()) for line in session)
    )
    has_ap_direct_dump = bool(
        not has_ap_markers
        and physical_cpu_match is not None
        and int(physical_cpu_match.group(1)) >= 2
        and has_handler_dump
    )
    dump_present = bool(has_ap_markers or has_ap_direct_dump or has_handler_dump)
    actual_target = (
        "CP" if ap_observer_cp_hang
        else ("AP" if (has_ap_markers or has_ap_direct_dump)
              else ("CP" if has_handler_dump else None))
    )
    actual_dumper = (
        "CP" if cp_takeover_ap_hang
        else ("AP" if ap_observer_cp_hang else actual_target)
    )
    actual_core = (
        None if cp_takeover_ap_hang
        else (0 if ap_observer_cp_hang
              else (int(core_match.group(1)) if core_match else (
                  requested_core if actual_target == "AP"
                  else (0 if actual_target == "CP" else None)
              )))
    )
    target_match: Optional[bool] = None
    target_warnings: List[str] = []
    if dump_present and expected_target:
        target_match = actual_target == expected_target
        if not target_match:
            target_warnings.append(
                "expected target {} but physical dump is {}".format(
                    expected_target, actual_target,
                )
            )
    if (
        dump_present and target_match is not False and expected_target == "AP"
        and expected_core is not None
    ):
        core_matches = actual_core == expected_core
        target_match = core_matches
        if not core_matches:
            target_warnings.append(
                "expected AP core {} but physical dump core is {}".format(
                    expected_core, actual_core,
                )
            )
    if requested_target and actual_target and requested_target != actual_target:
        target_warnings.append(
            "metadata requested target {} but physical dump is {}".format(
                requested_target, actual_target,
            )
        )
    if requested_mode and expected_mode and requested_mode != expected_mode:
        target_warnings.append(
            "expected mode {} but metadata says {}".format(
                expected_mode, requested_mode,
            )
        )
    warnings.extend(target_warnings)

    required = (
        ("@Dump-reason", "build time =>", "user except handler begin")
        if ap_observer_cp_hang else (
            "@Dump-time(AON-RTC)", "@Dump-reason", "build time =>",
            "user except handler begin",
        )
    )
    incomplete = ["missing required marker: {}".format(marker) for marker in required if not _contains(session, marker)]
    if not any(REGISTERS_RE.search(line.text) for line in session):
        incomplete.append("missing required marker: CPU registers")
    missing_handler_end = False
    if actual_target == "AP":
        if has_ap_markers:
            for marker in ("AP memory dump begin", "AP memory dump end"):
                if not _contains(session, marker):
                    incomplete.append("missing required marker: {}".format(marker))
        elif not _contains(session, "user except handler end"):
            missing_handler_end = True
        if core_match is None:
            if not cp_takeover_ap_hang:
                incomplete.append("missing required marker: SMP-Core-id")
    elif ap_observer_cp_hang:
        for marker in (
            "observer=AP target=CP reason=cp_heartbeat_timeout",
            "CP memory dump begin", "CP memory dump end",
        ):
            if not _contains(session, marker):
                incomplete.append("missing required heartbeat marker: {}".format(marker))
    elif not _contains(session, "user except handler end"):
        missing_handler_end = True
    if cp_takeover_ap_hang:
        if not heartbeat_timeout_seen:
            incomplete.append("missing required heartbeat timeout evidence")
        for marker in ("AP memory dump begin", "AP memory dump end"):
            if not _contains(session, marker):
                incomplete.append("missing required heartbeat marker: {}".format(marker))
        if not _contains(session, "user except handler end"):
            missing_handler_end = True
    ap_begin = next(
        (index for index, line in enumerate(session)
         if "ap memory dump begin" in line.text.lower()),
        None,
    )
    ap_end = (
        next(
            (index for index in range(ap_begin, len(session))
             if "ap memory dump end" in session[index].text.lower()),
            None,
        )
        if ap_begin is not None else None
    )
    # Local-owner and nested AP-memory scopes may intentionally snapshot the
    # same physical ranges. Validate continuity/overlap within each scope,
    # regardless of how the dump was classified, so cross-scope duplicates are
    # not reported as corruption.
    if ap_begin is not None and ap_end is not None:
        local_regions, local_incomplete, local_corrupt, local_mismatch = (
            _parse_regions(session[:ap_begin])
        )
        ap_regions, ap_incomplete, ap_corrupt, ap_mismatch = (
            _parse_regions(session[ap_begin:ap_end + 1])
        )
        regions = local_regions + ap_regions
        region_incomplete = local_incomplete + ap_incomplete
        corrupt = local_corrupt + ap_corrupt
        mismatch = local_mismatch + ap_mismatch
        skips = (
            _parse_skips(session[:ap_begin])
            + _parse_skips(session[ap_begin:ap_end + 1])
        )
        if cp_takeover_ap_hang and not local_regions:
            region_incomplete.append("no CP owner dump regions")
        if cp_takeover_ap_hang and not ap_regions:
            region_incomplete.append("no AP memory dump regions")
    else:
        regions, region_incomplete, corrupt, mismatch = _parse_regions(session)
        skips = _parse_skips(session)
    incomplete.extend(region_incomplete)
    if not regions:
        incomplete.append("no dump regions")
    if missing_handler_end:
        if regions and not region_incomplete and not corrupt:
            warnings.append(
                "user except handler end not observed after complete memory dump"
            )
        else:
            incomplete.append("missing required marker: user except handler end")
    build_version = _extract_build(session)
    mismatch.extend(_manifest_errors(
        regions, skips, golden_manifest, actual_target, build_version, power_state,
    ))
    reason_error = _reason_mismatch(expected_mode or requested_mode, fault_reason)
    mode_reason_match: Optional[bool] = None
    if (expected_mode or requested_mode) and fault_reason:
        mode_reason_match = reason_error is None
    if reason_error:
        warnings.append(reason_error)
    end_marker = (
        "CP memory dump end" if ap_observer_cp_hang
        else ("user except handler end" if cp_takeover_ap_hang
              else ("AP memory dump end" if has_ap_markers
                    else "user except handler end"))
    )
    end_line = next(
        (line.number for line in reversed(session) if end_marker.lower() in line.text.lower()),
        None,
    )
    boot_pass = _matches_any(rx, boot_expect, end_line or 0)
    recovery_pass = _matches_any(rx, recovery_expect, end_line or 0)
    # Empty recovery expectations mean no business recovery was requested.
    if not recovery_expect:
        recovery_pass = boot_pass
    if not boot_pass:
        warnings.append("boot expectation not observed")
    if not recovery_pass:
        warnings.append("recovery expectation not observed")
    dump_pass = bool(
        dump_present and not (host_errors or incomplete or corrupt or mismatch)
    )
    prior_dump_incomplete = False
    if (
        len(sessions) > 1
        and not scenario
        and not _contains(
            sessions[-2],
            "AP memory dump end"
            if _contains(sessions[-2], "AP memory dump begin")
            else "user except handler end",
        )
    ):
        prior_regions, prior_incomplete, prior_corrupt, _ = _parse_regions(
            sessions[-2]
        )
        prior_dump_incomplete = bool(
            not prior_regions or prior_incomplete or prior_corrupt
        )
        if not prior_dump_incomplete:
            warnings.append(
                "earlier dump omitted its final marker after complete memory dump"
            )
    if host_errors:
        result = "HOST_ERROR"
    elif not dump_present:
        result = "NOT_TRIGGERED"
    elif incomplete or prior_dump_incomplete:
        result = "DUMP_INCOMPLETE"
    elif corrupt:
        result = "DATA_CORRUPT"
    elif mismatch:
        result = "REGION_MISMATCH"
    else:
        result = "PASS"
    errors.extend(incomplete + corrupt + mismatch)
    return AnalysisResult(
        result=result, dump_pass=dump_pass, boot_pass=boot_pass,
        recovery_pass=recovery_pass, target=actual_target, core=actual_core,
        case_id=case_id, mode=mode, fault_reason=fault_reason,
        session_start_line=session[0].number, session_end_line=end_line,
        crc_lines=sum(item.crc_lines for item in regions),
        crc_failures=sum(item.crc_failures for item in regions),
        regions=len(regions),
        decoded_bytes=sum(item.decoded_bytes for item in regions),
        build_version=build_version, power_state=power_state, errors=errors,
        region_details=regions, skip_details=skips, sessions_found=len(sessions),
        dump_present=dump_present, expected_target=expected_target,
        expected_core=expected_core, expected_mode=expected_mode,
        requested_target=requested_target, requested_core=requested_core,
        requested_mode=requested_mode, actual_target=actual_target,
        actual_core=actual_core, actual_fault_reason=fault_reason,
        target_match=target_match, mode_reason_match=mode_reason_match,
        trigger_reject_reason=reject_reason, warnings=warnings,
        scenario=scenario, actual_dumper=actual_dumper,
        heartbeat_timeout_seen=heartbeat_timeout_seen,
        heartbeat_takeover_pass=(
            dump_pass
            and not any("heartbeat marker" in item for item in incomplete)
            if (cp_takeover_ap_hang or ap_observer_cp_hang) else None
        ),
        complete_dumps=complete_dumps,
    )


def analyze_file(path: Path, **kwargs: Any) -> AnalysisResult:
    return analyze_text(path.read_text(encoding="utf-8", errors="replace"), **kwargs)


def load_manifest(path: Optional[Path]) -> Optional[Dict[str, Any]]:
    if path is None:
        return None
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict) or not isinstance(value.get("regions"), list):
        raise ValueError("golden manifest must be an object containing a regions array")
    return value


def generate_manifest(results: Iterable[AnalysisResult]) -> Dict[str, Any]:
    entries: List[Dict[str, Any]] = []
    for result in results:
        counts: Dict[str, int] = {}
        for region in result.region_details:
            index = counts.get(region.name, 0)
            counts[region.name] = index + 1
            entries.append({
                "build_version": result.build_version, "target": result.target,
                "power_state": result.power_state, "region_name": region.name,
                "start": region.start, "end": region.end, "occurrence_index": index,
            })
    return {"schema_version": 1, "regions": entries}


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--target", choices=("AP", "CP"))
    parser.add_argument("--core", type=int, choices=(0, 1))
    parser.add_argument("--mode")
    parser.add_argument("--boot-expect", action="append", default=[])
    parser.add_argument("--recovery-expect", action="append", default=[])
    parser.add_argument("--golden", type=Path)
    parser.add_argument("--generate-golden", type=Path)
    parser.add_argument("--power-state", default="default")
    parser.add_argument("--output", type=Path, help="write JSON here instead of stdout")
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = _parser().parse_args(argv)
    try:
        manifest = load_manifest(args.golden)
        result = analyze_file(
            args.log, expected_target=args.target, expected_core=args.core,
            expected_mode=args.mode,
            boot_expect=args.boot_expect, recovery_expect=args.recovery_expect,
            golden_manifest=manifest, power_state=args.power_state,
        )
        payload = json.dumps(result.to_dict(), indent=2, ensure_ascii=False)
        if args.output:
            args.output.write_text(payload + "\n", encoding="utf-8")
        else:
            print(payload)
        if args.generate_golden:
            args.generate_golden.write_text(
                json.dumps(generate_manifest([result]), indent=2) + "\n", encoding="utf-8"
            )
        return 0 if result.result == "PASS" else 1
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(json.dumps({"result": "HOST_ERROR", "errors": [str(exc)]}), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
