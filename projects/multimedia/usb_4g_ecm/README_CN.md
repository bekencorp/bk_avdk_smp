# USB 4G ECM 上网工程

* [English](./README.md)

## 1. 项目概述

`usb_4g_ecm` 让 BK7259 以 USB host 身份连接 4G 复合设备（VID/PID `3505:0003`），同时跑两条通道：

- **CDC-ECM 网卡**：intf0/intf1 绑定 CherryUSB 的 `usbh_cdc_ecm` 驱动，接到 AP 侧 lwIP，应用直接用标准 socket 上网。
- **vendor AT 通道**：intf3（class `0xFF`/subclass `0x00`/protocol `0x03`）绑定本工程自带的类驱动，与网络并行收发 AT 命令。

两条通道共用一个 USB 控制器、同一次枚举，互不独占。

**不在本工程范围内**：PPP/RNDIS 切换、多模组通用适配框架、Wi-Fi 与 4G 自动选路、NAT 转发、低功耗、音频通话、OTA。AT 层只做到命令串行、响应解析、超时、URC 和断开取消，不是脚本引擎。

## 2. 硬件需求

- SoC/开发板：BK7259 系列开发板。
- 一根 USB 线把开发板的 USB host 口连到 4G 模组。
- host 口 VBUS 必须能给模组供电（`CONFIG_USB_VBAT_CONTROL_GPIO_ID=0x20`）。
- 模组需已插入可用 SIM 卡，且出厂固化为 ECM 模式。
- 串口控制台，用于 CLI 和日志。

## 3. 目录结构

```text
usb_4g_ecm/
├── ap/
│   ├── ap_main.c        # 启动顺序：lwIP → AT 层 → USB host
│   ├── usb_modem_at.c   # intf3 USB 传输层（claim、收线程、串行写）
│   ├── modem_at.c       # AT 管理层（组行、命令串行、结果码、URC）
│   ├── modem_at_cli.c   # 测试入口 modem_at 命令
│   └── config/bk7259_ap/defconfig
├── cp/                  # CP 侧不参与 USB
└── partitions/
```

分层边界：`usb_modem_at.c` 只管 USB（认领接口、把裸字节往上推、串行化 bulk 写），完全不解析 AT；行组装与 OK/ERROR/URC 判定都在 `modem_at.c`。

## 4. 编译

```bash
cd bk_avdk_smp
make bk7259 PROJECT=multimedia/usb_4g_ecm
```

产物：`build/bk7259/usb_4g_ecm/package/all-app.bin`。

改过 `defconfig` 之后必须先清掉旧构建目录，否则旧 config 会反向覆盖新加的行：

```bash
rm -rf build/bk7259/usb_4g_ecm
```

## 5. 启动顺序

`ap_main.c` 的初始化任务按固定顺序执行，顺序本身是必须的：

1. `net_ipv4stack_init()` —— ECM 驱动会把收到的包直接交给 `netif->input`，tcpip 线程和它的 mbox 必须先存在。
2. `modem_at_init()` —— 装上 intf3 传输层回调；类驱动的 connect 路径会调用它们，所以必须早于枚举。
3. `bk_usb_driver_init()` + `bk_usb_open(USB_HOST_MODE)` —— 控制器以 A-host 身份上电，开始枚举。

## 6. AT 通道测试命令

```text
modem_at                   # 查看链路在线状态
modem_at probe             # 跑一遍只读命令：AT / ATI / AT+CPIN? / AT+CSQ / AT+CEREG?
modem_at AT+CGDCONT?       # 发送任意命令（控制台按空格切分，内部会重新拼回）
```

输出形如 `AT+CSQ -> 0` 后跟响应行，其中数字是返回码：

| 返回码 | 含义 |
| --- | --- |
| `0` | 模组回了 `OK` |
| `-1` | 模组报错（`ERROR` / `+CME ERROR` / `+CMS ERROR`），错误行仍在响应里 |
| `-2` | 等结果码超时 |
| `-3` | 通道离线，或等待期间被拔出 |
| `-4` | USB 写失败 |
| `-5` | 已有命令在飞（一次只允许一条） |

## 7. 实现要点

**响应归属**：有命令在飞时，收到的行归这条命令；没有命令在飞时，收到的行是 URC。**不按 `+` 前缀判定** —— `+CSQ: 20,99` 在我们问了的时候是响应，没问的时候是 URC，按前缀判定两种情况必错一种。

**超时后的迟到应答**：命令超时后进入 DRAIN 状态，吞掉迟到的输出（有界 300 ms），避免它被当成下一条命令的应答。

**端点解析**：intf3 的 bulk IN/OUT 从 `altsetting[0]` 的描述符按方向位解析，不硬编码地址。该接口上的 interrupt IN 只记录不提交 URB —— AT 通道不需要通知，省一个管道槽和一个常驻 urb。

**拔插清理**：disconnect 路径先清在线标志和收线程 running 标志（必须在 kill urb 之前），再 kill urb，等收线程确认退出之后才 memset 实例、回收 hubport。

## 8. 关键配置

| 配置项 | 值 | 原因 |
| --- | --- | --- |
| `CONFIG_USBH_CDC_ECM` | `y` | ECM 类驱动，SDK 自带但默认不编 |
| `CONFIG_USB_HOST_FIFO_LAYOUT_4BULK` | `y` | ECM 两个 bulk + AT 两个 bulk，共 4 条 |
| `CONFIG_LWIP_TCPIP_THREAD_STACKSIZE` | `1024` | 默认 512 不够 ECM 的 netif input 路径 |
| `CONFIG_USB_RISCV_BRIDGE` | `y` | BK7259 的 USB 走 RISC-V bridge |

## 9. 资源占用

当前固件（AP 侧，`make` 输出的统计）：FLASH 481836 B（23.07%）、RAM 172676 B（43.91%）、IRAM 1432 B、DTCM 2304 B。

AT 通道自身的增量：rx 缓冲 512 B + tx 缓冲 256 B（均在 `USB_NOCACHE_RAM_SECTION`）、收线程栈 2048 B、行缓冲与响应缓冲各 257 B。

## 10. 已知限制

- 只支持 `3505:0003` 这一款模组，且依赖它出厂为 ECM 模式；换模组需要改匹配表和端点假设。
- intf3 的匹配依赖 protocol `0x03`。同模组 intf2（`0x04`）和 intf4（`0x05`）是诊断口，靠这一位区分；模组若重排 protocol 编号，匹配会失效。
- 单实例：同一时刻只认领一个 AT 接口。
- AT 命令一次一条，不排队，第二个调用方直接拿 `-5`。
- 该模组出厂的 ECM 配置没有字符串描述符，MAC 需要从别处获取。

## 11. 验证状态

**已验证**：构建通过（`EXIT=0`，新文件零告警）；类注册记录确实落在 `usbh_class_info` 段内（段内 3 条：hub、cdc_ecm、modem_at）。

**未验证，需开发板实测**：intf3 绑定是否只命中目标接口；拆包/同包多行/回显/URC/超时的实际行为；拔插与模组重启下的资源清理；ECM 与 AT 并行时的 FIFO 与管道占用；长稳。
