# 蓝牙 Headset 低功耗示例工程（AP 掉电 / RAM 保留）

* [English](./README.md)

## 这个工程是什么？

本工程是 [`headset`](../headset/README_CN.md) 工程的**低功耗变体**，用于演示 **AP 掉电（RAM 保留）** 低功耗方案下的经典蓝牙耳机/音箱（A2DP Sink + AVRCP + HFP HF）应用。

**业务功能、CLI 命令、代码结构、数据流原理与 `headset` 完全一致**，请直接参考：

> 📖 **[../headset/README_CN.md](../headset/README_CN.md)**

本文只说明本工程与 `headset` 的**关系**和**差异点**。

## 与 headset 的关系：共用同一份源码

本工程不复制任何 `.c/.h`，`ap/` 和 `cp/` 的源码通过各自的 `CMakeLists.txt` 直接引用 `../../headset` 下的文件（`set(HS_AP .../headset/ap)`、`set(HS_CP .../headset/cp)`）。因此：

- 改业务功能只需改 `headset/`，本工程自动同步；
- 本目录只保留与 `headset` 不同的部分：

| 保留文件 | 作用 |
| --- | --- |
| `ap/config/bk7259_ap/defconfig` | AP 侧配置（低功耗开关，见下） |
| `cp/config/bk7259/defconfig` | CP 侧配置（低功耗开关，见下） |
| `ap/Kconfig.projbuild` | `A2DP_SINK_DEMO` / `HFP_HF_DEMO` 选项（Kconfig 按目录扫描，需本地副本） |
| `ap/config/.../usr_gpio_cfg.h`、`cp/config/.../usr_gpio_cfg.h` | 板级 GPIO 配置 |
| `cp/vnd_cal.c`、`cp/vnd_cal.h` | 从 `headset/cp` 复制的本地副本（射频校准）。`bk_init` 在 `CONFIG_OVERRIDE_VND_CAL=y` 时把 `$PROJECT_DIR/cp` 写死为 `vnd_cal.h` 的搜索路径，故这两个文件必须实际存在于本目录，无法共享 |
| `partitions/` | 分区表 |

## 差异点：低功耗（AP 掉电 / RAM 保留）配置

相对普通 headset 场景，本工程通过 defconfig 使能 AP 掉电（RAM 保留）低功耗方案，关键开关如下：

**AP 侧（`ap/config/bk7259_ap/defconfig`）**

```text
CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y   # AP 掉电（RAM 保留）方案
CONFIG_PM_AP_FAST_BOOT_ENABLE=y               # AP 快速恢复（跳过完整 soc_init）
CONFIG_PSRAM_DATA_RETENTION_ENABLE=y          # 掉电期间 PSRAM 数据保持
CONFIG_FREERTOS_USE_TICKLESS_IDLE=2           # tickless 低功耗
CONFIG_GPIO_WAKEUP_SUPPORT=y
CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y
CONFIG_PM_AP_SUBPOWER_DOMAIN_DEFAULT_DISABLE=y
CONFIG_PM_AP_CPU_FRQ_DEFAUL=y
```

**CP 侧（`cp/config/bk7259/defconfig`）**

```text
CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y
CONFIG_PM_AP_FAST_BOOT_ENABLE=y
CONFIG_PSRAM_DATA_RETENTION_ENABLE=y
CONFIG_PM_AP_POWERDOWN_WHEN_LV=y              # 进入低压时允许 AP 掉电
CONFIG_DEEP_LV=y                              # Deep-LV 上下文保存/恢复
CONFIG_PM_ONLY_CP_ENABLE=y
CONFIG_BLUETOOTH_SUPPORT_LPO_ROSC=y
CONFIG_BLE_LV_SUPPORT=y
```

> 说明：`headset` 为普通（非低功耗）基线，`headset_lp` 在其基础上通过 defconfig 使能上述 AP 掉电（RAM 保留）低功耗方案。CP 侧差异更大：`headset_lp` 额外开启了 `DEEP_LV`、tickless、AON 电流优化、CP 外设时钟默认关断、`STA_PS`、`BLE_LV_SUPPORT` 等一整套低功耗项，而 `headset` 的 CP defconfig 仅保留 BT controller 基本配置。

## 编译

```bash
make bk7259 PROJECT=bluetooth/headset_lp
```

其余烧录、CLI 命令用法、测试判定等，请参考 [../headset/README_CN.md](../headset/README_CN.md)。
