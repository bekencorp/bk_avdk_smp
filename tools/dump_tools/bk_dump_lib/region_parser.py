#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass

from ap_dump_lib.dat_parser import DumpSession, StackRegion


@dataclass(frozen=True)
class DumpIntegrity:
    region_count: int
    decoded_bytes: int
    missing_notes: list[str]


def summarize_integrity(session: DumpSession) -> DumpIntegrity:
    decoded = sum(len(region.data) for region in session.stack_regions)
    notes: list[str] = []
    if not session.stack_regions:
        notes.append("no memory dump regions parsed")
    return DumpIntegrity(
        region_count=len(session.stack_regions),
        decoded_bytes=decoded,
        missing_notes=notes,
    )


__all__ = ["DumpIntegrity", "StackRegion", "summarize_integrity"]

