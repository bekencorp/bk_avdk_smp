.. _project_kvs_aws_sample:

kvs_aws_sample
=============================

概述
-----------------------------
本工程为 m3_7259_v2 平台上的 **AWS Kinesis Video Streams (KVS) WebRTC P2P** 示例。目标：由 **Master** 端发送 H.264/H.265/Opus 音视频数据，由 **Viewer** 端接收并处理。

**角色在运行时通过串口 CLI 命令选择**，同一固件可随时切换为 Master 或 Viewer，无需重新编译。


角色说明：Master 与 Viewer
-----------------------------

- **Master（主端 / 发送端）**
  - 连接 KVS 信令后，从 ``ap/`` 目录下的静态示例帧发送音视频：
    - ``ap/h264SampleFrames`` — H.264 视频
    - ``ap/h265SampleFrames`` — H.265 视频
    - ``ap/opusSampleFrames`` — Opus 音频
  - 需与 Viewer 使用**同一信令通道名（channel name）**才能互通。

- **Viewer（观看端 / 接收端）**
  - 连接同一信令通道后，接收 Master 发来的音视频流。
  - 收到的帧由 **Common.c** 中的 ``sampleVideoFrameHandler`` / ``sampleAudioFrameHandler`` 处理（默认打日志，可自行扩展为落盘、解码或送显）。

硬件要求
------------------------------
- 支持 m3_7259_v2 的开发板。
- 串口：用于 CLI 输入与日志输出（默认使用 LOG UART，见 defconfig 中 CONFIG_DUMP_BY_LOG_UART）。
- 网络：设备需能访问 AWS（Wi‑Fi 等），并在使用前完成网络配置（如 STA 连接）。

配置与编译
-----------------------------

配置工程
****************************
工程 defconfig（``ap/config/bk7259_ap/defconfig``）中已默认使能：

- **CONFIG_KVS_AWS** — KVS WebRTC 组件
- **CONFIG_CLI** — 串口 CLI（用于输入 ``kvs`` 命令）

如需 Data Channel 功能，可在 menuconfig 中勾选 **CONFIG_KVS_AWS_DATA_CHANNEL**（Component config → AWS KVS WebRTC）。

编译
****************************
在 SDK 根目录执行::

   make bk7259 PROJECT=kvs_aws_sample

烧录
****************************
按 m3_7259_v2 常规方式烧录生成的固件到设备。

使用方式（详细）
-----------------------------

整体流程
****************************
1. **上电** → 设备启动，完成 ``bk_init()`` 与 CLI 初始化。
2. **连接串口** → 使用终端工具（如 minicom、SecureCRT）连接 LOG UART，波特率与板级一致。4
3. **配置网络**（若未预配）→ 通过 CLI 配置 STA 连接，使设备能访问互联网（可先执行 ``ip``、``scan`` 等命令，具体以平台 CLI 为准）。
4. **选择角色并启动 KVS** → 在串口输入 ``kvs master`` 或 ``kvs viewer``（见下方命令说明）。
5. **观察日志** → 连接、收发、错误等信息会从串口输出。

CLI 命令说明
****************************
角色与信令通道均通过 **kvs** 命令指定，无需在 menuconfig 中区分 Master/Viewer。

命令格式::

   kvs <role> [channel_name]

- **role**（必填）：
  - ``master`` — 以 Master 身份启动，发送音视频。
  - ``viewer`` — 以 Viewer 身份启动，接收音视频。
- **channel_name**（可选）：信令通道名称。Master 与 Viewer 必须使用**相同的 channel_name** 才能建立 P2P。省略时使用默认通道名 ``kvs_aws_channel``。

示例
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- 使用默认通道启动 Master::

     kvs master

- 使用默认通道启动 Viewer::

     kvs viewer

- 使用自定义通道名（两端需一致）::

     kvs master my_channel
     kvs viewer my_channel

- 查看帮助（仅输入 ``kvs`` 或参数不全时会打印）::

     kvs

