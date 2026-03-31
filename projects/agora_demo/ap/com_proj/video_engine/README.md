# Video Engine 模块

## 📋 概述

Video Engine 提供了简洁的视频引擎管理接口，支持 DVP 摄像头的初始化、启动、停止和资源清理。所有配置基于 CONFIG 宏自动设置，使用简单方便。

## 🎯 核心功能

- ✅ 自动配置（基于 CONFIG 宏）
- ✅ 一键初始化（video_engine_init）
- ✅ 状态管理（is_started 标志）
- ✅ 完整的生命周期管理（init/start/stop/deinit）
- ✅ 状态查询（video_engine_is_running）
- ✅ 完善的错误处理和资源清理

## 🏗️ 架构设计

### 1. 核心组件

```
video_engine_ctx_t (上下文结构)
  ├─ camera_handle         摄像头句柄
  ├─ transfer_format       传输格式 (IMAGE_MJPEG/IMAGE_H264)
  ├─ transfer_task_handle  传输任务句柄
  ├─ transfer_task_running 传输任务运行标志
  └─ is_started           引擎启动状态标志
```

### 2. 静态配置结构

在 `video_engine.c` 中定义了静态配置 `camera_parameters`，基于编译时的 CONFIG 宏自动配置：

```c
static camera_parameters_t camera_parameters = {
    // Camera ID: 0 = DVP, 1 = UVC
    .id = 0,  // 基于 CONFIG_VIDEO_ENGINE_USE_DVP_CAMERA
    
    // Resolution
    .width = CONFIG_VIDEO_ENGINE_RESOLUTION_WIDTH,
    .height = CONFIG_VIDEO_ENGINE_RESOLUTION_HEIGHT,
    
    // Format: 0 = MJPEG, 1 = H264, 2 = H265
    .format = 0/1,  // 基于 CONFIG_VIDEO_ENGINE_JPEG_FORMAT 或 CONFIG_VIDEO_ENGINE_H264_FORMAT
    
    .protocol = 0,  // 保留字段
    .rotate = 0,    // 旋转角度
};
```

### 3. 核心函数

#### video_engine_init()

初始化并启动视频引擎（一步完成所有工作）：

```c
int video_engine_init(void)
{
    // 1. 检查是否已初始化
    // 2. 分配上下文内存
    // 3. 初始化帧队列
    // 4. 自动调用 video_engine_start()
    // 5. 返回结果
}
```

#### video_engine_start()

启动视频引擎（打开摄像头和启动传输任务）：

```c
int video_engine_start(void)
{
    // 1. 检查上下文有效性
    // 2. 检查是否已启动
    // 3. 使用 camera_parameters 打开摄像头
    // 4. 启动视频传输任务
    // 5. 设置 is_started = true
    // 6. 返回结果
}
```

#### video_engine_stop()

停止视频引擎（关闭摄像头和传输任务）：

```c
int video_engine_stop(void)
{
    // 1. 检查是否已启动
    // 2. 停止视频传输任务
    // 3. 关闭摄像头
    // 4. 设置 is_started = false
    // 5. 返回结果
}
```

#### video_engine_deinit()

完全清理视频引擎资源：

```c
int video_engine_deinit(void)
{
    // 1. 调用 video_engine_stop()
    // 2. 清理帧队列
    // 3. 释放上下文内存
    // 4. 返回结果
}
```

#### video_engine_is_running()

检查视频引擎运行状态：

```c
bool video_engine_is_running(void)
{
    if (g_video_engine_ctx == NULL) {
        return false;
    }
    return g_video_engine_ctx->is_started;
}
```

## 📊 完整调用流程

```
应用程序调用 video_engine_init()
    ↓
检查是否已初始化
    ↓
分配 g_video_engine_ctx 内存
    ↓
初始化 frame_queue (所有格式：MJPEG, H264, YUV)
    ↓
调用 video_engine_start()
    ↓
    ├─ 检查是否已启动（防止重复）
    ├─ 打开摄像头 (video_engine_camera_turn_on)
    │    ├─ 参数验证
    │    ├─ 配置 DVP (bk_dvp_config_t)
    │    ├─ 创建摄像头控制器
    │    └─ 打开摄像头
    ├─ 启动传输任务 (video_engine_transfer_start)
    │    ├─ 创建传输任务线程
    │    └─ 设置 transfer_task_running = true
    └─ 设置 is_started = true
    ↓
初始化完成，返回 BK_OK
```

## ⚙️ 配置宏

在项目配置文件 `Kconfig` 或 `sdkconfig.h` 中定义：

