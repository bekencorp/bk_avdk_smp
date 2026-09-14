# Wi-Fi Bridge 工程中文说明

* [English](./README.md)

本工程在 BK7258 SMP 上演示 Wi-Fi 桥接：STA 连接上游 AP 后创建桥接 SoftAP，下行站点通过 lwIP `br0` 与上游处于同一二层域。烧录后在 AP 核串口 CLI 控制。

## 1. 目录结构
```
bridge/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── app.rst                        # Sphinx 工程说明
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # AP 入口；桥接由 CLI 驱动
│   └── config/                    # BK7258 AP 侧配置（CONFIG_BRIDGE=y）
├── cp/                            # CP 核代码
│   ├── cp_main.c                  # CP 入口；拉起 AP 核
│   └── config/                    # BK7258 CP 侧配置（CONFIG_BRIDGE=y）
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- STA 上行 + SoftAP 下行桥接。
- 未指定 `bridge_ssid` 时，SoftAP SSID 默认为 `<sta_ssid>_brr`。
- 可选密码（`0` 表示上游为开放网络）以及 close 时是否保持 STA（`keep_sta`）。
- STA 获取 IP 后，在 Wi-Fi 驱动中继续完成桥接建链。
- 可用 `state` 查询链路状态。

## 3. 硬件与配置
- 硬件：BK7258 SMP 开发板；UART0 作为 AP 核 CLI。
- 软件：AP（`ap/config/bk7258_ap/config`）与 CP（`cp/config/bk7258/config`）均需 `CONFIG_BRIDGE=y`，两侧必须一致。
- 上游 AP 按普通 STA 方式连接即可。

## 4. 编译与烧录
```
make bk7258 PROJECT=wifi/bridge
```

按 SDK 烧录工具烧录 AP/CP 镜像后复位开发板。

## 5. 运行流程
1. 连接 AP 核串口 CLI。
2. 开启桥接（STA 加入上游 AP，随后创建 SoftAP）：

       bridge open <upstream_ssid> <password>
       bridge open <upstream_ssid> <password> <bridge_softap_ssid>
       bridge open <upstream_ssid> 0 <bridge_softap_ssid>
       bridge open <upstream_ssid> <password> <bridge_softap_ssid> 1

3. 查询状态：

       state

4. 关闭桥接：

       bridge close

5. 用手机或 PC 连接桥接 SoftAP，通过上游局域网 ping 验证。

## 6. 常见问题
- **`bridge open` 立刻失败**：确认 AP、CP 均已打开 `CONFIG_BRIDGE=y`，且烧录的是本工程固件。
- **STA 拿不到 IP / SoftAP 未起来**：检查上游 SSID/密码、射频环境，以及 `bk_bridge_fsm` / 建链线程日志。
- **`state` 显示 bridge down**：等待 GOT_IP；若建链回滚，根据串口日志定位失败步骤。
- **下行设备无法访问外网**：确认已加入 SoftAP（默认 `<sta_ssid>_brr`），且上游 AP 允许多 STA。
