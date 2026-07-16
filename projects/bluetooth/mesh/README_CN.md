# 蓝牙 Mesh 示例工程

本工程在 BK7259 AP 核运行 Zephyr BLE Host 和 Bluetooth Mesh 协议栈，CP 核运行
BLE controller-only，通过 HCI over IPC 通信。工程移植自 BK7258 Mesh demo，保留
`provisioner` / `provisionee` 双角色和旧 `ble_mesh` CLI。

## 支持能力

- 作为 `provisionee` 加入手机/nRF Mesh 或另一块开发板创建的 Mesh 网络。
- 作为 `provisioner` 创建 Mesh 网络，并自动配网附近的 `provisionee` 节点。
- Generic OnOff Server/Client、Health Server/Client、Config Server/Client。
- Beken vendor count model，用于双板收发计数验证。
- 天猫精灵灯 demo 入口保留：`ble_mesh tmall ...`。
- Host 协议栈：Zephyr v2.7.6 Bluetooth Host。

## 代码结构

- 工程入口：`projects/bluetooth/mesh/`
- AP demo：`projects/bluetooth/mesh/ap/ap_main.c`
- AP Zephyr Host/Mesh：`ap/components/bk_bluetooth/service/mesh/`
- AP HCI over IPC driver：`ap/components/bk_bluetooth/service/mesh/hci_ipc_driver.c`
- AP Mesh CLI：`ap/components/bk_bluetooth/service/mesh/mesh_cli.c`
- CP controller-only：`cp/components/bk_bluetooth/` 和 controller 预编译库

## 关键配置

AP：`projects/bluetooth/mesh/ap/config/bk7259_ap/defconfig`

```text
CONFIG_BLUETOOTH_AP=y
CONFIG_BLUETOOTH_HOST_ONLY=y
CONFIG_BLE=y
# CONFIG_BT is not set
CONFIG_BLE_MESH_ZEPHYR=y
CONFIG_BT_MESH_PB_ADV=y
CONFIG_BT_MESH_PB_GATT=y
CONFIG_BT_MESH_GATT_PROXY=y
CONFIG_ACLSEMI_BT_MESH_DEBUG_LEVEL=3
```

CP：`projects/bluetooth/mesh/cp/config/bk7259/defconfig`

```text
CONFIG_BTDM_CONTROLLER_ONLY=y
CONFIG_BLE=y
# CONFIG_BT is not set
```

## 编译和烧录

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=bluetooth/mesh
```

输出固件：

```text
build/bk7259/mesh/package/all-app.bin
```

## CLI 命令

所有 Mesh CLI 都通过 AP CLI 发送：

```text
ap_cmd ble_mesh provision init provisioner
ap_cmd ble_mesh provision init provisionee
ap_cmd ble_mesh provision deprovision <MAC>
ap_cmd ble_mesh provision send_count [interval_ms]
ap_cmd ble_mesh tmall init <product_id> <device_secret>
ap_cmd ble_mesh tmall click
ap_cmd ble_mesh led init [type]
ap_cmd ble_mesh led click
ap_cmd ble_mesh led send_count [interval_ms]
```

## 手机/nRF Mesh 配网

1. 烧录 `all-app.bin`，重启开发板，等待：

   ```text
   ble_mesh cli ready
   ```

2. 执行：

   ```text
   ap_cmd ble_mesh provision init provisionee
   ```

3. 手机端使用 nRF Mesh 或其他 Mesh Provisioner 扫描。
   - PB-GATT 广播的 Mesh Provisioning Service UUID 是 `0x1827`。
   - PB-ADV 是 Mesh Beacon，普通 BLE 扫描工具可能显示为 unknown 或不显示名称。
   - 若启用了名称广播，可看到默认设备名 `zephyr-mesh`；否则建议按 `0x1827` 过滤。

4. 配网、添加 AppKey、绑定 Generic OnOff Server 后，可在 App 中控制 OnOff。

## 双板测试

1. 两块板都烧录本工程固件并重启。
2. A 板作为 provisioner：

   ```text
   ap_cmd ble_mesh provision init provisioner
   ```

3. B 板作为 provisionee：

   ```text
   ap_cmd ble_mesh provision init provisionee
   ```

4. 组网成功后，A 板会打印节点加入和 AppKey/Model bind 相关日志。
5. A 板发送 vendor count：

   ```text
   ap_cmd ble_mesh provision send_count 1500
   ```

   取消定时发送：

   ```text
   ap_cmd ble_mesh provision send_count 0
   ```

## 常见问题

- **手机扫描不到名字**：
  Mesh PB-ADV 不带普通 BLE 名称；PB-GATT 按 UUID `0x1827` 查找。名称是否进入 proxy/provisioning 广播由 `CONFIG_BT_MESH_PROXY_USE_DEVICE_NAME` 控制。

- **看不到 OnOff / AppKey 绑定日志**：
  `BT_INFO` 需要 `CONFIG_ACLSEMI_BT_MESH_DEBUG_LEVEL >= 3`。当前工程默认设置为 3。

- **Composition Data 返回 `Message too big`**：
  这是 Mesh transport 分段限制，不是 ATT MTU。当前工程已把 `CONFIG_BT_MESH_TX_SEG_MAX` / `RX_SEG_MAX` 调整为 4，支持当前 demo 的 Composition Status。

- **PB-GATT 连接后出现 `Advertising failed: err -12`**：
  `-12` 是没有可用连接对象。当前默认 `CONFIG_BLE_CONN_NUM=1`，手机已连接后再尝试启动可连接广播会失败，通常可忽略；如需持续广播可增加连接数。
