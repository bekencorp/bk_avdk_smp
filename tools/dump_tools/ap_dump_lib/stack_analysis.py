#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass, field

from .dat_parser import DumpSession, StackRegion, TaskRow
from .elf_symbols import ElfSymbols, ResolvedAddress


STUCK_PATTERNS = (
    "rtos_get_semaphore",
    "jpeg_decode_wait_flexa_registered_ports_done",
    "vcdec_jpeg_decode_frame",
    "lwip_send",
    "ntwk_socket_sendto",
)


@dataclass
class CurrentCoreState:
    core: int
    tcb_address: int | None = None
    source: str = ""
    task: TaskRow | None = None
    chain: list[ResolvedAddress] = field(default_factory=list)
    note: str = ""


@dataclass
class StackChain:
    name: str
    source: str
    stack_range: tuple[int, int] | None = None
    addresses: list[int] = field(default_factory=list)
    resolved: list[ResolvedAddress] = field(default_factory=list)
    likely_stuck: str = ""


@dataclass
class ExceptionStack:
    core: int
    registers: dict[str, int]
    chain: list[ResolvedAddress]


@dataclass
class AnalysisResult:
    current_cores: list[CurrentCoreState]
    stack_chains: list[StackChain]
    exception_stacks: list[ExceptionStack]
    likely_stuck: str
    unresolved: list[int]


def _find_region_containing(regions: list[StackRegion], address: int) -> StackRegion | None:
    for region in regions:
        if region.contains(address, 4):
            return region
    return None


def read_px_current_tcbs(session: DumpSession, symbols: ElfSymbols) -> list[CurrentCoreState]:
    px_current = symbols.get("pxCurrentTCBs")
    cores = [CurrentCoreState(core=0), CurrentCoreState(core=1)]
    if not px_current:
        for core in cores:
            core.note = "pxCurrentTCBs symbol not found in ELF."
        return cores

    region = _find_region_containing(session.stack_regions, px_current.address)
    if not region:
        for core in cores:
            core.source = f"pxCurrentTCBs @ 0x{px_current.address:08x}"
            core.note = "Symbol found, but DAT does not contain the memory region holding pxCurrentTCBs."
        return cores

    for core in cores:
        value = region.read_u32(px_current.address + core.core * 4)
        core.tcb_address = value
        core.source = f"pxCurrentTCBs[{core.core}] from {region.name}"
    return cores


def _chain_from_addresses(name: str, source: str, addresses: list[int], symbols: ElfSymbols, stack_range=None) -> StackChain:
    deduped: list[int] = []
    seen = set()
    for addr in addresses:
        if addr in seen:
            continue
        seen.add(addr)
        deduped.append(addr)
    resolved = symbols.resolve_many(deduped)
    stuck = classify_stuck(resolved)
    return StackChain(name=name, source=source, stack_range=stack_range, addresses=deduped, resolved=resolved, likely_stuck=stuck)


def scan_stack_region(region: StackRegion, symbols: ElfSymbols, limit: int = 48) -> StackChain:
    addresses: list[int] = []
    for _, word in region.iter_words():
        if symbols.looks_like_code(word):
            addresses.append(word)
            if len(addresses) >= limit:
                break
    return _chain_from_addresses(region.name, f"stack-scan lines {region.line_start}-{region.line_end}", addresses, symbols, (region.start, region.end))


def build_task_chains(session: DumpSession, symbols: ElfSymbols) -> list[StackChain]:
    chains: list[StackChain] = []
    for row in session.task_rows:
        if row.addresses:
            chains.append(_chain_from_addresses(row.name, row.source, row.addresses, symbols, row.stack_range))
    for region in session.stack_regions:
        chain = scan_stack_region(region, symbols)
        if chain.addresses:
            chains.append(chain)
    return chains


def build_exception_stacks(session: DumpSession, symbols: ElfSymbols) -> list[ExceptionStack]:
    stacks: list[ExceptionStack] = []
    for block in session.registers:
        addrs = []
        for name in ("pc", "lr"):
            value = block.registers.get(name)
            if value is not None:
                addrs.append(value)
        # Add xPSR exception stack candidates if the traceback command is absent.
        if addrs:
            stacks.append(ExceptionStack(core=block.core, registers=block.registers, chain=symbols.resolve_many(addrs)))
    return stacks


def classify_stuck(resolved: list[ResolvedAddress]) -> str:
    names = " ".join((item.symbol.name if item.symbol else "") for item in resolved)
    for pattern in STUCK_PATTERNS:
        if pattern in names:
            return pattern
    if any("semaphore" in (item.symbol.name if item.symbol else "").lower() for item in resolved):
        return "semaphore wait"
    if any("mutex" in (item.symbol.name if item.symbol else "").lower() for item in resolved):
        return "mutex wait"
    return ""


def analyze(session: DumpSession, symbols: ElfSymbols) -> AnalysisResult:
    current_cores = read_px_current_tcbs(session, symbols)
    for core in current_cores:
        if core.tcb_address is None:
            continue
        tcb_region = _find_region_containing(session.stack_regions, core.tcb_address)
        if not tcb_region:
            continue
        px_top_of_stack = tcb_region.read_u32(core.tcb_address)
        if px_top_of_stack is None:
            continue
        stack_region = _find_region_containing(session.stack_regions, px_top_of_stack)
        if not stack_region:
            core.note = f"TCB found; pxTopOfStack={px_top_of_stack:#010x}, but stack bytes are not in DAT."
            continue
        chain = scan_stack_region(stack_region, symbols)
        core.chain = chain.resolved
        core.note = f"pxTopOfStack={px_top_of_stack:#010x}, stack={stack_region.name}"
    stack_chains = build_task_chains(session, symbols)
    exception_stacks = build_exception_stacks(session, symbols)
    unresolved: list[int] = []

    for trace in session.tracebacks:
        stack_chains.insert(0, _chain_from_addresses(f"traceback@line{trace.line_no}", "traceback command", trace.addresses, symbols))

    likely = ""
    for chain in stack_chains:
        if chain.likely_stuck:
            likely = chain.likely_stuck
            break
    if not likely:
        for ex in exception_stacks:
            likely = classify_stuck(ex.chain)
            if likely:
                break
    if not likely:
        lower_text = "\n".join(session.lines).lower()
        if "decode timeout" in lower_text or "flexa port" in lower_text:
            likely = "decode timeout / flexa port"
        elif session.dump_reason:
            likely = session.dump_reason
        else:
            likely = "unknown"

    for chain in stack_chains:
        for item in chain.resolved:
            if not item.symbol:
                unresolved.append(item.address)

    return AnalysisResult(
        current_cores=current_cores,
        stack_chains=stack_chains,
        exception_stacks=exception_stacks,
        likely_stuck=likely,
        unresolved=sorted(set(unresolved)),
    )