```c
// 摄像头类型（二选一）
#define CONFIG_VIDEO_ENGINE_USE_DVP_CAMERA 1
// #define CONFIG_VIDEO_ENGINE_USE_UVC_CAMERA 1

// 分辨率
#define CONFIG_VIDEO_ENGINE_RESOLUTION_WIDTH 640
#define CONFIG_VIDEO_ENGINE_RESOLUTION_HEIGHT 480

// 编码格式（二选一）
#define CONFIG_VIDEO_ENGINE_JPEG_FORMAT 1
// #define CONFIG_VIDEO_ENGINE_H264_FORMAT 1
```

## 📖 使用示例

### 场景 1: 标准使用流程（推荐）

这是最简单、最推荐的使用方式：

```c
#include "video_engine.h"

int main(void)
{
    // 1. 初始化视频引擎（自动配置并启动）
    int ret = video_engine_init();
    if (ret != BK_OK) {
        printf("Video engine init failed: %d\n", ret);
        return -1;
    }
    
    printf("Video engine started successfully\n");
    
    // 2. 检查运行状态
    if (video_engine_is_running()) {
        printf("Video engine is running\n");
    }
    
    // ... 应用程序运行 ...
    
    // 3. 清理资源
    video_engine_deinit();
    
    return 0;
}
```

### 场景 2: 完整生命周期管理

如果需要更精细的控制：

```c
#include "video_engine.h"

int main(void)
{
    int ret;
    
    // 1. 初始化（包括自动启动）
    ret = video_engine_init();
    if (ret != BK_OK) {
        printf("Init failed\n");
        return -1;
    }
    
    // 2. 应用运行...
    // ...
    
    // 3. 需要暂停时，停止视频引擎
    ret = video_engine_stop();
    if (ret != BK_OK) {
        printf("Stop failed\n");
    }
    
    // 4. 需要时，重新启动
    ret = video_engine_start();
    if (ret != BK_OK) {
        printf("Restart failed\n");
    }
    
    // 5. 程序退出前，完全清理
    video_engine_deinit();
    
    return 0;
}
```

### 场景 3: 状态检查

在运行过程中检查视频引擎状态：

```c
#include "video_engine.h"

void check_video_status(void)
{
    if (video_engine_is_running()) {
        printf("Video engine is running\n");
        // 执行需要视频的操作
    } else {
        printf("Video engine is not running\n");
        // 可以选择启动它
        int ret = video_engine_start();
        if (ret != BK_OK) {
            printf("Failed to start video engine\n");
        }
    }
}
```

### 场景 4: 错误处理示例

完整的错误处理流程：

```c
#include "video_engine.h"

int initialize_video_system(void)
{
    int ret;
    
    // 初始化视频引擎
    ret = video_engine_init();
    if (ret != BK_OK) {
        printf("ERROR: Failed to initialize video engine, ret=%d\n", ret);
        
        // 尝试清理可能的残留资源
        video_engine_deinit();
        return -1;
    }
    
    // 验证初始化成功
    if (!video_engine_is_running()) {
        printf("ERROR: Video engine initialized but not running\n");
        video_engine_deinit();
        return -1;
    }
    
    printf("Video system initialized successfully\n");
    return 0;
}
```

## 💾 内存占用分析

| 组件 | 内存占用 | 位置 | 说明 |
|------|---------|------|------|
| `video_engine_ctx_t` | ~24 B | Heap | 引擎上下文结构 |
| `camera_parameters` | ~12 B | .bss | 静态摄像头配置 |
| 传输任务栈 | 4 KB | Heap | 视频传输任务栈 |
| 帧缓冲队列 | 动态 | Heap | 根据格式分配 |
| - MJPEG 帧 | ~60-200 KB | Heap | 4个帧缓冲 |
| - H264 帧 | ~50-150 KB | Heap | 6个帧缓冲 |
| - YUV 帧 | ~300 KB-1.8 MB | Heap | 3个帧缓冲 |

**堆内存总计：** ~4-6 KB（不含帧缓冲） + 帧缓冲大小

## ✨ 设计优势

1. **简洁易用** - 一个函数完成初始化和启动
2. **自动配置** - 基于 CONFIG 宏，编译时确定
3. **状态管理** - `is_started` 标志防止重复操作
4. **完整生命周期** - init → start → stop → deinit
5. **错误处理** - 每步都有检查和回滚机制
6. **资源安全** - 防止内存泄漏和野指针
7. **并发安全** - 传输任务安全退出机制
8. **API 一致性** - 与 audio_engine 保持一致

## ⚠️ 注意事项

### 1. 初始化要求

