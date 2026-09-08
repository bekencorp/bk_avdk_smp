#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""boot_param_tool.py — 手动构造 / 解析 / 修改 boot_param 分区 (A/B 测试)。

用于摆布 A/B 启动记录测 case（NORMAL / TRIAL / 回滚 / ping-pong 等）。

on-flash 布局是 32 字节的 ab_flag_record_t，与固件 boot_param.h /
打包器 partition.py:process_boot_param() 一致。

boot_param 分区 = 8K = 两个 4K ping-pong sector。
固件取 magic/ver/size/crc 通过且 seq 最大的一份。

物理地址默认按芯片：
  bk7258: 0x7fc000  (secureboot_xip partitions_gen.h)
  bk7259: 0x6d5000
可用 --chip / --offset 覆盖。

注意：当前 BK7258 try 计数在 AON_PMU，不在 flash 的 try_count 字段；
TRIAL 回滚需 warm reset 累加 PMU，--try-count 对回滚阈值基本无效。

典型用法见文件末尾 EXAMPLES，或 `boot_param_tool.py -h`。
"""

import argparse
import os
import struct
import sys
import zlib

# ---- 与 boot_param.h 完全一致的常量 -------------------------------------
AB_FLAG_MAGIC       = 0x31464241      # 'A''B''F''1' 小端
AB_FLAG_STRUCT_VER  = 1
AB_FLAG_RECORD_SIZE = 32
AB_FLAG_CRC_LEN     = 28              # CRC 覆盖 head[0..0x1B]
AB_FLAG_SECTOR      = 0x1000          # 4K，每个 ping-pong 副本占一个扇区
AB_FLAG_COPIES      = 2
AB_TRY_MAX_DEFAULT  = 5
PART_SIZE_DEFAULT   = 0x2000          # 8K
# Chip → boot_param physical offset (from partitions_gen.h)
PART_PHY_OFFSETS = {
    'bk7258': 0x7fc000,
    'bk7259': 0x6d5000,
}
PART_PHY_OFFSET_DEFAULT = PART_PHY_OFFSETS['bk7258']

# head 布局(小端, 28 字节)，crc 单独追加为 <I
#   I magic | H struct_ver | H size | I seq | B exec_slot | B update_slot |
#   B boot_state | B dl_state | B try_max | B try_count | 2s rsvd0 | 8s rsvd1
HEAD_FMT = '<IHHIBBBBBB2s8s'
assert struct.calcsize(HEAD_FMT) == AB_FLAG_CRC_LEN

# 名称 <-> 数值 映射，命令行更友好
SLOTS  = {'A': 0, 'B': 1, '0': 0, '1': 1}
STATES = {'normal': 0x01, 'trial': 0x02, 'confirmed': 0x03}
DLS    = {'idle': 0, 'ongoing': 1, 'done': 2}
STATE_NAME = {v: k for k, v in STATES.items()}
DL_NAME    = {v: k for k, v in DLS.items()}
SLOT_NAME  = {0: 'A', 1: 'B'}


def crc32(data: bytes) -> int:
    """zlib/PKZIP CRC32 (poly 0xEDB88320, init/xor 0xFFFFFFFF).

    与固件 boot_param_crc32() 和打包器 zlib.crc32 一致。
    """
    return zlib.crc32(data) & 0xFFFFFFFF


def build_record(seq, exec_slot, update_slot, boot_state, dl_state,
                 try_max, try_count) -> bytes:
    """按字段打出一条 32 字节记录(head + crc)。reserved 一律 0 且参与 CRC。"""
    head = struct.pack(
        HEAD_FMT,
        AB_FLAG_MAGIC,
        AB_FLAG_STRUCT_VER,
        AB_FLAG_RECORD_SIZE,
        seq & 0xFFFFFFFF,
        exec_slot & 0xFF,
        update_slot & 0xFF,
        boot_state & 0xFF,
        dl_state & 0xFF,
        try_max & 0xFF,
        try_count & 0xFF,
        b'\x00' * 2,      # rsvd0
        b'\x00' * 8,      # rsvd1
    )
    rec = head + struct.pack('<I', crc32(head))
    assert len(rec) == AB_FLAG_RECORD_SIZE
    return rec


def parse_record(rec: bytes) -> dict:
    """解析 32 字节记录，返回字段 dict，并给出 CRC/魔数校验结果。"""
    if len(rec) < AB_FLAG_RECORD_SIZE:
        return {'valid': False, 'reason': 'too short'}
    rec = rec[:AB_FLAG_RECORD_SIZE]
    (magic, struct_ver, size, seq, exec_slot, update_slot,
     boot_state, dl_state, try_max, try_count, _r0, _r1) = struct.unpack(
        HEAD_FMT, rec[:AB_FLAG_CRC_LEN])
    stored_crc = struct.unpack('<I', rec[AB_FLAG_CRC_LEN:AB_FLAG_RECORD_SIZE])[0]
    calc_crc = crc32(rec[:AB_FLAG_CRC_LEN])

    reason = []
    if magic != AB_FLAG_MAGIC:
        reason.append('bad magic')
    if size != AB_FLAG_RECORD_SIZE:
        reason.append('bad size')
    if struct_ver == 0 or struct_ver > AB_FLAG_STRUCT_VER:
        reason.append('bad struct_ver')
    if stored_crc != calc_crc:
        reason.append('bad crc')
    all_ff = rec == b'\xff' * AB_FLAG_RECORD_SIZE

    return {
        'valid': (len(reason) == 0),
        'reason': ','.join(reason) if reason else 'ok',
        'all_ff': all_ff,
        'magic': magic, 'struct_ver': struct_ver, 'size': size, 'seq': seq,
        'exec_slot': exec_slot, 'update_slot': update_slot,
        'boot_state': boot_state, 'dl_state': dl_state,
        'try_max': try_max, 'try_count': try_count,
        'stored_crc': stored_crc, 'calc_crc': calc_crc,
    }


def _fmt_record(f: dict) -> str:
    """把一条记录格式化成单行。"""
    if f.get('all_ff'):
        return '<erased 0xFF sector (empty)>'
    exec_s = SLOT_NAME.get(f['exec_slot'], '?')
    upd_s = SLOT_NAME.get(f['update_slot'], '?')
    st = STATE_NAME.get(f['boot_state'], '?')
    dl = DL_NAME.get(f['dl_state'], '?')
    crc = (f"crc=0x{f['stored_crc']:08x}(ok)" if f['stored_crc'] == f['calc_crc']
           else f"crc=0x{f['stored_crc']:08x}!=calc0x{f['calc_crc']:08x}")
    return (
        f"valid={f['valid']}({f['reason']}) "
        f"seq={f['seq']} exec={f['exec_slot']}({exec_s}) "
        f"update={f['update_slot']}({upd_s}) "
        f"state=0x{f['boot_state']:02x}({st}) "
        f"dl=0x{f['dl_state']:02x}({dl}) "
        f"try={f['try_count']}/{f['try_max']} "
        f"{crc} ver={f['struct_ver']} size={f['size']}"
    )


def build_partition(record: bytes, sector: int, size: int,
                    base: bytes = None) -> bytes:
    """把一条记录放进指定 ping-pong sector。

    base 给出时以它为底(只覆盖目标 sector，另一个 sector 原样保留)，
    否则整个分区先填 0xFF 再写目标 sector。
    """
    if size < AB_FLAG_COPIES * AB_FLAG_SECTOR:
        raise SystemExit(
            f'partition size 0x{size:x} < 0x{AB_FLAG_COPIES*AB_FLAG_SECTOR:x} '
            f'(A/B ping-pong 需要两个 4K 扇区)')
    if sector < 0 or sector >= AB_FLAG_COPIES:
        raise SystemExit(f'sector {sector} 越界 [0..{AB_FLAG_COPIES-1}]')
    if base is not None:
        img = bytearray(base)
        if len(img) < size:
            img += b'\xff' * (size - len(img))
        else:
            img = img[:size]
    else:
        img = bytearray(b'\xff' * size)
    off = sector * AB_FLAG_SECTOR
    img[off:off + len(record)] = record
    return bytes(img)


def _slot(v):
    if v is None:
        return None
    k = str(v).upper() if str(v).upper() in SLOTS else str(v).lower()
    if str(v).upper() in ('A', 'B'):
        return SLOTS[str(v).upper()]
    if str(v) in ('0', '1'):
        return int(v)
    raise SystemExit(f'非法 slot: {v} (用 A/B 或 0/1)')


def _state(v):
    if v is None:
        return None
    s = str(v).lower()
    if s in STATES:
        return STATES[s]
    iv = int(v, 0)
    return iv


def _dl(v):
    if v is None:
        return None
    s = str(v).lower()
    if s in DLS:
        return DLS[s]
    return int(v, 0)


def cmd_build(a):
    rec = build_record(
        seq=a.seq,
        exec_slot=_slot(a.exec_slot),
        update_slot=_slot(a.update_slot) if a.update_slot is not None
        else _slot(a.exec_slot),
        boot_state=_state(a.state),
        dl_state=_dl(a.dl),
        try_max=a.try_max,
        try_count=a.try_count,
    )
    # 默认保留另一个 sector: --out 已存在就以它为底, 只覆盖目标 sector。
    # --fresh 则丢弃原文件, 整片重填 0xFF (两个 sector 都清)。
    base = None
    if not a.fresh and os.path.exists(a.out):
        with open(a.out, 'rb') as fp:
            base = fp.read()
        print(f'[build] 基于已存在的 {a.out} 更新 sector{a.sector} '
              f'(另一个 sector 保持不变; 加 --fresh 可整片重建)')
    img = build_partition(rec, a.sector, a.size, base=base)
    with open(a.out, 'wb') as fp:
        fp.write(img)
    print(f'[build] 写出 {a.out} ({len(img)} 字节 = 0x{len(img):x})，'
          f'记录放在 sector{a.sector} @ +0x{a.sector*AB_FLAG_SECTOR:x}')
    print(_fmt_record(parse_record(rec)))
    off = a.phy_offset
    print(f'\n烧录到设备: 物理地址 0x{off:x} ({a.chip})')
    print(f'  (例) bk_loader/串口工具 download {a.out} @ 0x{off:x}')


def cmd_dump(a):
    with open(a.file, 'rb') as fp:
        data = fp.read()
    print(f'[dump] {a.file} ({len(data)} 字节 = 0x{len(data):x})')
    best_idx, best_seq = -1, -1
    for i in range(AB_FLAG_COPIES):
        off = i * AB_FLAG_SECTOR
        chunk = data[off:off + AB_FLAG_RECORD_SIZE]
        if len(chunk) < AB_FLAG_RECORD_SIZE:
            print(f'sector{i} @+0x{off:x}: <缺失/不足>')
            continue
        f = parse_record(chunk)
        print(f'sector{i} @+0x{off:x}: {_fmt_record(f)}')
        if f['valid'] and f['seq'] > best_seq:
            best_seq, best_idx = f['seq'], i
    print('\n[firmware 选择] ', end='')
    if best_idx < 0:
        print('两个 sector 都无效 -> virgin(固件回落 slot A)')
    else:
        print(f'sector{best_idx} (seq={best_seq} 最大且有效)')


def cmd_edit(a):
    """就地修改现有分区镜像里某个 sector 的字段，重算 CRC 后写回。"""
    with open(a.file, 'rb') as fp:
        data = bytearray(fp.read())
    off = a.sector * AB_FLAG_SECTOR
    cur = parse_record(bytes(data[off:off + AB_FLAG_RECORD_SIZE]))
    if not cur['valid'] and not a.force:
        raise SystemExit(f'sector{a.sector} 当前记录无效 ({cur["reason"]})，'
                         f'加 --force 可强制以默认值重建')

    def pick(new, key, default):
        return new if new is not None else (cur.get(key, default))

    seq = a.seq if a.seq is not None else (cur.get('seq', 0) + 1)  # 默认自增
    rec = build_record(
        seq=seq,
        exec_slot=pick(_slot(a.exec_slot), 'exec_slot', 0),
        update_slot=pick(_slot(a.update_slot), 'update_slot', 0),
        boot_state=pick(_state(a.state), 'boot_state', STATES['normal']),
        dl_state=pick(_dl(a.dl), 'dl_state', 0),
        try_max=pick(a.try_max, 'try_max', AB_TRY_MAX_DEFAULT),
        try_count=pick(a.try_count, 'try_count', 0),
    )
    data[off:off + AB_FLAG_RECORD_SIZE] = rec
    out = a.out or a.file
    with open(out, 'wb') as fp:
        fp.write(data)
    print(f'[edit] sector{a.sector} 已更新 -> {out}')
    print(_fmt_record(parse_record(rec)))


def main():
    p = argparse.ArgumentParser(
        description='boot_param 分区构造/解析/修改工具 (A/B 测试用)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=EXAMPLES)
    p.add_argument('--chip', default='bk7258', choices=sorted(PART_PHY_OFFSETS.keys()),
                   help='芯片(决定默认烧录地址), 默认 bk7258')
    p.add_argument('--offset', type=lambda x: int(x, 0), default=None,
                   help='覆盖 boot_param 物理地址 (默认按 --chip)')
    sub = p.add_subparsers(dest='cmd', required=True)

    # build
    b = sub.add_parser('build', help='按字段生成 8K boot_param.bin')
    b.add_argument('--seq', type=lambda x: int(x, 0), default=1, help='序列号(越大越新), 默认1')
    b.add_argument('--exec-slot', default='A', help='已提交启动槽 A/B(或0/1), 默认A')
    b.add_argument('--update-slot', default=None, help='试启动/OTA目标槽 A/B, 默认=exec-slot')
    b.add_argument('--state', default='normal', help='normal/trial/confirmed(或数值), 默认normal')
    b.add_argument('--dl', default='idle', help='idle/ongoing/done(或数值), 默认idle')
    b.add_argument('--try-max', type=lambda x: int(x, 0), default=AB_TRY_MAX_DEFAULT, help='回滚阈值, 默认5')
    b.add_argument('--try-count', type=lambda x: int(x, 0), default=0,
                   help='flash 字段(当前 BK7258 回滚看 AON_PMU, 此值通常无效)')
    b.add_argument('--sector', type=int, default=0, choices=[0, 1], help='记录写入哪个ping-pong扇区, 默认0')
    b.add_argument('--size', type=lambda x: int(x, 0), default=PART_SIZE_DEFAULT, help='分区大小, 默认0x2000')
    b.add_argument('--out', default='boot_param.bin', help='输出文件, 默认boot_param.bin')
    b.add_argument('--fresh', action='store_true', help='整片重建(两个sector都清0xFF); 默认在已存在文件上保留另一sector')
    b.set_defaults(func=cmd_build)

    # dump
    d = sub.add_parser('dump', help='解析现有 boot_param.bin (两个扇区)')
    d.add_argument('file', help='boot_param 分区镜像(8K)')
    d.set_defaults(func=cmd_dump)

    # edit
    e = sub.add_parser('edit', help='就地修改某扇区字段(未给的字段保持原值, seq默认自增)')
    e.add_argument('file', help='boot_param 分区镜像(8K)')
    e.add_argument('--sector', type=int, default=0, choices=[0, 1], help='修改哪个扇区, 默认0')
    e.add_argument('--seq', type=lambda x: int(x, 0), default=None, help='不给则在原seq上+1')
    e.add_argument('--exec-slot', default=None, help='A/B')
    e.add_argument('--update-slot', default=None, help='A/B')
    e.add_argument('--state', default=None, help='normal/trial/confirmed')
    e.add_argument('--dl', default=None, help='idle/ongoing/done')
    e.add_argument('--try-max', type=lambda x: int(x, 0), default=None)
    e.add_argument('--try-count', type=lambda x: int(x, 0), default=None)
    e.add_argument('--force', action='store_true', help='原记录无效时用默认值强制重建')
    e.add_argument('--out', default=None, help='输出文件, 默认原地覆盖')
    e.set_defaults(func=cmd_edit)

    a = p.parse_args()
    a.phy_offset = a.offset if a.offset is not None else PART_PHY_OFFSETS[a.chip]
    a.func(a)


EXAMPLES = """\
示例 (BK7258 secureboot_xip A/B):
  # 1) NORMAL 从 A 启动
  boot_param_tool.py --chip bk7258 build --fresh --exec-slot A --state normal --out bp_A.bin

  # 2) NORMAL 强制从 B 启动
  boot_param_tool.py --chip bk7258 build --fresh --exec-slot B --state normal --out bp_B.bin

  # 3) TRIAL 试 B (回滚靠 warm reset 累加 AON_PMU, 不是 --try-count)
  boot_param_tool.py --chip bk7258 build --fresh --exec-slot A --update-slot B \\
      --state trial --try-max 5 --out bp_trial.bin

  # 4) ping-pong: sector1 seq 更大
  boot_param_tool.py --chip bk7258 build --fresh --sector 1 --seq 100 --exec-slot B --out bp_pp.bin

  # 解析
  boot_param_tool.py dump bp_trial.bin

烧录: 8K bin → 物理地址 0x7fc000 (bk7258) / 0x6d5000 (bk7259)。
"""


if __name__ == '__main__':
    sys.exit(main())
