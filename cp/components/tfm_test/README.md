# boot_param_tool.py — A/B 启动记录测试工具

在 OTA 代码就绪前，用来**手动构造 / 解析 / 修改** `boot_param` 分区里的 A/B 启动记录，
驱动阶段3 的各种启动场景（正常启动、指定 exec_slot、TRIAL 试启动、回滚、ping-pong 选新等）。

- 纯 Python3 标准库，无第三方依赖。
- 生成的记录 CRC 用 `zlib.crc32`，与固件 `boot_param_crc32()` 和打包器
  `beken_utils/scripts/partition.py:process_boot_param()` **逐字节一致**，因此工具烧的记录固件一定认。

---

## 1. 背景：boot_param 分区

| 项 | 值 |
| --- | --- |
| 物理地址 (bk7259) | `0x6d5000` |
| 分区大小 | `0x2000` (8K) = 两个 4K ping-pong 扇区 |
| 记录大小 | 32 字节 (`ab_flag_record_t`) |

分区 = **两个 4K ping-pong 扇区**，各存一份 32 字节记录：

- 启动时固件读两个扇区，取 `magic/struct_ver/size/crc` 校验通过、且 `seq` 最大 的那份为当前有效记录。
- 提交(commit)时写“**另一个**”扇区（`seq+1`），所以掉电不会毁掉当前有效副本。
- 全 `0xFF`（擦除态）视为无效；两个扇区都无效时固件回落到 slot A。

### 记录布局（32 字节，小端）

单一事实源：固件 `cp/.../common/bl2/boot_param.h`。改布局须同步固件 + 打包器 + 本工具，并 bump `struct_ver`。

| 偏移 | 字段 | 类型 | 说明 |
| --- | --- | --- | --- |
| 0x00 | magic | u32 | `0x31464241` ('A''B''F''1') |
| 0x04 | struct_ver | u16 | `1` |
| 0x06 | size | u16 | `32` |
| 0x08 | seq | u32 | 序列号，越大越新（ping-pong 选最大） |
| 0x0C | exec_slot | u8 | 已提交的启动槽 A=0 / B=1 |
| 0x0D | update_slot | u8 | 试启动 / OTA 目标槽 |
| 0x0E | boot_state | u8 | `1`=NORMAL `2`=TRIAL `3`=CONFIRMED |
| 0x0F | dl_state | u8 | `0`=IDLE `1`=ONGOING `2`=DONE |
| 0x10 | try_max | u8 | 回滚阈值（默认 5） |
| 0x11 | try_count | u8 | 当前试启动计数 |
| 0x12 | rsvd0[2] | u8×2 | 保留，置 0，参与 CRC |
| 0x14 | rsvd1[8] | u8×8 | 保留，置 0，参与 CRC |
| 0x1C | crc32 | u32 | 覆盖 `[0x00..0x1B]`（前 28 字节），zlib/PKZIP |

字段取值可用**名字或数值**：
- slot：`A`/`B` 或 `0`/`1`
- state：`normal`/`trial`/`confirmed` 或数值
- dl：`idle`/`ongoing`/`done` 或数值

---

## 2. 命令

```bash
python3 boot_param_tool.py <build|dump|edit> [选项]
python3 boot_param_tool.py -h        # 顶层帮助 + 全部示例
python3 boot_param_tool.py build -h  # 子命令帮助
```

### 2.1 `build` — 按字段生成 8K 分区镜像

| 选项 | 默认 | 说明 |
| --- | --- | --- |
| `--seq` | `1` | 序列号（越大越新） |
| `--exec-slot` | `A` | 已提交启动槽 |
| `--update-slot` | =exec-slot | 试启动 / OTA 目标槽 |
| `--state` | `normal` | normal/trial/confirmed |
| `--dl` | `idle` | idle/ongoing/done |
| `--try-max` | `3` | 回滚阈值 |
| `--try-count` | `0` | 当前试启动计数 |
| `--sector` | `0` | 记录写入哪个 ping-pong 扇区（0/1） |
| `--size` | `0x2000` | 分区大小 |
| `--out` | `boot_param.bin` | 输出文件 |

### 2.2 `dump` — 解析现有 8K 镜像

打印两个扇区的字段、CRC 校验结果，并算出**固件会选哪个扇区**：

```bash
python3 boot_param_tool.py dump boot_param.bin
```

### 2.3 `edit` — 就地改某扇区字段

未指定的字段保留原值，`--seq` 不给则在原值上 **+1**（模拟一次 commit）；原记录无效时加 `--force` 用默认值重建。

```bash
python3 boot_param_tool.py edit boot_param.bin --exec-slot B --update-slot B
```

---

## 3. 阶段3 测试场景

| 场景 | 命令 | 期望现象 |
| --- | --- | --- |
| 出厂/正常，从 A 启动 | `build --exec-slot A --state normal --out bp_A.bin` | BL2 `decide: NORMAL -> exec_slot 0`，从 slot A 启动 |
| 强制从 B 启动 | `build --exec-slot B --out bp_B.bin` | 从 secondary 槽经 remap 启动 |
| TRIAL 试启动 B | `build --exec-slot A --update-slot B --state trial --try-count 0 --out bp_trial.bin` | 每次上电 `try_count++` |
| TRIAL 即将耗尽 | `build --exec-slot A --update-slot B --state trial --try-count 2 --out bp_last.bin` | 下次上电触发回滚到 exec_slot A |
| ping-pong 选新 | `build --sector 1 --seq 100 --exec-slot B --out bp_pp.bin` | 固件选 sector1（seq 大） |

### 推荐验证流程

1. `build` 目标态 → 烧到 `0x6d5000` → 上电看 BL2 日志
   `boot_param ... decide ...` 与 `boot_validate_slot ... slot N valid`。
2. **回滚闭环**：故意把 preferred 槽镜像烧坏 1 字节 + `build` 让 preferred 指向坏槽 →
   上电应 fallback 到好槽并触发 `boot_param_reconcile_booted` 落库 → 复位后把 flash 里的
   `boot_param` 读回，`dump` 确认 `exec_slot` 已切到好槽、`seq` +1。
3. **ping-pong**：两个扇区都写有效记录、不同 seq，确认固件选大的。

---

## 4. 烧录 / 读回

- **烧录**：把生成的 8K bin 写到物理地址 `0x6d5000`（`build` 每次也会打印提示）。
  用 bk_loader / 串口工具 download 到该地址即可。
- **读回**：从设备把 `0x6d5000` 起 8K 读出为 bin，再 `dump` 解析，验证固件实际写回的记录。

---

## 5. 一致性约束

改动记录布局时，以下三处必须同步、并 bump `struct_ver`：

1. 固件 `cp/.../common/bl2/boot_param.h`（单一事实源）
2. 打包器 `tools/env_tools/beken_utils/scripts/partition.py:process_boot_param()`
3. 本工具 `boot_param_tool.py` 顶部常量与 `HEAD_FMT`

> 自检：本工具对“出厂默认记录”算出的 CRC = `0xa3fbec29`，与打包器一致。
