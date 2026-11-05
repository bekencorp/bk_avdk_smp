# BK7259 SD 卡测试工程

[English](README.md) | 中文

本工程从 `projects/app` 派生，不包含方案层代码。工程提供一个统一的 `sdcard` CLI 命令，
覆盖 FATFS 挂载/卸载、读写完整性测试、吞吐基准、随机读写、长时间稳定性测试，以及用于
在 CPU/总线负载下测试 SDIO 的后台 PSRAM 负载程序。

## 硬件要求

- BK7259 开发板，SDIO0 1-line（GPIO2/3/4）或 SDIO1 4-line（GPIO14~GPIO19）。
- 已格式化为 FAT 的 SD 卡。
- 115200 波特率串口，用于输入 `ap_cmd` 命令。

SDIO host 和 GPIO 映射在 `ap/config/bk7259_ap/defconfig` 中选择：

- SDIO0 1-line：CLK/CMD/DAT0 = GPIO2/GPIO3/GPIO4
- SDIO1 4-line：CLK/CMD/DAT0~DAT3 = GPIO14~GPIO19
- SD-NAND 电源使能：GPIO53 高有效（仅 SDIO1 模式）
- AP CPU 频率投票：480MHz
- FATFS 卷：`1:`（`sdio_sd`）

## 编译

在 SDK 根目录执行：

```sh
make clean PROJECT=test/sdcard
make bk7259 PROJECT=test/sdcard
```

固件生成路径：

```text
build/bk7259/sdcard/package/all-app.bin
```

## SDIO host 选择

SDIO0 1-line（与 `app` 工程兼容，不占用与 UART0 复用的 SDIO0 DAT1~DAT3）：

```text
CONFIG_SDCARD_HOST_ID=0
CONFIG_SDCARD_BUSWIDTH_4LINE=n
```

SDIO1 4-line（Beken Robot SD-NAND）：

```text
CONFIG_SDCARD_HOST_ID=1
CONFIG_SDCARD_BUSWIDTH_4LINE=y
```

`usr_gpio_cfg.h` 会在编译期选择对应引脚表，并拒绝不支持的 host/bus-width 组合。

## 速度模式

每次 clean build 前修改 `ap/config/bk7259_ap/defconfig`。

Default Speed，20MHz：

```text
CONFIG_SDCARD_HIGH_SPEED=n
CONFIG_SDCARD_CLOCK_FREQ_HZ=20000000
```

High Speed，40MHz：

```text
CONFIG_SDCARD_HIGH_SPEED=y
CONFIG_SDCARD_CLOCK_FREQ_HZ=40000000
```

## 命令

所有命令都通过 AP CLI 执行：

```text
ap_cmd sdcard <sub command>
```

| 命令 | 说明 |
|---|---|
| `sdcard mount` / `sdcard umount` | 挂载/卸载 `1:`，输出 FRESULT 和耗时 |
| `sdcard card` | 卡类型、容量、状态和当前 SDIO 配置 |
| `sdcard info` | 挂载状态、总容量/剩余/已用空间和簇大小 |
| `sdcard write <file> <size_kb> [sync] [seed]` | 纯写测试；`sync=1` 时每 64KB 调一次 `f_sync` |
| `sdcard read <file> [size_kb] [seed]` | 纯读测试；省略 `size_kb`（或传 0）读整个文件；给 `seed` 时校验 `write` 写入的数据 |
| `sdcard rwtest [size_kb] [rounds]` | 写 + 读回 + 校验循环，结束后删除临时文件 |
| `sdcard randrw [size_kb] [chunk_kb] [rounds]` | 随机偏移写 + 读回 + 校验，输出 MB/s 和 IOPS |
| `sdcard mounttest [rounds] [delay_ms]` | 反复 mount/unmount，输出每轮耗时和成功/失败计数 |
| `sdcard bench [size_mb] [rounds] [drive_capacity]` | 顺序写/读吞吐基准 |
| `sdcard soak start [size_kb] [duration_s]` | 后台写/读/校验稳定性测试；`duration_s=0` 表示直到手动停止 |
| `sdcard soak stop` / `sdcard soak status` | 停止 soak 或打印其统计 |
| `sdcard load start [work_kb] [mode] [chunk_kb]` | 启动 PSRAM/CPU/总线负载；mode `1=copy 2=fill 3=read` |
| `sdcard load stop` / `sdcard load status` | 停止或查看负载程序 |
| `sdcard loadtest [size_mb] [rounds] [work_kb]` | 一键测试：启动负载 → bench → 1MB 校验 → 停止负载 |
| `sdcard status` | 挂载、负载、soak 状态 |

