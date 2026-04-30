#include <common/bk_include.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/os.h>

#include "video_fps.h"

#define TAG "video-fps"

#define NTWK_VIDEO_FPS_DUMP_INTERVAL_MS 1000

#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static ntwk_video_fps_stats_t s_video_fps_stats = {0};
static ntwk_video_fps_stats_t s_video_fps_cached = {0};
static bool s_video_fps_initialized = false;
static uint32_t s_video_fps_begin_time = 0;
static beken_timer_t s_video_fps_timer = {0};

static void ntwk_video_fps_dump(void *param)
{
	(void)param;

	uint32_t fps_wifi = s_video_fps_stats.frame_count - s_video_fps_cached.frame_count;
	uint32_t wifi_kbps = (s_video_fps_stats.bytes - s_video_fps_cached.bytes) * 8 / 1024;
	uint32_t mean_time = s_video_fps_stats.total_time_ms - s_video_fps_cached.total_time_ms;

	if (fps_wifi == 0 && wifi_kbps == 0 && !s_video_fps_stats.begin_trs)
	{
		return;
	}

	if (fps_wifi != 0)
	{
		mean_time /= fps_wifi;
	}
	else
	{
		mean_time = s_video_fps_stats.begin_trs ? NTWK_VIDEO_FPS_DUMP_INTERVAL_MS : 0;
	}

	s_video_fps_cached = s_video_fps_stats;

	LOGD("wifi:%d[%d, %dkbps, %dms, %d-%d]\n",
		 fps_wifi, s_video_fps_stats.frame_count, wifi_kbps, mean_time,
		 s_video_fps_stats.begin_trs, s_video_fps_stats.end_trs);
}

bk_err_t ntwk_video_fps_init(void)
{
	bk_err_t ret = BK_OK;

	if (s_video_fps_initialized)
	{
		return BK_OK;
	}

	os_memset(&s_video_fps_stats, 0, sizeof(s_video_fps_stats));
	os_memset(&s_video_fps_cached, 0, sizeof(s_video_fps_cached));

	ret = rtos_init_timer(&s_video_fps_timer,
						  NTWK_VIDEO_FPS_DUMP_INTERVAL_MS,
						  ntwk_video_fps_dump,
						  NULL);
	if (ret != BK_OK)
	{
		LOGE("%s, init timer failed\n", __func__);
		return ret;
	}

	ret = rtos_start_timer(&s_video_fps_timer);
	if (ret != BK_OK)
	{
		LOGE("%s, start timer failed\n", __func__);
		rtos_deinit_timer(&s_video_fps_timer);
		os_memset(&s_video_fps_timer, 0, sizeof(s_video_fps_timer));
		return ret;
	}

	s_video_fps_initialized = true;

	return BK_OK;
}

bk_err_t ntwk_video_fps_deinit(void)
{
	if (!s_video_fps_initialized)
	{
		return BK_OK;
	}

	if (s_video_fps_timer.handle != NULL)
	{
		if (rtos_is_timer_running(&s_video_fps_timer))
		{
			rtos_stop_timer(&s_video_fps_timer);
		}
		rtos_deinit_timer(&s_video_fps_timer);
		os_memset(&s_video_fps_timer, 0, sizeof(s_video_fps_timer));
	}

	os_memset(&s_video_fps_stats, 0, sizeof(s_video_fps_stats));
	os_memset(&s_video_fps_cached, 0, sizeof(s_video_fps_cached));
	s_video_fps_begin_time = 0;
	s_video_fps_initialized = false;

	return BK_OK;
}

void ntwk_video_fps_reset(void)
{
	if (!s_video_fps_initialized)
	{
		return;
	}

	os_memset(&s_video_fps_stats, 0, sizeof(s_video_fps_stats));
	os_memset(&s_video_fps_cached, 0, sizeof(s_video_fps_cached));
	s_video_fps_begin_time = 0;
}

void ntwk_video_fps_frame_begin(void)
{
	if (!s_video_fps_initialized)
	{
		return;
	}

	s_video_fps_begin_time = rtos_get_time();
	s_video_fps_stats.begin_trs = true;
	s_video_fps_stats.end_trs = false;
}

void ntwk_video_fps_frame_end(uint32_t frame_len, int send_ret)
{
	if (!s_video_fps_initialized)
	{
		return;
	}

	uint32_t end_time = rtos_get_time();

	s_video_fps_stats.begin_trs = false;
	s_video_fps_stats.end_trs = true;

	if (send_ret < 0)
	{
		return;
	}

	s_video_fps_stats.frame_count++;
	s_video_fps_stats.bytes += frame_len;
	s_video_fps_stats.total_time_ms += end_time - s_video_fps_begin_time;
}

bk_err_t ntwk_video_fps_get_stats(ntwk_video_fps_stats_t *stats)
{
	if (!s_video_fps_initialized)
	{
		return BK_ERR_NOT_INIT;
	}

	if (stats == NULL)
	{
		LOGE("%s, stats is NULL\n", __func__);
		return BK_ERR_PARAM;
	}

	os_memcpy(stats, &s_video_fps_stats, sizeof(ntwk_video_fps_stats_t));

	return BK_OK;
}
