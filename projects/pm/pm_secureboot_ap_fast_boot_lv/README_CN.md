# 安全启动 + AP 快速恢复 + LV 低功耗工程（关闭 Flash AES）

* [English](./README.md)

## 1. 工程概述

`pm_secureboot_ap_fast_boot_lv` 以 `projects/pm/pm_secureboot_ap_fast_boot` 为基础，保留安全启动链、Overwrite OTA 布局和 AP 快速恢复，并在此基础上：

1. 将 CP 的低功耗路径从 Deep LV 改为 LV（`CONFIG_PM_LV_SLEEP_ENABLE=y`），缩短唤醒时间。
2. AP 掉电时保留蓝牙 Host 所需的 AP RAM（`CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y`），不再丢失协议栈状态。
3. 打开 AP PSRAM 保持自检（`CONFIG_PM_AP_PSRAM_RETENTION_CHECK=y`），并扩大 PSRAM 数据保持区。

简单理解：

1. 启动链：BootROM -> CP BL2（MCUboot）-> CP TF-M Secure -> CP 非安全应用。
2. CP 应用通过 `PM_BOOT_AP_MODULE_NAME_APP` 投票打开 AP，并注册 `ap_fast_boot` CLI。
3. AP 掉电前，PM 流程保存 AP CPU2 架构上下文，并把掉电的 DTCM 备份到保持供电的 AP SRAM。PSRAM 通过 data retention 原位保持。AP 再次上电后从保存的 FreeRTOS 执行点继续运行，而不是冷启动。
4. 冷启动时 CPU2 和 CPU3 一起启动；AP OFF 前 CPU3 hotplug offline，恢复后再上线。
5. 使用 LV 睡眠时，`sys_hal_enter_low_voltage()` 直接调用 `sys_hal_enter_lv()`，不再根据 bootloader 版本选择 Deep LV；唤醒后原地返回，不经过 Deep LV 的复位流程。
6. `CONFIG_PM_SLEEP_WAKE_TIME_HARDWARE_OR_BOOTLOADER=1190` 把硬件到 bootloader 的唤醒时间补偿从默认 4100 us 缩短为 1190 us，与 LV 唤醒路径匹配。

> LV 的睡眠深度比 Deep LV 浅，待机电流通常更高。本工程面向快速唤醒和短恢复时延，而不是最低睡眠电流。如果 Deep LV 电流比唤醒时延更重要，应使用 `pm_secureboot_ap_fast_boot`。

## 2. 工程目录

- `ap/ap_main.c`：AP 快速恢复连续性验证线程和 1 MiB PSRAM 保持自检
- `cp/cp_main.c`：安全启动进入非安全世界后投票打开 AP，并注册 `ap_fast_boot on|off|cycle`
- `config/bk7259/config`：启用安全固件打包
- `CMakeLists.txt`：构建配置阶段从 `projects/secureboot_overwrite_no_en` 复制示例签名密钥到 `config/key/`
- `partitions/bk7259/auto_partitions.csv`：单执行槽（`primary_all`）和 `ota` 暂存区布局
- `partitions/bk7259/security.csv`：安全打包配置
- `partitions/bk7259/ota.csv`：Overwrite OTA 策略和版本字段
- `partitions/bk7259/ram_regions.csv`：安全世界 RAM、低功耗保持区和 AP PSRAM 数据保持区
- `partitions/bk7259_ap/ppc.csv`、`ppro_default.csv`：AP 外设安全/特权属性和 PPRO 默认值
- `cp/vnd_cal.c/h`：`bk_init()` 使用的厂商校准覆盖

## 3. 编译与烧录

在 SDK 根目录执行：

```text
make bk7259 PROJECT=pm/pm_secureboot_ap_fast_boot_lv
```

也可以使用 `.ci` 中的 Docker 构建方式：

```text
./dbuild.sh make bk7259 PROJECT=pm/pm_secureboot_ap_fast_boot_lv
```

生成镜像位于 `build/bk7259/pm_secureboot_ap_fast_boot_lv/package`。

