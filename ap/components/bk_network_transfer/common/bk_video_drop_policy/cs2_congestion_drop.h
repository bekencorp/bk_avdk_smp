// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <components/video_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CS2 多会话抗网络抖动 drop 策略（本文件是 CS2 拥塞/丢帧/强制 IDR 的唯一归属）。
 *
 * 一套策略、两个作用阶段，统一由 NTWK_CS2_CONGESTION_DROP 控制：
 *  1) 全局预投递丢帧（编码输出口）：
 *     只有当所有订阅会话都拥塞时，整帧不入 ready 队列（回收到 free 队列），
 *     省编码/排队开销。见 ntwk_cs2_congestion_drop_check()。
 *  2) 每会话准入（fanout 发送口）：
 *     对每个来的帧，逐会话看该会话的 CS2 TX buffer 是否吃得下：
 *       - 吃得下 -> 标记该会话发送；
 *       - 吃不下 -> 该会话跳过整帧并置 waiting_key，之后只在 buffer 排空
 *         且遇到关键帧时才干净恢复（避免花屏），并请求一次强制 IDR。
 *     见 ntwk_cs2_cong_video_admit() 系列。
 *
 * 强制 IDR 仲裁器（限频 gap / streak / healthy）为编码器级单一资源，两阶段共用。
 *
 * 该模块刻意不 include ntwk_cs2_service.h（lwip/PPCS），只按 session index 维护
 * 每会话策略状态；连接/收发由 ntwk_cs2_service.c 负责。
 */

/* 从 CS2 视频包解析出的每帧元信息，供准入策略使用。 */
typedef struct
{
    bool valid;
    bool first_frag;
    bool last_frag;
    bool is_key;
    uint8_t frame_id;
    uint8_t cnt;
} ntwk_cs2_video_pkt_meta_t;

/* ---------------- 全局预投递丢帧（编码输出口） ---------------- */

/** 初始化全局丢帧策略状态（从 Kconfig 读取水位/限频参数）。 */
void ntwk_cs2_congestion_drop_init(void);

/** 复位全局丢帧运行态。 */
void ntwk_cs2_congestion_drop_reset(void);

/**
 * @brief 下发前判断当前编码帧是否应被（全局）丢弃。
 * @param frame frame_buffer_t*，仅处理 PIXEL_FMT_H264。
 * @return true -> 丢弃（回收到 free 队列）；false -> 放行入 ready 队列。
 */
bool ntwk_cs2_congestion_drop_check(const void *frame);

/**
 * @brief 编码回调消费一次性强制 IDR 请求（全局丢帧 + 每会话准入两路合并）。
 * @return true 表示下一帧强制输出 IDR。
 */
bool ntwk_cs2_congestion_consume_force_idr(void);

/* ---------------- 强制 IDR 仲裁器（编码器级） ---------------- */

/** 请求一次强制 IDR（内部按 gap/streak 限频）。 */
void ntwk_cs2_request_force_idr(void);

/** 消费强制 IDR 请求（仅 CS2 每会话准入这一路，编码回调使用）。 */
bool ntwk_cs2_consume_force_idr(void);

/* ---------------- 每会话准入（fanout 发送口） ---------------- */

/** 复位全部策略状态：强制 IDR 仲裁器 + 所有会话准入状态。用于服务 init/deinit。 */
void ntwk_cs2_cong_reset_all(void);

/** 复位单个会话槽的准入状态。会话分配/释放时调用（index 为 session 数组下标）。 */
void ntwk_cs2_cong_reset_session(int index);

/** 订阅/退订时（重新）武装该会话槽的 waiting_key。 */
void ntwk_cs2_cong_on_subscribe(int index, bool enable);

/** 当前是否启用 CS2 拥塞 drop 策略。 */
bool ntwk_cs2_cong_is_enabled(void);

/** 解析一个 CS2 视频包为 meta（first/last 分片、是否关键帧）。 */
void ntwk_cs2_cong_parse_video_packet(uint8_t *data, uint32_t length,
                                      image_format_t video_type,
                                      ntwk_cs2_video_pkt_meta_t *meta);

/**
 * @brief 每会话准入决策：当前（分片）帧要不要发给该会话。
 * @param index   session 数组下标。
 * @param backlog 该会话 CS2 视频通道当前 TX 积压字节数。
 * @param meta    当前包的元信息。
 * @return true -> 发送；false -> 跳过。
 */
bool ntwk_cs2_cong_video_admit(int index, uint32_t backlog,
                               const ntwk_cs2_video_pkt_meta_t *meta);

/**
 * @brief 每会话 audio 准入决策。
 * @param index   session 数组下标。
 * @param backlog 该会话 CS2 audio 通道当前 TX 积压字节数。
 * @return true -> 发送；false -> 跳过。
 */
bool ntwk_cs2_cong_audio_admit(int index, uint32_t backlog);

/** 标记该会话本帧发送被截断（失败/部分）：丢掉剩余、置 waiting_key、请求强制 IDR。 */
void ntwk_cs2_cong_mark_truncated(int index);

/** 一帧发送结束（最后一片）后调用，更新 healthy/streak。 */
void ntwk_cs2_cong_note_frame_end(void);

#ifdef __cplusplus
}
#endif
