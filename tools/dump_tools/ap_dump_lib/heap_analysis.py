#!/usr/bin/env python3
"""Offline MEM_DEBUG heap walk + canary check from dump memory + ELF."""
from __future__ import annotations

import re
import subprocess
from dataclasses import dataclass, field
from pathlib import Path

from .dat_parser import DumpSession, StackRegion
from .elf_symbols import ElfSymbols

MEM_OVERFLOW_TAG = 0xCD
MEM_CHECK_TAG_LEN = 4
HEAP_BLOCK_ALLOCATED_BIT = 0x80000000
# heap_4 / heap_4_1 with portBYTE_ALIGNMENT=8: HeapBlockLink_t is 8 bytes.
HEAP_BLOCK_LINK_SIZE = 8
# AP bk_heap_debug_info_t (list_head + 2 ptrs + time_line + wantedSize).
AP_HEAP_DEBUG_INFO_SIZE = 24
# CP FreeRTOS BlockLink_t with MEM_DEBUG pointer-style fields (no head_magic).
CP_BLOCK_LINK_DEBUG_SIZE = 32
MAX_HEAP_BLOCKS = 20000


@dataclass
class HeapBlockIssue:
    heap: str
    user_ptr: int
    wanted_size: int
    alloc_func: str
    alloc_line: int
    alloc_task: str
    alloc_tick: int
    tag_addr: int
    tag_offset: int
    expect: int
    actual: int
    note: str = ""


@dataclass
class HeapListResult:
    name: str
    head_addr: int | None
    layout: str
    blocks: int = 0
    checked: int = 0
    overflows: list[HeapBlockIssue] = field(default_factory=list)
    note: str = ""
    truncated: bool = False


@dataclass
class HeapAnalysisResult:
    memory_bytes: int
    lists: list[HeapListResult] = field(default_factory=list)
    note: str = ""

    @property
    def overflow_count(self) -> int:
        return sum(len(item.overflows) for item in self.lists)

    @property
    def blocks_checked(self) -> int:
        return sum(item.checked for item in self.lists)


class DumpMemoryMap:
    """Byte-addressable view over decoded stack/mem dump regions."""

    def __init__(self, regions: list[StackRegion]):
        self._data: dict[int, int] = {}
        for region in regions:
            for offset, value in enumerate(region.data):
                self._data[region.start + offset] = value

    def __len__(self) -> int:
        return len(self._data)

    def has(self, address: int, size: int = 1) -> bool:
        if size <= 0:
            return False
        return all((address + i) in self._data for i in range(size))

    def read_u32(self, address: int) -> int | None:
        raw = self.read_bytes(address, 4)
        return None if raw is None else int.from_bytes(raw, "little")

    def read_bytes(self, address: int, size: int) -> bytes | None:
        if not self.has(address, size):
            return None
        return bytes(self._data[address + i] for i in range(size))

    def read_c_string(self, address: int, max_len: int = 64) -> str | None:
        if address == 0 or not self.has(address, 1):
            return None
        out = bytearray()
        for i in range(max_len):
            if not self.has(address + i, 1):
                break
            value = self._data[address + i]
            if value == 0:
                break
            if 32 <= value < 127:
                out.append(value)
            else:
                break
        return out.decode("ascii", errors="ignore") if out else None