- ✅ **必须先调用** `video_engine_init()`，它会自动调用 `video_engine_start()`
- ✅ **不需要手动调用** `video_engine_start()`（除非停止后需要重启）
- ❌ **避免重复初始化**，`video_engine_init()` 会检查并返回已初始化状态

### 2. 配置宏要求

确保在 `Kconfig` 或 `sdkconfig.h` 中正确定义：

```c
CONFIG_VIDEO_ENGINE_USE_DVP_CAMERA      // 摄像头类型
CONFIG_VIDEO_ENGINE_RESOLUTION_WIDTH    // 分辨率宽度
CONFIG_VIDEO_ENGINE_RESOLUTION_HEIGHT   // 分辨率高度
CONFIG_VIDEO_ENGINE_JPEG_FORMAT         // JPEG 格式
// 或
CONFIG_VIDEO_ENGINE_H264_FORMAT         // H264 格式
```

### 3. 调用顺序

```c
// ✅ 正确顺序
video_engine_init();           // 初始化（包含启动）
video_engine_is_running();     // 检查状态
// ... 使用 ...
video_engine_deinit();         // 清理

// ❌ 错误示例 1：重复初始化
video_engine_init();
video_engine_init();           // 错误：已初始化

// ❌ 错误示例 2：未初始化就启动
video_engine_start();          // 错误：上下文为 NULL

// ❌ 错误示例 3：未清理就退出
video_engine_init();
// ... 使用 ...
exit(0);                       // 错误：资源泄漏
```

### 4. 线程安全

- ⚠️ API 函数**不是线程安全**的，多线程调用需要加锁
- ✅ 传输任务内部使用队列，任务间通信是安全的

### 5. 错误处理

```c
int ret = video_engine_init();
if (ret != BK_OK) {
    // 初始化失败，内部已经清理资源
    // 可以安全地再次尝试初始化
    LOGE("Init failed, ret=%d\n", ret);
}

// stop 和 deinit 会尽力清理，即使部分失败
video_engine_deinit();  // 总是调用以防万一
```

## 🔍 调试支持

### 日志输出

模块使用分级日志，可通过 TAG 过滤：

```bash
# 过滤视频引擎日志
TAG=video_engine

# 关键日志示例
LOGI: video_engine_init: Video engine initialized successfully
LOGI: video_engine_start: Starting video engine (id:0, 640x480, format:1)
LOGI: video_engine_stop: Video engine stopped successfully
LOGE: video_engine_start: video_engine_camera_turn_on failed, ret=-1
```

### 状态检查

```c
// 运行时检查引擎状态
if (video_engine_is_running()) {
    printf("Video engine is active\n");
} else {
    printf("Video engine is inactive\n");
}
```

### 常见问题排查

| 问题 | 可能原因 | 解决方法 |
|-----|---------|---------|
| `init` 失败 | 内存不足 | 检查堆内存，减少帧缓冲数量 |
| `start` 失败 | 摄像头打开失败 | 检查硬件连接和 GPIO 配置 |
| `stop` 部分失败 | 任务未正常退出 | 检查传输任务是否卡住 |
| 帧队列满 | 处理速度慢 | 优化网络发送或增加队列 |

## 📁 相关文件

### 核心文件

- **`video_engine.h`** (190行) - 公开 API 声明
- **`video_engine.c`** (670行) - 核心实现
  - Lines 64-76: `camera_parameters` 静态配置
  - Lines 186-236: `video_engine_init()` 初始化
  - Lines 238-268: `video_engine_deinit()` 清理
  - Lines 556-596: `video_engine_start()` 启动
  - Lines 608-655: `video_engine_stop()` 停止
  - Lines 664-670: `video_engine_is_running()` 状态查询
  - Lines 123-181: `video_engine_transfer_task()` 传输任务

### 依赖文件

- **`video_frame_que.h`** (61行) - 帧队列接口
- **`video_frame_que.c`** (532行) - 帧队列实现
- **`network_transfer.c`** - 网络传输接口

### 配置文件

- **`Kconfig`** (48行) - 编译配置选项
- **`CMakeLists.txt`** (16行) - 构建配置

## 🔗 参考资料

- **参考实现**: `audio_engine.c` / `audio_engine.h` (API 设计参考)
- **类型定义**: `media_types.h` (图像格式、帧结构)
- **摄像头控制**: `bk_camera_ctlr.h` (摄像头 API)
- **DVP 驱动**: `dvp_camera_types.h` (DVP 摄像头类型)

## 📝 版本历史

- **v2.0** - 重构为 init/start/stop/deinit 模式，增加状态管理
- **v1.x** - 初始版本，基本摄像头和传输功能
