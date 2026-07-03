# ISP 功能测试用例

## 约定
- **MIPI sensor 默认参数**：1920x1080@20fps
- **DVP sensor 默认参数**：1280x720@30fps
- **输出格式默认值**：23 (NV12)

## 命令格式

### 1. isp detect
检测所有连接的传感器

### 2. isp open
```
isp open <mipi|dvp> <mp|sp> <sensor_width> <sensor_height> <fps> <isp_output_width> <isp_output_height> <frame|flexa> [output_fmt]
```

### 3. isp close
```
isp close <mp|sp>
```

### 4. isp read
```
isp read <mp|sp>
```
单次（阻塞轮询）读取一帧并以 hexdump 打印，便于快速验证数据通路。

### 5. isp dvp_cb（DVP 帧 callback 接口）
```
isp dvp_cb on [mp|sp]
isp dvp_cb off
```
以 **callback 回调**的形式把 DVP（或 ISP）帧数据持续推给应用层：
- `on`：注册示例回调并启动后台采集任务，每读到一帧就回调一次（默认 mp 通道）。
- `off`：停止采集任务并清除回调。

应用集成方式：调用 `isp_dvp_register_frame_cb()` 注册自己的回调，再调用
`isp_dvp_capture_start()` 启动采集；回调原型见 `ap/include/isp_cli.h` 的
`isp_dvp_frame_cb_t`。帧 buffer 在回调返回后即被释放，回调内需尽快消费或拷贝出去。

> 注意：采集任务基于 **frame 模式**（`bk_isp_camera_read`），因此需先用
> `isp open dvp <mp|sp> ... frame` 打开对应通道；flexa 模式不支持读帧回调。

---

## 测试用例列表

### 一、基本功能测试

#### 1.1 传感器检测
- **用例 1.1.1**: `isp detect`
  - **预期**: 检测到传感器，打印传感器名称、支持的分辨率和帧率

---

### 二、MIPI 传感器测试（默认 1920x1080@20fps）

#### 2.1 MP 通道 - Frame 模式
- **用例 2.1.1**: `isp open mipi mp 1920 1080 20 1920 1080 frame`
  - **描述**: MIPI MP通道，传感器输出1920x1080@20fps，ISP输出1920x1080@20fps，frame模式
  - **预期**: 成功打开，ISR统计正常

- **用例 2.1.2**: `isp open mipi mp 1920 1080 20 1920 1080 frame 23`
  - **描述**: 同上，显式指定输出格式23 (NV12)
  - **预期**: 成功打开

- **用例 2.1.3**: `isp open mipi mp 1920 1080 20 1280 720 frame`
  - **描述**: MIPI MP通道，传感器输出1920x1080@20fps，ISP输出1280x720@20fps（缩小），frame模式
  - **预期**: 成功打开（支持缩小）

- **用例 2.1.4**: `isp open mipi mp 1920 1080 20 2560 1440 frame`
  - **描述**: MIPI MP通道，传感器输出1920x1080@20fps，ISP输出2560x1440（放大）
  - **预期**: **应该失败**（硬件不支持放大）

#### 2.2 MP 通道 - Flexa 模式
- **用例 2.2.1**: `isp open mipi mp 1920 1080 20 1920 1080 flexa`
  - **描述**: MIPI MP通道，flexa模式，传感器输出1920x1080@20fps，ISP输出1920x1080@20fps
  - **预期**: 成功打开，ISR统计正常

- **用例 2.2.2**: `isp open mipi mp 1920 1080 20 1280 720 flexa`
  - **描述**: MIPI MP通道，flexa模式，传感器输出1920x1080@20fps，ISP输出1280x720@20fps（缩小）
  - **预期**: 成功打开

#### 2.3 SP 通道 - Frame 模式
- **用例 2.3.1**: `isp open mipi sp 1920 1080 20 1920 1080 frame`
  - **描述**: MIPI SP通道，frame模式，传感器输出1920x1080@20fps，ISP输出1920x1080@20fps
  - **预期**: 成功打开

- **用例 2.3.2**: `isp open mipi sp 1920 1080 20 640 480 frame`
  - **描述**: MIPI SP通道，传感器输出1920x1080@20fps，ISP输出640x480@20fps（缩小）
  - **预期**: 成功打开

