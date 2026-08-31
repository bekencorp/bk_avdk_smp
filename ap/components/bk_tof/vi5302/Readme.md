# VI5302 ToF Driver (Visionics Official SDK)

本目录为 **Visionics（纬感）VI5302** dToF 测距芯片的官方 MCU SDK，已集成进 Beken `bk_tof` 组件。  
芯片通过 I2C 与主机通信，上电后需先下载内置固件，再进行标定与测距。

原厂参考包（不参与编译）：`projects/beken_robot/VI5302_G_MCU_C01_R00_R14_20260320/`

---

## 1. VI5302 简介

| 项目 | 说明 |
|------|------|
| 类型 | Direct Time-of-Flight（dToF）单点测距 |
| 通信 | I2C，8-bit 地址 `0xD8`（7-bit `0x6C`） |
| 寄存器 | **16-bit** 地址（先发高字节再发低字节） |
| 控制脚 | **XSHUT**：硬件复位/使能（低复位，高工作） |
| 中断 | **GPIO1/INT**：测距完成（可选硬件中断或读寄存器轮询） |
| 工作方式 | 主机 I2C 下发命令；测距算法在芯片固件内运行 |

典型输出：`correction_tof`（校正距离 mm）、`confidence`（置信度，一般 >30 认为有效）。

---

## 2. 目录与模块

```
components/bk_tof/
├── include/bk_tof_vi5302.h    # 应用层 API（推荐只用这层）
├── tof_vi5302.c               # BK 封装：init 流程、后台线程、回调
├── Kconfig / CMakeLists.txt
└── vi5302/                    # Visionics 官方 SDK（本目录）
    ├── VI5302_User_Handle.c/h # ★ 平台移植层（唯一必改文件）
    ├── VI5302_API.c/h         # 寄存器命令、测距、标定
    ├── VI5302_Firmware.c/h    # 芯片固件二进制 (~7KB)
    ├── VI5302_System_Data.c/h # 运行时 system data 读写
    ├── VI5302_Algorithm.c/h   # pileup/noise 补偿、置信度算法
    ├── VI5302_Efuse.c/h       # OTP 读写（产线用，产品运行时不需要）
    └── Readme.md
```

### 数据流

```
应用 (bk_tof_vi5302_*)
    → tof_vi5302.c
        → VI5302_API.c（命令 / 读数 / 标定）
            → VI5302_User_Handle.c（I2C / GPIO / 延时）
            → VI5302_Firmware.c（上电下载固件）
            → VI5302_Algorithm.c（距离后处理）
            → VI5302_System_Data.c（积分时间等参数）
```

---

## 3. 不同平台如何适配

**原则：只改 `VI5302_User_Handle.c`（及板级 `usr_gpio_cfg.h` / defconfig），其余 `.c/.h` 保持原厂代码不动。**

### 3.1 必须实现的接口（User_Handle）

原厂在 `VI5302_User_Handle.h` 中声明，移植时需全部实现：

| 函数 | 作用 | 适配要点 |
|------|------|----------|
| `IIC_Write_X_Bytes()` | I2C 写 | 16-bit 寄存器地址 + 数据；`dev_addr=0xD8` |
| `IIC_Read_X_Bytes()` | I2C 读 | 同上 |
| `VI5302_IIC_*` | 封装读写 | 固定设备地址 `VI5302_IIC_DEV_ADDR`，一般直接调上面两个 |
| `VI5302_Delay_Ms()` | 毫秒延时 | RTOS `delay_ms` / `sleep` 等 |
| `VI5302_XSHUT_Enable()` | 复位脚 | `1`=拉高工作，`0`=拉低复位 |
| `VI5302_GPIO_Interrupt_Handle()` | 中断回调 | INT 下降沿时置位 `VI5302_GPIO_Interrupt_status` |

BK7259 额外提供（同文件内，供 `tof_vi5302.c` 调用）：

| 函数 | 作用 |
|------|------|
| `vi5302_board_hal_init()` | 配置 XSHUT/INT GPIO、初始化 I2C |
| `vi5302_board_hal_deinit()` | 反初始化 |
| `vi5302_board_enable_irq()` | 开关 INT 硬件中断 |