class ElfVirtualMemory:
    """Read NUL-terminated strings from ELF PT_LOAD images by VMA."""

    def __init__(self, elf_path: Path):
        self.segments: list[tuple[int, int, bytes]] = []
        self._load(elf_path)

    def _load(self, elf_path: Path) -> None:
        readelf = None
        for name in ("arm-none-eabi-readelf", "readelf"):
            try:
                subprocess.check_output([name, "-v"], stderr=subprocess.DEVNULL)
                readelf = name
                break
            except Exception:
                continue
        if not readelf:
            return
        try:
            text = subprocess.check_output(
                [readelf, "-l", "-W", str(elf_path)], text=True, errors="ignore"
            )
        except Exception:
            return

        # Program Headers:
        #   Type Offset VirtAddr PhysAddr FileSiz MemSiz Flg Align
        #   LOAD 0x...  0x...     ...
        load_re = re.compile(
            r"^\s*LOAD\s+0x(?P<off>[0-9a-fA-F]+)\s+0x(?P<vaddr>[0-9a-fA-F]+)\s+"
            r"0x[0-9a-fA-F]+\s+0x(?P<filesz>[0-9a-fA-F]+)\s+0x(?P<memsz>[0-9a-fA-F]+)",
            re.MULTILINE,
        )
        raw = elf_path.read_bytes()
        for match in load_re.finditer(text):
            offset = int(match.group("off"), 16)
            vaddr = int(match.group("vaddr"), 16)
            filesz = int(match.group("filesz"), 16)
            if filesz <= 0 or offset + filesz > len(raw):
                continue
            self.segments.append((vaddr, filesz, raw[offset:offset + filesz]))

    def read_c_string(self, address: int, max_len: int = 64) -> str | None:
        if address == 0:
            return None
        for vaddr, size, blob in self.segments:
            if vaddr <= address < vaddr + size:
                off = address - vaddr
                end = blob.find(b"\x00", off, off + max_len)
                chunk = blob[off: end if end >= 0 else off + max_len]
                if not chunk:
                    return None
                if all(32 <= b < 127 for b in chunk):
                    return chunk.decode("ascii")
                return None
        return None


def _resolve_name(
    address: int,
    dump_mem: DumpMemoryMap,
    elf_mem: ElfVirtualMemory,
) -> str:
    if address == 0:
        return ""
    text = dump_mem.read_c_string(address) or elf_mem.read_c_string(address)
    if text:
        return text
    return f"0x{address:08x}"


def _build_memory_map(session: DumpSession) -> DumpMemoryMap:
    return DumpMemoryMap(session.stack_regions)


def _check_canary(
    dump_mem: DumpMemoryMap,
    tag_addr: int,
    tag_len: int,
) -> tuple[bool, int, int]:
    """Return (ok, offset, actual_byte). ok True when all tags match."""
    if tag_len <= 0:
        return True, 0, MEM_OVERFLOW_TAG
    raw = dump_mem.read_bytes(tag_addr, tag_len)
    if raw is None:
        return True, 0, MEM_OVERFLOW_TAG  # missing memory: skip, not a hit
    for offset, value in enumerate(raw):
        if value != MEM_OVERFLOW_TAG:
            return False, offset, value
    return True, 0, MEM_OVERFLOW_TAG


