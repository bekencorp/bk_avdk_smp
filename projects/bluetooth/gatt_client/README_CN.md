# GATT Client 示例工程 (BK7259)

* [English](./README.md)

## 1. 项目概述

本工程用于演示 BK7259 SMP 平台上的 Bluetooth GATT Client。BLE Host 运行在 AP 侧，Controller 运行在 CP 侧。

本示例包含：

- BLE 扫描启动/停止控制
- 连接正在广播的 Peripheral
- 连接后自动执行 GATT 服务发现
- ATT read/write、notify CCC 控制、手动发现、bonding 和白名单命令
- `.it.csv` 集成测试用例入口

源码路径：`projects/bluetooth/gatt_client/ap/gatt_client_demo.c`

### 1.1 测试环境

- 硬件：BK7259 开发板
- 对端设备：另一块运行 `projects/bluetooth/gatt_server` 的开发板
- 串口：UART0 用于烧录、日志和 CLI
- 固件输出：`build/bk7259/gatt_client/package/all-app.bin`

## 2. 目录结构

```text
gatt_client/
├── README.md
├── README_CN.md
├── .ci                         # CI 编译命令
├── .it.csv                     # 集成测试用例
├── ap/
│   ├── ap_main.c
│   ├── gatt_client_demo.c
│   ├── gatt_client_demo.h
│   └── CMakeLists.txt          # 复制 .it.csv 到 build 目录
├── cp/
├── partitions/
└── 配置文件
```

## 3. 功能说明

- CLI 命令组：`ap_cmd ble_gattc`
- 启动/停止 BLE 扫描
- 按对端地址连接/断开
- 连接后自动 SDP 发现
- 按 handle 执行 read/write
- 使能/关闭 notify CCC
- 手动发现 service、characteristic、descriptor
- Bonding 和白名单命令

## 4. 编译与运行

### 4.1 编译方法

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=bluetooth/gatt_client
```

Docker 编译：

```bash
./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_client
```

工程 `.ci` 文件中保存了 CI 使用的 docker 编译命令：

```text
./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_client
```

### 4.2 烧录

使用 BKFIL 烧录生成的镜像：

```text
build/bk7259/gatt_client/package/all-app.bin
```

### 4.3 CLI 命令

BK7259 SMP 上 AP 侧命令必须带 `ap_cmd` 前缀。如果不带 `ap_cmd`，串口 shell 会提示 `cmd NOT found: ble_gattc`。

常用命令：

```text
ap_cmd ble_gattc help
ap_cmd ble_gattc scan 1
ap_cmd ble_gattc scan 0
ap_cmd ble_gattc conn <addr> [addr_type]
ap_cmd ble_gattc disconn
ap_cmd ble_gattc read <val_handle_hex>
ap_cmd ble_gattc write <val_handle_hex> <data>
ap_cmd ble_gattc notifyindcate_en <0|1> <desc_handle_hex>
ap_cmd ble_gattc discover_service <sh> <eh> <uuid> <uuid_len>
ap_cmd ble_gattc discover_char <sh> <eh> <uuid> <uuid_len>
ap_cmd ble_gattc discover_desc <sh> <eh>
ap_cmd ble_gattc bond
ap_cmd ble_gattc scan_filter <0|1>
```

命令提交成功时返回：

```text
BLE GATTC RSP:OK
```

命令提交失败时返回：

```text
BLE GATTC RSP:ERROR
```

### 4.4 如何判断测试成功或失败

重启后，出现以下日志表示 client 初始化完成：

```text
gatt_client_demo_init success
```

扫描测试中，`ap_cmd ble_gattc scan 1` 应返回 `BLE GATTC RSP:OK`；如果附近存在 BLE 广播设备，后续可能打印 `ADV_IND` 或 `ADV_NONCONN_IND` 等日志。

与 `gatt_server` 连接测试时，需要等待自动发现日志：

```text
APPC_SERVICE_CONNECTED
==>Get GATT Service UUID:0xFA00
==>Get GATT Characteristic UUID:0xEA01
==>Get GATT Characteristic Description UUID:0x2902
```

请始终使用当前连接日志里打印的 handle。与配套 `gatt_server` 示例连接时，常见 handle 如下：

```text
0xEA01 notify value handle: 0x12
0xEA01 CCC descriptor handle: 0x13
0xEA05 read/write value handle: 0x17
0xEA06 read/write value handle: 0x19
0xEA07 read/write value handle: 0x1B
```

### 4.5 集成测试命令

`.it.csv` 是运行时集成测试入口。测试平台会逐行读取用例，向指定设备串口发送“测试命令”，在超时时间内等待“期望结果”字符串；匹配到期望字符串即判定该 case 通过。

当前用例包括：

```text
ap_cmd ble_gattc help
ap_cmd ble_gattc scan 1
ap_cmd ble_gattc scan 0
```

期望结果包括：

```text
BLE GATTC RSP:OK
```

当前静态 CSV 覆盖稳定的单板冒烟测试。完整的 connect/read/write/notify 验证需要第二块板，并且对端地址需要从 scan 日志动态获取，因此不适合在 `.it.csv` 里硬编码，文档中以双板手动流程说明。

## 5. 与 GATT Server 联调

1. A 板烧录 `projects/bluetooth/gatt_server`。
2. B 板烧录 `projects/bluetooth/gatt_client`。
3. A 板确认出现 `gatt_server_demo_init success` 和 `start adv success`。
4. B 板执行 `ap_cmd ble_gattc scan 1`。
5. B 板从扫描日志中找到 server 的 `adv_addr`。
6. B 板执行 `ap_cmd ble_gattc scan 0`。
7. B 板执行 `ap_cmd ble_gattc conn <adv_addr>`。
8. B 板等待 `APPC_SERVICE_CONNECTED` 以及服务 `0xFA00` 的发现日志。
9. 如果发现日志中的 handle 为 `0x17`，B 板可执行 `ap_cmd ble_gattc write 17 ssid_ab`，再执行 `ap_cmd ble_gattc read 17`。
10. 如果发现日志中的 CCC handle 为 `0x13`，B 板可执行 `ap_cmd ble_gattc notifyindcate_en 1 13`。
11. A 板执行 `ap_cmd ble_gatts notify`。
12. B 板执行 `ap_cmd ble_gattc disconn`。
13. A 板下次连接前执行 `ap_cmd ble_gatts adv_en 1`。

## 6. 对端 MAC 地址说明

推荐直接使用 `ap_cmd ble_gattc scan 1` 打印出的 `adv_addr`。

如果需要手动计算，GATT Server 的 BLE public address 通常等于 flash base MAC 最后一个字节加 1。例如 base MAC 为 `c8:47:8c:b5:09:61` 时，BLE 地址为 `c8:47:8c:b5:09:62`。

## 7. 注意事项

1. AP 侧日志可能带有 `ap0:` 或 `ap1:` 前缀。
2. GATT discovery 在连接成功后会自动执行。
3. 请使用当前连接日志打印出的 handle，不要假设所有固件版本 handle 都固定不变。
4. 断开连接后，server demo 需要执行 `ap_cmd ble_gatts adv_en 1` 才能再次被连接。