1. 通过 BKFIL 下载该目录下的 `bootloader.bin`。
2. 把 `otp_efuse_config.json` 中的默认密钥烧到 OTP。
3. 下载 `all-app.bin`。
4. 复位开发板。

## 4. 运行与验证

1. CP 日志应显示 `CP NS world reached (secure boot OK)`、`AP vote ON ret=0` 和 `CLI ready: ap_fast_boot on|off|cycle`。
2. AP 打印 `AP main entry count=1`，`ap_resume_test` 线程打印 `RESUME_PROOF`，包含 `seq`、`stack`、`heap_value`、`psram_size`、`main_entries` 和 `core`。
3. 在 CP UART0（`GPIO_10` RX / `GPIO_11` TX）执行：

```text
ap_fast_boot cycle
```

该命令先给 AP 投 OFF 票，等待 1 秒，再投 ON 票。成功恢复后：

- `main_entries` 仍为 `1`，说明 AP 的 `main()` 没有再次执行。
- `seq` 从掉电前的值继续递增。
- `stack` 保持 `0x12345678`，`heap_value` 保持 `0xA55A5AA5`。
- 恢复周期打印 `PSRAM_RETENTION PASS` 和 `PSRAM_INCREMENT_PATTERN PASS`。

4. 只验证单个方向时，使用 `ap_fast_boot off` 或 `ap_fast_boot on`。
5. OTA：通过已启用的 `http_ota` / `https_ota` 流程把本工程生成的签名包 `ota.bin` 写入 `ota` 暂存分区，然后复位。BL2 将镜像安装到 `primary_all`，`ota_control` 保存掉电恢复日志。

## 5. 关键配置

### 5.1 CP 侧

关键配置位于 `cp/config/bk7259/defconfig`：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | 启用 Tickless Idle |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y` | AP 掉电期间保持 PSRAM 数据 |
| `CONFIG_PM_LV_SLEEP_ENABLE=y` | 进入 LV 睡眠，而不是 Deep LV |
| `CONFIG_DEEP_LV=y` | 保留 Deep LV 支持代码（公共流程和 AP fast boot 依赖） |
| `CONFIG_PM_SLEEP_WAKE_TIME_HARDWARE_OR_BOOTLOADER=1190` | 唤醒时间补偿，单位 us（默认 4100） |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | 低功耗期间以 CP 为常驻核 |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | CP 进入 LV 睡眠时允许 AP 掉电 |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE=y` | 启用 AP 快速恢复 |
| `CONFIG_PM_AP_SRAM_POWER_CTRL=y` | 按 bank 控制 AP SRAM 电源 |
| `CONFIG_PM_AP_MEM3_POWER_DOWN` ... `MEM6_POWER_DOWN=y` | 关闭未使用的 AP SRAM bank 电源 |
| `CONFIG_PM_AP_CPU2_CACHE_POWER_DOWN=y`、`CONFIG_PM_AP_CPU3_CACHE_POWER_DOWN=y` | 关闭 AP CPU2/CPU3 cache 电源 |
| `CONFIG_AON_LDO_SEL=2` | 选择 AON LDO 电压（档位 2 = 0.7 V） |
| `CONFIG_AON_ENCP=y` | 保持 ENCP/OTP 共享电源域供电（OTP 访问后休眠而非掉电），保证 TrustEngine 正常工作 |
| `CONFIG_TOUCH=y`、`CONFIG_TOUCH_TEST=y` | 当前工程默认启用 Touch 及测试 |
| `CONFIG_BTDM_CONTROLLER_ONLY=y` | CP 只运行蓝牙控制器 |
| `CONFIG_BLE_LV_SUPPORT=y` | 启用 BLE 低压睡眠支持 |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y` | AP 掉电但 RAM 保持时保留蓝牙协议栈状态 |
| `CONFIG_BLUETOOTH_RF_MODE_IQ_HIGH_PLL=y` | 使用 IQ 模式并共享 Wi-Fi PLL |
| `CONFIG_BLUETOOTH_SUPPORT_LPO_ROSC=y` | 蓝牙支持 RC 振荡器 LPO 时钟 |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y`、`CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y` | 启用静态和动态 GPIO 唤醒源 |
| `# CONFIG_EASY_FLASH is not set` | CP 侧不编译 EasyFlash |
| `CONFIG_PM_POWER_VOTE_RECORD=y`、`CONFIG_PM_CLOCK_VOTE_RECORD=y` | 保留 PM 投票记录用于调试 |
| `CONFIG_TFM_BL2="ON"`、`CONFIG_TFM_ISOLATION_LEVEL=2` | 编译 TF-M BL2，隔离等级 2 |
| `CONFIG_BL2_UPGRADE_STRATEGY="OVERWRITE_ONLY"` | 使用 Overwrite OTA 策略 |
| `CONFIG_TFM_AP_BOOT_NSC=y`、`CONFIG_TFM_SLEEP_RETENTION_NSC=y` | 提供 AP 启动和睡眠保持 NSC 服务 |
| `# CONFIG_TFM_AP_SHIM_L2_CACHE_DISABLE is not set` | AP 启动 shim 保留 L2（PL310）初始化 |