def _walk_ap_list(
    name: str,
    head_addr: int,
    dump_mem: DumpMemoryMap,
    elf_mem: ElfVirtualMemory,
) -> HeapListResult:
    result = HeapListResult(name=name, head_addr=head_addr, layout="ap_heap_manager")
    head = dump_mem.read_u32(head_addr)
    if head is None:
        result.note = "used_list head not present in dump memory"
        return result

    # Empty / uninitialized list: next == head, or NULL.
    cur = head
    if cur in (0, head_addr):
        result.note = "empty used list"
        return result

    seen: set[int] = set()
    while cur != head_addr:
        if cur in seen:
            result.note = "list cycle detected; aborting walk"
            break
        if result.blocks >= MAX_HEAP_BLOCKS:
            result.truncated = True
            result.note = f"truncated after {MAX_HEAP_BLOCKS} blocks"
            break
        seen.add(cur)
        result.blocks += 1

        # cur points at bk_heap_debug_info_t.node (== start of debug info).
        debug = cur
        if not dump_mem.has(debug, AP_HEAP_DEBUG_INFO_SIZE):
            result.note = f"block metadata missing at 0x{debug:08x}"
            break

        next_ptr = dump_mem.read_u32(debug) or 0
        func_ptr = dump_mem.read_u32(debug + 8) or 0
        task_ptr = dump_mem.read_u32(debug + 12) or 0
        time_line = dump_mem.read_u32(debug + 16) or 0
        wanted = dump_mem.read_u32(debug + 20) or 0
        alloc_tick = time_line & 0xFFFF
        alloc_line = (time_line >> 16) & 0xFFFF
        user_ptr = debug + AP_HEAP_DEBUG_INFO_SIZE

        link_addr = debug - HEAP_BLOCK_LINK_SIZE
        block_size_raw = dump_mem.read_u32(link_addr + 4)
        if block_size_raw is None or (block_size_raw & HEAP_BLOCK_ALLOCATED_BIT) == 0:
            result.overflows.append(
                HeapBlockIssue(
                    heap=name,
                    user_ptr=user_ptr,
                    wanted_size=wanted,
                    alloc_func=_resolve_name(func_ptr, dump_mem, elf_mem),
                    alloc_line=alloc_line,
                    alloc_task=_resolve_name(task_ptr, dump_mem, elf_mem),
                    alloc_tick=alloc_tick,
                    tag_addr=user_ptr + wanted,
                    tag_offset=0,
                    expect=MEM_OVERFLOW_TAG,
                    actual=0,
                    note="heap block header missing or not marked allocated",
                )
            )
            cur = next_ptr
            continue

        allocated = (block_size_raw & ~HEAP_BLOCK_ALLOCATED_BIT) - HEAP_BLOCK_LINK_SIZE
        if allocated < AP_HEAP_DEBUG_INFO_SIZE + wanted:
            result.overflows.append(
                HeapBlockIssue(
                    heap=name,
                    user_ptr=user_ptr,
                    wanted_size=wanted,
                    alloc_func=_resolve_name(func_ptr, dump_mem, elf_mem),
                    alloc_line=alloc_line,
                    alloc_task=_resolve_name(task_ptr, dump_mem, elf_mem),
                    alloc_tick=alloc_tick,
                    tag_addr=user_ptr + wanted,
                    tag_offset=0,
                    expect=MEM_OVERFLOW_TAG,
                    actual=0,
                    note=f"invalid sizes allocated={allocated} wanted={wanted}",
                )
            )
            cur = next_ptr
            continue

        tag_len = allocated - AP_HEAP_DEBUG_INFO_SIZE - wanted
        tag_addr = user_ptr + wanted
        result.checked += 1
        ok, tag_off, actual = _check_canary(dump_mem, tag_addr, tag_len)
        if not ok:
            result.overflows.append(
                HeapBlockIssue(
                    heap=name,
                    user_ptr=user_ptr,
                    wanted_size=wanted,
                    alloc_func=_resolve_name(func_ptr, dump_mem, elf_mem),
                    alloc_line=alloc_line,
                    alloc_task=_resolve_name(task_ptr, dump_mem, elf_mem),
                    alloc_tick=alloc_tick,
                    tag_addr=tag_addr,
                    tag_offset=tag_off,
                    expect=MEM_OVERFLOW_TAG,
                    actual=actual,
                )
            )

        cur = next_ptr
        if cur == 0:
            result.note = "encountered NULL next pointer"
            break

    if result.blocks == 0 and not result.note:
        result.note = "empty used list"
    return result


