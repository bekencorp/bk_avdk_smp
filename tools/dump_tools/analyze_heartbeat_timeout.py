#!/usr/bin/env python3
"""Heartbeat-timeout AP dump analyzer.

Inputs:
    <log>   serial log captured at CP `mb_ipc_task:292` heartbeat-timeout assert
    <elf>   matching AP app.elf

Outputs (in --output-dir, default <log_stem>_analysis/):
    extract.txt    FreeRTOS globals + tasks + task/IRQ recorders
    msp_core0.txt  per-core MSP stack walk
    msp_core1.txt
    peri.txt       peripheral register snapshot
    report.md      auto-generated review report with smoking-gun summary

The .txt artefacts are intended for further AI inspection (each is a
human-readable plain-text view of one analysis dimension). The .md report is
the consolidated review summary.

Examples:
    # All artefacts into ./<log_stem>_analysis/
    analyze_heartbeat_timeout.py /tmp/dump.log /tmp/app.elf

    # Just the report
    analyze_heartbeat_timeout.py /tmp/dump.log /tmp/app.elf \
        --report-only -o /tmp/dump_report.md

    # Reuse an existing output directory
    analyze_heartbeat_timeout.py /tmp/dump.log /tmp/app.elf -o ./out

Exit codes:
    0  success
    1  smoking gun detected (an IRQ ring entry with done=0)
    2  fatal error (bad input, missing tooling)
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Allow running both as a script and as `python -m`.
THIS_DIR = Path(__file__).resolve().parent
if str(THIS_DIR) not in sys.path:
    sys.path.insert(0, str(THIS_DIR))

from heartbeat_timeout import core, decoders, extract, msp_walk, peri_regs, report  # noqa: E402
from heartbeat_timeout.symbols import load_symbols  # noqa: E402


def _parse_args(argv: list[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Analyze BK7259 AP heartbeat-timeout dump (log + app.elf).",
    )
    p.add_argument("log", type=Path, help="Serial log captured during heartbeat-timeout dump.")
    p.add_argument("elf", type=Path, help="AP app.elf matching the dump build.")
    p.add_argument(
        "-o", "--output",
        type=Path,
        help=(
            "Output target. By default `<log_stem>_analysis/` (a directory). "
            "With --report-only this is treated as the .md path. "
            "With --no-report this is treated as a directory."
        ),
    )
    p.add_argument(
        "--report-only", action="store_true",
        help="Skip extract/msp/peri text files and emit only the .md report.",
    )
    p.add_argument(
        "--no-report", action="store_true",
        help="Skip report.md, only emit extract/msp/peri text files.",
    )
    p.add_argument(
        "--stdout-report", action="store_true",
        help="Print the report to stdout in addition to (or instead of) writing it.",
    )
    return p.parse_args(argv)


def _write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    args = _parse_args(sys.argv[1:] if argv is None else argv)

    log = args.log.resolve()
    elf = args.elf.resolve()
    if not log.is_file():
        print(f"error: log not found: {log}", file=sys.stderr)
        return 2
    if not elf.is_file():
        print(f"error: elf not found: {elf}", file=sys.stderr)
        return 2

    sym = load_symbols(elf)

    regions = core.parse_dump_regions(log)
    memmap = core.build_memory_map(regions)

    extract_result = extract.build_extract(memmap, sym, elf)
    msp0 = msp_walk.walk_core(memmap, elf, sym, 0)
    msp1 = msp_walk.walk_core(memmap, elf, sym, 1)
    peri_report = peri_regs.decode_peripherals(memmap, regions)
    decoder_summary = decoders.summarise_for_report(elf, peri_report, memmap)
    decoder_text = decoders.render_extras(elf, peri_report, memmap)
    header = report._scan_header(log)
    report_md = report.render_report(
        log, elf, header, extract_result, msp0, msp1, peri_report,
        decoder_summary=decoder_summary,
    )

    # Resolve output targets.
    default_dir = log.parent / f"{log.stem}_analysis"
    out_dir: Path | None = None
    report_path: Path | None = None
    if args.report_only:
        report_path = args.output or default_dir / "report.md"
    elif args.no_report:
        out_dir = args.output or default_dir
    else:
        out_dir = args.output or default_dir
        report_path = out_dir / "report.md"

    if out_dir is not None:
        out_dir.mkdir(parents=True, exist_ok=True)
        _write(out_dir / "extract.txt", extract.render_extract_text(log, regions, memmap, extract_result))
        _write(out_dir / "msp_core0.txt", msp_walk.render_msp_text(msp0))
        _write(out_dir / "msp_core1.txt", msp_walk.render_msp_text(msp1))
        _write(
            out_dir / "peri.txt",
            peri_regs.render_peri_text(peri_report) + decoder_text,
        )

    if report_path is not None:
        _write(report_path, report_md)
    if args.stdout_report or (args.report_only and args.output is None):
        sys.stdout.write(report_md)

    # Status banner to stderr so it doesn't poison the report stdout mode.
    written: list[str] = []
    if out_dir is not None:
        written.extend([
            str(out_dir / "extract.txt"),
            str(out_dir / "msp_core0.txt"),
            str(out_dir / "msp_core1.txt"),
            str(out_dir / "peri.txt"),
        ])
    if report_path is not None:
        written.append(str(report_path))
    if written:
        print("wrote:", file=sys.stderr)
        for path in written:
            print(f"  {path}", file=sys.stderr)

    smoking = any(
        s and s.stuck_record is not None
        for s in (extract_result.irq_summary_core0, extract_result.irq_summary_core1)
    )
    return 1 if smoking else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        raise SystemExit(130)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(2)
