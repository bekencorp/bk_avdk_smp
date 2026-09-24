#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def write_markdown(path: Path, report: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(report + "\n", encoding="utf-8")


__all__ = ["write_markdown"]