注意：执行 ``kvs master`` 或 ``kvs viewer`` 后，会进入 KVS 主循环；若需切换角色，需重启设备或等当前会话结束后再输入新命令（视具体实现而定）。

运行与输出
-----------------------------

操作说明
*****************************
- **Master**：输入 ``kvs master [channel_name]`` 后，设备连接 KVS 信令，从 h264/h265/opusSampleFrames 读取帧并发送。
- **Viewer**：在另一台设备（或同一台设备重启后）输入 ``kvs viewer [channel_name]``，使用相同 channel_name 连接同一信令通道，即可接收对端音视频；帧在 Common.c 的 ``sampleVideoFrameHandler`` / ``sampleAudioFrameHandler`` 中处理。

输出
*****************************
根据日志级别，串口可看到连接建立、收发、错误等日志；Viewer 端收到帧时由上述回调打点或扩展处理。

依赖
-----------------------------
- **kvs_aws**、**bk_common**、**lwip_intf_v2_1**、**bk_cli**。
- 可选：**CONFIG_KVS_AWS_DATA_CHANNEL**（Data Channel 功能）。

凭证配置
-----------------------------
KVS 信令与 API 调用需要 AWS 凭证。本示例在 **Common.c** 的 ``createSampleConfiguration()`` 中通过 **getenv()** 读取配置；在嵌入式平台上，若运行时不提供环境变量，则需在启动前设置（如 setenv）或修改代码从 Flash/配置表读取。支持两种方式：

方式一：Access Key（默认，未定义 IOT_CORE_ENABLE_CREDENTIALS 时）
********************************************************************

**必须设置的“环境变量”（或平台等价配置）：**

- **AWS_ACCESS_KEY_ID** — IAM 用户的 Access Key ID。
- **AWS_SECRET_ACCESS_KEY** — 对应该 Access Key 的 Secret Access Key。

获取方式：在 AWS 控制台 → IAM → 用户 → 安全凭证中创建访问密钥；该用户需具备 Kinesis Video Streams 与信令相关权限（如 ``kinesisvideo:*``、``signaling:*`` 等）。

**可选：**

- **AWS_DEFAULT_REGION** — 使用的 AWS 区域（如 ``us-east-1``、``cn-north-1``）。未设置时使用代码中的 ``DEFAULT_AWS_REGION``。
- **AWS_SESSION_TOKEN** — 使用临时凭证（如 STS AssumeRole）时必填；长期 Access Key 可不设。
- **AWS_KVS_CACERT_PATH** — TLS 根证书文件路径。未设置时使用**编译期默认路径**：工程通过 CMake 将 ``KVS_CA_CERT_PATH`` 设为 ``ap/certs/cert.pem``（见 ``ap/CMakeLists.txt``），即默认使用 ``projects/kvs_aws_sample/ap/certs/cert.pem``。若使用自签名或不同 CA，可替换该文件或设置本变量指向其他路径。
- **AWS_KVS_LOG_LEVEL** — 日志级别（数值），不设则使用默认级别。
- **CONTROL_PLANE_URI** — 若需指定信令控制面 URL，可设置该环境变量。

方式二：IoT Core 凭证（编译选项 IOT_CORE_ENABLE_CREDENTIALS=ON）
********************************************************************

若在 CMake 中打开 ``IOT_CORE_ENABLE_CREDENTIALS``（如 ``-DIOT_CORE_ENABLE_CREDENTIALS=ON``），则使用 IoT 预置凭证，不再使用 Access Key，此时必须设置：

- **AWS_IOT_CORE_CREDENTIAL_ENDPOINT** — IoT 凭证服务端点。
- **AWS_IOT_CORE_CERT** — 设备证书（或证书路径/内容，视 SDK 实现而定）。
- **AWS_IOT_CORE_PRIVATE_KEY** — 设备私钥。
- **AWS_IOT_CORE_ROLE_ALIAS** — 用于获取临时凭证的 Role Alias。
- **AWS_IOT_CORE_THING_NAME** — IoT Thing 名称；未在命令行指定 channel 时也会用作默认 channel 名。