#### 2.4 SP 通道 - Flexa 模式
- **用例 2.4.1**: `isp open mipi sp 1920 1080 20 1920 1080 flexa`
  - **描述**: MIPI SP通道，flexa模式，传感器输出1920x1080@20fps，ISP输出1920x1080@20fps
  - **预期**: 成功打开

---

### 三、DVP 传感器测试（默认 1280x720@30fps）

#### 3.1 MP 通道 - Frame 模式
- **用例 3.1.1**: `isp open dvp mp 1280 720 30 1280 720 frame`
  - **描述**: DVP MP通道，传感器输出1280x720@30fps，ISP输出1280x720@30fps，frame模式
  - **预期**: 成功打开

- **用例 3.1.2**: `isp open dvp mp 1280 720 30 640 360 frame`
  - **描述**: DVP MP通道，传感器输出1280x720@30fps，ISP输出640x360@30fps（缩小）
  - **预期**: 成功打开

- **用例 3.1.3**: `isp open dvp mp 1280 720 30 1920 1080 frame`
  - **描述**: DVP MP通道，传感器输出1280x720@30fps，ISP输出1920x1080（放大）
  - **预期**: **应该失败**（硬件不支持放大）

#### 3.2 MP 通道 - Flexa 模式
- **用例 3.2.1**: `isp open dvp mp 1280 720 30 1280 720 flexa`
  - **描述**: DVP MP通道，flexa模式，传感器输出1280x720@30fps，ISP输出1280x720@30fps
  - **预期**: 成功打开

#### 3.3 SP 通道 - Frame 模式
- **用例 3.3.1**: `isp open dvp sp 1280 720 30 1280 720 frame`
  - **描述**: DVP SP通道，frame模式，传感器输出1280x720@30fps，ISP输出1280x720@30fps
  - **预期**: 成功打开

#### 3.4 SP 通道 - Flexa 模式
- **用例 3.4.1**: `isp open dvp sp 1280 720 30 1280 720 flexa`
  - **描述**: DVP SP通道，flexa模式，传感器输出1280x720@30fps，ISP输出1280x720@30fps
  - **预期**: 成功打开

#### 3.5 DVP 帧 callback 接口（frame 模式）
- **用例 3.5.1**:
  ```
  isp open dvp mp 1280 720 30 1280 720 frame
  isp dvp_cb on mp
  # 观察持续打印 dvp_cb frame[...] 日志
  isp dvp_cb off
  isp close mp
  ```
  - **描述**: 打开 DVP MP 通道（frame 模式），启动 callback 采集任务，
    每帧回调一次，打印帧序号/分辨率/格式/大小/首字节
  - **预期**:
    - `isp dvp_cb on` 后持续看到 `dvp_cb frame[...]` 日志，帧序号递增
    - `isp dvp_cb off` 后日志停止，采集任务退出，无内存泄漏

- **用例 3.5.2**: `isp dvp_cb on`（未先 open 通道）
  - **描述**: 通道未打开就启动采集
  - **预期**: **应该失败**，提示通道未初始化/frame size 为 0

- **用例 3.5.3**: SP 通道 callback
  ```
  isp open dvp sp 1280 720 30 640 360 frame
  isp dvp_cb on sp
  isp dvp_cb off
  isp close sp
  ```
  - **描述**: DVP SP 通道的 callback 采集
  - **预期**: SP 帧持续回调，关闭后停止

---

### 四、MP 和 SP 同时工作测试

#### 4.1 先打开 MP，再打开 SP
- **用例 4.1.1**: 
  ```
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp open mipi sp 1920 1080 20 640 480 frame
  ```
  - **描述**: 先打开MP通道，再打开SP通道
  - **预期**: 
    - MP通道成功打开
    - SP通道成功打开（只调用create_instance和start，不重新初始化）
    - ISR统计显示两个通道都在工作

#### 4.2 先打开 SP，再打开 MP
- **用例 4.2.1**: 
  ```
  isp open mipi sp 1920 1080 20 640 480 frame
  isp open mipi mp 1920 1080 20 1920 1080 frame
  ```
  - **描述**: 先打开SP通道，再打开MP通道
  - **预期**: 
    - SP通道成功打开
    - MP通道成功打开（只调用create_instance和start，不重新初始化）
    - ISR统计显示两个通道都在工作

#### 4.3 MP 和 SP 不同模式组合
- **用例 4.3.1**: 
  ```
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp open mipi sp 1920 1080 20 640 480 flexa
  ```
  - **描述**: MP使用frame模式，SP使用flexa模式
  - **预期**: 两个通道都能正常工作

