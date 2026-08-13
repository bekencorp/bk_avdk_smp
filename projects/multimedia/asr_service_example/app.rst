.. _project_app:

app
=============================

Overview
-----------------------------

本工程演示 BK7259 上基于 ADK 的语音识别（ASR）服务，引擎为 **BEKEN_KWS**（TFLite-Micro + NPU）。

支持两种工作模式：

1. ``startwithmic``：ASR 服务直接打开板载/UAC 麦克风采集并识别。
2. ``startnomic``：通过 Voice Service 获取麦克风数据流，再交给 ASR 识别。

识别结果通过回调打印关键词日志（与 doorbell 工程词表一致，例如 ``nihaobotong``、``zaijianbotong`` 等）。

Hardware Requirements
------------------------------

- BK7259 开发板
- 板载麦克风（或 UAC 麦克风）
- 串口终端接 **CP UART**（默认波特率 **115200**）
- AP 日志通过 mailbox 回传到 CP 串口；AP CLI 需加 ``ap_cmd`` 前缀

Configure and Build
-----------------------------

Configure the Project
****************************

工程默认配置见 ``ap/config/bk7259_ap/defconfig``，关键项包括：

- ``CONFIG_ASR_SERVICE`` / ``CONFIG_ASR_SERVICE_WITH_MIC``
- ``CONFIG_BEKEN_KWS`` / ``CONFIG_TFLITE_MICRO`` / ``CONFIG_NPU``
- ``CONFIG_VOICE_SERVICE``（用于 ``startnomic``）
- ``CONFIG_ADK_*``（MIC/Speaker/RAW/AEC/RSP/G711）

Build the Project
****************************

::

    make bk7259 PROJECT=multimedia/asr_service_example

Flash
****************************

按平台常规方式烧录 AP/CP 固件后上电。

Running and Output
------------------------------

Operate
*****************************

上电后在 **CP 串口**输入（AP 命令需 ``ap_cmd`` 前缀）。

可先确认命令已注册::

    ap_cmd help

1. 板载麦 + 16 kHz（推荐，与 KWS 默认采样率一致）::

    ap_cmd asr_service startwithmic onboard 16000

2. 板载麦 + AEC::

    ap_cmd asr_service startwithmic onboard 16000 1

3. 经 Voice Service 取流识别::

    ap_cmd asr_service startnomic onboard 16000 1

4. 停止::

    ap_cmd asr_service stop

命令格式：

::

    ap_cmd asr_service {startwithmic|startnomic|stop} [onboard|uac] [8000|16000] [aec_en]

- ``aec_en``：可选，``0/1`` 或 ``aec``；UAC 路径不启用 AEC。
- 采样率为 ``8000`` 时会启用重采样到 ASR 引擎所需的 16 kHz。

Output
*****************************

启动成功返回 ``CMDRSP:OK``。说出支持的唤醒/命令词后，日志类似：

::

    asr_cli:nihaobotong, cmd: 1
    asr_cli:zaijianbotong, cmd: 2
    asr_cli:play music, cmd: 3