可选：**AWS_IOT_CORE_CERTIFICATE_ID** — 若设置，将作为信令通道名使用。

CA 证书与 TLS
****************************
- 默认：使用 ``ap/certs/cert.pem`` 作为 TLS 根证书（编译时写入 ``KVS_CA_CERT_PATH``）。请确保该文件为当前 AWS 区域/端点所信任的根证书或 CA 链（通常为 Amazon Trust Services 等）。
- 自定义路径：在运行前设置环境变量 **AWS_KVS_CACERT_PATH** 指向你的 CA 证书文件路径（或目录，代码会遍历目录查找 ``.pem``，见 ``lookForSslCert()``）。

嵌入式上如何提供“环境变量”
****************************
本示例通过 ``getenv()`` 读取上述名称。在无标准环境变量的 RTOS 上，可任选其一：

1. **启动时 setenv**：在 ``main()`` 或 ``bk_init()`` 之后、调用 ``kvs_cli_init()`` 或执行 ``kvs master/viewer`` 之前，用平台提供的 ``setenv("AWS_ACCESS_KEY_ID", "你的Key", 1)`` 等写入；若平台无 setenv，需在 C 库或 RTOS 中实现或挂接。
2. **改代码写死或从 Flash 读**：在 ``createSampleConfiguration()`` 中不再依赖 ``GETENV(ACCESS_KEY_ENV_VAR)``，改为使用静态缓冲区或从 NVS/Flash 读取的凭证，并传给 ``createStaticCredentialProvider()``（注意安全：不要将密钥明文提交到代码仓库）。
3. **使用 IoT Core 方式**：见下方「方式 A：IoT Core + 预置证书 + Role Alias」。

更安全的凭证方式（推荐）
-----------------------------

方式 A：IoT Core + 预置证书 + Role Alias（设备不存 Access Key）
********************************************************************

设备只保存 **IoT 设备证书 + 私钥**，通过 AWS IoT 凭证接口换取**临时凭证**，无需在设备上配置 IAM Access Key。

**1. 在 AWS 控制台准备**

- **IoT 事物 (Thing)**：IoT Core → 管理 → 事物 → 创建事物，记下事物名称（如 ``MyKvsDevice``）。
- **设备证书**：为该事物创建或绑定 X.509 证书（IoT → 安全 → 证书），下载或生成：
  - 设备证书（如 ``xxx-certificate.pem.crt``）
  - 私钥（如 ``xxx-private.pem.key``）
- **策略 (Policy)**：IoT → 安全 → 策略，创建策略，允许该证书连接并获取凭证，例如：:

    {
      "Version": "2012-10-17",
      "Statement": [
        {
          "Effect": "Allow",
          "Action": "iot:Connect",
          "Resource": "arn:aws:iot:区域:账号:client/${iot:Connection.Thing.ThingName}"
        },
        {
          "Effect": "Allow",
          "Action": "iot:Subscribe",
          "Resource": "arn:aws:iot:区域:账号:topicfilter/$aws/credentials/*"
        },
        {
          "Effect": "Allow",
          "Action": "iot:Receive",
          "Resource": "arn:aws:iot:区域:账号:topic/$aws/credentials/*"
        }
      ]
    }

  将策略附加到该证书。
- **IAM 角色 + Role Alias**：
  - IAM 中创建一个角色，信任关系为 ``iot.amazonaws.com``，并附加有 KVS/信令权限的策略（如 ``AmazonKinesisVideoStreamsFullAccess`` 或最小权限）。
  - IoT Core → 安全 → 角色别名，创建角色别名（如 ``KvsCredentialRoleAlias``），选择上述 IAM 角色。
- **凭证端点**：格式为 ``https://你的-iot-端点.iot.区域.amazonaws.com/role-aliases/角色别名/credentials``，或查阅 AWS 文档 “IoT 临时凭证” 获取端点格式。中国区使用 ``.iot.区域.amazonaws.com.cn``。

**2. 设备端配置**

