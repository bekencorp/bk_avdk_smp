"""Discover heartbeat-timeout-relevant symbol addresses from the AP ELF.

The original analysis scripts hard-coded every address from the build under
investigation. Once the symbol layout shifted (e.g. 14:30 -> 17:56 build on
0518), every analyzer file had to be hand-edited. This module uses
``arm-none-eabi-nm`` to look them up at runtime, keeping the tool stable
across builds.
"""
from __future__ import annotations

import re
import subprocess
from dataclasses import dataclass
from pathlib import Path


NM_TOOLS = ("arm-none-eabi-nm", "nm")

# Symbols required for full analysis. ``None`` defaults are kept so the
# extract step can degrade gracefully when a symbol is renamed/removed.
REQUIRED_SYMBOLS = (
    # Coredump / heartbeat state
    "s_bk_exception_magic",
    "s_core_id",
    "s_bk_assert_info",
    "s_hb_paused",
    # FreeRTOS scheduler state
    "uxCurrentNumberOfTasks",
    "xSchedulerRunning",
    "xTickCount",
    "pxCurrentTCBs",
    # Task switch recorder
    "s_task_cnt_core0",
    "s_task_cnt_core1",
    "s_task_recorder_core0",
    "s_task_recorder_core1",
    # Interrupt recorder
    "s_interrupt_core0_dump",
    "s_interrupt_core1_dump",
    # MSP regions
    "__StackTopCore0",
    "__StackLimitCore0",
    "__StackTopCore1",
    "__StackLimitCore1",
)


# nm -S output:  "<addr> <size> <type> <name>"  (size present for BSS/DATA)
# Fallback nm:   "<addr> <type> <name>"          (no size)
_NM_S_RE = re.compile(
    r"^(?P<addr>[0-9a-fA-F]+)\s+(?P<size>[0-9a-fA-F]+)\s+(?P<type>[A-Za-z?])\s+(?P<name>\S+)\s*$"
)
_NM_RE = re.compile(r"^(?P<addr>[0-9a-fA-F]+)\s+(?P<type>[A-Za-z?])\s+(?P<name>\S+)\s*$")


@dataclass
class SymbolTable:
    elf: Path
    addrs: dict[str, int]
    sizes: dict[str, int]

    def get(self, name: str) -> int | None:
        return self.addrs.get(name)

    def size_of(self, name: str) -> int | None:
        return self.sizes.get(name)

    def require(self, name: str) -> int:
        v = self.addrs.get(name)
        if v is None:
            raise KeyError(f"symbol '{name}' not found in {self.elf}")
        return v

    @property
    def core0_msp_range(self) -> tuple[int, int] | None:
        lo = self.addrs.get("__StackLimitCore0")
        hi = self.addrs.get("__StackTopCore0")
        return (lo, hi) if lo and hi else None

    @property
    def core1_msp_range(self) -> tuple[int, int] | None:
        lo = self.addrs.get("__StackLimitCore1")
        hi = self.addrs.get("__StackTopCore1")
        return (lo, hi) if lo and hi else None


def _run_nm(args: list[str]) -> str:
    last_exc: Exception | None = None
    for tool in NM_TOOLS:
        try:
            return subprocess.run(
                [tool, *args],
                capture_output=True, text=True, check=True, timeout=120,
            ).stdout
        except FileNotFoundError as exc:
            last_exc = exc
        except subprocess.CalledProcessError as exc:
            last_exc = exc
    raise RuntimeError(f"failed to run nm: {last_exc}")


def load_symbols(elf: Path, wanted: tuple[str, ...] = REQUIRED_SYMBOLS) -> SymbolTable:
    """Return ``SymbolTable`` with all addresses + sizes found in ``elf``.

    Names not present in the ELF are silently omitted; callers may then
    fall back to alternate names or skip features that require them.
    """
    raw = _run_nm(["-S", str(elf)])
    wanted_set = set(wanted)
    addrs: dict[str, int] = {}
    sizes: dict[str, int] = {}
    for line in raw.splitlines():
        m = _NM_S_RE.match(line)
        if m:
            name = m.group("name")
            if name in wanted_set:
                addrs[name] = int(m.group("addr"), 16)
                sizes[name] = int(m.group("size"), 16)
            continue
        m = _NM_RE.match(line)
        if m:
            name = m.group("name")
            if name in wanted_set:
                addrs[name] = int(m.group("addr"), 16)
    return SymbolTable(elf=elf, addrs=addrs, sizes=sizes)
