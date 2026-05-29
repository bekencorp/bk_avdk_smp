# AP Dump Analyzer 使用说明

`analyze_ap_dump.py` 用来分析 BK7259 AP 侧异常 dump。它会结合 AP 侧 `app.elf` 和串口导出的 `.DAT` 日志，生成一份 Markdown 报告，帮助快速定位当前任务、调用栈、中断状态和可能卡住的位置。

## 适用场景

当你手上有 AP 侧异常日志时，可以用这个工具辅助分析，例如：

- watchdog 超时
- assert / hard fault
- JPEG decode timeout
- 某个任务疑似卡住
- 多核 AP dump
- 日志中包含 `user except handler begin/end`

## 输入文件

工具主要需要两个文件：

- `app.elf`：用于把地址解析成函数名和源码行号。
- `.DAT` 日志：设备异常或死机时串口导出的 dump 日志。

一定要使用和日志匹配的 `app.elf`。如果 ELF 和日志不是同一次构建产物，地址解析结果可能不准确。

## 基本用法

最简单的方式是把 `app.elf` 和 `.DAT` 放在同一个目录，然后执行：

```bash
python3 /home/gang.peng/bk_avdk_smp_dev_7259v2_version/tools/analyze_ap_dump.py --output report.md
```

工具会自动：

1. 查找当前目录下的 `app.elf`。
2. 查找当前目录下最新的 `.DAT` 或 `.dat`。
3. 默认分析最后一次 dump session。
4. 生成 `report.md`。

## 指定 ELF 和日志

如果文件不在当前目录，或者当前目录里有多个 `app.elf`，可以手动指定：

```bash
python3 /home/gang.peng/bk_avdk_smp_dev_7259v2_version/tools/analyze_ap_dump.py \
  --elf build/bk7259/app/bk7259_ap/app.elf \
  --dat crash_log.DAT \
  --output report.md
```

如果不加 `--output`，报告会直接打印到终端：

```bash
python3 /home/gang.peng/bk_avdk_smp_dev_7259v2_version/tools/analyze_ap_dump.py \
  --elf app.elf \
  --dat crash_log.DAT
```

## 多次 Dump 日志

有些 `.DAT` 文件里可能包含多次重启或多次异常。可以先列出所有 session：

```bash
python3 /home/gang.peng/bk_avdk_smp_dev_7259v2_version/tools/analyze_ap_dump.py \
  --elf app.elf \
  --dat crash_log.DAT \
  --list-sessions
```

输出类似：

```text
[0] lines=1-320 build=2026-04-25 reason=watchdog timeout regions=6 tracebacks=1
[1] lines=500-980 build=2026-04-25 reason=hard fault regions=7 tracebacks=2
```

默认分析最后一次 session。如果要分析第 0 次：

```bash
python3 /home/gang.peng/bk_avdk_smp_dev_7259v2_version/tools/analyze_ap_dump.py \
  --elf app.elf \
  --dat crash_log.DAT \
  --session-index 0 \
  --output report_session0.md
```

## 报告内容

生成的 Markdown 报告主要包含：

- `Summary`：总体结论，包括 dump 原因、build time、可能卡点。
- `Current Core State`：每个 core 当前运行的 TCB，来自 `pxCurrentTCBs`。
- `Task Overview`：从日志中解析出的任务列表。
- `Task Call Chains`：任务调用栈，包括 traceback 和栈扫描恢复出的调用链。
- `Interrupt Analysis`：中断 recorder 状态，哪些 IRQ 已完成，哪些可能未退出。
- `Exception Stack Analysis`：异常时 PC、LR、MSP、PSP 等寄存器解析。
- `Raw Evidence`：关键原始日志行，方便回看证据。

## 常见示例

### 分析当前目录日志

```bash
cd /tmp/ap_dump_case
python3 /home/gang.peng/bk_avdk_smp_dev_7259v2_version/tools/analyze_ap_dump.py \
  --output report.md
```

### 分析构建目录里的 ELF 和单独保存的日志

```bash
python3 /home/gang.peng/bk_avdk_smp_dev_7259v2_version/tools/analyze_ap_dump.py \
  --elf /home/gang.peng/bk_avdk_smp_dev_7259v2_version/build/bk7259/doorbell/bk7259_ap/app.elf \
  --dat /tmp/crash_20260425.DAT \
  --output /tmp/crash_report.md
```

### 只分析第一次 Dump

```bash
python3 /home/gang.peng/bk_avdk_smp_dev_7259v2_version/tools/analyze_ap_dump.py \
  --elf app.elf \
  --dat long_run.DAT \
  --session-index 0 \
  --output first_dump.md
```

## 注意事项

- `app.elf` 必须和 `.DAT` 日志来自同一次或兼容的构建。
- Thumb 返回地址会自动归一化，不需要手动处理 `0x...1` 这类地址。
- 如果当前目录下有多个 `app.elf`，工具会要求使用 `--elf` 明确指定。
- 如果日志里没有 dump memory 区域，部分 `pxCurrentTCBs` 或 stack scan 信息可能无法还原，但 traceback 和寄存器仍会尽量解析。
- 工具优先使用 `arm-none-eabi-nm`、`arm-none-eabi-readelf`、`arm-none-eabi-addr2line`；找不到时会尝试普通 `nm`、`readelf`、`addr2line`。

