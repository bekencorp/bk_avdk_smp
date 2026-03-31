#ifndef __NETWORK_TRANSFER_H__
#define __NETWORK_TRANSFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <components/media_types.h>
#include <components/bk_voice_service_types.h>

/**
 * @brief network type
 */
typedef enum {
    NETWORK_TYPE_VOLC_RTC = 0,    /**< 火山引擎RTC */
    NETWORK_TYPE_AGORA_RTC,       /**< 声网RTC */
    NETWORK_TYPE_MAX
} network_type_t;

/**
 * @brief 音频发送回调函数类型
 * @param data_ptr 音频数据指针
 * @param data_len 数据长度
 * @param audio_type 音频编码类型
 * @return int 发送结果
 */
typedef int (*audio_tx_callback_t)(uint8_t *data_ptr, size_t data_len, audio_enc_type_t audio_type);

/**
 * @brief 视频发送回调函数类型
 * 
 * 该回调函数用于发送视频帧数据到网络。frame_buffer_t 结构包含了完整的帧信息，
 * 包括数据指针、数据长度、格式、时间戳等。
 * 
 * @param frame 视频帧缓冲区指针，包含以下信息：
 *              - frame->frame: 视频数据指针
 *              - frame->length: 数据长度
 *              - frame->fmt: 像素格式 (PIXEL_FMT_JPEG/PIXEL_FMT_H264/等)
 *              - frame->width: 帧宽度
 *              - frame->height: 帧高度
 *              - frame->sequence: 帧序号
 *              - frame->timestamp: 时间戳
 *              - frame->h264_type: H264帧类型（仅H264格式有效）
 * 
 * @return int 发送结果
 *         - BK_OK (0): 成功
 *         - BK_FAIL (-1): 失败
 */
typedef int (*video_tx_callback_t)(frame_buffer_t *frame);
/**
 * @brief 网络传输启动回调函数类型
 * @param user_data 用户数据
 * @return bk_err_t 启动结果
 */
typedef bk_err_t (*ntwk_trans_start_callback_t)(void *user_data);

/**
 * @brief 网络传输停止回调函数类型
 * @param user_data 用户数据
 * @return bk_err_t 停止结果
 */
typedef bk_err_t (*ntwk_trans_stop_callback_t)(void *user_data);

/**
 * @brief 网络传输预配置回调函数类型
 * @param user_data 用户数据
 * @return bk_err_t 预配置结果
 */
typedef bk_err_t (*ntwk_trans_pre_config_callback_t)(void *user_data);
/**
 * @brief 网络传输更新回调函数类型
 * @param user_data 用户数据
 * @return bk_err_t 更新结果
 */
typedef bk_err_t (*ntwk_trans_update_callback_t)(void *user_data, void *update_info);
/**
 * @brief 网络传输模块上下文结构体
 */
typedef struct {
    network_type_t network_type;           /**< network type */
    void *network_config;              /**< network特定配置 */
    audio_tx_callback_t audio_tx_cb; /**< 音频接收回调 */
    video_tx_callback_t video_tx_cb; /**< 视频接收回调 */
    ntwk_trans_start_callback_t start_cb; /**< 网络传输开始回调 */
    ntwk_trans_stop_callback_t stop_cb; /**< 网络传输停止回调 */
    ntwk_trans_pre_config_callback_t pre_config_cb; /**< 网络传输预配置回调 */
    ntwk_trans_update_callback_t update_cb; /**< 网络传输更新回调 */
    void *user_data;               /**< 用户数据 */
    bool is_started;               /**< 是否已启动 */
    bool initialized;                      /**< 初始化标志 */
} ntwk_trans_ctx_t;
/**
 * @brief 初始化网络传输模块
 * @return int 初始化结果
 */
int ntwk_trans_init(void);

/**
 * @brief 反初始化网络传输模块
 * @return int 反初始化结果
 */
int ntwk_trans_deinit(void);

/**
 * @brief 更新网络传输
 * @param user_data 用户数据
 * @param update_info 更新信息
 * @return int 更新结果
 */
int ntwk_trans_update(void *user_data, void *update_info);
/**
 * @brief 启动网络传输
 * @param user_data 用户数据指针，传递给启动回调函数
 * @return int 0表示成功，负数表示失败
 */
int ntwk_trans_start(void *user_data);
/**
 * @brief 停止网络传输
 * @param user_data 用户数据指针，传递给停止回调函数
 * @return int 0表示成功，负数表示失败
 */
int ntwk_trans_stop(void *user_data);
/**
 * @brief 发送音频数据
 * @param data 音频数据指针
 * @param size 数据大小
 * @param audio_type 音频编码类型
 * @return int 发送结果
 */
int ntwk_trans_send_audio(const uint8_t *data, size_t size, audio_enc_type_t audio_type);


/**
 * @brief 处理并发送视频帧
 * @param frame 视频帧缓冲区指针
 * @return int 0表示成功，负数表示失败
 */
int ntwk_trans_send_video(frame_buffer_t *frame); 
/**
 * @brief 获取当前network类型
 * @return network_type_t network类型
 */
network_type_t ntwk_trans_get_network_type(void);
/**
 * @brief 获取音频编码器类型
 * @return audio_enc_type_t 音频编码器类型
 */
audio_enc_type_t ntwk_trans_get_audio_encoder_type(void);

/**
 * @brief 接收音频数据
 * @param data 音频数据指针
 * @param size 数据大小
 * @return int 接收结果
 */
int ntwk_trans_recv_audio(const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif
#endif /* __NETWORK_TRANSFER_H__ */
