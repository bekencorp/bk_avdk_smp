# DVP Example 测试用例

默认 DVP sensor：GC2145，1280x720@30fps。

## 1. 传感器检测

```text
dvp detect
```

预期：检测到 DVP sensor，并打印支持的分辨率和帧率。

## 2. MP 通道 Frame 模式

```text
dvp open mp 1280 720 30 1280 720 frame
```

预期：打开成功，返回 `CMDRSP:OK`，ISR 统计正常。

缩小输出：

```text
dvp open mp 1280 720 30 640 360 frame
```

预期：打开成功。

不支持放大输出：

```text
dvp open mp 1280 720 30 1920 1080 frame
```

预期：失败，返回 `CMDRSP:ERROR`。

## 3. SP 通道 Frame 模式

```text
dvp open sp 1280 720 30 640 360 frame
dvp read sp
dvp close sp
```

预期：SP 通道打开成功，`dvp read sp` 能读取并打印一帧数据。

## 4. Frame Callback

MP 通道 callback：

```text
dvp open mp 1280 720 30 1280 720 frame
dvp cb on mp
# 观察持续打印 dvp_cb frame[...] 日志
dvp cb off
dvp close mp
```

预期：`dvp cb on mp` 后持续看到 `dvp_cb frame[...]`，帧序号递增；`dvp cb off` 后日志停止，采集任务退出。

SP 通道 callback：

```text
dvp open sp 1280 720 30 640 360 frame
dvp cb on sp
dvp cb off
dvp close sp
```

预期：SP 帧持续回调，关闭后停止。

未 open 通道直接启动 callback：

```text
dvp cb on mp
```

预期：失败，提示 camera/channel 未初始化或 frame size 为 0。

## 5. Flexa 模式冒烟

```text
dvp open mp 1280 720 30 1280 720 flexa
dvp close mp
```

预期：通道打开/关闭成功。callback 采集基于 frame 模式，flexa 模式下不用于 `dvp cb` 验证。