def _walk_cp_list(
    name: str,
    head_addr: int,
    dump_mem: DumpMemoryMap,
    elf_mem: ElfVirtualMemory,
    *,
    block_header_size: int = CP_BLOCK_LINK_DEBUG_SIZE,
    node_offset: int = 8,
) -> HeapListResult:
    result = HeapListResult(name=name, head_addr=head_addr, layout="cp_freertos_heap4")
    head_next = dump_mem.read_u32(head_addr)
    if head_next is None:
        result.note = "xUsed head not present in dump memory"
        return result

    cur = head_next
    if cur in (0, head_addr):
        result.note = "empty used list"
        return result

    seen: set[int] = set()
    while cur != head_addr:
        if cur in seen:
            result.note = "list cycle detected; aborting walk"
            break
        if result.blocks >= MAX_HEAP_BLOCKS:
            result.truncated = True
            result.note = f"truncated after {MAX_HEAP_BLOCKS} blocks"
            break
        seen.add(cur)
        result.blocks += 1

        block = cur - node_offset
        if not dump_mem.has(block, block_header_size):
            result.note = f"block metadata missing at 0x{block:08x}"
            break

        next_ptr = dump_mem.read_u32(cur) or 0
        block_size_raw = dump_mem.read_u32(block + 4) or 0
        func_ptr = dump_mem.read_u32(block + 16) or 0
        task_ptr = dump_mem.read_u32(block + 20) or 0
        time_line = dump_mem.read_u32(block + 24) or 0
        wanted = dump_mem.read_u32(block + 28) or 0
        alloc_tick = time_line & 0xFFFF
        alloc_line = (time_line >> 16) & 0xFFFF
        user_ptr = block + block_header_size

        if (block_size_raw & HEAP_BLOCK_ALLOCATED_BIT) == 0:
            result.overflows.append(
                HeapBlockIssue(
                    heap=name,
                    user_ptr=user_ptr,
                    wanted_size=wanted,
                    alloc_func=_resolve_name(func_ptr, dump_mem, elf_mem),
                    alloc_line=alloc_line,
                    alloc_task=_resolve_name(task_ptr, dump_mem, elf_mem),
                    alloc_tick=alloc_tick,
                    tag_addr=user_ptr + wanted,
                    tag_offset=0,
                    expect=MEM_OVERFLOW_TAG,
                    actual=0,
                    note="block not marked allocated",
                )
            )
            cur = next_ptr
            continue

        block_size = block_size_raw & ~HEAP_BLOCK_ALLOCATED_BIT
        if block_size < block_header_size + wanted:
            result.overflows.append(
                HeapBlockIssue(
                    heap=name,
                    user_ptr=user_ptr,
                    wanted_size=wanted,
                    alloc_func=_resolve_name(func_ptr, dump_mem, elf_mem),
                    alloc_line=alloc_line,
                    alloc_task=_resolve_name(task_ptr, dump_mem, elf_mem),
                    alloc_tick=alloc_tick,
                    tag_addr=user_ptr + wanted,
                    tag_offset=0,
                    expect=MEM_OVERFLOW_TAG,
                    actual=0,
                    note=f"invalid sizes block={block_size} wanted={wanted}",
                )
            )
            cur = next_ptr
            continue

        tag_len = block_size - block_header_size - wanted
        tag_addr = user_ptr + wanted
        result.checked += 1
        ok, tag_off, actual = _check_canary(dump_mem, tag_addr, tag_len)
        if not ok:
            result.overflows.append(
                HeapBlockIssue(
                    heap=name,
                    user_ptr=user_ptr,
                    wanted_size=wanted,
                    alloc_func=_resolve_name(func_ptr, dump_mem, elf_mem),
                    alloc_line=alloc_line,
                    alloc_task=_resolve_name(task_ptr, dump_mem, elf_mem),
                    alloc_tick=alloc_tick,
                    tag_addr=tag_addr,
                    tag_offset=tag_off,
                    expect=MEM_OVERFLOW_TAG,
                    actual=actual,
                )
            )

        cur = next_ptr
        if cur == 0:
            result.note = "encountered NULL next pointer"
            break

    if result.blocks == 0 and not result.note:
        result.note = "empty used list"
    return result


def analyze_heap(session: DumpSession, symbols: ElfSymbols) -> HeapAnalysisResult:
    dump_mem = _build_memory_map(session)
    elf_mem = ElfVirtualMemory(symbols.elf_path)
    result = HeapAnalysisResult(memory_bytes=len(dump_mem))

    if len(dump_mem) == 0:
        result.note = "no decoded dump memory available for heap walk"
        return result

    ap_lists = (
        ("s_sram_used", "sram"),
        ("s_hsram_used", "hsram"),
        ("s_psram_used", "psram"),
        ("s_psram_cache_used", "psram_cache"),
    )
    found_ap = False
    for sym_name, label in ap_lists:
        sym = symbols.get(sym_name)
        if not sym:
            continue
        found_ap = True
        result.lists.append(_walk_ap_list(label, sym.address, dump_mem, elf_mem))

    if found_ap:
        return result

    cp_lists = (
        ("xUsed", "sram"),
        ("xPsramUsed", "psram"),
    )
    found_cp = False
    for sym_name, label in cp_lists:
        sym = symbols.get(sym_name)
        if not sym:
            continue
        found_cp = True
        result.lists.append(_walk_cp_list(label, sym.address, dump_mem, elf_mem))

    if not found_cp:
        result.note = (
            "no MEM_DEBUG used-list symbols (s_sram_used / xUsed) found in ELF; "
            "heap canary walk skipped"
        )
    return result


