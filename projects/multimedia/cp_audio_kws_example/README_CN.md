# CP Audio KWS 示例工程

* [English](./README.md)

## 1. 工程概述

本工程用于验证 BK7259 CP 侧音频链路、提示音播放和 KWS/ASR 集成入口。当前工程重点覆盖：

- CP 侧 `voice_service` mic-to-speaker 回环链路
- CP 侧 audio pipeline、audio driver/HAL、codec 组件的集成编译
- `audio_obs` 运行时统计日志，用于观察 mic/aec/eq/enc/dec/spk/kws 等组件耗时和异常计数
- CP 侧 `player_service` 播放和提示音链路验证
- CP 侧 ASR/KWS stub 链路验证

说明：当前 CP KWS/ASR 算法库可能仍是 stub 版本，`audio asr_turn_on` 主要验证服务链路和数据流是否正常，不代表真实唤醒算法效果。

## 2. 适用硬件与测试环境

- 推荐开发板：BK7259 系列或兼容平台
- 串口终端：连接 CP 默认日志/CLI 串口
- 音频硬件：板载麦克风、板载 speaker 或功放输出
- 如遇 BLE/Wi-Fi 对音频测试有干扰，可先关闭 BLE 再测试

## 3. 目录结构

```
cp_audio_kws_example/
├── ap/
│   ├── ap_main.c
│   └── config/
├── cp/
│   ├── cp_main.c
│   ├── cli_voice_service.c      # voice_service 与 audio_obs CLI
│   ├── cli_player_service.c     # player_service CLI
│   ├── cp_asr_cli.c             # audio asr_turn_on/off CLI
│   ├── cp_kws_cli.c             # KWS stub 辅助代码，当前未编入 CLI
│   ├── cp_prompt_cli.c          # 提示音辅助代码，当前未编入 CLI
│   ├── audio_param/
│   └── config/
└── CMakeLists.txt
```

## 4. 编译与固件

在 SDK 根目录执行：

```
make clean
CCACHE_DISABLE=1 make bk7259 PROJECT=multimedia/cp_audio_kws_example
```

构建成功后，完整烧录固件位于：

```
build/bk7259/cp_audio_kws_example/package/all-app.bin
```

OTA 包位于：

```
build/bk7259/cp_audio_kws_example/package/app_pack.rbl
```

## 5. 烧录与启动检查

使用常用烧录工具烧录 `all-app.bin`。启动后在 CP 串口确认出现类似日志：

```
cp_audio_kws_example CP init done
cp_audio_kws_example CP main running
```

如需减少无线侧干扰，可先执行：

```
AT+BLEPOWER=0
```

## 6. CLI 使用与测试示例

### 6.1 打开 audio_obs 统计

`audio_obs` 用于运行时选择要观察的音频组件，并设置统计窗口周期。它不会启动音频链路，只控制统计日志。

```
audio_obs [bitmap|eq,spk,mic,aec,enc,dec,kws,all,off] [interval_ms]
```

常用示例：

```
audio_obs all 5000
audio_obs eq,spk 5000
audio_obs off
```

统计日志通常包含处理次数、错误/timeout/short/zero/done 计数，以及处理耗时的 min/max/avg/p95/p99。

### 6.2 Mic-to-Speaker 回环验证

启动 16 kHz、无 AEC、PCM 编解码、板载 mic 到板载 speaker、单声道输出：

```
voice_service start onboard 16000 0 pcm pcm onboard 16000 1 call
```

预期：

- CLI 返回 `CMDRSP:OK`
- 板载 speaker 能听到 mic 回环声音
- 打开 `audio_obs` 后可看到 mic/eq/spk 等组件统计日志
- 不应出现明显异响、断续或持续 short/timeout/error

停止：

```
voice_service stop
```

`voice_service` 参数格式：

```
voice_service start <mic_type> <mic_sample_rate> <aec_en> <enc_type> <dec_type> <spk_type> <spk_sample_rate> <spk_chl> [dac_source]
```

常用参数：

