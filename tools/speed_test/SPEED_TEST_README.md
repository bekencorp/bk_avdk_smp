# BK7259 4G ECM 自动化测速工具使用说明

## 功能概述

该脚本通过串口连接 BK7259 设备，自动执行 `dltest` 命令进行 4G ECM 下行速度测试，并生成详细的测速报告。

## 核心功能

1. **自动化测试**：执行 20 次 `dltest` 命令
2. **实时监控**：实时显示测试输出
3. **结果解析**：自动解析每次测试的速度和流量
4. **统计报告**：生成详细的统计报告（最小值、最大值、平均值）
5. **文件保存**：自动保存测试报告到文件

## 脚本参数说明

在 `speed_test_monitor.py` 文件开头可以调整以下参数：

```python
SERIAL_PORT = '/dev/cu.usbserial-120'  # 串口设备路径
BAUD_RATE = 115200                     # 运行时波特率
TEST_COUNT = 20                        # 测试次数
INTERVAL_SEC = 5                       # 每次测试间隔（秒）
TEST_DURATION_SEC = 10                 # 每次测试持续时间（秒）
WAIT_TIMEOUT_SEC = 15                  # 等待测试完成超时（秒）
```

## 使用前提

1. **设备连接**：
   - BK7259 已通过 USB 串口连接到电脑
   - 串口设备为 `/dev/cu.usbserial-120` 或 `/dev/tty.usbserial-120`

2. **固件要求**：
   - 已烧录 `usb_4g_ecm` 项目固件
   - 4G 模块已连接并正常工作
   - 设备已获取 IP 地址（通过 DHCP）

3. **重要说明**：
   - BK7259 默认串口连接到 **CP 核心**（提示符为 `#`）
   - `dltest` 命令注册在 **AP 核心**
   - 脚本会自动使用 `ap_cmd dltest` 前缀来调用 AP 侧的测速命令

3. **Python 依赖**：
   ```bash
   pip3 install pyserial
   ```

## 使用步骤

### 1. 检查串口设备

```bash
ls -l /dev/cu.* /dev/tty.*
```

确认你的串口设备路径，如果不是 `/dev/cu.usbserial-120`，请修改脚本中的 `SERIAL_PORT` 参数。

### 2. 运行测试脚本

```bash
cd /Users/soong/Documents/work/ai_work/armino/bk7259
python3 speed_test_monitor.py
```

### 3. 查看实时输出

脚本会实时显示：
- 每次测试的时间戳
- 设备的原始输出
- DNS 解析结果
- 每秒的下行速度
- 平均速度和总字节数

示例输出：
```
============================================================
第 1/20 次测速 [2026-10-23 10:30:15]
============================================================
  dns mirrors.ustc.edu.cn -> 202.38.95.110
  [0-1] sec 12345 Kbits/sec
  [1-2] sec 13456 Kbits/sec
  ...
  [0-10] sec avg 12800 Kbits/sec, 16000000 bytes

✓ #1 [10:30:15] 平均 12800 Kbits/sec, 总共 16000000 bytes
```

### 4. 查看测试总结

所有测试完成后，脚本会显示：
```
======================================================================
测试总结报告 - 2026-10-23 10:35:45
======================================================================

总测试次数: 20
成功: 20 次
失败: 0 次
成功率: 100.0%

下行速度统计:
  最小值: 11500 Kbits/sec (1437.50 KB/s)
  最大值: 13500 Kbits/sec (1687.50 KB/s)
  平均值: 12600 Kbits/sec (1575.00 KB/s)

总数据传输:
  最小值: 14375000 bytes (14038.09 KB)
  最大值: 16875000 bytes (16479.49 KB)
  平均值: 15750000 bytes (15380.86 KB)
```

### 5. 查看保存的报告

报告会自动保存为 `speed_test_report_YYYYMMDD_HHMMSS.txt`，例如：
```bash
cat speed_test_report_20261023_103545.txt
```

## 中断测试

如果需要提前停止测试，按 `Ctrl+C` 即可安全退出。脚本会保存已完成的测试结果。

## 常见问题

### 1. 串口权限问题
```bash
# macOS
sudo chmod 666 /dev/cu.usbserial-120

# Linux
sudo usermod -a -G dialout $USER  # 需要重新登录
```

### 2. 串口被占用
确保没有其他程序（如终端、screen、minicom）正在使用该串口：
```bash
lsof | grep usbserial
```

### 3. 设备无响应
- 检查设备是否已启动并进入 CLI 模式
- 尝试手动发送命令验证：
  ```bash
  screen /dev/cu.usbserial-120 115200
  # 输入 dltest 查看是否有响应
  ```

### 4. 测试超时
可能原因：
- 4G 网络连接不稳定
- 设备未获取到 IP 地址
- 下载服务器不可达

解决方法：
- 增加 `WAIT_TIMEOUT_SEC` 参数
- 手动执行 `dltest` 确认网络是否正常

## 自定义测试参数

### 修改测试次数
```python
TEST_COUNT = 50  # 改为 50 次
```

### 修改测试间隔
```python
INTERVAL_SEC = 10  # 改为 10 秒间隔
```

### 修改超时时间
```python
WAIT_TIMEOUT_SEC = 20  # 改为 20 秒超时
```

## 输出文件说明

生成的报告文件包含：
- 测试配置信息（时间、串口、测试次数）
- 统计信息（成功率、平均速度）
- 每次测试的详细结果

## 脚本架构

- `SpeedTestResult`: 单次测速结果数据类
- `BK7259SpeedMonitor`: 核心监控类
  - `connect()`: 连接串口
  - `send_command()`: 发送命令
  - `read_until_pattern()`: 读取并解析输出
  - `run_single_test()`: 执行单次测试
  - `run_all_tests()`: 执行所有测试
  - `print_summary()`: 打印统计报告
  - `save_report()`: 保存报告到文件

## 相关资料

- BK7259 项目文档: `bk_avdk_smp/projects/multimedia/usb_4g_ecm/README.md`
- 测速源码: `bk_avdk_smp/projects/multimedia/usb_4g_ecm/ap/speed_test.c`
- 官方文档: https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7259/zh_CN/v4.0.1/

## 版本历史

- v1.0 (2026-10-23): 初始版本，支持基本的自动化测速和报告生成
