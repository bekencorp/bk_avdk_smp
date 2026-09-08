#!/usr/bin/env python3
"""
BK7258 secureboot_xip OTA test harness.

Design:
  1) cases.json     — single source of truth (id / priority / steps / expect)
  2) results.json   — pass/fail/skip notes filled while testing
  3) this CLI       — list / plan / show / record / report / check-pack / mutate

Typical flow:
  python3 ota_xip_test.py plan --priority P0
  python3 ota_xip_test.py show H-01
  python3 ota_xip_test.py check-pack --build-dir <.../bk7258/_build>
  python3 ota_xip_test.py mutate corrupt-header -i ota.bin -o ota_bad.bin
  # ... run on board, watch BL2/AP logs ...
  python3 ota_xip_test.py record H-01 pass --note "slot B confirmed"
  python3 ota_xip_test.py report
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional

HERE = Path(__file__).resolve().parent
CASES_PATH = HERE / "cases.json"
RESULTS_PATH = HERE / "results.json"

OTA_GLOBAL_HDR = 32
OTA_IMG_HDR = 32


def load_cases() -> Dict[str, Any]:
    with CASES_PATH.open(encoding="utf-8") as f:
        return json.load(f)


def load_results() -> Dict[str, Any]:
    if not RESULTS_PATH.exists():
        return {"updated": None, "results": {}}
    with RESULTS_PATH.open(encoding="utf-8") as f:
        return json.load(f)


def save_results(data: Dict[str, Any]) -> None:
    data["updated"] = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    with RESULTS_PATH.open("w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
        f.write("\n")


def filter_cases(
    cases: List[Dict[str, Any]],
    priority: Optional[str],
    category: Optional[str],
    pending_only: bool,
    results: Dict[str, Any],
) -> List[Dict[str, Any]]:
    out = []
    for c in cases:
        if priority and c["priority"] != priority:
            continue
        if category and c["category"] != category:
            continue
        st = results.get("results", {}).get(c["id"], {}).get("status")
        if pending_only and st in ("pass", "fail", "skip"):
            continue
        out.append(c)
    return out


def cmd_list(args: argparse.Namespace) -> int:
    data = load_cases()
    results = load_results()
    rows = filter_cases(
        data["cases"], args.priority, args.category, args.pending, results
    )
    for c in rows:
        st = results.get("results", {}).get(c["id"], {}).get("status", "-")
        print(f"{c['id']:6} {c['priority']:3} {c['category']:8} [{st:4}] {c['title']}")
    print(f"\n{len(rows)} case(s)")
    return 0


def cmd_show(args: argparse.Namespace) -> int:
    data = load_cases()
    results = load_results()
    case = next((c for c in data["cases"] if c["id"] == args.case_id), None)
    if not case:
        print(f"unknown case: {args.case_id}", file=sys.stderr)
        return 1
    res = results.get("results", {}).get(args.case_id, {})
    print(f"ID       : {case['id']}")
    print(f"Priority : {case['priority']}")
    print(f"Category : {case['category']}")
    print(f"Auto     : {case.get('auto', 'manual')}")
    print(f"Title    : {case['title']}")
    print(f"Setup    : {case['setup']}")
    print(f"Steps    : {case['steps']}")
    print(f"Expect   : {case['expect']}")
    if case.get("log_patterns"):
        print(f"Log peek : {', '.join(case['log_patterns'])}")
    print(f"Result   : {res.get('status', 'pending')}")
    if res.get("note"):
        print(f"Note     : {res['note']}")
    if res.get("when"):
        print(f"When     : {res['when']}")
    return 0


def cmd_plan(args: argparse.Namespace) -> int:
    data = load_cases()
    results = load_results()
    print("=== Suggested execution order ===\n")
    for phase in data.get("phases", []):
        print(f"## {phase['name']}")
        for cid in phase["ids"]:
            case = next((c for c in data["cases"] if c["id"] == cid), None)
            if not case:
                continue
            if args.priority and case["priority"] != args.priority:
                continue
            st = results.get("results", {}).get(cid, {}).get("status", "pending")
            mark = {"pass": "OK", "fail": "NG", "skip": "--"}.get(st, "  ")
            print(f"  [{mark}] {cid}  {case['title']}")
        print()

    pending = filter_cases(
        data["cases"], args.priority or "P0", None, True, results
    )
    print(f"## Remaining P0 pending: {len(pending) if (args.priority or 'P0') == 'P0' else 'n/a'}")
    if not args.priority or args.priority == "P0":
        for c in filter_cases(data["cases"], "P0", None, True, results):
            print(f"  - {c['id']} {c['title']}")
    return 0


def cmd_record(args: argparse.Namespace) -> int:
    data = load_cases()
    if not any(c["id"] == args.case_id for c in data["cases"]):
        print(f"unknown case: {args.case_id}", file=sys.stderr)
        return 1
    results = load_results()
    results.setdefault("results", {})[args.case_id] = {
        "status": args.status,
        "note": args.note or "",
        "when": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    }
    save_results(results)
    print(f"recorded {args.case_id} = {args.status}")
    return 0


def cmd_report(args: argparse.Namespace) -> int:
    data = load_cases()
    results = load_results()
    tallies = {"pass": 0, "fail": 0, "skip": 0, "pending": 0}
    by_pri: Dict[str, Dict[str, int]] = {}
    for c in data["cases"]:
        st = results.get("results", {}).get(c["id"], {}).get("status", "pending")
        tallies[st] = tallies.get(st, 0) + 1
        by_pri.setdefault(c["priority"], {"pass": 0, "fail": 0, "skip": 0, "pending": 0})
        by_pri[c["priority"]][st] = by_pri[c["priority"]].get(st, 0) + 1

    total = len(data["cases"])
    print(f"OTA XIP test report  (updated {results.get('updated')})")
    print(f"total={total}  pass={tallies['pass']}  fail={tallies['fail']}  "
          f"skip={tallies['skip']}  pending={tallies['pending']}")
    for pri in ("P0", "P1", "P2"):
        if pri not in by_pri:
            continue
        b = by_pri[pri]
        print(f"  {pri}: pass={b['pass']} fail={b['fail']} skip={b['skip']} pending={b['pending']}")

    fails = [
        c for c in data["cases"]
        if results.get("results", {}).get(c["id"], {}).get("status") == "fail"
    ]
    if fails:
        print("\nFAIL:")
        for c in fails:
            note = results["results"][c["id"]].get("note", "")
            print(f"  {c['id']} {c['title']}  {note}")

    if args.markdown:
        md = HERE / "results_report.md"
        lines = [
            "# OTA XIP test report",
            "",
            f"- updated: {results.get('updated')}",
            f"- pass/fail/skip/pending: "
            f"{tallies['pass']}/{tallies['fail']}/{tallies['skip']}/{tallies['pending']}",
            "",
            "| ID | Pri | Cat | Status | Title | Note |",
            "|----|-----|-----|--------|-------|------|",
        ]
        for c in data["cases"]:
            r = results.get("results", {}).get(c["id"], {})
            lines.append(
                f"| {c['id']} | {c['priority']} | {c['category']} | "
                f"{r.get('status', 'pending')} | {c['title']} | {r.get('note', '')} |"
            )
        md.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"\nwrote {md}")
    return 0 if tallies["fail"] == 0 else 2


def _resolve_build_dir(build_dir: Path) -> Path:
    candidates = [
        build_dir,
        build_dir / "_build",
        build_dir / "bk7258" / "_build",
        build_dir / "bk7258" / "secureboot_xip" / "bk7258" / "_build",
    ]
    for c in candidates:
        if (c / "ota.bin").exists() or (c / "install" / "ota.bin").exists():
            return c
    return build_dir


def cmd_check_pack(args: argparse.Namespace) -> int:
    """Automated checks for Pack category (P-01 style)."""
    bdir = _resolve_build_dir(Path(args.build_dir).resolve())
    aes = args.aes  # none | fixed
    errors: List[str] = []
    infos: List[str] = []

    def find(*names: str) -> Optional[Path]:
        for n in names:
            for base in (bdir, bdir / "install"):
                p = base / n
                if p.exists():
                    return p
        return None

    ota = find("ota.bin")
    ota_crc = find("ota_crc.bin")
    ota_aes = find("ota_aes.bin")
    ota_aes_crc = find("ota_aes_crc.bin")
    all_app = find("all-app.bin")
    ota_h = None
    for p in bdir.rglob("_ota.h"):
        ota_h = p
        break

    if not ota:
        errors.append(f"missing ota.bin under {bdir}")
    else:
        sz = ota.stat().st_size
        infos.append(f"ota.bin size={sz}")
        if sz <= OTA_GLOBAL_HDR + OTA_IMG_HDR:
            errors.append("ota.bin too small (no payload?)")
        # content-sized: should NOT be multi-MB full-slot pad for typical app
        if args.max_ota_mb and sz > args.max_ota_mb * 1024 * 1024:
            errors.append(
                f"ota.bin {sz} > {args.max_ota_mb}MB — possible leftover --pad"
            )

    if aes == "none":
        if ota_aes:
            errors.append(f"unexpected {ota_aes.name} when AES=NONE")
        if not ota_crc:
            errors.append("missing ota_crc.bin (NONE path should CRC)")
        else:
            infos.append(f"ota_crc.bin size={ota_crc.stat().st_size}")
        if ota_h:
            txt = ota_h.read_text(encoding="utf-8", errors="ignore")
            if "CONFIG_OTA_ENCRYPTED" in txt and "#define CONFIG_OTA_ENCRYPTED" in txt:
                # crude: last define wins in our generator (single define)
                for line in txt.splitlines():
                    if line.startswith("#define") and "CONFIG_OTA_ENCRYPTED" in line:
                        if line.strip().endswith("1"):
                            errors.append("_ota.h CONFIG_OTA_ENCRYPTED=1 but AES=NONE")
                        else:
                            infos.append("_ota.h CONFIG_OTA_ENCRYPTED=0 OK")
    elif aes == "fixed":
        if not ota_aes:
            errors.append("missing ota_aes.bin when AES=FIXED")
        if not ota_aes_crc:
            errors.append("missing ota_aes_crc.bin when AES=FIXED")
        if ota_h:
            txt = ota_h.read_text(encoding="utf-8", errors="ignore")
            for line in txt.splitlines():
                if line.startswith("#define") and "CONFIG_OTA_ENCRYPTED" in line:
                    if not line.strip().endswith("1"):
                        errors.append("_ota.h CONFIG_OTA_ENCRYPTED!=1 but AES=FIXED")
                    else:
                        infos.append("_ota.h CONFIG_OTA_ENCRYPTED=1 OK")

    if all_app:
        infos.append(f"all-app.bin size={all_app.stat().st_size}")

    for i in infos:
        print(f"[OK]  {i}")
    for e in errors:
        print(f"[NG]  {e}")

    if errors:
        print(f"\ncheck-pack FAILED ({len(errors)} error(s))")
        return 1
    print("\ncheck-pack PASSED")
    if args.record:
        results = load_results()
        cid = "P-01" if aes == "none" else "P-02"
        results.setdefault("results", {})[cid] = {
            "status": "pass",
            "note": f"check-pack aes={aes} build={bdir}",
            "when": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        }
        save_results(results)
        print(f"recorded {cid}=pass")
    return 0


def cmd_mutate(args: argparse.Namespace) -> int:
    src = Path(args.infile)
    dst = Path(args.outfile)
    if not src.exists():
        print(f"missing input: {src}", file=sys.stderr)
        return 1
    data = bytearray(src.read_bytes())

    if args.action == "corrupt-header":
        # Smash first CRC data unit inside image payload (after OTA hdr if present)
        off = args.offset
        if off < 0:
            off = OTA_GLOBAL_HDR + OTA_IMG_HDR  # default: start of XIP payload in ota.bin
        n = min(args.bytes, len(data) - off)
        if n <= 0:
            print("offset past EOF", file=sys.stderr)
            return 1
        for i in range(n):
            data[off + i] ^= 0xFF
        print(f"corrupted {n} byte(s) at offset {off}")
    elif args.action == "truncate":
        keep = args.keep
        if keep <= 0 or keep >= len(data):
            print("keep must be in (0, filesize)", file=sys.stderr)
            return 1
        data = data[:keep]
        print(f"truncated to {keep} bytes")
    elif args.action == "zero-fill":
        off = args.offset if args.offset >= 0 else 0
        n = min(args.bytes, len(data) - off)
        data[off : off + n] = b"\x00" * n
        print(f"zero-filled {n} byte(s) at {off}")
    else:
        print(f"unknown action {args.action}", file=sys.stderr)
        return 1

    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data)
    print(f"wrote {dst} ({len(data)} bytes)")
    print("Next: flash/HTTP this image, reboot, check BL2 SoftCRC / EBADIMAGE / rollback logs.")
    return 0


def cmd_worksheet(args: argparse.Namespace) -> int:
    """Print a copy-paste worksheet for one case (manual board steps)."""
    data = load_cases()
    case = next((c for c in data["cases"] if c["id"] == args.case_id), None)
    if not case:
        print(f"unknown case: {args.case_id}", file=sys.stderr)
        return 1
    print("=" * 60)
    print(f"CASE {case['id']}  [{case['priority']}/{case['category']}]")
    print(case["title"])
    print("=" * 60)
    print("\n[0] Prepare artifacts (if recipe exists)")
    print(f"    python3 {Path(__file__).name} prepare {case['id']} "
          f"[--build-dir <_build>]")
    print("\n[1] Setup")
    print(f"    {case['setup']}")
    print("\n[2] Steps")
    print(f"    {case['steps']}")
    print("\n[3] Expect")
    print(f"    {case['expect']}")
    if case.get("log_patterns"):
        print("\n[4] Log anchors to capture")
        for p in case["log_patterns"]:
            print(f"    - {p}")
    print("\n[5] After test")
    print(f"    python3 {Path(__file__).name} record {case['id']} pass|fail --note '...'")
    print()
    return 0


def cmd_prepare(args: argparse.Namespace) -> int:
    from recipes import P0_PREPARE_IDS, RECIPES, prepare_case, prepare_many

    build_dir = Path(args.build_dir).resolve() if args.build_dir else None
    if args.case_id == "p0":
        prepare_many(P0_PREPARE_IDS, build_dir=build_dir)
        print(f"\nPrepared P0 set under {HERE / 'artifacts'}")
        return 0
    if args.case_id == "list":
        print("Recipes:", ", ".join(sorted(RECIPES)))
        print("P0 batch:", ", ".join(P0_PREPARE_IDS))
        return 0
    prepare_case(args.case_id, build_dir=build_dir)
    return 0


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="BK7258 secureboot_xip OTA test harness",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    sub = p.add_subparsers(dest="cmd", required=True)

    sp = sub.add_parser("list", help="List cases")
    sp.add_argument("--priority", choices=["P0", "P1", "P2"])
    sp.add_argument("--category", choices=[
        "Pack", "Happy", "Trial", "Sticky", "SoftCRC", "Security", "Power", "Negative",
    ])
    sp.add_argument("--pending", action="store_true", help="Only unfinished")
    sp.set_defaults(func=cmd_list)

    sp = sub.add_parser("show", help="Show one case detail")
    sp.add_argument("case_id")
    sp.set_defaults(func=cmd_show)

    sp = sub.add_parser("worksheet", help="Print manual worksheet for one case")
    sp.add_argument("case_id")
    sp.set_defaults(func=cmd_worksheet)

    sp = sub.add_parser(
        "prepare",
        help="Generate boot_param/mutate artifacts for a case (or 'p0' / 'list')",
    )
    sp.add_argument("case_id", help="case id, or 'p0' for batch, or 'list'")
    sp.add_argument(
        "--build-dir",
        default=None,
        help="pack _build dir (needed for P-01/C-01/N-01/E-03)",
    )
    sp.set_defaults(func=cmd_prepare)

    sp = sub.add_parser("plan", help="Print phased execution plan")
    sp.add_argument("--priority", choices=["P0", "P1", "P2"])
    sp.set_defaults(func=cmd_plan)

    sp = sub.add_parser("record", help="Record pass/fail/skip")
    sp.add_argument("case_id")
    sp.add_argument("status", choices=["pass", "fail", "skip"])
    sp.add_argument("--note", default="")
    sp.set_defaults(func=cmd_record)

    sp = sub.add_parser("report", help="Summarize results")
    sp.add_argument("--markdown", action="store_true")
    sp.set_defaults(func=cmd_report)

    sp = sub.add_parser("check-pack", help="Auto-check pack outputs (P-01/P-02)")
    sp.add_argument(
        "--build-dir",
        required=True,
        help="e.g. build/bk7258/secureboot_xip/bk7258/_build",
    )
    sp.add_argument("--aes", choices=["none", "fixed"], default="none")
    sp.add_argument("--max-ota-mb", type=float, default=4.0,
                    help="Fail if ota.bin larger (detect leftover pad)")
    sp.add_argument("--record", action="store_true", help="Record P-01/P-02 pass on success")
    sp.set_defaults(func=cmd_check_pack)

    sp = sub.add_parser("mutate", help="Build bad OTA images for negative cases")
    sp.add_argument("action", choices=["corrupt-header", "truncate", "zero-fill"])
    sp.add_argument("-i", "--infile", required=True)
    sp.add_argument("-o", "--outfile", required=True)
    sp.add_argument("--offset", type=int, default=-1)
    sp.add_argument("--bytes", type=int, default=34)
    sp.add_argument("--keep", type=int, default=1024, help="for truncate")
    sp.set_defaults(func=cmd_mutate)

    return p


def main(argv: Optional[List[str]] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