---

### 五、关闭通道测试

#### 5.1 单独关闭 MP
- **用例 5.1.1**: 
  ```
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp close mp
  ```
  - **描述**: 打开MP后关闭MP
  - **预期**: 
    - MP通道成功关闭
    - ISP资源未完全释放（因为SP可能还在使用）
    - ISR统计停止

#### 5.2 单独关闭 SP
- **用例 5.2.1**: 
  ```
  isp open mipi sp 1920 1080 20 640 480 frame
  isp close sp
  ```
  - **描述**: 打开SP后关闭SP
  - **预期**: 
    - SP通道成功关闭
    - ISP资源未完全释放（因为MP可能还在使用）
    - ISR统计停止

#### 5.3 关闭所有通道
- **用例 5.3.1**: 
  ```
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp open mipi sp 1920 1080 20 640 480 frame
  isp close mp
  isp close sp
  ```
  - **描述**: 打开MP和SP后，依次关闭
  - **预期**: 
    - 关闭MP后，ISP资源未完全释放
    - 关闭SP后，ISP资源完全释放（timer、ISR回调、controller等）

#### 5.4 重复关闭
- **用例 5.4.1**: 
  ```
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp close mp
  isp close mp
  ```
  - **描述**: 关闭已关闭的通道
  - **预期**: 第二次关闭应该返回成功（已关闭状态）

---

### 六、错误处理测试

#### 6.1 参数错误
- **用例 6.1.1**: `isp open mipi mp 1920 1080 20 2560 1440 frame`
  - **描述**: ISP输出分辨率大于传感器输出分辨率（放大）
  - **预期**: **应该失败**，返回错误（硬件不支持放大）

- **用例 6.1.2**: `isp open mipi mp 0 1080 20 1920 1080 frame`
  - **描述**: 传感器宽度为0
  - **预期**: **应该失败**，参数验证错误

- **用例 6.1.3**: `isp open mipi mp 1920 1080 0 1920 1080 frame`
  - **描述**: 帧率为0
  - **预期**: **应该失败**，参数验证错误

- **用例 6.1.4**: `isp open mipi mp 1920 1080 20 0 1080 frame`
  - **描述**: ISP输出宽度为0
  - **预期**: **应该失败**，参数验证错误

- **用例 6.1.5**: `isp open invalid mp 1920 1080 20 1920 1080 frame`
  - **描述**: 无效的相机类型
  - **预期**: **应该失败**，参数验证错误

- **用例 6.1.6**: `isp open mipi invalid 1920 1080 20 1920 1080 frame`
  - **描述**: 无效的通道类型
  - **预期**: **应该失败**，参数验证错误

- **用例 6.1.7**: `isp open mipi mp 1920 1080 20 1920 1080 invalid`
  - **描述**: 无效的flexa模式
  - **预期**: **应该失败**，参数验证错误

#### 6.2 命令格式错误
- **用例 6.2.1**: `isp open`
  - **描述**: 缺少参数
  - **预期**: **应该失败**，显示Usage信息

- **用例 6.2.2**: `isp close`
  - **描述**: 缺少参数
  - **预期**: **应该失败**，显示Usage信息

- **用例 6.2.3**: `isp open mipi mp 1920 1080 20 1920 1080 frame`
  - **描述**: 缺少ISP输出高度参数
  - **预期**: **应该失败**，显示Usage信息

---

### 七、不同输出格式测试

#### 7.1 不同格式值
- **用例 7.1.1**: `isp open mipi mp 1920 1080 20 1920 1080 frame 23`
  - **描述**: 输出格式23 (NV12)
  - **预期**: 成功打开

- **用例 7.1.2**: `isp open mipi mp 1920 1080 20 1920 1080 frame 24`
  - **描述**: 输出格式24（如果支持）
  - **预期**: 根据实际支持的格式决定

---

### 八、不同帧率测试

#### 8.1 不同帧率
- **用例 8.1.1**: `isp open mipi mp 1920 1080 15 1920 1080 frame`
  - **描述**: 传感器15fps，ISP输出@15fps
  - **预期**: 成功打开，ISR统计显示约15fps

- **用例 8.1.2**: `isp open mipi mp 1920 1080 30 1920 1080 frame`
  - **描述**: 传感器30fps，ISP输出@30fps
  - **预期**: 成功打开，ISR统计显示约30fps

---

### 九、不同分辨率测试

