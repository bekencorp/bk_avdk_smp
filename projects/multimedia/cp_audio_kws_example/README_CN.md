# CP Audio KWS 示例工程

* [English](./README.md)

## 1. 工程概述

本工程用于验证 BK7259 CP 侧音频链路、提示音播放和 KWS/ASR 集成入口。当前工程重点覆盖：

- CP 侧 `voice_service` mic-to-speaker 回环链路
- CP 侧 audio pipeline、audio driver/HAL、codec 组件的集成编译
- `audio_obs` 运行时统计日志，用于观察 mic/aec/eq/enc/dec/spk/kws 等组件耗时和异常计数
- CP 侧 `player_service` 播放和提示音链路验证
- CP 侧 `audio_play` / `audio_record` 组件的独立验证（`cp_audio_play`、`cp_audio_record`）
- CP 侧 `spk_service` 三路混音（A2DP/CALL/HINT）验证（`cp_spk`）
- CP 侧 ASR 服务链路验证

说明：当前 CP KWS/ASR 算法库可能仍是 stub 版本，`audio asr_turn_on` 主要验证服务链路和数据流是否正常，不代表真实唤醒算法效果。

说明：喇叭（DAC）和麦克风（ADC）各只有一份硬件资源，本工程用 `cp_audio_res` 仲裁层做互斥保护，冲突的命令会被拒绝并打印当前持有者，而不是直接崩溃。详见 6.7 节。

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
│   ├── cp_audio_res.c/.h        # 喇叭/mic 资源仲裁层，所有音频 CLI 都经过它
│   ├── cli_voice_service.c      # voice_service 与 audio_obs CLI
│   ├── cli_player_service.c     # player_service CLI
│   ├── cp_asr_cli.c             # audio asr_turn_on/off CLI
│   ├── cp_audio_play_cli.c      # cp_audio_play CLI（audio_play 组件）
│   ├── cp_audio_record_cli.c    # cp_audio_record CLI（audio_record 组件）
│   ├── cp_spk_service_cli.c     # cp_spk CLI（spk_service 三路混音）
│   ├── cp_prompt_cli.c          # cp_prompt CLI（最短的解码+喇叭自检通路）
│   ├── audio_param/
│   └── config/
└── CMakeLists.txt
```

依赖的 CP 组件：`cp/components/audio_play`、`cp/components/audio_record`，由 `CONFIG_AUDIO_PLAY` / `CONFIG_AUDIO_RECORD` 控制。两者均从 AP 同名组件移植而来，仅有的差异是去掉 CP 不存在的 `avdk_utils` 依赖，以及 CPU 频率投票改为 CP 枚举支持的 `PM_CPU_FRQ_240M`。

此外 `cp/components/bk_audio` 中有两处修改**只做在 CP 侧，有意不同步到 AP**：AP 是音频主测环境，改动风险高。升级或与 AP 对拉时不要当成「CP 落后」覆盖掉。

- `audio_streams/onboard_speaker_stream/v2.0/onboard_speaker_stream_v2.c`：DAC 声道数固定在 init，运行时不再改写 `dest_data_width`，单声道主源改为在读取时上混成 L=R；采样率变化时改为从固定的 DMA 输出块反推输入帧长。修复三路混音下改写正在运行的 REPEAT 模式 DMA 通道导致的死机。
- `audio_streams/raw_stream/raw_stream.c`：关闭 `RAW_READ_DATA_COUNT` / `RAW_WRITE_DATA_COUNT`。其 `count_util_t` 是文件静态量，而 `raw_stream` 可多实例，并发打开时会留下孤儿定时器，一个周期后在 `timer_callback1` 中 assert。

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

使用常用烧录工具烧录 `all-app.bin`。

注意：`primary_cp_app` 分区已从 1600K 扩到 2048K（CP 侧音频组件增加后原分区仅剩约 10 KB 余量），其后的 `primary_ap_app`、`ota`、`usr_config`、`easyflash`、`easyflash_ap` 整体后移 448K，因此**必须整片烧录，不能只烧 app 分区**。`easyflash` 位置变化会导致旧的 KV 持久化数据失效，首次启动按空白重新初始化。`sys_rf`（0x7FE000）和 `sys_net`（0x7FF000）是固定偏移，不受影响，RF 校准值和 MAC 不会丢——但不要手工执行全片 erase，否则会误伤这两块。

启动后在 CP 串口确认出现类似日志：

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

### 6.5 ASR 服务链路验证

启动 CP ASR 服务链路（mic → AEC → asr_service，末端识别回调为 stub）：

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

### 6.6 audio_play / audio_record 组件验证

这两条命令直接驱动 `audio_play` / `audio_record` 组件，不经过 voice/player service，是验证组件本身最短的路径。

`cp_audio_play`：

```
cp_audio_play open [rate] [chans] [vol_db]    # 默认 16000 1 -7
cp_audio_play tone [ms]                       # rate/16 Hz 正弦，默认 1000ms
cp_audio_play clip                            # 内置 16k 单声道 PCM 提示音
cp_audio_play volume <db>
cp_audio_play pause|resume|mute|unmute
cp_audio_play close
cp_audio_play info
```

`clip` 要求当前是 16000 Hz 单声道，用其他参数 open 会被拒绝并提示。

预期：

- `cp_audio_play open ok: 16000 Hz 1 ch, frame <N> B, vol -7 dB`
- `tone` 期间喇叭能听到 `rate/16` Hz（默认 1 kHz）单音，结束打印 `tone done: <N>/<N> frames, ret=0`；两个帧数相等且 `ret=0` 才算通过，出现 `write short at frame` 说明喇叭侧背压异常
- `clip` 打印 `clip done: <总字节数>, ret=0`，并能听到完整的唤醒提示音
- `close` 后 `cp_aud_res` 中 speaker 回到 `none`

`cp_audio_record`：

```
cp_audio_record open [rate] [chans] [gain_db] # 默认 16000 1 16
cp_audio_record read [ms]                     # 打印采集字节数与峰值，默认 1000ms
cp_audio_record gain <db>
cp_audio_record pause|resume
cp_audio_record close
cp_audio_record info
cp_audio_record loop [ms] [rate]              # mic -> spk_service CALL，默认 3000ms 16000
```

`read` 打印的 peak 是判断 mic 是否真的在工作的关键：静默时接近 0，说话时明显跳变。`loop` 会同时占用 mic 和喇叭，要求 mic 当前空闲，否则提示 `loop needs the mic, close it first`；喇叭侧同理，`cp_audio_play` 还开着时要先 `close`。

预期：

- `cp_audio_record open ok: 16000 Hz 1 ch, frame <N> B, gain 16 dB`
- `read 1000` 打印 `read done: 32000 bytes, peak <P>, ret=0`；字节数应等于 `rate x chans x 2 x ms / 1000`，默认参数即 32000
- 对着 mic 说话时 `peak` 明显跳变，静默时接近 0；peak 恒为 0 说明 mic 没有真正采到数据
- `loop` 期间能从喇叭听到自己的声音，结束打印 `loop done: <N>/<N> frames, ret=0`
- `close` 后 `cp_aud_res` 中 mic 回到 `none`

### 6.7 spk_service 三路混音验证

`cp_spk` 按 headset 的架构驱动 `spk_service`，三路音源（A2DP/CALL/HINT）通过 DAC 硬件混音，是本工程内存占用的峰值场景。

```
cp_spk init | deinit
cp_spk <a2dp|call|hint> tone [ms] [rate]      # 裸 PCM 灌进对应音源 FIFO
cp_spk <a2dp|call|hint> mp3 | wav             # 解码通路，等同 BT 音乐/通话
cp_spk mix [ms]                               # 三路同时，峰值内存
cp_spk stop [a2dp|call|hint]                  # 不带参数停全部
cp_spk volume <db> | mute <0|1> | srcmute <a2dp|call|hint> <0|1>
cp_spk info
```

建议在 `cp_spk mix` 前后各执行一次 `memshow` 观察 PSRAM 堆的峰值压力。

### 6.8 最短放音自检

`cp_prompt` 内置一小段 16 kHz 单声道 G711A 数据，自建 `raw_stream → g711_decoder → onboard_speaker_stream_v2` 三级 pipeline 播放，不依赖 voice_service / player_service / spk_service 任何一个。

```
cp_prompt play [ms]                           # 默认 500ms，最长 3000ms
```

喇叭没声音时用它做二分定位：`cp_prompt play` 有声说明 DAC 和解码链正常，问题在上层服务。

### 6.9 资源仲裁与互斥保护

喇叭和 mic 各只有一份硬件资源。所有音频 CLI 都通过 `cp_audio_res` 登记占用，冲突时后来者被拒绝并打印当前持有者。

```
cp_aud_res                                    # 查看 speaker/mic 当前持有者
```

各命令的资源占用：

| 命令 | 占喇叭 | 占 mic | owner | 归还时机 |
|---|---|---|---|---|
| `cp_audio_play open` | 是 | — | `cp_audio_play` | `cp_audio_play close` |
| `cp_audio_record open` | — | 是 | `cp_audio_record` | `cp_audio_record close` |
| `cp_audio_record loop` | 是 | 是 | `spk_service` + `cp_audio_record` | mic 自动；喇叭需 `cp_spk deinit` |
| `cp_spk init` 或任一 `cp_spk` 播放 | 是 | — | `spk_service` | `cp_spk deinit` |
| `voice_service start` | 是 | 是 | `voice_service` | `voice_service stop` |
| `player_service playback start` | 是 | — | `player_service` | 播放结束自动 |
| `cp_prompt play` | 是 | — | `cp_prompt` | 播放结束自动 |
| `audio asr_turn_on` | — | 是 | `asr` | `audio asr_turn_off` |

`spk_service` 是唯一的例外：它是常驻服务而不是 per-job 资源。所有用到它的命令（`cp_spk` 各子命令、`cp_audio_record loop`）都只"确保它起来"，喇叭令牌由 `cp_spk` 统一持有，**只有 `cp_spk deinit` 才归还**。

这是有意设计的。`SPK_SERVICE_IDLE_LINGER_MS`（默认 5000）让最后一路音源 detach 后 DAC 再多活 5 秒，用来桥接切歌、通话转音乐这类短间隙，避免 DAC 背靠背重初始化产生 pop 声。如果每个 job 结束就归还令牌，就会出现"令牌显示空闲、DAC 其实还在跑"的 5 秒窗口；此时另一条通路去初始化喇叭，会和仍在运行的实例抢同一份全局状态。让令牌的持有期覆盖硬件的存活期，这个窗口就不存在。

所以归零的判断标准是：

- 除 `spk_service` 以外的 owner，每组测完都应回到 `none`，否则说明某条路径漏了 release。
- 看到 `speaker owner=spk_service` 是正常的，敲一次 `cp_spk deinit` 即归还。

典型互斥用例，预期是被拒绝并打印 owner，而不是 dump：

```
cp_spk init
cp_audio_play open          # 预期被拒，owner=spk_service
cp_spk deinit

