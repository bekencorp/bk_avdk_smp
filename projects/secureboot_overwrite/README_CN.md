# 安全启动 Overwrite OTA 工程

* [English](./README.md)

## 工程概述

`secureboot_overwrite` 演示 BK7259 安全启动流程和单执行槽 Overwrite OTA 升级方案。

生成的软件包包含安全启动元数据、二级引导程序、安全服务固件、CP 应用和 AP 应用。升级时，签名并压缩的镜像写入 `ota` 暂存分区；复位后，引导程序校验镜像并覆盖可执行的 `primary_all` 槽。`ota_control` 分区记录升级进度，用于掉电恢复。

## 工程目录

- `ap/` 和 `cp/`：AP/CP 非安全应用入口和配置
- `config/bk7259/config`：启用安全固件打包
- `config/key/`：示例签名密钥文件
- `partitions/bk7259/auto_partitions.csv`：单执行槽和 OTA 暂存区布局
- `partitions/bk7259/security.csv`：安全打包配置
- `partitions/bk7259/pack.json`：输出镜像组成

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=secureboot_overwrite
```

## 运行与验证

1. 执行编译命令，生成的安全启动镜像，生成路径build\bk7259\secureboot_overwrite\package；
2. 通过BKFIL工具下载生成路径下的bootloader.bin，烧写otp_efuse_config.json中的默认密钥到OTP；
3. 下载all-app.bin；
4. 复位开发板，确认 CP 日志显示已进入非安全应用，且 AP 能正常启动。
5. 通过已启用的 OTA 流程将兼容的签名 OTA 镜像写入 `ota` 暂存分区。
6. 再次复位，确认引导程序将镜像安装到 `primary_all`。

## 安全注意事项

- `config/key/` 中的密钥仅供 SDK 示例使用，量产前必须替换为受保护的产品密钥。
- 私钥不得提交到源码仓库，也不得放入固件交付包。
- 修改分区名称或大小前，必须检查 `auto_partitions.csv` 中记录的安全打包和引导程序约束。