### 5.2 AP 侧

关键配置位于 `ap/config/bk7259_ap/defconfig`：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | 启用 AP Tickless Idle |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y` | AP 掉电期间保持 PSRAM 数据 |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE=y` | 从保存的 FreeRTOS 执行点恢复 |
| `CONFIG_PM_AP_PSRAM_RETENTION_CHECK=y` | 每次真实恢复后执行 1 MiB PSRAM 保持自检 |
| `CONFIG_PM_AP_WFI_ENABLE=y` | 允许 AP 空闲时进入 WFI |
| `CONFIG_PM_AP_SUBPOWER_DOMAIN_DEFAULT_DISABLE=y` | 默认关闭 VIDEO_POST、H26E、ISP、NPU 子电源域 |
| `CONFIG_PM_AP_CPU_FRQ_DEFAUL=y` | AP 初始化后投票设置默认 CPU 频率 |
| `CONFIG_DEEP_LV=y` | AP fast boot 依赖（AP 通过 deep LV 路径掉电） |
| `# CONFIG_L2_CACHE_ENABLE is not set` | AP 侧 L2 cache 关闭 |
| `CONFIG_AP_SECURE_CACHE_NSC=y` | 使用安全 cache NSC 服务 |
| `CONFIG_BLUETOOTH_AP=y` | 在 AP 运行蓝牙 Host |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y` | AP 恢复后不重新初始化蓝牙协议栈 |
| `CONFIG_SECURE_OTA_OVERWRITE=y`、`CONFIG_OTA_HTTPS=y` | 启用基于 HTTPS 的安全 Overwrite OTA |
| `CONFIG_EASY_FLASH_V4=y`、`CONFIG_VFS=y`、`CONFIG_FATFS=y`、`CONFIG_LITTLEFS=y` | 启用文件系统栈 |
| `CONFIG_SDCARD=y`、`CONFIG_FATFS_SDCARD=y` | 启用 SD 卡 Host 和基于 SD 的 FATFS |
| `CONFIG_AT=y`、`CONFIG_DEFAULT_AT_PORT=5` | 在端口 5 启用 AT 命令 |
| `CONFIG_PSA_MBEDTLS=y`、`CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=4096` | 启用 PSA mbedTLS，TLS 记录 4 KB |
| `CONFIG_IPERF_TEST=y`、`CONFIG_DEMOS_IPERF=y`、`CONFIG_VFS_TEST=y` | 保留测试/示例模块 |
| `CONFIG_DUMP_BY_LOG_UART=y` | 通过日志串口输出 dump |

### 5.3 LV、快速恢复、蓝牙保持与 PSRAM 自检

- **LV 替代 Deep LV**：CP 无条件进入 `sys_hal_enter_lv()`。该路径不经过 bootloader 重启，也没有 Deep LV 调试 GPIO 标记。`CONFIG_DEEP_LV=y` 仍然保留，一方面是公共 Deep LV 代码需要，另一方面 AP 的 `PM_AP_FAST_BOOT_ENABLE` 依赖它。
- **唤醒时间补偿**：`CONFIG_PM_SLEEP_WAKE_TIME_HARDWARE_OR_BOOTLOADER=1190`（us）取代默认 4100 us，使 RTC 补偿与更短的 LV 唤醒时延匹配。
- **AP 快速恢复**：AP 掉电前保存 CPU2 架构上下文，并把掉电的 DTCM 备份到保持供电的 AP SRAM；唤醒时从复位向量恢复上下文，恢复 DTCM 并重新使能中断，然后继续原 FreeRTOS 执行点。
- **蓝牙保持**：CP 只运行控制器，Host 运行在 AP。打开 `BLUETOOTH_SUPPORT_AP_PWD_RETENTION` 后，AP RAM 保持供电，恢复时不再重新初始化 Host 协议栈，因此已建立的 BLE 连接可以继续。
- **PSRAM 保持自检**：AP 在 `AP_PSRAM_DATA_SECTION` 的 `.psram.bss` 中分配 1 MiB 递增 pattern，掉电前记录 CRC，真实恢复后同时校验 CRC 和 pattern；`PSRAM_RETENTION_COMPARE` 用于判断两次独立扫描结果是否一致。

### 5.4 Flash 与 RAM 布局

Flash（8 MB，`partitions/bk7259/auto_partitions.csv`）：

| 分区 | 偏移 | 大小 |
| --- | --- | --- |
| `bl1_control` | 0x0 | 4 KB |
| `boot_flag` | 0x1000 | 4 KB |
| `partition` | 0x2000 | 4 KB |
| `primary_manifest` | 0x3000 | 4 KB |
| `bl2` | 0x4000 | 96 KB |
| `sys_its`、`sys_ps` | 0x1C000 / 0x1E000 | 各 8 KB |
| `primary_tfm_s` | 0x20000 | 320 KB |
| `primary_cpu0_app` | 0x70000 | 1644 KB |
| `primary_ap_app` | 0x20B000 | 1428 KB |
| `ota` | 0x370000 | 3072 KB |
| `ota_control` | 0x670000 | 8 KB |
| `usr_config` | 0x6B8000 | 60 KB |
| `easyflash` / `easyflash_ap` | 0x6C7000 / 0x6C9000 | 各 8 KB |
| `sys_rf`、`sys_net` | 0x7FE000 / 0x7FF000 | 各 4 KB |

`primary_all` 是唯一执行槽：320 KB + 1644 KB + 1428 KB = 3392 KB（53 个 64 KB 压缩块）。工程没有 `secondary_*` 分区。

RAM（`partitions/bk7259/ram_regions.csv`）：

| 区域 | 类型 | 偏移 | 大小 |
| --- | --- | --- | --- |
| `CP_SPE_RAM` | secure | `0x28000000` | 80 KB |
| `AP_SPE_RAM` | secure | `0x28100000` | 4 KB |
| `CP_RAM` | SRAM | `0x28000000` | 0x05EB00 |
| `SWAP` | SRAM | CP_RAM 之后 | 4 KB |
| `LOWPOWER` | SRAM | `0x2805FB00` | 1 KB |
| `AP_HSRAM_HEAP` | SRAM | `0x38101000` | 0x07F000 |
| `AP_RAM` | SRAM | heap 之后 | 384 KB |
| `CP_PSRAM_HEAP` | PSRAM | coded slab 之后 | 128 KB |
| `AP_PSRAM_DATA_SECTION` | PSRAM | `0x74E20000` | 1.25 MB |
| `AP_PSRAM_HEAP` | PSRAM | data 区之后 | 640 KB |
| `AP_PSRAM_CACHE_HEAP` | PSRAM | heap 之后 | 128 KB |
| `AP_PSRAM_CODE_SECTION` | PSRAM | cache heap 之后 | 128 KB |

## 6. 与 pm_secureboot_ap_fast_boot 的区别

| 项目 | `pm_secureboot_ap_fast_boot` | `pm_secureboot_ap_fast_boot_lv` |
| --- | --- | --- |
| CP 睡眠方式 | Deep LV | LV（`CONFIG_PM_LV_SLEEP_ENABLE=y`） |
| 唤醒时间补偿 | 默认 4100 us | 1190 us |
| CP 蓝牙形态 | BLE 仅从机 | BTDM 仅控制器 |
| AP 掉电时的蓝牙 | `AP_PWD_ALL`（AP RAM 丢失，协议栈重新初始化） | `AP_PWD_RETENTION`（AP RAM 保持，协议栈继续运行） |
| AP PSRAM 保持自检 | 关闭 | 开启，1 MiB pattern |
| `AP_PSRAM_DATA_SECTION` | 0x110000（1.0625 MB） | 0x140000（1.25 MB） |
| `AP_PSRAM_HEAP` | 0x040000（256 KB） | 0x0A0000（640 KB） |
| `AP_PSRAM_CODE_SECTION` | 0x080000（512 KB） | 0x020000（128 KB） |

## 7. 安全配置与注意事项

- 本工程 `partitions/bk7259/security.csv` 实际配置为：`secureboot_en=TRUE`、`sig_verify_en=FALSE`、`flash_aes_type=NONE`、`crc_en=FALSE`。
- `secureboot_en=TRUE` 会构建安全启动链和分区布局（BL2、TF-M Secure、`primary_manifest`、`bl1_control`），但它本身不等于强制验签。
- `sig_verify_en=FALSE` 时打包器在 Flash 偏移 0x100 写入 `BEKEN` magic，BL2 跳过签名验证，但仍校验镜像 hash。量产如果需要强制镜像签名，必须把 `sig_verify_en` 改为 `TRUE`（写入 `BK.SB`）。另外，OTP 的 `secure_boot_supported`（bit 3）一旦烧录也会强制验签，与 magic 无关；生成的 `otp_efuse_config.json` 只写 ROTPK hash，不会烧 bit 3。
- `flash_aes_type=NONE` 表示 Flash 明文存储，引导程序按明文经数据总线写 `primary_all`，不走 CBUS XTS-AES。
- 构建时复用 `projects/secureboot_overwrite_no_en` 的 EC-P256 示例密钥；`config/key/*.pem` 在配置阶段生成且已被 git 忽略。量产前必须替换为受保护的产品密钥，私钥不得提交到源码仓库或放入固件交付包。
- OTA 镜像压缩后写入 `ota`（`encrypt=FALSE`），`ota_control` 保存安装日志。修改分区名称或大小前，必须检查 `auto_partitions.csv` 中的打包器和引导程序约束。
- CP 和 AP 两侧的 `CONFIG_PM_AP_FAST_BOOT_ENABLE` 与 `CONFIG_PSRAM_DATA_RETENTION_ENABLE` 必须同时打开，且 `sig_verify_en` 要与 OTP 的烧录配置保持一致。

## 8. 常见问题

### 8.1 AP 的 `main_entries` 大于 1

说明 AP 走了冷启动路径。检查两侧的 `CONFIG_PM_AP_FAST_BOOT_ENABLE` 和 `CONFIG_PSRAM_DATA_RETENTION_ENABLE`，并确认恢复测试前 AP 是真正掉电，而不只是关时钟。

### 8.2 `PSRAM_RETENTION FAIL` 或 `PSRAM_INCREMENT_PATTERN FAIL`

检查 PSRAM data retention 配置、`AP_PSRAM_DATA_SECTION` 大小以及 AP OFF 期间 PSRAM 供电。若 `PSRAM_RETENTION_COMPARE FAIL` 且 `crc_pass != pattern_pass`，通常说明 PSRAM 读取不稳定，而不是稳定的数据损坏。

### 8.3 AP 恢复后 BLE 断开

确认两侧都使用 `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y`，而不是 `AP_PWD_ALL`。`AP_PWD_ALL` 会丢失 AP RAM，恢复后必须重新初始化蓝牙协议栈。

### 8.4 电流比预期高

LV 的睡眠深度比 Deep LV 浅。先确认产品是否真的需要 `CONFIG_PM_LV_SLEEP_ENABLE`，否则可对比 `pm_secureboot_ap_fast_boot`。正式测流前还应关闭 PSRAM 保持自检、mbedTLS 测试、iperf/VFS 测试和日志 dump。
