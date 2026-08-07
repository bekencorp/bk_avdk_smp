# BK7259 Dump 主机自动化工具

本目录只依赖 Python 3.8+ 标准库。`run_dump_test.py` 不直接访问串口或 HTTP，
所有设备操作均通过配置指定的 `view_log.sh`，且 subprocess 不使用 shell。

## 安全配置

复制示例（真实配置不要提交）：

```bash
cp config.example.json config.json
chmod 600 config.json
```

填写串口、测试 Wi-Fi 和运行参数。密码不会写入报告或控制台；由于
`view_log.sh` 会记录 TX，执行器会在每次调用后原子替换日志中的密码（包括设备
回显）。密码仍不可避免地短暂出现在子进程参数中，因此只能在可信测试主机运行。

Flash 用例必须同时满足：

1. `flash.enabled=true`
2. `flash.confirmed_safe=true`
3. `flash.address` 为合法非零地址

运行前仍须人工对照当前固件分区表，确认整个 `[address, address+length)` 不属于
bootloader、固件、OTA、分区表、校准区或文件系统元数据。程序启动时会再次打印
Flash 地址和长度。QSPI、文件系统及需要外部 SoftAP peer/特殊 CLI 的案例也由
`requires` 自动跳过；只有实际具备能力时才启用对应配置节。

Flash 压测不是“写完再 Dump”：执行器先启动设备端
`ap_dump_flash_load` 后台任务，在确认 `DUMP_LOAD_READY` 后以 10–300 ms
随机延迟触发 AP/CP Dump，从而覆盖擦除、写入或回读校验窗口。设备端只接受非零、
4 KiB 对齐、长度为 4 KiB 整数倍且不超过 64 KiB 的区域，并将单次负载限制为
16 轮；这些检查不能替代人工分区确认。

## Fault 模式

AP 和 CP 的 Dump CLI 保留 `task_assert`、`isr_assert`、`isr_crash`、
`critical_assert`、`critical_crash`，并支持在 task、ISR、critical 三种上下文
触发 `badpc`、`udf`、`divzero`，模式名为 `<context>_<fault>`。例如：

```text
ap_cmd ap_dump_test CASE_001 isr_udf 0
ap_cmd ap_dump_test CASE_002 isr_divzero 1
cp_dump_test CASE_003 critical_badpc
```

AP1 的 ISR 模式仍由 AP0 发起 IPI，在 AP1 IPI callback 上下文触发。`badpc`
调用 0 地址函数指针；`udf` 执行 GCC ARM `udf` 指令；`divzero` 使用 volatile
操作数，依赖系统初始化已启用的 divide-by-zero trap。

## Heartbeat 与 Owner/Follower

固定用例还覆盖两个 heartbeat timeout 方向，以及 AP0/CP、CP/AP0、
AP0/AP1、AP1/AP0 四个确定性 owner/follower 组合；AP0/AP1 同时争抢 owner
是非 mandatory 加权用例。每个编排用例在重启后读取 retained status，并校验
`scenario/phase/winner/follower`、owner 的 `follower_wait=1`、完整 Dump 数量恰好
为 1，以及 follower 未产生第二份 Dump。

case 可用 `dump_end_expect` 和 `dump_timeout_s` 覆盖默认结束标志和超时。
Heartbeat 命令通过 `skip_dump_cli_ready` 跳过不适用的 Dump CLI 探测。

## 先检查计划

`--dry-run` 会完成配置安全校验、固定/随机选择和报告生成，但绝不调用设备：

```bash
python3 run_dump_test.py --config config.json --dry-run
```

固定 mandatory 用例始终先按 `fixed_repeats` 执行，之后才按 `weight` 随机抽取；
同一个 `random_seed` 可复现选择和随机等待/长 ping 窗口。

## 短跑和夜跑

短跑建议把 `fixed_repeats` 设为 1、`random_cases` 设为 0，并把随机等待设为 0。

```bash
python3 run_dump_test.py --config config.json
```

夜跑可使用示例默认的 3 次固定基础用例和 50 次加权随机用例。每个 case 有独立
日志，同一 case 的准备、触发、Dump、重启、恢复都追加到该日志。输出目录包含：

- `result.json`：完整结构化结果
- `result.csv`：每 case 一行
- `report.md`：晨间摘要与失败日志位置
- `report.html`：自包含、可打印的彩色明细报告（无需 JavaScript 或第三方资源）
- `NNNN_CASE_ID.log`：原始且已脱敏日志

SIGINT/SIGTERM 会停止选择新 case，并保存部分报告。

## 离线分析和 golden manifest

