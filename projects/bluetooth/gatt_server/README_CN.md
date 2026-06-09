# GATT Server 示例工程 (BK7259)

* [English](./README.md)

## 1. 项目概述

本工程用于演示 BK7259 SMP 平台上的 Bluetooth GATT Server。BLE Host 运行在 AP 侧，Controller 运行在 CP 侧。

本示例包含：

- BLE 广播参数配置以及广播启动/停止控制
- 服务 UUID 为 `0xFA00` 的 GATT 数据库
- 示例特征值的 ATT read/write 处理
- Client 使能 CCCD 后，通过 CLI 触发 notify
- `.it.csv` 集成测试用例入口

源码路径：`projects/bluetooth/gatt_server/ap/gatt_server_demo.c`

### 1.1 测试环境

- 硬件：BK7259 开发板
- 对端设备：另一块运行 `projects/bluetooth/gatt_client` 的开发板，或 nRF Connect 等手机 BLE 工具
- 串口：UART0 用于烧录、日志和 CLI
- 固件输出：`build/bk7259/gatt_server/package/all-app.bin`

### 1.2 UUID 说明

广播数据和连接后的 GATT 数据库使用不同 UUID：

- 广播 Service Data UUID：`0xFE01`
- GATT Primary Service UUID：`0xFA00`
- Notify 特征 UUID：`0xEA01`，带 CCCD
- Read/Write 特征 UUID：`0xEA05`、`0xEA06`、`0xEA07`
- Write-only 特征 UUID：`0xEA02`

广播名称为 `BK_XXYYZZ`，由 BLE MAC 地址派生。

## 2. 目录结构

```text
gatt_server/
├── README.md
├── README_CN.md
├── .ci                         # CI 编译命令
├── .it.csv                     # 集成测试用例
├── ap/
│   ├── ap_main.c
│   ├── gatt_server_demo.c
│   ├── gatt_server_demo.h
│   └── CMakeLists.txt          # 复制 .it.csv 到 build 目录
├── cp/
├── partitions/
└── 配置文件
```

## 3. 功能说明

- 上电后自动启动广播
- CLI 命令组：`ap_cmd ble_gatts`
- 手动控制广播：`adv_en 1` / `adv_en 0`
- Notify 测试命令：`notify`
- Bonding 命令：`bond`
- BK7258 示例中的 GATT Authorization 功能在 BK7259 上不支持，因此本工程未包含

Central 设备连接后，协议栈会自动停止广播。断开连接后，本示例不会自动恢复广播；如需再次连接，请执行 `ap_cmd ble_gatts adv_en 1` 或重启开发板。

## 4. 编译与运行

### 4.1 编译方法

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=bluetooth/gatt_server
```

Docker 编译：

```bash
./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_server
```

工程 `.ci` 文件中保存了 CI 使用的 docker 编译命令：

```text
./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_server
```

### 4.2 烧录

使用 BKFIL 烧录生成的镜像：

```text
build/bk7259/gatt_server/package/all-app.bin
```

### 4.3 CLI 命令

BK7259 SMP 上 AP 侧命令必须带 `ap_cmd` 前缀：

```text
ap_cmd ble_gatts help
ap_cmd ble_gatts adv_en 1
ap_cmd ble_gatts adv_en 0
ap_cmd ble_gatts notify
ap_cmd ble_gatts bond
```

命令提交成功时返回：

```text
BLE GATTS RSP:OK
```

命令提交失败时返回：

```text
BLE GATTS RSP:ERROR
```

### 4.4 如何判断测试成功或失败

重启后，出现以下日志表示 server 初始化完成：

```text
gatt_server_demo_init success
```

典型广播成功日志包括：

```text
create gatt db success
set adv paramters success
set adv data success
start adv success
```

手动 CLI 测试中，`BLE GATTS RSP:OK` 表示命令已被接受。`notify` 和 `bond` 需要处于已连接状态，否则会返回 `BLE GATTS RSP:ERROR`。

### 4.5 集成测试命令

`.it.csv` 是运行时集成测试入口。测试平台会逐行读取用例，向指定设备串口发送“测试命令”，在超时时间内等待“期望结果”字符串；匹配到期望字符串即判定该 case 通过。

当前用例包括：

```text
ap_cmd ble_gatts help
ap_cmd ble_gatts adv_en 0
ap_cmd ble_gatts adv_en 1
```

期望结果包括：

```text
BLE GATTS RSP:OK
```

当前静态 CSV 覆盖稳定的单板冒烟测试。完整的 GATT read/write/notify 验证需要第二块板，并且 client 连接地址需要从 scan 日志动态获取，因此不适合在 `.it.csv` 里硬编码，文档中以双板手动流程说明。

## 5. 与 GATT Client 联调

1. A 板烧录 `projects/bluetooth/gatt_server`。
2. B 板烧录 `projects/bluetooth/gatt_client`。
3. A 板确认出现 `gatt_server_demo_init success` 和 `start adv success`。
4. B 板执行 `ap_cmd ble_gattc scan 1`。
5. B 板从扫描日志中获取 `adv_addr`，执行 `ap_cmd ble_gattc conn <adv_addr>`。
6. B 板等待发现服务 `0xFA00` 的日志。
7. B 板执行 `ap_cmd ble_gattc notifyindcate_en 1 <ccc_handle>` 使能 notify。
8. A 板执行 `ap_cmd ble_gatts notify`。

## 6. 注意事项

1. AP 侧日志可能带有 `ap0:` 或 `ap1:` 前缀。
2. CP 侧日志可能不带 AP 前缀。
3. BK7259 SMP 上所有 AP 侧 Bluetooth CLI 命令都需要使用 `ap_cmd`。
4. Client 断开后，server 需要执行 `ap_cmd ble_gatts adv_en 1` 才能再次被连接。