### 默认值与限制

| 命令 | 默认值 / 限制 |
|---|---|
| `write` | 必须给 `size_kb`，1..65536；`sync` 0/1，默认 0；`seed` 默认 0x5A |
| `read` | `size_kb` 可选，0..65536（0 或省略 = 整个文件）；`seed` 可选，但必须先给 `size_kb` |
| `rwtest` | `size_kb` 1024、`rounds` 1；size 1..65536，rounds 1..100 |
| `randrw` | 1024KB / 4KB / 1 轮；size 1..65536，chunk 1..64KB，rounds 1..100；chunk ≤ size |
| `mounttest` | 10 轮、100ms 间隔；rounds 1..1000，delay 0..10000ms |
| `bench` | 16MB、3 轮；size 1..64MB，rounds 1..10；drive capacity 默认取 host 配置 |
| `soak` | 1024KB、60 秒；size 1..65536KB，duration 0..86400 秒 |
| `load` | 128KB copy、64KB chunk；work 1..192KB |
| `loadtest` | 16MB、3 轮、128KB 负载 |

### 命令行为说明

- 读校验：先执行 `sdcard write <file> <size_kb> <sync> <seed>`，再执行
  `sdcard read <file> <size_kb> <seed>`，size 和 seed 必须一致。seed 是位置参数，
  因此使用 seed 时不能省略 `size_kb`。
- `mounttest` 会先卸载，然后每轮执行一次真实的 mount + unmount，结束后保持卸载状态。
- `soak` 在后台线程运行。运行期间其他 FS 命令
  （`mount/umount/info/write/read/rwtest/randrw/mounttest/bench/loadtest`）
  会被拒绝；`soak stop` 会在当前 64KB 分块结束后退出。
  `card`、`status` 和 `load` 系列命令仍可使用。
- FS 类命令（`card/info/write/read/rwtest/randrw/bench/soak start/loadtest`）
  会自动挂载 `1:` 并保持挂载；需要释放时执行 `sdcard umount`。
  `mounttest` 会先卸载，结束后保持卸载状态。
- 不要混用 `fatfstest M/U` 和 `sdcard`，因为两者会在同一卷上各自持有 FATFS 对象。
  首次使用未格式化的卡时，仍可用 `fatfstest F 1` 格式化。
- 临时文件：`1:/sdtest_bench.bin`、`1:/sdtest_rw.bin`、`1:/sdtest_rand.bin`、
  `1:/sdtest_soak.bin`，对应测试结束后会自动删除。

## 典型流程

基础完整性和吞吐：

```text
ap_cmd sdcard mount
ap_cmd sdcard info
ap_cmd sdcard rwtest 1024 3
ap_cmd sdcard bench 16 3
ap_cmd sdcard umount
```

纯写、纯读，以及写缓存 flush 影响：

```text
ap_cmd sdcard write test.bin 1024 0 90
ap_cmd sdcard read test.bin 1024 90
ap_cmd sdcard write test.bin 1024 1 90
ap_cmd sdcard read test.bin 1024 90
```

挂载/卸载稳定性：

```text
ap_cmd sdcard mounttest 10 100
```

CPU/总线负载下测试 SDIO：

```text
ap_cmd sdcard load start 128 1 64
ap_cmd sdcard load status
ap_cmd sdcard bench 16 3
ap_cmd sdcard rwtest 1024 1
ap_cmd sdcard load stop
```

一键负载测试：

```text
ap_cmd sdcard loadtest 16 3 128
```

长时间稳定性，可选带负载：

```text
ap_cmd sdcard load start 128 1 64
ap_cmd sdcard soak start 1024 600
ap_cmd sdcard soak status
ap_cmd sdcard soak stop
ap_cmd sdcard load stop
```

所有结果都带 `[SDTEST]` 前缀（模块内部 tag）；吞吐记录包含 `load=on/off`，
方便直接对比有负载/无负载的结果。
