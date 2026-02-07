# 视频录制与播放示例工程（video_player_example）

* [English](./README.md)

## 硬件需求

- **SoC/开发板**：BK7258 系列（本工程以 `bk7258` 为例进行编译与运行）
- **存储**：SD 卡（建议 FATFS/FAT32），挂载点为 `/sd0`
- **显示**：RGB LCD（示例工程默认使用 `st7282` ，480x272）
- **摄像头**：DVP 摄像头（示例日志以 `gc2145` 为例；需与工程配置匹配）
- **音频**：板载麦克风 + 板载扬声器（用于录制/播放音频）

## 相关文档

* 有关视频文件录制/播放的详细信息，请参阅：

  - `视频文件录制 <../../../developer-guide/multimedia/video_file_recording.html>`_
  - `视频文件播放 <../../../developer-guide/multimedia/video_file_playback.html>`_

* 有关视频文件录制/播放的 API 参考，请参阅：

  - `视频文件录制 API <../../../api-reference/multimedia/bk_video_record.html>`_
  - `视频文件播放器 API <../../../api-reference/multimedia/bk_video_player.html>`_

## 视频录制与播放

本工程提供播放器与录制两套 CLI 测试命令：

- **播放器**：`video_play_engine` （Engine 层）与 `video_play_playlist` （Playlist 层，带播放列表与 EOF 策略）
- **录制**：`video_record`

### 播放器（video_play_engine / video_play_playlist）

本项目提供两套播放控制接口：

- **Engine 层**：`video_play_engine`，直接控制 `bk_video_player_engine`，适合验证解复用/解码/显示链路。
- **Playlist 层**：`video_play_playlist`，控制 `bk_video_player_playlist` （包含播放列表与 EOF 策略），适合做应用态播放控制。

支持 AVI/MP4 容器文件，视频为 JPEG（MJPEG），音频支持 PCM/AAC（依文件与解码器选择）。

#### Engine CLI（video_play_engine）

入口命令：`ap_cmd video_play_engine ...`

- `start [file_path]`
- `stop`
- `pause` / `resume`
- `seek <time_ms>`
- `ff <time_ms>` / `rewind <time_ms>`
- `avsync <offset_ms>`：设置音画同步偏移（毫秒，范围 [-5000, 5000]）
- `volume <0-100>` / `vol_up <step>` / `vol_down <step>`
- `mute <on|off>`
- `status`
- `info [file_path]` ：获取媒体信息（支持未播放文件；失败会返回错误码）

示例：

```bash
ap_cmd video_play_engine start /sd0/record.mp4
ap_cmd video_play_engine info /sd0/record.mp4
ap_cmd video_play_engine ff 3000
ap_cmd video_play_engine rewind 3000
ap_cmd video_play_engine seek 500
ap_cmd video_play_engine avsync -200
ap_cmd video_play_engine volume 80
ap_cmd video_play_engine mute on
ap_cmd video_play_engine mute off
ap_cmd video_play_engine stop
```

#### Playlist CLI（video_play_playlist）

入口命令：`ap_cmd video_play_playlist ...`

注意事项：

- **必须先 `start` 再 `add`**：否则会提示 `Video player playlist not started`。
- `next/prev/play` 会触发切换播放；为避免残留音频导致听感延时，**Playlist 层在切歌接口内部** 通过外部传入的 `audio_output_reset_cb` 做音频输出 reset（stop + flush + start）。

常用命令：

- `start [file_path]`
- `stop`
- `pause` / `resume`
- `add <file_path>` / `remove <file_path>` / `clear`
- `next` / `prev`
- `play <file_path|index>`
- `list`
- `status`
- `seek <time_ms>`
- `ff <time_ms>` / `rewind <time_ms>`
- `avsync <offset_ms>`：设置音画同步偏移（毫秒，范围 [-5000, 5000]）
- `info [file_path]`
- `play_mode <stop|repeat|loop>`
- `volume <0-100>` / `vol_up <step>` / `vol_down <step>`
- `mute <on|off>`

示例（播放列表 + 循环）：

```bash
ap_cmd video_play_playlist start /sd0/record.mp4
ap_cmd video_play_playlist add /sd0/record.avi
ap_cmd video_play_playlist play_mode loop
ap_cmd video_play_playlist avsync -200
ap_cmd video_play_playlist next
```

常见问题：

- `Video player playlist not started`
  - 原因：调用 `video_play_playlist add/remove/next/...` 前没有 `start`。
  - 处理：先执行 `ap_cmd video_play_playlist start [file_path]`。
- 频繁 `next` 导致音频“拖尾/延时”
  - 现象：`voice_write_task_main: voice write start N` 的 N 变大，听感音频落后视频。
  - 处理：切换前执行 `stop + flush + start` 清理 voice_write 内部缓冲（示例工程已集成）。

### 录制（video_record）

本项目提供 `video_record` CLI 命令，用于录制 **DVP 摄像头视频 + 麦克风音频** 到 SD 卡文件。

- **容器**：AVI（默认）/ MP4（通过关键字选择）
- **视频编码**：MJPEG（默认）/ H264（通过关键字选择）
- **音频**：PCM（默认）/ AAC（关键字 `aac` ）
- **预览**：录制过程中可将 YUV 帧刷新到 LCD 做预览

入口命令：`ap_cmd video_record ...`

开始录制：

```bash
ap_cmd video_record start [file_path] [width] [height] [format] [type]
```

停止录制：

```bash
ap_cmd video_record stop
```

参数说明（start）：

- **file_path**：输出文件路径，默认 `/sd0/record.avi`
- **width/height**：分辨率，默认 `480 320`
- **format**：视频格式关键字
  - `mjpeg` / `jpeg`：MJPEG
  - `h264`：H264
- **type**：容器类型关键字
  - `mp4`：MP4 容器（会将 `.avi` 后缀自动替换为 `.mp4` ）
  - 不传：AVI 容器
- **aac**：启用 AAC 音频编码

示例：

1) AVI + MJPEG + PCM（默认）：

```bash
ap_cmd video_record start /sd0/record.avi 480 320 mjpeg
```

2) AVI + MJPEG + AAC：

```bash
ap_cmd video_record start /sd0/record.avi 480 320 mjpeg aac
```

3) MP4 + MJPEG + AAC：

```bash
ap_cmd video_record start /sd0/record.mp4 480 320 mjpeg mp4 aac
```

注意事项：

- 录制开始前会尝试挂载 SD 卡；若失败请先确认 SD 卡与文件系统正常。
- DVP 回调中为避免阻塞，编码帧通过队列转交给录制线程；当队列满时会丢帧并周期性打印统计。
- 录制会占用 DVP/LCD/voice 等资源，若存在其他语音服务运行可能冲突，需要先停止相关模块。

## 工程说明

本工程为 `video_player_example`，用于演示 **视频录制（`video_record`）与视频播放（`video_play_engine` / `video_play_playlist`）** 的完整链路（SD 卡文件 + 解复用/解码 + LCD 显示 + 音频输出）。

## 编译与运行

编译：

```bash
make bk7258 PROJECT=video_player_example
```

运行：

- 烧录后通过串口执行 `ap_cmd video_record ...`、`ap_cmd video_play_engine ...`、`ap_cmd video_play_playlist ...` 进行验证
- 命令执行成功打印：`CMDRSP:OK`
- 命令执行失败打印：`CMDRSP:ERROR`