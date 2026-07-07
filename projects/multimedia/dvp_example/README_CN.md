# DVP 示例工程

* [English](./README.md)

## 1. 项目概述

本工程从 `multimedia/isp_example` 中拆出 DVP 摄像头测试能力，专门用于 BK7259 DVP sensor 的检测、MP/SP 通道打开、单帧读取以及帧 callback 验证。

默认硬件配置：

- 核心板：BK7259_QF128_12.3X12.3_V4.0
- PSRAM：32M
- DVP sensor：GC2145，默认 1280x720@30fps

主要软件依赖：`CONFIG_ISP`、`CONFIG_BK_CAMERA`、`CONFIG_FRAME_BUFFER`、`CONFIG_DVP_CAMERA`、`CONFIG_DVP_GC2145`、`CONFIG_MEDIA_SERVICE`。当前 SDK 的 GC2145 DVP raw path 还链接到 MIPI/CSI 辅助符号，因此 defconfig 保留 `CONFIG_MIPI_CSI`、`CONFIG_CSI_CAMERA`、`CONFIG_CSI_GC2053` 作为底层链接依赖，但本工程 CLI 只暴露 DVP 测试入口。

## 2. 目录结构

```text
dvp_example/
├── DVP_TEST_CASES.md          # DVP CLI 测试用例
├── ap/
│   ├── src/ap_main.c          # 初始化 media_service、frame_buffer、CLI
│   ├── src/dvp_cli.c          # 注册 dvp 命令
│   └── src/dvp_func_test.c    # DVP detect/open/read/callback 实现
├── cp/
└── partitions/
```

## 3. CLI 命令

```text
dvp detect
dvp open <mp|sp> <sensor_w> <sensor_h> <fps> <out_w> <out_h> <frame|flexa> [output_fmt]
dvp read <mp|sp>
dvp cb <on|off> [mp|sp]
dvp close <mp|sp>
```

示例：

```text
dvp detect
dvp open mp 1280 720 30 1280 720 frame
dvp cb on mp
dvp cb off
dvp close mp
```

`dvp cb on` 会注册示例 callback，并启动后台采集任务。日志持续打印 `dvp_cb frame[...]` 表示帧数据已经通过 callback 推到应用层。帧 buffer 在 callback 返回后释放，真实业务需要在回调里尽快消费或拷贝。

## 4. 编译

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/dvp_example -j$(nproc)
```

完整测试步骤见 `DVP_TEST_CASES.md`。
