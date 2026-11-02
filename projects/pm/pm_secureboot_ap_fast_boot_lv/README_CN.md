# 安全启动 + AP 快速恢复工程（关闭 Flash AES）

* [English](./README.md)

## 工程概述

`pm_secureboot_ap_fast_boot` 以 `projects/secureboot_overwrite_no_en` 为底，放在 `projects/pm` 下，并合入 `projects/pm/ap_fast_boot` 的 AP 快速恢复。

安全启动和镜像验签仍然打开（`secureboot_en` / `sig_verify_en`）。`partitions/bk7259/security.csv` 中 `flash_aes_type` 为 `NONE`。升级时签名并压缩的镜像写入 `ota` 暂存分区；复位后引导程序校验并覆盖 `primary_all`。因为未开 Flash AES，引导程序按明文经数据总线写 `primary_all`。

AP 掉电前保存 CPU2 架构上下文，并把 DTCM 备份到保持供电的 AP SRAM。PSRAM 通过 data retention 原位保持。AP 再次上电后从原有 FreeRTOS 执行点继续运行。冷启动时 CPU2 和 CPU3 一起启动；AP OFF 前 CPU3 hotplug offline，恢复后再上线。分区表、TF-M、签名密钥和 Overwrite OTA 布局沿用安全启动工程，不改成普通 `ap_fast_boot` 的非安全分区。

## 工程目录

- `ap/ap_main.c`：AP 快速恢复连续性验证线程
- `cp/cp_main.c`：安全启动后投票打开 AP，并注册 `ap_fast_boot` CLI
- `config/bk7259/config`：启用安全固件打包
- `config/key/`：构建时从 `projects/secureboot_overwrite_no_en` 复制 SDK 示例密钥
- `partitions/bk7259/auto_partitions.csv`：单执行槽和 OTA 暂存区布局
- `partitions/bk7259/security.csv`：安全打包配置（`flash_aes_type=NONE`）
- `partitions/bk7259/ram_regions.csv`：含安全世界 RAM 和低功耗保留区

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=pm/pm_secureboot_ap_fast_boot
```

## 运行与验证

1. 执行编译命令，生成的安全启动镜像在 `build/bk7259/pm/pm_secureboot_ap_fast_boot/package`。
2. 通过 BKFIL 下载该目录下的 `bootloader.bin`，并把 `otp_efuse_config.json` 中的默认密钥烧到 OTP。
3. 下载 `all-app.bin`。
4. 复位开发板。CP 日志应显示已进入非安全应用（`CP NS world reached`），并自动投票打开 AP。
5. AP 的 `ap_resume_test` 线程打印 `RESUME_PROOF`。冷启动后 `main_entries` 为 1。
6. 在 CP UART0（GPIO10/GPIO11）执行：

```text
ap_fast_boot cycle
```

成功恢复后 `seq` 继续递增，`stack` 保持 `0x12345678`，`heap_value` 保持 `0xA55A5AA5`，`main_entries` 仍为 1。
7. 通过已启用的 OTA 流程（`http_ota` / `https_ota`）将本工程生成的签名包 `ota.bin` 写入 `ota` 暂存分区，复位后确认引导程序安装到 `primary_all`。

## 安全注意事项

- 构建时复用 `projects/secureboot_overwrite_no_en` 中的 SDK 示例密钥，量产前必须替换为受保护的产品密钥。
- 私钥不得提交到源码仓库，也不得放入固件交付包。
- 修改分区名称或大小前，必须检查 `auto_partitions.csv` 中记录的安全打包和引导程序约束。
- 关闭 Flash AES 并不等于关闭安全启动：验签仍然需要，只是不做片上密文存储。
- CP 与 AP 两侧的 `CONFIG_PM_AP_FAST_BOOT_ENABLE` 和 `CONFIG_PSRAM_DATA_RETENTION_ENABLE` 必须同时打开。
