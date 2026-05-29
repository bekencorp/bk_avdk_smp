#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

from ap_dump_lib.dat_parser import parse_dat
from ap_dump_lib.elf_symbols import ElfSymbols
from ap_dump_lib.interrupt_analysis import summarize_interrupts
from ap_dump_lib.report import render_report
from ap_dump_lib.stack_analysis import analyze

DEBUG_LOG_PATH = Path("/home/gang.peng/bk_avdk_smp_dev_7259v2_version/.cursor/debug-7095db.log")
DEBUG_SESSION_ID = "7095db"
DEBUG_RUN_ID = "ap-heartbeat-20260516-night"


def _agent_debug_log(hypothesis_id: str, location: str, message: str, data: dict) -> None:
    try:
        payload = {
            "sessionId": DEBUG_SESSION_ID,
            "runId": DEBUG_RUN_ID,
            "hypothesisId": hypothesis_id,
            "location": location,
            "message": message,
            "data": data,
            "timestamp": int(time.time() * 1000),
        }
        with DEBUG_LOG_PATH.open("a", encoding="utf-8") as fp:
            fp.write(json.dumps(payload, ensure_ascii=False, sort_keys=True) + "\n")
    except Exception:
        pass


def discover_elf(cwd: Path) -> Path:
    direct = cwd / "app.elf"
    if direct.is_file():
        return direct
    matches = sorted(cwd.glob("**/app.elf"), key=lambda p: p.stat().st_mtime, reverse=True)
    if len(matches) == 1:
        return matches[0]
    if not matches:
        raise RuntimeError("Could not find app.elf in the current directory. Use --elf PATH.")
    raise RuntimeError("Found multiple app.elf files. Use --elf PATH:\n" + "\n".join(str(p) for p in matches[:20]))


def discover_dat(cwd: Path) -> Path:
    matches = sorted(list(cwd.glob("*.DAT")) + list(cwd.glob("*.dat")), key=lambda p: p.stat().st_mtime, reverse=True)
    if not matches:
        raise RuntimeError("Could not find a .DAT log in the current directory. Use --dat PATH.")
    return matches[0]


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Analyze BK7259 AP app.elf plus UART/coredump .DAT and emit Markdown."
    )
    parser.add_argument("--elf", type=Path, help="Path to AP app.elf (default: ./app.elf or unique nested app.elf).")
    parser.add_argument("--dat", type=Path, help="Path to .DAT log (default: newest *.DAT/*.dat in cwd).")
    parser.add_argument(
        "--session-index",
        type=int,
        help="Analyze a specific zero-based dump session. Default: last user except handler session.",
    )
    parser.add_argument("-o", "--output", type=Path, help="Write Markdown report to this path instead of stdout.")
    parser.add_argument("--list-sessions", action="store_true", help="List parsed sessions and exit.")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    cwd = Path.cwd()
    elf_path = (args.elf or discover_elf(cwd)).resolve()
    dat_path = (args.dat or discover_dat(cwd)).resolve()

    if not elf_path.is_file():
        raise FileNotFoundError(str(elf_path))
    if not dat_path.is_file():
        raise FileNotFoundError(str(dat_path))

    sessions, session = parse_dat(dat_path, args.session_index)
    if args.list_sessions:
        for item in sessions:
            print(
                f"[{item.index}] lines={item.start_line}-{item.end_line} "
                f"build={item.build_time or '-'} reason={item.dump_reason or '-'} "
                f"regions={len(item.stack_regions)} tracebacks={len(item.tracebacks)}"
            )
        return 0

    symbols = ElfSymbols(elf_path)
    analysis = analyze(session, symbols)
    interrupts = summarize_interrupts(session, symbols)
    _agent_debug_log(
        "H1-H4",
        "tools/dump_tools/analyze_ap_dump.py:76",
        "offline AP dump analysis summary",
        {
            "elf": str(elf_path),
            "dat": str(dat_path),
            "session_index": session.index,
            "session_lines": [session.start_line, session.end_line],
            "dump_reason": session.dump_reason,
            "stack_regions": len(session.stack_regions),
            "tracebacks": len(session.tracebacks),
            "likely_stuck": analysis.likely_stuck,
            "current_cores": [
                {
                    "core": core.core,
                    "tcb": core.tcb_address,
                    "task": core.task.name if core.task else None,
                    "source": core.source,
                    "note": core.note,
                }
                for core in analysis.current_cores
            ],
            "last_interrupts": [
                {
                    "core": summary.core,
                    "total": summary.total,
                    "depth": summary.depth,
                    "source": summary.source,
                    "last_irq": summary.records[-1].irq if summary.records else None,
                    "last_seq": summary.records[-1].seq if summary.records else None,
                    "last_enter": summary.records[-1].enter if summary.records else None,
                    "last_exit": summary.records[-1].exit if summary.records else None,
                    "incomplete": [{"irq": r.irq, "seq": r.seq, "enter": r.enter, "exit": r.exit} for r in summary.incomplete],
                }
                for summary in interrupts
            ],
        },
    )
    report = render_report(elf_path, dat_path, len(sessions), session, symbols, analysis, interrupts)

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report, encoding="utf-8")
    else:
        print(report)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(2)