#### 9.1 MIPI 传感器不同分辨率
- **用例 9.1.1**: `isp open mipi mp 1280 720 20 1280 720 frame`
  - **描述**: MIPI传感器使用1280x720@20fps（如果支持），ISP输出1280x720@20fps
  - **预期**: 根据传感器实际支持的分辨率决定

- **用例 9.1.2**: `isp open mipi mp 640 480 20 640 480 frame`
  - **描述**: MIPI传感器使用640x480@20fps（如果支持），ISP输出640x480@20fps
  - **预期**: 根据传感器实际支持的分辨率决定

#### 9.2 ISP 输出不同分辨率（缩小）
- **用例 9.2.1**: `isp open mipi mp 1920 1080 20 1600 900 frame`
  - **描述**: ISP输出1600x900@20fps（缩小）
  - **预期**: 成功打开（支持缩小）

- **用例 9.2.2**: `isp open mipi mp 1920 1080 20 800 600 frame`
  - **描述**: ISP输出800x600@20fps（缩小）
  - **预期**: 成功打开（支持缩小）

- **用例 9.2.3**: `isp open mipi mp 1920 1080 20 320 240 frame`
  - **描述**: ISP输出320x240@20fps（缩小）
  - **预期**: 成功打开（支持缩小）

---

### 十、压力测试

#### 10.1 多次打开/关闭
- **用例 10.1.1**: 
  ```
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp close mp
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp close mp
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp close mp
  ```
  - **描述**: 多次打开/关闭MP通道
  - **预期**: 每次都能成功打开和关闭，无内存泄漏

#### 10.2 快速切换
- **用例 10.2.1**: 
  ```
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp close mp
  isp open mipi sp 1920 1080 20 640 480 frame
  isp close sp
  ```
  - **描述**: 快速在MP和SP之间切换
  - **预期**: 每次切换都能成功

#### 10.3 MP和SP同时工作长时间运行
- **用例 10.3.1**: 
  ```
  isp open mipi mp 1920 1080 20 1920 1080 frame
  isp open mipi sp 1920 1080 20 640 480 frame
  # 等待一段时间（例如30秒），观察ISR统计
  isp close mp
  isp close sp
  ```
  - **描述**: MP和SP同时工作，长时间运行
  - **预期**: 
    - 两个通道都能稳定工作
    - ISR统计正常
    - 无内存泄漏

---

### 十一、边界测试

#### 11.1 最大分辨率
- **用例 11.1.1**: `isp open mipi mp 1920 1080 20 1920 1080 frame`
  - **描述**: 使用最大支持分辨率（根据硬件规格）
  - **预期**: 成功打开

#### 11.2 最小分辨率
- **用例 11.2.1**: `isp open mipi mp 1920 1080 20 160 120 frame`
  - **描述**: ISP输出最小分辨率@20fps
  - **预期**: 根据硬件支持决定

#### 11.3 最大帧率
- **用例 11.3.1**: `isp open mipi mp 1920 1080 30 1920 1080 frame`
  - **描述**: 使用传感器支持的最大帧率30fps，ISP输出@30fps
  - **预期**: 成功打开，ISR统计显示正确的帧率

---

## 测试检查点

### 每个测试用例应检查：
1. **命令执行结果**: 返回 `CMDRSP:OK` 或 `CMDRSP:ERROR`
2. **ISR统计**: 如果通道打开成功，应该看到定时打印的ISR统计信息
   - 格式: `MP:fps[frame_cnt, line_cnt, expected_line] | SP:fps[frame_cnt, line_cnt, expected_line]`
3. **内存泄漏**: 打开/关闭后检查heap内存，确认无内存泄漏
4. **错误处理**: 错误情况下应该返回明确的错误信息

---

## 测试优先级

### 高优先级（必须测试）:
1. 基本功能测试（用例 1.1.1, 2.1.1, 2.1.3, 2.1.4）
2. MP和SP同时工作（用例 4.1.1, 4.2.1）
3. 关闭通道（用例 5.3.1）
4. 错误处理 - 放大检测（用例 6.1.1）

### 中优先级（建议测试）:
1. 不同模式测试（用例 2.2.1, 2.4.1）
2. 不同输出格式（用例 7.1.1）
3. 多次打开/关闭（用例 10.1.1）

### 低优先级（可选测试）:
1. 不同帧率（用例 8.1.1, 8.1.2）
2. 边界测试（用例 11.1.1, 11.2.1, 11.3.1）

