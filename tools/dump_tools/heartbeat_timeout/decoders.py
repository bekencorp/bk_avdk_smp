"""Bit-level decoders for high-value AP peripherals + ELF callback resolution.

Adds three sections to the heartbeat-timeout analyzer output:

  * **HPDMA channel decode** — each of the 4 channels' REG_0x10..REG_0x1F is
    bit-decoded (ctrl, addresses, xsize/ysize, status, step, etc.). The
    decoder runs entirely off the v3 1 KiB HPDMA snapshot.
  * **HPDMA registered ISR callbacks** — reads ``s_hpdma_half_finish_isr``,
    ``s_hpdma_finish_isr`` and ``s_hpdma_bus_err_isr`` arrays from the
    SRAM dump (each entry is ``{callback, user_data}``) and resolves the
    callback pointers via ``addr2line`` to source location.
  * **HSPL channel owner** — for HSPL0/1 STA blocks, decodes each channel's
    OWNER_VALID / OWNER_ID bits plus the resource name registered at that
    channel (HSPL0 ch15 = ``UART_LOG``, HSPL1 ch9 = ``VENC``, etc.).

Together these turn the raw ``peri.txt`` dump into something a reviewer can
read top-to-bottom without paging the M55 datasheet.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

from .core import addr2line, read_bytes, read_u32
from .peri_regs import PeriRegion, PeriReport
from .symbols import SymbolTable


# ============================================================================
# HPDMA
# ============================================================================
HPDMA_BASE        = 0x4C300000
HPDMA_CH_STRIDE   = 0x40          # 16 registers × 4 bytes
HPDMA_CH_OFFSET   = 0x40          # first channel starts at base + 0x40
HPDMA_CHAN_COUNT  = 4             # SOC_HPDMA_CHAN_NUM_PER_UNIT for BK7259

HPDMA_DATA_WIDTH_NAMES = ("8b", "16b", "32b", "64b", "128b", "?5", "?6", "?7")
HPDMA_BURST_NAMES = ("1 beat", "4 beat", "8 beat", "16 beat")


@dataclass
class HpdmaChannel:
    """Decoded view of a single HPDMA channel (REG_0x10..REG_0x1F)."""
    ch: int
    raw: tuple[int, ...]            # 16 raw 32-bit registers as captured

    @property
    def ctrl(self) -> int: return self.raw[0]
    @property
    def dest_start_addr(self) -> int: return self.raw[1]
    @property
    def src_start_addr(self) -> int: return self.raw[2]
    @property
    def dest_loop_end(self) -> int: return self.raw[3]
    @property
    def xsize(self) -> int: return self.raw[4]
    @property
    def src_loop_end(self) -> int: return self.raw[5]
    @property
    def ysize(self) -> int: return self.raw[6]
    @property
    def req_mux(self) -> int: return self.raw[7]
    @property
    def src_pause(self) -> int: return self.raw[8]
    @property
    def dst_pause(self) -> int: return self.raw[9]
    @property
    def src_rd_addr(self) -> int: return self.raw[10]
    @property
    def dst_wr_addr(self) -> int: return self.raw[11]
    @property
    def status(self) -> int: return self.raw[12]
    @property
    def step(self) -> int: return self.raw[13]
    @property
    def remain(self) -> int: return self.raw[14]
    @property
    def next_ll(self) -> int: return self.raw[15]

    @property
    def is_programmed(self) -> bool:
        return any(self.raw)

    @property
    def enable(self) -> int: return self.ctrl & 1
    @property
    def src_width(self) -> int: return (self.ctrl >> 2) & 0x7
    @property
    def dst_width(self) -> int: return (self.ctrl >> 5) & 0x7
    @property
    def src_xsize_v(self) -> int: return self.xsize & 0xFFFF
    @property
    def dst_xsize_v(self) -> int: return (self.xsize >> 16) & 0xFFFF
    @property
    def src_ysize_v(self) -> int: return self.ysize & 0xFFFF
    @property
    def dst_ysize_v(self) -> int: return (self.ysize >> 16) & 0xFFFF
    @property
    def status_finish_int(self) -> int: return (self.status >> 19) & 1
    @property
    def status_half_finish_int(self) -> int: return (self.status >> 18) & 1
    @property
    def status_bus_err_int(self) -> int: return (self.status >> 20) & 1
    @property
    def status_fifo_err_int(self) -> int: return (self.status >> 17) & 1
    @property
    def status_desc_num(self) -> int: return self.status & 0xFFFF
    @property
    def src_step_v(self) -> int: return self.step & 0x7FFF
    @property
    def dst_step_v(self) -> int: return (self.step >> 15) & 0x7FFF
    @property
    def finish_int_en(self) -> int: return (self.req_mux >> 30) & 1
    @property
    def half_finish_int_en(self) -> int: return (self.req_mux >> 31) & 1
    @property
    def bus_err_int_en(self) -> int: return (self.req_mux >> 22) & 1
    @property
    def fifo_err_int_en(self) -> int: return (self.req_mux >> 23) & 1
    @property
    def src_burst(self) -> int: return (self.req_mux >> 24) & 3
    @property
    def dst_burst(self) -> int: return (self.req_mux >> 26) & 3
    @property
    def src_req(self) -> int: return self.req_mux & 0x3F
    @property
    def dst_req(self) -> int: return (self.req_mux >> 6) & 0x3F


def parse_hpdma_block(block: bytes) -> list[HpdmaChannel]:
    """Return the decoded channels found in an HPDMA 1 KiB snapshot."""
    if len(block) < HPDMA_CH_OFFSET + HPDMA_CHAN_COUNT * HPDMA_CH_STRIDE:
        return []
    chans: list[HpdmaChannel] = []
    for ch in range(HPDMA_CHAN_COUNT):
        off = HPDMA_CH_OFFSET + ch * HPDMA_CH_STRIDE
        words = struct.unpack_from("<16I", block, off)
        chans.append(HpdmaChannel(ch=ch, raw=words))
    return chans


def _addr_label(addr: int) -> str:
    """Friendly label for a 32-bit address: SRAM / PSRAM / Flash / etc.

    The cacheability tags below follow the AP MPU programming of the
    BK7259V2 (non ``PSRAM_INTERLEAVE``) build — see
    ``ap/middleware/soc/bk7259_ap/mpu_cfg.c``::

        region 12  0x60000000..0x63FFFFFF  AttrIdx=1  Normal NC
        region 13  0x64000000..0x64DA6FFF  AttrIdx=1  Normal NC
        region 14  0x64DA7000..0x67FFFFFF  AttrIdx=3  Normal WB-RA-WA
        region 10  0x68000000..0x6FFFFFFF  AttrIdx=3  Normal WB-RA-WA (QSPI1)

    The ``CONFIG_AP_PSRAM_CODE_SECTION_ADDR`` boundary (default
    ``0x64DA7000`` in current night builds) is hard-coded here; if your
    build moves that line, update the constant below or wire it up to
    the symbol table.
    """
    if addr == 0:
        return ""
    if 0x281d0000 <= addr < 0x28200000:
        return "AP MSP region"
    if 0x28000000 <= addr < 0x28200000:
        return "AP SRAM/IRAM"
    if 0x04132000 <= addr < 0x041C0000:
        return "Flash XIP"
    if 0x60000000 <= addr < 0x64000000:
        return "PSRAM (non-cacheable)"
    # AP cacheable code/heap section starts at CONFIG_AP_PSRAM_CODE_SECTION_ADDR;
    # 0x64DA7000 is the current night-build value, treat anything below it as NC.
    AP_PSRAM_CODE_BASE = 0x64DA7000
    if 0x64000000 <= addr < AP_PSRAM_CODE_BASE:
        return "PSRAM (non-cacheable)"
    if AP_PSRAM_CODE_BASE <= addr < 0x68000000:
        return "PSRAM (cacheable WB)"
    if 0x68000000 <= addr < 0x70000000:
        return "QSPI1 (cacheable WB)"
    if 0x44000000 <= addr < 0x4C400000:
        return "MMIO"
    if 0x80000000 <= addr < 0xF0000000:
        return "PPB / device memory"
    return ""


def render_hpdma_channels(chans: list[HpdmaChannel]) -> list[str]:
    """Markdown-friendly per-channel breakdown for ``peri.txt``."""
    lines: list[str] = []
    lines.append("")
    lines.append("=== HPDMA channel decode (REG_0x10..REG_0x1F per channel) ===")
    active = [c for c in chans if c.is_programmed]
    if not active:
        lines.append("  no channel is programmed (all zero)")
        return lines
    for c in active:
        lines.append("")
        lines.append(f"  ch{c.ch} @ {HPDMA_BASE + HPDMA_CH_OFFSET + c.ch * HPDMA_CH_STRIDE:#010x}")
        src_lbl = _addr_label(c.src_start_addr)
        dst_lbl = _addr_label(c.dest_start_addr)
        lines.append(
            f"    CTRL=0x{c.ctrl:08x}  enable={c.enable}  mode={(c.ctrl >> 1) & 1}  "
            f"src_w={HPDMA_DATA_WIDTH_NAMES[c.src_width]}  dst_w={HPDMA_DATA_WIDTH_NAMES[c.dst_width]}  "
            f"src_inc={(c.ctrl >> 8) & 1}  dst_inc={(c.ctrl >> 9) & 1}  "
            f"src_loop={(c.ctrl >> 10) & 1}  dst_loop={(c.ctrl >> 11) & 1}  "
            f"prio={(c.ctrl >> 12) & 7}  fast={(c.ctrl >> 15) & 1}  "
            f"cfg_cache={(c.ctrl >> 16) & 0xF}"
        )
        lines.append(f"    SRC  0x{c.src_start_addr:08x}  ({src_lbl})")
        lines.append(f"    DST  0x{c.dest_start_addr:08x}  ({dst_lbl})")
        # Beats per row × bytes per beat ≈ row size.
        try:
            row_bytes = c.src_xsize_v * (1 << c.src_width)
        except Exception:
            row_bytes = 0
        lines.append(
            f"    XSIZE src={c.src_xsize_v} beats  dst={c.dst_xsize_v} beats  "
            f"(row≈{row_bytes} B with src_w={HPDMA_DATA_WIDTH_NAMES[c.src_width]})"
        )
        lines.append(f"    YSIZE src={c.src_ysize_v}  dst={c.dst_ysize_v}  (2D rect)")
        lines.append(
            f"    STEP src_step={c.src_step_v}  dst_step={c.dst_step_v}  "
            f"REMAIN_LEN={c.remain}  NEXT_LL={c.next_ll:#x}"
        )
        burst_s = HPDMA_BURST_NAMES[c.src_burst]
        burst_d = HPDMA_BURST_NAMES[c.dst_burst]
        lines.append(
            f"    REQ_MUX=0x{c.req_mux:08x}  src_req={c.src_req}  dst_req={c.dst_req}  "
            f"burst src={burst_s}/dst={burst_d}  "
            f"finish_int_en={c.finish_int_en}  half_finish_int_en={c.half_finish_int_en}  "
            f"bus_err_int_en={c.bus_err_int_en}  fifo_err_int_en={c.fifo_err_int_en}"
        )
        # Status bits — what the ISR sees right now.
        flag = []
        if c.status_finish_int:      flag.append("**finish_int=1**")
        if c.status_half_finish_int: flag.append("half_finish_int=1")
        if c.status_bus_err_int:     flag.append("**bus_err_int=1**")
        if c.status_fifo_err_int:    flag.append("**fifo_err_int=1**")
        flag_str = "  ".join(flag) if flag else "no pending"
        lines.append(
            f"    STATUS=0x{c.status:08x}  desc_num={c.status_desc_num}  "
            f"finish_cnt={(c.status >> 24) & 0xF}  half_finish_cnt={(c.status >> 28) & 0xF}  "
            f"[{flag_str}]"
        )
        if c.src_rd_addr or c.dst_wr_addr:
            lines.append(
                f"    PROGRESS src_rd_addr=0x{c.src_rd_addr:08x}  "
                f"dst_wr_addr=0x{c.dst_wr_addr:08x}"
            )
    return lines


# ============================================================================
# HPDMA registered ISR callbacks
# ============================================================================
HPDMA_ISR_ARRAYS = (
    "s_hpdma_half_finish_isr",
    "s_hpdma_finish_isr",
    "s_hpdma_bus_err_isr",
)


@dataclass
class HpdmaIsrSlot:
    array_name: str
    ch: int
    addr: int
    callback: int
    user_data: int


def collect_hpdma_isr_slots(
    elf: Path,
    memmap,
    extra_symbol_names: Iterable[str] = HPDMA_ISR_ARRAYS,
) -> list[HpdmaIsrSlot]:
    """Read the three ``s_hpdma_*_isr[]`` arrays out of the dump memory.

    Each array is ``hpdma_isr_info_t[NUM_CHANNELS]``, layout
    ``{ callback_fn, user_data }`` (= 8 bytes / entry). We pull the address
    of each symbol via ``nm -S`` (extending the auto-discovered symbol
    table on demand) and decode whatever the dump captured.
    """
    # Quick extra nm pass for the three arrays — symbols.py only loads a
    # fixed list, so we run nm again here. We tolerate symbols that are not
    # present (e.g. on older builds that named them differently).
    import re, subprocess

    pat = re.compile(
        r"^(?P<addr>[0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+[BbDd]\s+(?P<name>\S+)\s*$"
    )
    addrs: dict[str, int] = {}
    try:
        out = subprocess.run(
            ["arm-none-eabi-nm", "-S", str(elf)],
            capture_output=True, text=True, check=True, timeout=60,
        ).stdout
    except (FileNotFoundError, subprocess.CalledProcessError):
        try:
            out = subprocess.run(
                ["nm", "-S", str(elf)],
                capture_output=True, text=True, check=True, timeout=60,
            ).stdout
        except Exception:
            return []
    wanted = set(extra_symbol_names)
    for line in out.splitlines():
        m = pat.match(line)
        if m and m.group("name") in wanted:
            addrs[m.group("name")] = int(m.group("addr"), 16)

    slots: list[HpdmaIsrSlot] = []
    for name in extra_symbol_names:
        base = addrs.get(name)
        if base is None:
            continue
        for ch in range(HPDMA_CHAN_COUNT):
            entry_addr = base + ch * 8
            callback = read_u32(memmap, entry_addr)
            user_data = read_u32(memmap, entry_addr + 4)
            if callback is None and user_data is None:
                continue
            slots.append(
                HpdmaIsrSlot(
                    array_name=name, ch=ch, addr=entry_addr,
                    callback=callback or 0, user_data=user_data or 0,
                )
            )
    return slots


def render_hpdma_isr_callbacks(elf: Path, slots: list[HpdmaIsrSlot]) -> list[str]:
    lines: list[str] = []
    lines.append("")
    lines.append("=== HPDMA registered ISR callbacks (s_hpdma_*_isr[ch]) ===")
    if not slots:
        lines.append("  array symbols not in dump (symbol absent or memory not captured)")
        return lines
    # Resolve all non-zero callback pointers in one addr2line pass.
    addrs = [s.callback for s in slots if s.callback]
    resolved = addr2line(elf, addrs) if addrs else []
    resolved_map = dict(zip([f"0x{a:08x}" for a in addrs], resolved))
    current = ""
    for slot in slots:
        if slot.array_name != current:
            current = slot.array_name
            lines.append("")
            lines.append(f"  {current}:")
        if not slot.callback:
            lines.append(f"    ch{slot.ch}: <unset>")
            continue
        sym = resolved_map.get(f"0x{slot.callback:08x}", "??")
        # addr2line returns "func at file:line" possibly preceded by addr.
        sym_short = sym.split("\n", 1)[0].strip()
        lines.append(
            f"    ch{slot.ch}: cb=0x{slot.callback:08x} user=0x{slot.user_data:08x}  "
            f"-> {sym_short}"
        )
    return lines


# ============================================================================
# HSPL channel owner
# ============================================================================
HSPL_STA_OWNER_VALID_BIT  = 1 << 5
HSPL_STA_OWNER_MASK       = 0xF << 1
HSPL_STA_OWNER_SHIFT      = 1
HSPL_LOCK_SUCCESS_BIT     = 1 << 0   # also visible in some STA encodings


# Resource mapping from bk_hspl_res_t (see ap/middleware/driver/hspl/hspl_res_lock.h)
# Resources 0..15 -> HSPL_0 channels 0..15
# Resources 16..31 -> HSPL_1 channels 0..15
HSPL_RES_NAMES = (
    "FLASH",    "CLOCK",    "POWER",    "SYS",
    "RTC",      "ANA",      "FUSE",     "TRNG",
    "SYS_SW_REGS", "SPI",   "GPIO",     "PWM",
    "ADC",      "DAC",      "PMU",      "UART_LOG",
    "OS",       "LVGL",     "AUDIO",    "VIDEO",
    "GPU",      "NPU",      "DSP",      "ISP",
    "VDEC",     "VENC",     "SDIO",     "SDIO_HS",
    "SDIO_HS_HS","USB",     "USER1",    "USER2",
)


def hspl_resource_name(hspl_id: int, channel: int) -> str:
    res = hspl_id * 16 + channel
    if 0 <= res < len(HSPL_RES_NAMES):
        return HSPL_RES_NAMES[res]
    return f"res{res}"


@dataclass
class HsplChannelState:
    hspl_id: int
    channel: int
    sta: int

    @property
    def owner_valid(self) -> int:
        return 1 if (self.sta & HSPL_STA_OWNER_VALID_BIT) else 0

    @property
    def owner_id(self) -> int:
        return (self.sta & HSPL_STA_OWNER_MASK) >> HSPL_STA_OWNER_SHIFT

    @property
    def resource(self) -> str:
        return hspl_resource_name(self.hspl_id, self.channel)


def parse_hspl_states(block: bytes, hspl_id: int) -> list[HsplChannelState]:
    """Return per-channel state from a 64-byte HSPLn_STA snapshot."""
    states: list[HsplChannelState] = []
    for ch in range(min(16, len(block) // 4)):
        sta = struct.unpack_from("<I", block, ch * 4)[0]
        states.append(HsplChannelState(hspl_id=hspl_id, channel=ch, sta=sta))
    return states


HSPL_INSTANCE_SCOPE = {
    0: "CP/AP cross-core (CP M52 + AP M55)",
    1: "AP SMP-internal only (CP cannot use this instance)",
}


def render_hspl_states(peri: PeriReport) -> list[str]:
    """Render HSPL0/1 STA blocks.

    Important: ``owner_id`` is the hardware master-ID of whoever acquired the
    lock, and the master-ID space is **per HSPL instance**:

    * HSPL0 (`0x45010000`) - shared by CP M52 and AP M55, owner ID space spans
      both cores. A non-zero owner does not by itself tell you which side it
      is - cross-reference with the requester resource.
    * HSPL1 (`0x480C0000`) - **AP M55 internal only** (per
      ``ap/middleware/driver/hspl/README.md`` and ``hspl_driver.c``: CP cannot
      lock HSPL1 channels, the timeout IRQ is wired to AP M55). owner_id on
      HSPL1 therefore **never** identifies CP. Treat it as "some AP-side
      master that read LOCK[ch] successfully" - typically AP CPU0/CPU1, or an
      AP-side bus master that has access to that HSPL bus.

    We deliberately do not map owner_id to a human name here because the
    master-ID tieing is chip-specific and not exposed in this codebase.
    """
    lines: list[str] = []
    lines.append("")
    lines.append("=== HSPL channel owner decode (STA registers) ===")
    found_any = False
    for hspl_id, region_name in ((0, "HSPL0_STA"), (1, "HSPL1_STA")):
        region = next((r for r in peri.regions if r.name == region_name), None)
        if region is None:
            continue
        block = region.raw_dump_block or region.data
        if not block:
            continue
        found_any = True
        states = parse_hspl_states(block, hspl_id)
        lines.append("")
        scope = HSPL_INSTANCE_SCOPE.get(hspl_id, "unknown scope")
        lines.append(f"  HSPL{hspl_id} @ {region.start:#010x}  [{scope}]")
        held = [s for s in states if s.owner_valid or s.sta != 0]
        if not held:
            lines.append("    all channels idle (STA=0)")
            continue
        for s in held:
            tag = "**LOCKED**" if s.owner_valid else "non-zero"
            extra = ""
            if s.sta & HSPL_LOCK_SUCCESS_BIT:
                extra = " [bit0=lock_success-read-set]"
            lines.append(
                f"    ch{s.channel:2d} ({s.resource:<10s}) STA=0x{s.sta:08x}  "
                f"owner_valid={s.owner_valid}  owner_id={s.owner_id}  {tag}{extra}"
            )
        if hspl_id == 1:
            lines.append(
                "    note: HSPL1 owner_id is an AP-internal master-ID; "
                "it never refers to CP."
            )
    if not found_any:
        lines.append("  no HSPL_STA region captured")
    return lines


# ============================================================================
# Top-level helper used by the CLI
# ============================================================================
def render_extras(
    elf: Path,
    peri: PeriReport,
    memmap,
) -> str:
    """Compose the decoder block appended to ``peri.txt``."""
    hpdma_region = next((r for r in peri.regions if r.name == "HPDMA"), None)
    hpdma_block = hpdma_region.raw_dump_block if hpdma_region else None
    chans = parse_hpdma_block(hpdma_block) if hpdma_block else []
    isr_slots = collect_hpdma_isr_slots(elf, memmap) if chans else []
    out: list[str] = []
    out.extend(render_hpdma_channels(chans))
    out.extend(render_hpdma_isr_callbacks(elf, isr_slots))
    out.extend(render_hspl_states(peri))
    return "\n".join(out) + "\n"


# ============================================================================
# Helpers used by report.py — return a compact summary for the markdown.
# ============================================================================
@dataclass
class DecoderSummary:
    hpdma_active_channels: list[int] = field(default_factory=list)
    hpdma_finish_int_channels: list[int] = field(default_factory=list)
    hpdma_busy_int_channels: list[int] = field(default_factory=list)
    hpdma_finish_callback: dict[int, str] = field(default_factory=dict)
    hpdma_channel_summary: dict[int, str] = field(default_factory=dict)
    hspl_locked: list[tuple[int, int, str, int]] = field(default_factory=list)  # (hspl_id, ch, res, owner)
    hpdma_block_present: bool = False


def summarise_for_report(elf: Path, peri: PeriReport, memmap) -> DecoderSummary:
    summary = DecoderSummary()
    hpdma_region = next((r for r in peri.regions if r.name == "HPDMA"), None)
    block = hpdma_region.raw_dump_block if hpdma_region else None
    chans: list[HpdmaChannel] = parse_hpdma_block(block) if block else []
    summary.hpdma_block_present = bool(chans)
    for c in chans:
        if not c.is_programmed:
            continue
        summary.hpdma_active_channels.append(c.ch)
        if c.status_finish_int:
            summary.hpdma_finish_int_channels.append(c.ch)
        if c.status_bus_err_int or c.status_fifo_err_int:
            summary.hpdma_busy_int_channels.append(c.ch)
        src_lbl = _addr_label(c.src_start_addr)
        dst_lbl = _addr_label(c.dest_start_addr)
        row_bytes = c.src_xsize_v * (1 << c.src_width) if c.src_width < 5 else 0
        summary.hpdma_channel_summary[c.ch] = (
            f"src=0x{c.src_start_addr:08x}({src_lbl})  "
            f"dst=0x{c.dest_start_addr:08x}({dst_lbl})  "
            f"row≈{row_bytes}B × {c.src_ysize_v} rows  "
            f"status=0x{c.status:08x}"
        )

    if chans:
        slots = collect_hpdma_isr_slots(elf, memmap)
        if slots:
            addrs = [s.callback for s in slots if s.callback]
            resolved = addr2line(elf, addrs) if addrs else []
            resolved_map = dict(zip([f"0x{a:08x}" for a in addrs], resolved))
            for s in slots:
                if s.array_name != "s_hpdma_finish_isr" or not s.callback:
                    continue
                sym = resolved_map.get(f"0x{s.callback:08x}", "??")
                summary.hpdma_finish_callback[s.ch] = sym.split("\n", 1)[0].strip()

    # HSPL
    for hspl_id, region_name in ((0, "HSPL0_STA"), (1, "HSPL1_STA")):
        region = next((r for r in peri.regions if r.name == region_name), None)
        if region is None:
            continue
        block = region.raw_dump_block or region.data
        if not block:
            continue
        for s in parse_hspl_states(block, hspl_id):
            if not s.owner_valid:
                continue
            summary.hspl_locked.append((hspl_id, s.channel, s.resource, s.owner_id))

    return summary
