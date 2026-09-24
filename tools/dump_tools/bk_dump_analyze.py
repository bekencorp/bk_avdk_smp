#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent
if str(THIS_DIR) not in sys.path:
    sys.path.insert(0, str(THIS_DIR))

from bk_dump_lib.analyzers import (  # noqa: E402
    analyze_ap_dump,
    analyze_cp_dump,
    analyze_heartbeat_dump,
    list_ap_sessions,
)
from bk_dump_lib.input_discovery import (  # noqa: E402
    default_paths,
    discover_cp_elf,
    discover_elf,
    discover_log,
)
from bk_dump_lib.mode import SUPPORTED_MODES, normalize_mode  # noqa: E402


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Analyze BK7259 dump log plus app.elf and print a diagnostic report.",
    )
    parser.add_argument("--log", type=Path, help="Path to dump log (.DAT/.log/.txt).")
    parser.add_argument("--elf", type=Path, help="Path to app.elf (AP or CP, matching the dump side).")
    parser.add_argument(
        "--cp-elf",
        type=Path,
        help="Optional alias of --elf for CP dumps; same file is still named app.elf.",
    )
    parser.add_argument(
        "--mode",
        choices=SUPPORTED_MODES,
        default="auto",
        help="Dump analysis mode. Default: auto.",
    )
    parser.add_argument(
        "--session-index",
        type=int,
        help="Analyze a specific zero-based dump session. Default: last session.",
    )
    parser.add_argument(
        "--list-sessions",
        action="store_true",
        help="List dump sessions and exit.",
    )
    parser.add_argument("-o", "--output", type=Path, help="Write Markdown report to this path.")
    parser.add_argument(
        "--artifacts",
        type=Path,
        help="Write first-stage artifacts into this directory.",
    )
    return parser.parse_args(argv)


def write_artifacts(out_dir: Path, console: str, markdown: str) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "console.txt").write_text(console + "\n", encoding="utf-8")
    (out_dir / "report.md").write_text(markdown + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    paths = default_paths(Path(__file__))

    log_path = discover_log(paths, args.log)
    mode = normalize_mode(args.mode, log_path)

    if args.list_sessions:
        if mode == "heartbeat":
            print("heartbeat mode does not support session listing; pass --mode ap-exception/cp-exception.")
            return 2
        print(list_ap_sessions(log_path))
        return 0

    if mode == "cp-exception":
        # Allow --elf as a convenient alias when user already points at the CP ELF.
        elf_path = discover_cp_elf(paths, args.cp_elf or args.elf)
        result = analyze_cp_dump(log_path, elf_path, args.session_index)
    elif mode in ("ap-exception", "handoff"):
        elf_path = discover_elf(paths, args.elf)
        result = analyze_ap_dump(log_path, elf_path, args.session_index)
    elif mode == "heartbeat":
        if args.session_index is not None:
            print("warning: --session-index is ignored in heartbeat mode", file=sys.stderr)
        elf_path = discover_elf(paths, args.elf)
        result = analyze_heartbeat_dump(log_path, elf_path)
    else:
        raise RuntimeError(f"unsupported resolved mode: {mode}")

    print(result.console)

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(result.markdown + "\n", encoding="utf-8")
        print(f"\nwrote Markdown report: {args.output}", file=sys.stderr)

    if args.artifacts:
        write_artifacts(args.artifacts, result.console, result.markdown)
        print(f"wrote artifacts: {args.artifacts}", file=sys.stderr)

    return result.exit_code


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        raise SystemExit(130)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(2)
