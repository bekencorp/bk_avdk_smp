# Flash/UART 并发专项测试

## 测试范围

- 专用 Flash 分区：`flash_stress`
- 起始地址：`0x500000`
- 长度：`0x4B000`（300KB）
- UART：UART2，GPIO41（TX）与 GPIO40（RX）
- 数据帧：256 字节，包含序号、反码、变化数据和 CRC32
- 默认配置：9600 bps、100 帧、每 2000ms 一帧
- UART RX 使用 DMA；TX 使用 16 字节分块 PIO 并在块间让出调度，避免低波特率下 TX DMA 连续突发造成 FIFO 丢字节
- 发送周期使用 AON RTC 硬件时基计算，避免 Flash 擦写关中断期间 RTOS tick 漏计导致实际周期偏离 2 秒

> `flash_stress` 是破坏性测试分区。测试会反复擦除并重写其中全部数据，不得存放业务数据。

## 硬件连接

1. Debug UART 连接电脑，用于输入命令和查看日志，波特率 115200。
2. 使用跳线短接 GPIO41（UART2 TX）与 GPIO40（UART2 RX）。
3. 若改用外部 UART 对端设备，双方必须共地，并由对端原样回送收到的 256 字节。

## 推荐流程

1. 查询并确认分区：

   `ap_cmd flash_uart_stress info`

2. 冒烟测试（约 20 秒）：

   `ap_cmd flash_uart_stress start 2 9600 10 2000`

3. 查询进度：

   `ap_cmd flash_uart_stress status`

4. 正式测试（约 3 分 20 秒）：

   `ap_cmd flash_uart_stress start 2 9600 100 2000`

5. 可选覆盖另外两档波特率：

   `ap_cmd flash_uart_stress start 2 38400 100 2000`

   `ap_cmd flash_uart_stress start 2 115200 100 2000`

6. 一小时稳定性测试：

   `ap_cmd flash_uart_stress start 2 9600 1800 2000`

7. 需要提前结束时：

   `ap_cmd flash_uart_stress stop`

## 通过判据

最终日志必须为 `result=PASS`，并同时满足：

- `frames` 等于配置的总帧数；
- `tx_err`、`len_err`、`seq_err`、`crc_err`、`data_err`、`extra_rx` 均为 0；
- `schedule_err` 为 0，发送周期最小值和最大值在目标周期 ±100ms 内；
- `passes` 大于 0；
- `api_err` 和 `verify_err` 均为 0；
- `erase`、`write`、`verify` 统计均覆盖至少一次 300KB。

`result=ABORTED` 只表示人工停止，不算通过；任何 `result=FAIL` 都需要保留完整日志分析。
若出现 `result=STUCK`，说明工作任务未能安全退出，程序会保留相关资源以避免越界访问，此时必须保存日志并重启开发板。

## 负向与重复性检查

- 拔掉 GPIO40/GPIO41 跳线后运行，应报告长度或数据错误并最终失败。
- 运行中执行 `stop`，最终应为 `ABORTED`。
- 测试结束后再次执行 `start`，应可正常开始，不应死机或提示遗留任务仍在运行。

## 研发排障实验记录

以下内容用于说明方案选择，不属于最终验收路径：

1. 低波特率 TX DMA 连续发送实验中曾出现 `N-1` 及 256 字节只接收 194 字节的现象。128 字节分块发送可以改善结果，但仍依赖通用 TX DMA 路径，因此最终改为 16 字节分块 PIO；RX 继续使用 DMA。
2. 使用 RTOS tick 计算发送周期时，Flash 擦写临界区会造成实际周期偏移，因此最终改用 AON RTC 作为独立硬件时基。
3. Flash 任务优先级高于控制任务时曾出现调度超限。最终优先级为 RX 高于控制、控制高于 Flash。
4. `CONFIG_SYSTICK_32K=y` 的对照固件曾出现 AP tick 不增长及约 8 秒心跳超时。临时恢复 sleep wakeup ticktimer 开关后现象消失，但该做法与 Gerrit `#95665` 的设计不一致，最终代码没有采用。
5. 最终工程关闭 `CONFIG_SYSTICK_32K`，使用 Cortex-M 内核时钟驱动 SysTick。板上检查确认 tick 持续递增且无心跳超时，需求一、二、三短测均通过。

实验期间使用过 `TXDMA_DIAG`、`RXDMA_DIAG`、心跳时间戳日志、A/B 临时工程及备份文件。这些内容仅用于定位问题，不是正式功能依赖。

## 测试通过后的清理

1. 保留测试固件 SHA256、完整串口日志和最终 PASS 汇总，作为问题单附件。
2. 删除本地和编译服务器上的 `*.bak-*`、`test/platform_pio16_aon_ab/`、临时 BIN、临时构建目录及诊断日志。
3. 对照仓库基线检查并清除通用 TX DMA、心跳诊断、临时 SysTick 寄存器操作等实验性源码差异。
4. `git status --short` 中只应保留评审确认的需求实现和本文档，不应出现备份文件、编译产物、权限变化或换行差异。
5. 清理后重新执行 clean build，并至少运行需求一 10 帧、需求二 3 档波特率和需求三 10 帧冒烟测试。
