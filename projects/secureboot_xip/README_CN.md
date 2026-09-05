# 安全启动 XIP OTA 工程

* [English](./README.md)

## 工程概述

`secureboot_xip` 演示 BK7259 安全启动流程和双执行槽 Direct-XIP OTA 升级方案。

生成的软件包包含安全启动元数据、二级引导程序、安全服务固件、CP 应用和 AP 应用。成对的 `primary_*` 与 `secondary_*` 分区提供两个签名应用槽，启动选择信息保存在 `boot_param` 中。

## 工程目录

- `ap/` 和 `cp/`：AP/CP 非安全应用入口和配置
- `config/bk7259/config`：启用安全固件打包
- `config/key/`：示例签名密钥文件
- `partitions/bk7259/auto_partitions.csv`：主、备可执行槽布局
- `partitions/bk7259/security.csv`：安全打包配置
- `partitions/bk7259/pack.json`：输出镜像组成

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=secureboot_xip
```

## 运行与验证

1. 执行编译命令，生成的安全启动镜像，生成路径build\bk7259\secureboot_xip\package；
2. 通过BKFIL工具下载生成路径下的bootloader.bin，烧写otp_efuse_config.json中的默认密钥到OTP；
3. 下载all-app.bin；
4. 复位开发板，确认 CP 日志显示已进入非安全应用，且 AP 能正常启动。
5. 通过已启用的 OTA 流程将兼容的签名 OTA 镜像安装到非活动执行槽。
6. 再次复位，确认引导程序选择并启动更新后的槽。

## 安全注意事项

- `config/key/` 中的密钥仅供 SDK 示例使用，量产前必须替换为受保护的产品密钥。
- 私钥不得提交到源码仓库，也不得放入固件交付包。
- 主、备分区必须保持配对兼容。修改布局前，请先检查安全打包工具和引导程序约束。
