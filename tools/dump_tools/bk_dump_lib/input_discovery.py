#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


LOG_SUFFIXES = {".dat", ".log", ".txt"}


@dataclass(frozen=True)
class ToolPaths:
    sdk_root: Path
    dump_tools_dir: Path
    input_dir: Path
    build_dir: Path


def default_paths(script_path: Path) -> ToolPaths:
    dump_tools_dir = script_path.resolve().parent
    sdk_root = dump_tools_dir.parent.parent
    return ToolPaths(
        sdk_root=sdk_root,
        dump_tools_dir=dump_tools_dir,
        input_dir=dump_tools_dir / "input",
        build_dir=sdk_root / "build",
    )


def _newest(paths: list[Path]) -> Path | None:
    if not paths:
        return None
    return sorted(paths, key=lambda p: p.stat().st_mtime, reverse=True)[0]


def discover_log(paths: ToolPaths, explicit: Path | None = None) -> Path:
    if explicit is not None:
        log = explicit.expanduser().resolve()
        if not log.is_file():
            raise FileNotFoundError(f"log file not found: {log}")
        return log

    if not paths.input_dir.is_dir():
        raise RuntimeError(
            f"input directory not found: {paths.input_dir}\n"
            "Create it or pass --log explicitly."
        )

    candidates = [
        p for p in paths.input_dir.iterdir()
        if p.is_file() and p.suffix.lower() in LOG_SUFFIXES
    ]
    selected = _newest(candidates)
    if selected is None:
        raise RuntimeError(
            f"Could not find a dump log in {paths.input_dir}. "
            "Put a .DAT/.log/.txt file there or pass --log PATH."
        )
    return selected.resolve()


def _unique_or_raise(matches: list[Path], what: str, hint: str) -> Path:
    if len(matches) == 1:
        return matches[0].resolve()
    if not matches:
        raise RuntimeError(hint)
    shown = "\n".join(str(p) for p in matches[:20])
    more = "" if len(matches) <= 20 else f"\n... {len(matches) - 20} more"
    raise RuntimeError(
        f"Found multiple {what}. Pass an explicit path:\n{shown}{more}"
    )


def discover_elf(paths: ToolPaths, explicit: Path | None = None) -> Path:
    """Discover AP app.elf: input/app.elf, then build/**/bk7259_ap/app.elf."""
    if explicit is not None:
        elf = explicit.expanduser().resolve()
        if not elf.is_file():
            raise FileNotFoundError(f"ELF file not found: {elf}")
        return elf

    input_elf = paths.input_dir / "app.elf"
    if input_elf.is_file():
        return input_elf.resolve()

    if not paths.build_dir.is_dir():
        raise RuntimeError(
            f"Could not find app.elf in {paths.input_dir}, and build directory "
            f"does not exist: {paths.build_dir}. Pass --elf PATH."
        )

    ap_matches = sorted(paths.build_dir.glob("**/bk7259_ap/app.elf"))
    if ap_matches:
        return _unique_or_raise(
            ap_matches,
            "AP app.elf under build/**/bk7259_ap/",
            f"Could not find AP app.elf in {paths.input_dir} or {paths.build_dir}. Pass --elf PATH.",
        )

    matches = sorted(paths.build_dir.glob("**/app.elf"))
    return _unique_or_raise(
        matches,
        "app.elf under build",
        f"Could not find app.elf in {paths.input_dir} or {paths.build_dir}. Pass --elf PATH.",
    )


def discover_cp_elf(paths: ToolPaths, explicit: Path | None = None) -> Path:
    """Discover CP app.elf: same input/app.elf name, then build/**/bk7259/app.elf.

    AP/CP dumps are analyzed one side at a time, so input/ also uses app.elf
    (not a separate cp_app.elf name).
    """
    if explicit is not None:
        elf = explicit.expanduser().resolve()
        if not elf.is_file():
            raise FileNotFoundError(f"CP ELF file not found: {elf}")
        return elf

    input_elf = paths.input_dir / "app.elf"
    if input_elf.is_file():
        return input_elf.resolve()

    if not paths.build_dir.is_dir():
        raise RuntimeError(
            f"Could not find app.elf in {paths.input_dir}, and build directory "
            f"does not exist: {paths.build_dir}. Pass --elf / --cp-elf PATH."
        )

    cp_matches = [
        p for p in sorted(paths.build_dir.glob("**/bk7259/app.elf"))
        if "bk7259_ap" not in p.parts
    ]
    return _unique_or_raise(
        cp_matches,
        "CP app.elf under build/**/bk7259/",
        f"Could not find CP app.elf in {paths.input_dir} or {paths.build_dir}. "
        "Pass --elf PATH (put the matching app.elf under tools/dump_tools/input/).",
    )
