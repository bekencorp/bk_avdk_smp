#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

from ap_dump_lib.dat_parser import strip_log_prefix


SUPPORTED_MODES = ("auto", "ap-exception", "cp-exception", "heartbeat", "handoff")

_CPU_REGS_RE = re.compile(r"CPU(\d+)\s+Current regs", re.IGNORECASE)


def _fault_cores(text: str) -> set[int]:
    cores: set[int] = set()
    for line in text.splitlines():
        clean = strip_log_prefix(line)
        m = _CPU_REGS_RE.search(clean)
        if m:
            cores.add(int(m.group(1)))
    return cores


def detect_mode(log_path: Path) -> str:
    raw = log_path.read_text(encoding="utf-8", errors="ignore")
    text = raw.lower()
    cores = _fault_cores(raw)
    ap_cores = cores & {2, 3}
    cp_cores = cores & {0, 1}

    # Heartbeat: CP asserts on AP heartbeats and dumps AP memory. Prefer this over
    # generic "AP memory dump begin" which also appears inside AP exception dumps.
    if "heartbeat timeout" in text or "hb timeout" in text:
        return "heartbeat"

    # Explicit CP->AP trap handoff path.
    if "ipc_ap_trap_handle" in text and not ap_cores:
        return "handoff"

    # CPU register banners are the strongest side discriminator.
    if cp_cores and not ap_cores:
        return "cp-exception"
    if ap_cores:
        return "ap-exception"

    # Handoff without a clear Current regs banner.
    if "ap memory dump begin" in text and "user except handler begin" not in text:
        return "handoff"

    if (
        "user except handler begin" in text
        or "current regs:" in text
        or "@dump-reason:" in text
    ):
        return "ap-exception"
    if "mb_ipc_task" in text and "assert" in text:
        return "heartbeat"
    return "ap-exception"


def normalize_mode(mode: str, log_path: Path) -> str:
    if mode not in SUPPORTED_MODES:
        raise ValueError(f"unsupported mode: {mode}")
    if mode == "auto":
        return detect_mode(log_path)
    return mode
