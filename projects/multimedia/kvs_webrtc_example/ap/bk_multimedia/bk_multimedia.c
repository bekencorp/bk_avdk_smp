#include <string.h>

#include <os/os.h>
#include <common/bk_err.h>

#include "bk_multimedia.h"
#include "doorbell_comm.h"
#include "doorbell_devices.h"
#if CONFIG_VOICE_SERVICE
#include "doorbell_audio_device.h"
#endif

#define TAG "kvs-media"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static volatile int s_peer_media_refcnt;

static void start_camera_and_transfer(void)
{
	camera_parameters_t param;
	int ret;

	memset(&param, 0, sizeof(param));
	param.protocol = 0;
	param.rotate = 0;

#if CONFIG_USB_CAMERA
	param.id = UVC_DEVICE_ID;
	param.width = 1280;
	param.height = 720;
	param.format = 1; /* H.264 over transfer; UVC MJPEG decode + encode in doorbell_devices */
#else
	param.id = 0;
	param.width = 1920;
	param.height = 1080;
	param.format = 0;
#endif

	ret = doorbell_camera_turn_on(&param);
	if (ret != BK_OK) {
		LOGE("doorbell_camera_turn_on failed, ret=%d\n", ret);
		return;
	}

	ret = doorbell_video_transfer_turn_on();
	if (ret != BK_OK) {
		LOGE("doorbell_video_transfer_turn_on failed, ret=%d\n", ret);
	}

#if CONFIG_VOICE_SERVICE
	{
		audio_parameters_t ap;

		memset(&ap, 0, sizeof(ap));
		ap.aec = 1;
		ap.uac = 0;
		ap.rmt_recorder_sample_rate = DB_SAMPLE_RARE_16K;
		ap.rmt_player_sample_rate = DB_SAMPLE_RARE_16K;
		ap.rmt_recorder_fmt = CODEC_FORMAT_G711A;
		ap.rmt_player_fmt = CODEC_FORMAT_G711A;
		ret = doorbell_audio_turn_on(&ap);
		if (ret != BK_OK) {
			LOGE("doorbell_audio_turn_on failed, ret=%d\n", ret);
		}
	}
#endif

	LOGI("KVS peer connected: camera + transfer (+audio) started\n");
}

static void stop_camera_and_transfer(void)
{
	(void)doorbell_video_transfer_turn_off();
#if CONFIG_VOICE_SERVICE
	(void)doorbell_audio_turn_off();
#endif
	(void)doorbell_camera_turn_off();
	LOGI("KVS peer disconnected: camera + transfer stopped\n");
}

void mm_on_peer_connected(void)
{
	int prev = __sync_fetch_and_add(&s_peer_media_refcnt, 1);
	if (prev > 0) {
		return;
	}
	start_camera_and_transfer();
}

void mm_on_peer_disconnected(void)
{
	int prev = __sync_fetch_and_sub(&s_peer_media_refcnt, 1);
	if (prev > 1) {
		return;
	}
	if (prev == 1) {
		stop_camera_and_transfer();
		return;
	}
	/* prev <= 0: spurious disconnect without matching connect */
	(void)__sync_fetch_and_add(&s_peer_media_refcnt, 1);
}
