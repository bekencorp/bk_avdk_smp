#!/usr/bin/env python3
from __future__ import annotations

import bisect
import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass(order=True)
class Symbol:
    address: int
    name: str
    kind: str = ""
    size: int = 0


@dataclass
class ResolvedAddress:
    address: int
    lookup_address: int
    symbol: Symbol | None
    file_line: str = "??:0"
    function_line: str = "??"

    @property
    def display(self) -> str:
        sym = self.symbol.name if self.symbol else "??"
        off = ""
        if self.symbol and self.lookup_address >= self.symbol.address:
            delta = self.lookup_address - self.symbol.address
            off = f"+0x{delta:x}" if delta else ""
        return f"0x{self.address:08x} -> {sym}{off} ({self.file_line})"


def _which(names: list[str]) -> str:
    for name in names:
        found = shutil.which(name)
        if found:
            return found
    raise RuntimeError(f"Could not find any of: {', '.join(names)}")


def _run(args: list[str]) -> str:
    return subprocess.check_output(args, text=True, errors="ignore")


def normalize_thumb_return(address: int) -> int:
    if address & 1:
        return address & ~1
    return address


class ElfSymbols:
    def __init__(self, elf_path: Path):
        self.elf_path = elf_path
        self.nm = _which(["arm-none-eabi-nm", "nm"])
        self.addr2line = _which(["arm-none-eabi-addr2line", "addr2line"])
        self.readelf = shutil.which("arm-none-eabi-readelf") or shutil.which("readelf")
        self.symbols = self._load_symbols()
        self.by_name = {sym.name: sym for sym in self.symbols}
        self.addresses = [sym.address for sym in self.symbols]

    def _load_symbols(self) -> list[Symbol]:
        symbols: dict[tuple[int, str], Symbol] = {}
        for line in _run([self.nm, "-an", str(self.elf_path)]).splitlines():
            parts = line.split()
            if len(parts) < 3:
                continue
            try:
                addr = int(parts[0], 16)
            except ValueError:
                continue
            kind = parts[1]
            name = parts[2]
            if name.startswith("$"):
                continue
            symbols[(addr, name)] = Symbol(address=addr, kind=kind, name=name)

        if self.readelf:
            # Static symbols such as s_interrupt_core0_dump keep useful sizes in readelf.
            for line in _run([self.readelf, "-sW", str(self.elf_path)]).splitlines():
                m = re.match(
                    r"\s*\d+:\s+(?P<value>[0-9a-fA-F]+)\s+(?P<size>\d+)\s+"
                    r"(?P<type>\S+)\s+\S+\s+\S+\s+\S+\s+(?P<name>\S+)",
                    line,
                )
                if not m:
                    continue
                addr = int(m.group("value"), 16)
                name = m.group("name")
                if name.startswith("$"):
                    continue
                size = int(m.group("size"))
                key = (addr, name)
                if key in symbols:
                    symbols[key].size = size
                elif addr:
                    symbols[key] = Symbol(address=addr, name=name, size=size, kind=m.group("type"))

        result = sorted(symbols.values(), key=lambda s: (s.address, s.name))
        return [sym for sym in result if sym.address != 0]

    def get(self, name: str) -> Symbol | None:
        return self.by_name.get(name)

    def nearest(self, address: int) -> Symbol | None:
        if not self.addresses:
            return None
        idx = bisect.bisect_right(self.addresses, address) - 1
        if idx < 0:
            return None
        sym = self.symbols[idx]
        if sym.size and address >= sym.address + sym.size:
            # Keep nearest text symbol useful for stripped/zero-size function symbols, but
            # avoid claiming object symbols cover unrelated memory.
            if sym.kind.upper() not in {"T", "W", "FUNC"}:
                return None
        return sym

    def looks_like_code(self, address: int) -> bool:
        lookup = normalize_thumb_return(address)
        sym = self.nearest(lookup)
        if not sym:
            return False
        kind = sym.kind.upper()
        return kind in {"T", "W", "FUNC"} or kind.endswith("T")

    def resolve(self, address: int, normalize_thumb: bool = True) -> ResolvedAddress:
        lookup = normalize_thumb_return(address) if normalize_thumb else address
        if normalize_thumb and (address & 1):
            candidates = [address & ~1]
            if (address & ~1) >= 4:
                candidates.append((address & ~1) - 4)
            for candidate in candidates:
                sym = self.nearest(candidate)
                if sym and (sym.kind.upper() in {"T", "W", "FUNC"} or sym.kind.upper().endswith("T")):
                    lookup = candidate
                    break
        sym = self.nearest(lookup)
        function_line = "??"
        file_line = "??:0"
        try:
            out = _run([self.addr2line, "-piaf", "-e", str(self.elf_path), f"0x{lookup:x}"]).strip()
            if out:
                # -piaf output commonly has one compact line.
                function_line = out
                if " at " in out:
                    file_line = out.rsplit(" at ", 1)[1]
        except Exception:
            pass
        return ResolvedAddress(address=address, lookup_address=lookup, symbol=sym, file_line=file_line, function_line=function_line)

    def resolve_many(self, addresses: list[int], normalize_thumb: bool = True) -> list[ResolvedAddress]:
        return [self.resolve(addr, normalize_thumb=normalize_thumb) for addr in addresses]