### 3.2 板级配置（每块 PCB 必查）

在 **`VI5302_User_Handle.c` 顶部宏** 或 Kconfig 中定义：

```c
#define TOF_I2C_ID        I2C_ID_x      /* I2C 控制器编号 */
#define TOF_XSHUT_PIN     GPIO_xx       /* XSHUT 输出 */
#define TOF_INT_PIN       GPIO_xx       /* INT 输入，下降沿 */
/* I2C 速率建议 400kHz，原厂支持最高 1MHz */
```

在 **`usr_gpio_cfg.h`** 中配置引脚功能（示例：SCH-Robot V2）：

| 信号 | GPIO | 说明 |
|------|------|------|
| I2C1_SCL | 45 | 可与 IMU 等器件共用总线 |
| I2C1_SDA | 46 | |
| XSHUT | 47 | 输出 |
| TOF_INT | 48 | 输入，上拉 |

**供电**：确认 TOF_VDD / 外设 3.3V 在 `init` 前已拉高（如 Robot 板 `GPIO_53` / `media_board_power_on()`）。

### 3.3 I2C 地址换算

官方 SDK 使用 **8-bit 写地址 `0xD8`**；Beken `bk_i2c_*` 使用 **7-bit `0x6C`**：

```c
dev_addr_7bit = addr_8bit >> 1;   /* 0xD8 → 0x6C */
```

寄存器访问使用 **`I2C_MEM_ADDR_SIZE_16BIT`**（与原厂一致：先高 8 位地址，再低 8 位）。

### 3.4 可选：硬件中断 vs 寄存器轮询

在 `tof_vi5302.c` / 应用 init 前通过 Kconfig 或代码设置：

| 模式 | `VI5302_Interrupt_Mode_Status` | Kconfig |
|------|-------------------------------|---------|
| 寄存器轮询（默认） | `0x00` | `CONFIG_TOF_VI5302_HW_IRQ=n` |
| GPIO 硬件中断 | `0x88` | `CONFIG_TOF_VI5302_HW_IRQ=y` |

硬件 IRQ 时需正确连接 INT 引脚并在 `usr_gpio_cfg.h` 中配置为输入中断。

### 3.5 可选：OTP / Efuse（一般产品不需要）

| 函数 | 场景 |
|------|------|
| `VI5302_7V5_Enable()` | 写 Efuse 需 ~7.5V 编程电压；无 OTP 烧录则空实现 |
| `VI5302_Efuse_Read/Write()` | 产线一次性写入；**正常运行不调用** |

### 3.6 多设备共用 I2C 总线

- VI5302 地址固定 `0x6C`，注意与 IMU、传感器地址不冲突。
- 避免多个模块重复 `bk_i2c_init()` 覆盖波特率；建议统一在一个地方初始化 I2C，User_Handle 仅做 memory read/write。
- 测距期间尽量不要长时间占用总线。

### 3.7 不需要改动的文件

| 文件 | 说明 |
|------|------|
| `VI5302_API.c` | 官方命令与测距逻辑 |
| `VI5302_Firmware.c` | 固件 blob，随 SDK 版本更新 |
| `VI5302_Algorithm.c` | 原厂调校算法 |
| `VI5302_System_Data.c` | system data 协议 |
| `VI5302_Efuse.c` | 保留即可，可不调用 |

升级 SDK：用新版本覆盖上述文件，**保留** 已移植的 `VI5302_User_Handle.c`。

---

## 4. 应用层 API（推荐入口）

头文件：`components/bk_tof/include/bk_tof_vi5302.h`