voice_service start onboard 16000 0 pcm pcm onboard 16000 1 call
cp_spk init                 # 预期被拒，owner=voice_service
cp_audio_record open        # 预期被拒，mic 已被 voice_service 占
audio asr_turn_on 0 0 16000 1   # 预期被拒，mic 冲突
voice_service stop
cp_aud_res                  # 预期全部回到 none
```

`loop` 跑完后喇叭仍归 `spk_service`，验证一下这是可归还的而不是泄漏：

```
cp_audio_record loop 3000 16000
cp_aud_res                  # 预期 speaker owner=spk_service refs=1, mic owner=none
cp_audio_play open          # 预期被拒，owner=spk_service
cp_spk deinit
cp_aud_res                  # 预期 speaker/mic 都是 none
```

连续跑两次 `loop` 再 `cp_spk deinit`，`refs` 应始终是 1 而不是 2 —— 令牌只在服务第一次起来时取一次。

说明：`audio_play` 组件自带的 `aud` 命令不经过 `cp_audio_res` 仲裁，本工程**有意不注册**它，避免与 `cp_spk` 同时操作共享喇叭导致重复初始化。需要单独验证组件时，可参考 `projects/bluetooth/headset` 在 `cp_main.c` 中调用 `audio_play_cli_init()`。

## 7. 常见日志与诊断

- `voice_service start` 返回错误时，先检查参数个数和 codec 组合是否受支持。
- 回环有异响时，优先打开 `audio_obs all 5000`，观察 mic、eq、spk 是否有持续 short/timeout/error，及 p95/p99 是否异常。
- 播放或提示音断续时，重点查看 `ONBOARD_SPK` 和 decoder/raw stream 相关统计。
- ASR 当前若打印 `kws=stub` 或 `lib=not_linked`，说明算法库未接入，属于预期行为。现有 CPU KWS 库是 AP/M55 硬浮点 + MVE 编译的，CP/M52 是软浮点，链不进来，因此 `CONFIG_CP_KWS_CPU_LIB` 保持关闭。在 M52 版算法库到位之前，`audio asr_turn_on` 是唯一有意义的 ASR 验证路径：它跑完整的 mic/AEC/asr_service 链路，只有末端识别回调是 stub。
- 命令被拒绝并打印 `owner=xxx` 时，是 `cp_audio_res` 仲裁在起作用，属于预期的互斥保护。先停掉持有者再重试，参见 6.9 节。
- 如无线侧影响音频测试，可先执行 `AT+BLEPOWER=0` 后再验证。