```bash
python3 dump_integrity.py case.log --target AP --core 1 \
  --boot-expect 'cli.*ready' --recovery-expect 'got ip'

python3 dump_integrity.py baseline.log --target CP \
  --boot-expect 'cli.*ready' --generate-golden golden.json

python3 dump_integrity.py case.log --target CP \
  --boot-expect 'cli.*ready' --golden golden.json

python3 run_dump_test.py --analyze-log case.log --target AP --core 0

# 不连接硬件，按当前 cases/config 重新统计既有 run 并覆盖四种报告
python3 run_dump_test.py --config config.json --cases cases.json \
  --reanalyze-run logs/dump_test/20260727_203559
```

golden 应来自同一 build、target 和 power state 的多份人工确认完整 Dump；生成后
建议审核并合并稳定 region 项。解析器只接收 `view_log` 的 RX 行，忽略 TX/SYS；
无方向前缀的原始固件日志按 RX 处理。

## 判定口径

主结果只回答“实际产生的 Dump 是否完整”。`dump_pass`、`boot_pass`、
`recovery_pass` 独立计算，Dump 主结果优先级为：

`HOST_ERROR` → `NOT_TRIGGERED` → `DUMP_INCOMPLETE` → `DATA_CORRUPT` →
`REGION_MISMATCH` → `PASS`。

`PHYS GAP`、`BUFFER OVERFLOW`、连接/工具错误均为 `HOST_ERROR`。准备窗口未确认是
`PRECONDITION_FAILED`；功能关闭或 Flash 不安全分别为 `SKIPPED_DISABLED` /
`SKIPPED_UNSAFE`。`DUMP_TEST_REJECT ... reason=timer_busy` 等拒绝会保留在
`trigger_reject_reason`，没有物理 Dump 时仍归类为 `NOT_TRIGGERED`。

物理 target 不采信 `DUMP_TEST_BEGIN` metadata。普通 direct dump 保持原判定；
AP heartbeat timeout 中 CP 接管并 Dump AP 时记录
`actual_dumper=CP, actual_target=AP`，CP heartbeat timeout 中 AP observer
Dump CP 时记录 `actual_dumper=AP, actual_target=CP`。metadata 只记录为
`requested_target/core/mode`，
case 定义记录为 `expected_*`，物理内容记录为 `actual_*`。兼容字段
`target/core/fault_reason` 与 `actual_*` 含义相同。

target/core/mode/reason 不符只写入 `warnings` 和对应 match 字段，不影响
`dump_pass`；boot/recovery pattern 未命中也只改变独立 flag 并产生 warning。
`dump_pass` 仅由采集可靠性、必需 marker/region 完整性、Base64/CRC 和
region 覆盖/golden manifest 决定。

完整性分析还会从 mode 推导预期 `@Dump-reason`：assert 对应 `Assert`，
udf/divzero 对应 `UsageFault`，badpc/crash 接受 `MemFault`、`BusFault` 或
`HardFault`。reason 前可有日志前缀且匹配不区分大小写；reason 与 mode 不符时
只产生 warning，并在 `actual_fault_reason` 字段保留原始原因文本。

报告除原有 Dump/target/recovery 指标外，还包含 `winner_mismatch`、
`status_fail`、`follower_silent_fail` 和 heartbeat takeover pass/total。
`dump_fail` 只统计实际有 Dump 但完整性失败的项；
`NOT_TRIGGERED`、`HOST_ERROR`、precondition/skip 不进入 `dump_eligible`，
通过率为 `dump_pass / dump_eligible`。target 不匹配的 Dump 不计入业务
`recovery_fail`，其 recovery flag/warning 仍完整保留。

AP 与 CP 的结束规则不同：AP 必须包含 `AP memory dump begin` 和
`AP memory dump end`，不要求 `user except handler end`；CP 必须到达
`user except handler end`。Heartbeat profile 额外要求稳定 timeout/observer
marker；CP-hang observer 以 `CP memory dump end` 为结束。boot/recovery
只接受各自结束行之后出现的匹配。
Region 内纯空白 RX 行不作为 payload，也不参与 Base64/CRC 统计。

`--reanalyze-run` 只读取既有原始日志，按每条记录对应 case 的 target/core/mode
重新执行完整性分析；它不会修改 `.log`。原 run 的 `started_at`/`finished_at`
保持不变，并新增 `reanalyzed_at`。case 的起止时间优先从日志首尾
`[HH:MM:SS.mmm]` 推导。

## 自测

```bash
python3 tests/test_dump_integrity.py -v
python3 -m py_compile dump_integrity.py run_dump_test.py tests/test_dump_integrity.py
```