def render_heap_console(heap: HeapAnalysisResult) -> list[str]:
    lines = ["Heap Integrity (offline MEM_DEBUG canary)"]
    if heap.note and not heap.lists:
        lines.append(f"  {heap.note}")
        return lines

    lines.append(
        f"  Dump bytes : {heap.memory_bytes}  lists={len(heap.lists)}  "
        f"checked={heap.blocks_checked}  overflows={heap.overflow_count}"
    )
    for item in heap.lists:
        status = "PASS" if not item.overflows else f"FAIL({len(item.overflows)})"
        head = "unknown" if item.head_addr is None else f"0x{item.head_addr:08x}"
        extra = f" note={item.note}" if item.note else ""
        lines.append(
            f"  {item.name}: {status} blocks={item.blocks} checked={item.checked} "
            f"head={head} layout={item.layout}{extra}"
        )
        for issue in item.overflows[:8]:
            lines.append(
                f"    Mem Overflow user=0x{issue.user_ptr:08x} "
                f"tag=0x{issue.tag_addr:08x}+{issue.tag_offset} "
                f"expect=0x{issue.expect:02x} actual=0x{issue.actual:02x} "
                f"alloc={issue.alloc_func}:{issue.alloc_line} "
                f"task={issue.alloc_task} tick={issue.alloc_tick}"
                + (f" ({issue.note})" if issue.note else "")
            )
        if len(item.overflows) > 8:
            lines.append(f"    ... {len(item.overflows) - 8} more")
    if heap.note:
        lines.append(f"  note: {heap.note}")
    return lines


def render_heap_markdown(heap: HeapAnalysisResult) -> list[str]:
    lines = ["## Heap Integrity (offline)", ""]
    if heap.note and not heap.lists:
        lines.append(f"_{heap.note}_")
        lines.append("")
        return lines

    lines.append(
        f"- Dump decoded bytes: `{heap.memory_bytes}`"
    )
    lines.append(
        f"- Blocks checked: `{heap.blocks_checked}`; canary overflows: `{heap.overflow_count}`"
    )
    lines.append("")

    rows: list[list[str]] = []
    for item in heap.lists:
        rows.append(
            [
                item.name,
                item.layout,
                "unknown" if item.head_addr is None else f"0x{item.head_addr:08x}",
                str(item.blocks),
                str(item.checked),
                str(len(item.overflows)),
                item.note or "",
            ]
        )
    if rows:
        lines.append("| Heap | Layout | Head | Blocks | Checked | Overflows | Note |")
        lines.append("| --- | --- | --- | --- | --- | --- | --- |")
        for row in rows:
            lines.append("| " + " | ".join(row) + " |")
        lines.append("")

    issues = [issue for item in heap.lists for issue in item.overflows]
    if issues:
        lines.append("### Mem Overflow Details")
        lines.append("")
        lines.append(
            "| Heap | User | Tag | Expect | Actual | Alloc | Task | Tick | Note |"
        )
        lines.append("| --- | --- | --- | --- | --- | --- | --- | --- | --- |")
        for issue in issues[:64]:
            lines.append(
                "| "
                + " | ".join(
                    [
                        issue.heap,
                        f"0x{issue.user_ptr:08x}",
                        f"0x{issue.tag_addr:08x}+{issue.tag_offset}",
                        f"0x{issue.expect:02x}",
                        f"0x{issue.actual:02x}",
                        f"{issue.alloc_func}:{issue.alloc_line}",
                        issue.alloc_task,
                        str(issue.alloc_tick),
                        issue.note,
                    ]
                )
                + " |"
            )
        if len(issues) > 64:
            lines.append("")
            lines.append(f"_... {len(issues) - 64} more overflows omitted._")
        lines.append("")
    else:
        lines.append("_No canary overflows detected in walked used-lists._")
        lines.append("")
        lines.append(
            "Note: in-object corruption that does not break the trailing `0xcd` "
            "canary (for example an intra-allocation function-pointer stomp) is "
            "not reported here."
        )
        lines.append("")

    return lines