- **证书与私钥**：将设备证书和私钥放到设备可访问的路径（如 Flash 或文件系统）。本示例中 ``createLwsIotCredentialProvider()`` 需要的是**文件路径**，即：
  - ``AWS_IOT_CORE_CERT`` → 设备证书文件的完整路径（如 ``/cert/device.pem``）。
  - ``AWS_IOT_CORE_PRIVATE_KEY`` → 私钥文件的完整路径（如 ``/cert/device.key``）。
- **环境变量（或 ap_main.c 中 setenv）**：在运行 KVS 前设置：
  - ``AWS_IOT_CORE_CREDENTIAL_ENDPOINT`` — 上一步的凭证端点 URL。
  - ``AWS_IOT_CORE_CERT`` — 设备证书**文件路径**。
  - ``AWS_IOT_CORE_PRIVATE_KEY`` — 设备私钥**文件路径**。
  - ``AWS_IOT_CORE_ROLE_ALIAS`` — 角色别名（如 ``KvsCredentialRoleAlias``）。
  - ``AWS_IOT_CORE_THING_NAME`` — 事物名称（如 ``MyKvsDevice``）。
- **CA 证书**：``pCaCertPath`` 仍用于 TLS 连接 IoT 端点，默认使用 ``ap/certs/cert.pem``，确保其包含 AWS IoT 根 CA。

**3. 编译启用 IoT 凭证**

在工程中打开 IoT 凭证开关，使 ``Common.c`` 走 ``createLwsIotCredentialProvider()`` 分支：

- 在 ``projects/kvs_aws_sample/ap/CMakeLists.txt`` 中，将 ``KVS_USE_IOT_CREDENTIALS`` 设为 ``ON``（见该文件内注释），或配置时传入：:

    cmake ... -DKVS_USE_IOT_CREDENTIALS=ON

  然后重新编译、烧录。启用后**不再**需要 ``AWS_ACCESS_KEY_ID`` / ``AWS_SECRET_ACCESS_KEY``。

方式 B：设备只与后端通信，由后端带密钥调 AWS
********************************************************************

设备**不存任何 AWS 长期密钥**，只与自有后端通信；后端持有 IAM 密钥或角色，调用 AWS 获取临时凭证（或信令 Token），再下发给设备使用。

**架构简述**

- 设备 ↔ 你的后端：HTTPS 或其它安全通道。
- 后端 ↔ AWS：使用 IAM Access Key 或 IAM 角色调用 KVS 信令 API 或 STS ``GetSessionToken`` / ``AssumeRole`` 等，拿到临时 Access Key / Secret Key / Session Token。
- 后端把临时凭证（或封装好的 Token）下发给设备；设备在调用 KVS SDK 时使用这些临时凭证。

**本示例中的实现思路**

当前示例只支持两种凭证来源：静态 Access Key（环境变量）和 IoT 凭证接口。要采用“后端下发临时凭证”方式，需要：

1. **实现自定义凭证提供者**：参考 SDK 中的 ``AwsCredentialProvider`` 接口（见 ``kvs_aws`` 组件中 ``createStaticCredentialProvider`` / ``getCredentialsFn``），实现一个提供者：在 ``getCredentialsFn`` 里向你的后端发起请求（如 HTTPS），获取临时 ``accessKeyId``、``secretKey``、``sessionToken``（及过期时间），填入 ``AwsCredentials`` 并返回。
2. **在 Common.c 中使用**：在 ``createSampleConfiguration()`` 中，不调用 ``createStaticCredentialProvider()`` 或 ``createLwsIotCredentialProvider()``，改为调用你实现的“后端凭证提供者”的创建函数，将返回的 ``pCredentialProvider`` 赋给 ``pSampleConfiguration->pCredentialProvider``。
3. **后端接口**：由你自行实现。例如提供 ``GET/POST /get-temp-credentials``，后端用 IAM 调用 STS 拿到临时凭证后返回 JSON；设备解析后填入 ``AwsCredentials``。注意后端需做好鉴权（如设备证书或 Token），避免接口被滥用。

这样设备上只需配置后端 URL 和与后端之间的认证信息，无需配置任何 AWS Access Key。