```c
#include <bk_tof_vi5302.h>

/* 1. 初始化（XSHUT 复位 → 下载固件 → 配置帧率/积分） */
bk_tof_vi5302_init();

/* 2. 恢复已保存的标定（产线或首次 cali 后写入 Flash） */
bk_tof_vi5302_apply_cali(cg_pos, offset);

/* 3. 注册连续测距回调 */
bk_tof_vi5302_register_callback(my_cb, NULL);
bk_tof_vi5302_start_continuous();

/* 4. 或阻塞读一次 */
tof_vi5302_result_t r;
bk_tof_vi5302_start_once();
bk_tof_vi5302_read(&r, 500);   /* timeout ms */

/* 5. 产线标定（仅需一次） */
bk_tof_vi5302_factory_calibrate(500);  /* 500mm 标准距离，Xtalk 需空 FOV */
```

结果结构：

```c
typedef struct {
    uint16_t distance_mm;  /* 校正后距离 */
    uint8_t  confidence;   /* >30 通常有效 */
    uint8_t  status;
} tof_vi5302_result_t;
```

CLI 示例（`cli_test_file.c`）：`tof start|stop|read|once|cali [mm]`

---

## 5. 初始化流程（内部）

`bk_tof_vi5302_init()` 等价于原厂 `VI5302_main()` 的非阻塞精简版：

1. `vi5302_board_hal_init()` — I2C + GPIO  
2. `VI5302_Chip_Register_Init()` — XSHUT 复位  
3. `VI5302_Download_Firmware()` — 写入 `VI5302_firmware_buff`，检查 `REG 0x08 == 0x66`  
4. `VI5302_Set_Integralcounts_Frame(30, 131072)` — 默认 30fps  
5. （可选）`bk_tof_vi5302_apply_cali()` — 加载标定  
6. 启动后台线程，等待测距中断/轮询  

> **注意**：`0x66` 为固件启动一次性标志，已在 `Download_Firmware()` 内检查，**不要**在 init 中重复调用 `Get_VI5302_Download_Firmware_Status()`。

---

## 6. 标定说明

| 标定 | 官方 API | 条件 | 保存字段 |
|------|----------|------|----------|
| Xtalk（串扰） | `VI5302_Xtalk_Calibration()` | 空 FOV，无遮挡 | `VI5302_Cali_CG_Pos` |
| Offset（零点） | `VI5302_Offset_Calibration(500, &offset)` | 500mm 处标准反射板 | `VI5302_Cali_Offset` |

模组 **只需标定一次**，结果存 **MCU Flash**，开机 `bk_tof_vi5302_apply_cali(cg_pos, offset)` 恢复。  
**不要**写入芯片 Efuse（一次性，不可改）。

---

## 7. Kconfig

```
CONFIG_TOF_ENABLE=y
CONFIG_TOF_VI5302_ENABLE=y
CONFIG_TOF_VI5302_HW_IRQ=n    # n=寄存器轮询, y=GPIO 中断
```

---

## 8. 移植检查清单

- [ ] `VI5302_User_Handle.c`：I2C 16-bit 地址读写正常  
- [ ] I2C 7-bit 地址 `0x6C`（由 `0xD8` 右移）  
- [ ] XSHUT / INT 引脚与原理图一致，`usr_gpio_cfg.h` 已配置  
- [ ] TOF 供电在 init 前已打开  
- [ ] `VI5302_Read_ChipVersion()` 返回 `0x02`  
- [ ] 固件下载 log：`firmware download ok`  
- [ ] 测距 `confidence > 30`，距离合理  
- [ ] 产线标定已保存并在 boot 时 `apply_cali`  

---

## 9. 常见问题

| 现象 | 可能原因 |
|------|----------|
| `Download_Firmware fail REG_0x08=0x00` | I2C 不通、未供电、XSHUT 未拉高、地址错误 |
| `chip id mismatch` | 同上，或焊接/总线冲突 |
| 距离偏差大 | 未做或未加载 Xtalk/Offset 标定 |
| 与 IMU 冲突 | 同 I2C 重复 init 或总线被占用 |

---

## 10. 参考

- 原厂示例主流程：`VI5302_User_Handle.c` → `VI5302_main()`（参考包内，阻塞 demo）  
- BK 封装：`components/bk_tof/tof_vi5302.c`  
- Robot V2 板级：`projects/beken_robot/ap/config/bk7259_ap/usr_gpio_cfg.h`（P45–P48）