---

# Heartbeat-timeout 分析工具 (`analyze_heartbeat_timeout.py`)

`analyze_heartbeat_timeout.py` 专门用于 BK7259 **AP 心跳超时**场景的离线分析。
这类 dump 由 CP 端 `mb_ipc_task:292` 心跳超时 assert 触发，AP 侧没有保存
CPU 寄存器，需要通过 SRAM + PSRAM + 外设寄存器快照来还原现场。

它和 `analyze_ap_dump.py` 互补：

| 工具 | 适用场景 |
|---|---|
| `analyze_ap_dump.py` | AP 自己进 `user except handler` 抓的 `.DAT` (watchdog / hardfault / assert) |
| `analyze_heartbeat_timeout.py` | CP 心跳超时主动抓的 dump（AP 还在跑，没存 CPU regs） |

## 输入

- `<log>`：CP 抓的 dump 串口日志（`.log` / `.DAT` 都行）。
- `<elf>`：和 dump 对应的 AP `app.elf`。
  所有符号地址都通过 `arm-none-eabi-nm -S` 实时查询，不需要硬编码。

## 基本用法

```bash
python3 /home/gang.peng/bk_avdk_smp_dev_7259v2_version/tools/dump_tools/analyze_heartbeat_timeout.py \
    /path/to/heartbeat_dump.log /path/to/app.elf
```

默认把 5 个产物写到 `<log_stem>_analysis/`：

| 文件 | 内容 |
|---|---|
| `extract.txt` | FreeRTOS 全局变量、当前任务、task switch recorder、IRQ recorder 末 64 条 |
| `msp_core0.txt` | core0 MSP 栈中的代码地址 + EXC_RETURN 反推的异常帧 |
| `msp_core1.txt` | core1 同上 |
| `peri.txt` | 所有 peripheral register 快照的原始 hex |
| `report.md` | 自动生成的 review 报告（smoking-gun、指纹检测、下一步建议） |

### 只要报告

```bash
python3 .../dump_tools/analyze_heartbeat_timeout.py log elf --report-only \
    -o report.md
```

### 只要原始文件供二次 AI 分析

```bash
python3 .../dump_tools/analyze_heartbeat_timeout.py log elf --no-report \
    -o ./out_for_ai
```

### 报告直接打到 stdout

```bash
python3 .../dump_tools/analyze_heartbeat_timeout.py log elf --report-only \
    --stdout-report
```

## 自动检测的"冒烟点"

工具会在 `report.md` 里高亮以下信号，便于人眼快速复核：

- **IRQ recorder 最后一条 `done=0`** —— ISR 进入未退出，是 AHB hang 的直接指纹。
  发生时进程退出码 = `1`（baseline / 正常 dump 退出码 = `0`，便于 CI 守护脚本判断）。
- **PSRAM0 Reg 0x10 = `0x03xxxxxx`** —— 已观察到所有 hang dump 都命中，idle baseline 为 0。
- **MSP 栈 EXC_RETURN 嵌套帧** —— 优先推荐含 `*_isr` / `bk_int_dispatch` /
  `hpdma_isr` / `vsi_isp_isr` / `dpu_isr` 等 ISR 入口的那一帧作为"innermost active frame"。
- **v2/v3 patch 区段缺失** —— 若 dump 内没有 PPHS / PPRO / SYS_AHBP /
  DPU / GPU / ISP_MI 寄存器，会提示该 build 早于补丁。

## 自适应能力

| 维度 | 自动适配 |
|---|---|
| Build 间符号地址漂移 | 用 `nm -S` 重新解析，不需要改脚本 |
| 中断 recorder size | 用符号 size 计算 `N_entries`（旧版 40 / 新版 512 自动切换） |
| Peripheral 区段重命名 | 0516 的 `PSRAM` 与 0518 的 `PSRAM0` 通过起始地址匹配自动对齐 |
| v2/v3 patch 增补区段 | 表里有就抓，没有就标 `not in dump`，不报错 |

## 退出码

- `0`：无冒烟点（正常 baseline）
- `1`：检测到 ISR 未退出（hang 嫌疑）
- `2`：输入文件 / 工具链错误

## 推荐工作流

1. 抓到一份 heartbeat-timeout dump，先跑工具：

   ```bash
   python3 .../analyze_heartbeat_timeout.py dump.log app.elf
   echo "exit=$?"
   ```

2. 看 `report.md` 第 1、5、6 节确认 smoking gun。
3. 若需要更细粒度的人/AI 复核，把 `extract.txt` + `msp_core*.txt` + `peri.txt`
   送给 AI 做进一步分析。
4. 拿到多份 dump 后，直接 diff `report.md` / `peri.txt` 即可快速看出共性。

