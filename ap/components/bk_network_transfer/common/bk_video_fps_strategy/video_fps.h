#pragma once

#include <common/bk_err.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint32_t frame_count;
	uint32_t bytes;
	uint32_t total_time_ms;
	bool begin_trs;
	bool end_trs;
} ntwk_video_fps_stats_t;

#if CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE

bk_err_t ntwk_video_fps_init(void);
bk_err_t ntwk_video_fps_deinit(void);
void ntwk_video_fps_reset(void);
void ntwk_video_fps_frame_begin(void);
void ntwk_video_fps_frame_end(uint32_t frame_len, int send_ret);
bk_err_t ntwk_video_fps_get_stats(ntwk_video_fps_stats_t *stats);

#endif

#ifdef __cplusplus
}
#endif
