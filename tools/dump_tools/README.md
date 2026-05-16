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

