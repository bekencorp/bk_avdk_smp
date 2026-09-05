# 安全启动 Overwrite OTA 工程（关闭 Flash AES）

* [English](./README.md)

## 工程概述

`secureboot_overwrite_no_en` 与 `secureboot_overwrite` 同为 BK7259 安全启动 + 单执行槽 Overwrite OTA，差别是 **关闭 Flash AES**。

安全启动和镜像验签仍然打开（`secureboot_en` / `sig_verify_en`）。`partitions/bk7259/security.csv` 中 `flash_aes_type` 为 `NONE`，而不是 `FIXED`。升级时签名并压缩的镜像仍写入 `ota` 暂存分区；复位后引导程序校验并覆盖 `primary_all`。`ota_control` 记录升级进度，用于掉电恢复。因为未开 Flash AES，引导程序按明文经数据总线写 `primary_all`，不走 CBUS XTS-AES。

用于在不烧 Flash AES 密钥的情况下把 Overwrite OTA 跑通。量产若需要片上加密，请改用 `secureboot_overwrite`。

## 工程目录

- `ap/` 和 `cp/`：AP/CP 非安全应用入口和配置
- `config/bk7259/config`：启用安全固件打包
- `config/key/`：示例签名密钥文件
- `partitions/bk7259/auto_partitions.csv`：单执行槽和 OTA 暂存区布局
- `partitions/bk7259/security.csv`：安全打包配置（`flash_aes_type=NONE`）
- `partitions/bk7259/pack.json`：输出镜像组成

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=secureboot_overwrite_no_en
```

## 运行与验证

1. 执行编译命令，生成的安全启动镜像在 `build/bk7259/secureboot_overwrite_no_en/package`。
2. 通过 BKFIL 下载该目录下的 `bootloader.bin`，并把 `otp_efuse_config.json` 中的默认密钥烧到 OTP。
3. 下载 `all-app.bin`。
4. 复位开发板，确认 CP 日志显示已进入非安全应用，且 AP 能正常启动。
5. 通过已启用的 OTA 流程（`http_ota`）将本工程生成的签名包 `ota.bin` 写入 `ota` 暂存分区。
6. 再次复位，确认引导程序将镜像安装到 `primary_all`。

## 安全注意事项

- `config/key/` 中的密钥仅供 SDK 示例使用，量产前必须替换为受保护的产品密钥。
- 私钥不得提交到源码仓库，也不得放入固件交付包。
- 修改分区名称或大小前，必须检查 `auto_partitions.csv` 中记录的安全打包和引导程序约束。
- 关闭 Flash AES 并不等于关闭安全启动：验签仍然需要，只是不做片上密文存储。