- `mic_type`: `onboard`
- `mic_sample_rate`: `8000` 或 `16000`
- `aec_en`: `0` 关闭 AEC；`1` 打开软件 AEC
- `enc_type`/`dec_type`: `pcm`、`g711a`、`g711u`、`aac`、`g722`
- `spk_type`: `onboard`
- `spk_chl`: `1` 单声道；`2` 双声道
- `dac_source`: 可选，`call`、`a2dp`、`hint`、`prompt`；其中 `prompt` 才启用多源提示音混入

### 6.3 Player/提示音直接播放验证

提示音可以不依赖 `voice_service` 直接播放。单独播放内置数组音源：

```
player_service playback start array 1
```

停止播放：

```
player_service playback stop
```

### 6.4 多源提示音混入验证

`player_service prompt_tone` 子命令用于验证多路音频混入已有 speaker path，不是普通的 standalone 播放命令。该路径依赖 `voice_service` 已经创建好的 speaker element，必须按下面顺序执行：

1. 先启动 mic-to-speaker 回环：

```
voice_service start onboard 16000 0 pcm pcm onboard 16000 1 prompt
```

2. 再启动提示音混入。低内存场景优先使用 `array 0`，它是 16 kHz/16-bit/mono PCM 提示音，不需要额外 MP3/WAV decoder：

```
player_service prompt_tone start 1 array 0
```

3. 停止提示音并释放 player/port 资源。即使日志已经出现 `PLAYER_EVENT_FINISH`，也要执行 stop：

```
player_service prompt_tone stop 1
```

4. 停止回环：

```
voice_service stop
```

若未先启动 `voice_service`，`prompt_tone` 多源混入路径会因 `gl_voice_service_handle` 为空而失败。只想单独播放提示音时，请使用上一节的 `player_service playback start array <id>`。

说明：16 kHz 多源测试使用 `prompt` 模式时，speaker 以 A2DP 作为主通路、CALL 作为提示音 source，更贴近后续蓝牙听歌场景；但 A2DP 16 kHz 可能会重采样到 48 kHz，因此内存压力会高于 CALL/HINT-only 测试。

说明：`array 1` 是 MP3，`array 2` 是 WAV。在 `voice_service` 已经打开 AEC/codec/EQ 的情况下，再混入 MP3/WAV 可能因 decoder 额外申请内存而失败，日志类似 `mp3_decoder_init: Memory exhausted`。

### 6.5 ASR/KWS stub 链路验证

启动 CP ASR/KWS stub 服务链路：

```
audio asr_turn_on 0 0 16000 1
```

参数含义：

```
audio asr_turn_on <aec> <uac> <sample_rate> [asr_en]
```

- `aec`: `0` 关闭 AEC；`1` 打开 AEC
- `uac`: 当前 CP 示例不支持 UAC mic，使用 `0`
- `sample_rate`: `8000` 或 `16000`
- `asr_en`: `0` 或 `1`

预期日志：

```
cp_asr turn_on done: service pipeline running
cp_asr frame=... bytes=... kws=stub
cp_asr kws_stub result=success
cp_asr result handler: recognition success
```

停止：

```
audio asr_turn_off
```

## 7. 常见日志与诊断

- `voice_service start` 返回错误时，先检查参数个数和 codec 组合是否受支持。
- 回环有异响时，优先打开 `audio_obs all 5000`，观察 mic、eq、spk 是否有持续 short/timeout/error，及 p95/p99 是否异常。
- 播放或提示音断续时，重点查看 `ONBOARD_SPK` 和 decoder/raw stream 相关统计。
- ASR/KWS 当前若打印 `kws=stub` 或 `lib=not_linked`，说明算法库未接入，属于预期的 stub 行为。
- `cp_prompt` 和 `cp_kws` 当前没有注册为 CLI 命令；请使用 `player_service` 和 `audio asr_turn_on/off` 验证当前固件。
- 如无线侧影响音频测试，可先执行 `AT+BLEPOWER=0` 后再验证。

