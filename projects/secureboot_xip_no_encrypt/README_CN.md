# 安全启动 XIP OTA 工程（关闭 Flash AES）

* [English](./README.md)

## 工程概述

`secureboot_xip_no_encrypt` 与 `secureboot_xip` 同为 BK7258 安全启动 + 双执行槽 Direct-XIP OTA，差别是 **关闭 Flash AES**。

安全启动和镜像验签仍然打开（`secureboot_en` / `sig_verify_en`）。`partitions/bk7258/security.csv` 中 `flash_aes_type` 为 `NONE`，而不是 `FIXED`。成对的 `primary_*` 与 `secondary_*` 分区提供两个签名应用槽，启动选择信息保存在 `boot_param` 中。因为未开 Flash AES，引导程序和核按明文经数据总线读写执行槽，不走 CBUS XTS-AES。

生成的软件包包含安全启动元数据、二级引导程序、CP 应用和 AP 应用。

用于在不烧 Flash AES 密钥的情况下把 Direct-XIP OTA 跑通。量产若需要片上加密，请改用 `secureboot_xip`。

## 工程目录

- `ap/` 和 `cp/`：AP/CP 非安全应用入口和配置
- `config/bk7258/config`：启用安全固件打包
- `config/key/`：示例签名密钥文件
- `partitions/bk7258/auto_partitions.csv`：主、备可执行槽布局
- `partitions/bk7258/security.csv`：安全打包配置（`flash_aes_type=NONE`）
- `partitions/bk7258/pack.json`：输出镜像组成

## 编译

在 SDK 根目录执行：

```text
make bk7258 PROJECT=secureboot_xip_no_encrypt
```

## 运行与验证

1. 执行编译命令，生成的安全启动镜像在 `build/bk7258/secureboot_xip_no_encrypt/package`。
2. 通过 BKFIL 下载该目录下的 `bootloader.bin`，并把 `otp_efuse_config.json` 中的默认密钥烧到 OTP。
3. 下载 `all-app.bin`。
4. 复位开发板，确认 CP 日志显示已进入非安全应用，且 AP 能正常启动。
5. 通过已启用的 OTA 流程（`http_ota`）将本工程生成的签名包 `ota.bin` 安装到非活动执行槽。
6. 再次复位，确认引导程序选择并启动更新后的槽。

## 安全注意事项

- `config/key/` 中的密钥仅供 SDK 示例使用，量产前必须替换为受保护的产品密钥。
- 私钥不得提交到源码仓库，也不得放入固件交付包。
- 主、备分区必须保持配对兼容。修改布局前，请先检查 `auto_partitions.csv` 中的安全打包和引导程序约束。
- 关闭 Flash AES 并不等于关闭安全启动：验签仍然需要，只是不做片上密文存储。
