#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <avdk_utils.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_frame_buffer.h>
#include <stdint.h>
#include <os/str.h>
#include <os/os.h>
#include <os/mem.h>
#include "cli.h"
#include <bk_private/bk_cli.h>
#include "video_recorder_cli.h"
#include "video_player_common.h"
#include "audio_recorder_device.h"
#include "bk_video_recorder.h"
#include "modules/vcenc/vcenc_types.h"
#include "app_camera.h"
#include "app_codec.h"
#include "doorbell_img_manager.h"
#include <components/bk_encode/bk_h264_encode_ctlr.h>
#include <components/bk_flexa_bond.h>
#include "cache.h"

#define TAG "video_recorder_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

static const struct cli_command s_video_recorder_commands[] =
{
    {"video_record", "video_record start|stop [path] [w] [h] [mjpeg|h264] [mp4] [aac|pcm|g711a|g711u|g722]", cli_video_record_cmd},
};

static bk_video_recorder_handle_t video_recorder_handle = NULL;
static audio_recorder_device_handle_t video_recorder_audio_recorder_handle = NULL;
static void *video_recorder_isp_encode_bond = NULL;
static bool video_recorder_camera_started = false;
static bool video_recorder_h264_mode = false;
static uint32_t video_recorder_frame_total_count = 0;
static uint32_t video_recorder_frame_miss_count = 0;
static uint8_t video_recorder_audio_temp_buffer[4096];
static char video_recorder_output_path[256] = {0};

#define VIDEO_RECORDER_MAX_DURATION_MS   (5 * MINUTES)

static beken2_timer_t video_recorder_auto_stop_timer = {0};
static uint32_t video_recorder_session_id = 0;
static beken_thread_t video_recorder_auto_stop_thread = NULL;
static bool video_recorder_auto_stop_timer_inited = false;
static bool video_recorder_auto_stop_timer_started = false;

static int video_recorder_get_frame_cb(void *user_data, video_recorder_frame_data_t *frame_data)
{
    (void)user_data;

    if (frame_data == NULL)
    {
        return -1;
    }

    frame_buffer_t *frame = (frame_buffer_t *)bk_encoded_complete_data_request(50);
    if (frame == NULL || frame->frame == NULL || frame->length == 0)
    {
        video_recorder_frame_miss_count++;
        return -1;
    }

    flush_dcache(frame->frame, (long)frame->length);

    video_recorder_frame_total_count++;
    frame_data->data = frame->frame;
    frame_data->length = frame->length;
    frame_data->width = frame->width;
    frame_data->height = frame->height;
    frame_data->frame_buffer = frame;

    if (video_recorder_h264_mode)
    {
        /* h264_type carries VCENC_OUT_IFRAME / VCENC_OUT_PFRAME, not NAL bitmask. */
        frame_data->is_key_frame = (frame->h264_type == (uint32_t)VCENC_OUT_IFRAME);
    }
    else
    {
        frame_data->is_key_frame = 1;
    }

    return 0;
}

static int video_recorder_get_audio_cb(void *user_data, video_recorder_audio_data_t *audio_data)
{
    (void)user_data;

    if (audio_data == NULL || video_recorder_audio_recorder_handle == NULL)
    {
        return -1;
    }

    uint32_t data_len = 0;
    avdk_err_t ret = audio_recorder_device_read(video_recorder_audio_recorder_handle,
                                                video_recorder_audio_temp_buffer,
                                                sizeof(video_recorder_audio_temp_buffer),
                                                &data_len);
    if (ret == AVDK_ERR_OK && data_len > 0)
    {
        uint8_t *audio_buffer = psram_malloc(data_len);
        if (audio_buffer == NULL)
        {
            LOGE("%s: Failed to allocate audio buffer\n", __func__);
            return -1;
        }
        os_memcpy(audio_buffer, video_recorder_audio_temp_buffer, data_len);
        audio_data->data = audio_buffer;
        audio_data->length = data_len;
        return 0;
    }

    return -1;
}

static void video_recorder_release_frame_cb(void *user_data, video_recorder_frame_data_t *frame_data)
{
    (void)user_data;

    if (frame_data == NULL)
    {
        return;
    }

    frame_buffer_t *frame = (frame_buffer_t *)frame_data->frame_buffer;
    if (frame != NULL)
    {
        bk_encoded_complete_data_free_request((uint8_t *)frame);
    }

    frame_data->data = NULL;
    frame_data->length = 0;
    frame_data->frame_buffer = NULL;
}

static void video_recorder_release_audio_cb(void *user_data, video_recorder_audio_data_t *audio_data)
{
    (void)user_data;

    if (audio_data != NULL && audio_data->data != NULL)
    {
        psram_free(audio_data->data);
        audio_data->data = NULL;
        audio_data->length = 0;
    }
}

int cli_video_recorder_init(void)
{
    return cli_register_commands(s_video_recorder_commands,
                                 sizeof(s_video_recorder_commands) / sizeof(struct cli_command));
}

static void video_recorder_cancel_auto_stop_timer(void)
{
    if (video_recorder_auto_stop_timer_started)
    {
        (void)rtos_stop_oneshot_timer(&video_recorder_auto_stop_timer);
        video_recorder_auto_stop_timer_started = false;
    }
    if (video_recorder_auto_stop_timer_inited)
    {
        (void)rtos_deinit_oneshot_timer(&video_recorder_auto_stop_timer);
        video_recorder_auto_stop_timer_inited = false;
    }
}

static void video_recorder_stop_isp_encode_bond(void)
{
    if (video_recorder_isp_encode_bond == NULL)
    {
        return;
    }

    if (video_recorder_h264_mode)
    {
        bk_flexa_isp_h264e_bond_stop(video_recorder_isp_encode_bond);
    }
    else
    {
        bk_flexa_isp_jpege_bond_stop(video_recorder_isp_encode_bond);
    }
    video_recorder_isp_encode_bond = NULL;
}

static avdk_err_t video_recorder_start_isp_encode_bond(void)
{
    void *isp_handle = app_isp_handle_get();
    if (isp_handle == NULL)
    {
        LOGE("%s: app_isp_handle_get failed\n", __func__);
        return AVDK_ERR_INVAL;
    }

    avdk_err_t ret = AVDK_ERR_OK;
    if (video_recorder_h264_mode)
    {
        bk_h264_encode_ctlr_handle_t enc_handle =
            (bk_h264_encode_ctlr_handle_t)app_h264_encode_handle_get();
        if (enc_handle == NULL)
        {
            LOGE("%s: app_h264_encode_handle_get failed\n", __func__);
            return AVDK_ERR_INVAL;
        }
        ret = bk_flexa_isp_h264e_bond_start(&video_recorder_isp_encode_bond, isp_handle, enc_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_flexa_isp_h264e_bond_start failed, ret=%d\n", __func__, ret);
        }
    }
    else
    {
        bk_jpeg_encode_ctlr_handle_t enc_handle =
            (bk_jpeg_encode_ctlr_handle_t)app_jpeg_encode_handle_get();
        if (enc_handle == NULL)
        {
            LOGE("%s: app_jpeg_encode_handle_get failed\n", __func__);
            return AVDK_ERR_INVAL;
        }
        ret = bk_flexa_isp_jpege_bond_start(&video_recorder_isp_encode_bond, isp_handle, enc_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_flexa_isp_jpege_bond_start failed, ret=%d\n", __func__, ret);
        }
    }

    return ret;
}

static void video_recorder_stop_encode_and_camera(void)
{
    video_recorder_stop_isp_encode_bond();

    if (video_recorder_h264_mode)
    {
        (void)app_h264e_turn_off();
    }
    else
    {
        (void)app_jpege_turn_off();
    }

    if (video_recorder_camera_started)
    {
        (void)app_isp_camera_turn_off();
        video_recorder_camera_started = false;
    }
}

static avdk_err_t video_recorder_stop_internal(bool from_timeout)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (video_recorder_handle == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    video_recorder_cancel_auto_stop_timer();

    if (from_timeout)
    {
        LOGW("%s: Auto stopping recording after %u ms\n", __func__, (unsigned)VIDEO_RECORDER_MAX_DURATION_MS);
    }

    ret = bk_video_recorder_stop(video_recorder_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_video_recorder_stop failed, ret=%d\n", __func__, ret);
    }

    if (video_recorder_frame_total_count > 0)
    {
        LOGI("%s: Frame statistics - Total: %u, Missed polls: %u\n",
             __func__, video_recorder_frame_total_count, video_recorder_frame_miss_count);
    }
    else
    {
        LOGW("%s: No encoded video frames captured (check ISP-encoder flexa bond)\n", __func__);
    }
    video_recorder_frame_total_count = 0;
    video_recorder_frame_miss_count = 0;

    avdk_err_t tmp = bk_video_recorder_close(video_recorder_handle);
    if (tmp != AVDK_ERR_OK && ret == AVDK_ERR_OK)
    {
        ret = tmp;
    }

    tmp = bk_video_recorder_delete(video_recorder_handle);
    if (tmp != AVDK_ERR_OK && ret == AVDK_ERR_OK)
    {
        ret = tmp;
    }
    video_recorder_handle = NULL;

    if (video_recorder_audio_recorder_handle != NULL)
    {
        audio_recorder_device_stop(video_recorder_audio_recorder_handle);
        audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
        video_recorder_audio_recorder_handle = NULL;
    }

    video_recorder_stop_encode_and_camera();
    return ret;
}

static void video_recorder_auto_stop_thread_fn(void *arg)
{
    uint32_t sid = (uint32_t)(uintptr_t)arg;

    if (sid != video_recorder_session_id)
    {
        video_recorder_auto_stop_thread = NULL;
        rtos_delete_thread(NULL);
        return;
    }

    (void)video_recorder_stop_internal(true);
    video_recorder_auto_stop_thread = NULL;
    rtos_delete_thread(NULL);
}

static void video_recorder_auto_stop_timeout(void *larg, void *rarg)
{
    (void)rarg;
    uint32_t sid = (uint32_t)(uintptr_t)larg;

    if (sid != video_recorder_session_id)
    {
        return;
    }

    if (video_recorder_auto_stop_thread != NULL)
    {
        return;
    }

    (void)rtos_create_thread(&video_recorder_auto_stop_thread,
                             6,
                             "vr_auto_stop",
                             (beken_thread_function_t)video_recorder_auto_stop_thread_fn,
                             2048,
                             (beken_thread_arg_t)(uintptr_t)sid);
}

static bk_err_t video_recorder_start_auto_stop_timer(uint32_t sid)
{
    video_recorder_cancel_auto_stop_timer();

    bk_err_t ret = rtos_init_oneshot_timer(&video_recorder_auto_stop_timer,
                                          VIDEO_RECORDER_MAX_DURATION_MS,
                                          video_recorder_auto_stop_timeout,
                                          (void *)(uintptr_t)sid,
                                          NULL);
    if (ret != BK_OK)
    {
        return ret;
    }
    video_recorder_auto_stop_timer_inited = true;

    ret = rtos_start_oneshot_timer(&video_recorder_auto_stop_timer);
    if (ret != BK_OK)
    {
        (void)rtos_deinit_oneshot_timer(&video_recorder_auto_stop_timer);
        video_recorder_auto_stop_timer_inited = false;
        video_recorder_auto_stop_timer_started = false;
        return ret;
    }
    video_recorder_auto_stop_timer_started = true;
    return ret;
}

static void video_recorder_update_camera_board_config(uint32_t width, uint32_t height)
{
    camera_board_config_t *board = app_camera_board_config_get();
    if (board == NULL)
    {
        return;
    }

    /* Sensor runs at native resolution; ISP scales to the recording size. */
    if (width > board->mipi.sensor_max_width || width == 0)
    {
        width = board->mipi.sensor_max_width;
    }
    if (height > board->mipi.sensor_max_height || height == 0)
    {
        height = board->mipi.sensor_max_height;
    }

    board->isp.mp_width = (uint16_t)width;
    board->isp.mp_height = (uint16_t)height;
    board->isp.mp_format = BK_PIXEL_FORMAT_NV12;
    board->isp.mp_flexa = true;
    board->isp.mp_enable = true;
    app_camera_board_config_set(board);
}

void cli_video_record_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    avdk_err_t ret = AVDK_ERR_GENERIC;
    bool h264_mode = false;
    uint32_t record_type = VIDEO_RECORDER_TYPE_AVI;

    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        goto exit;
    }

    if (os_strcmp(argv[1], "start") == 0)
    {
        if (video_recorder_handle != NULL)
        {
            LOGE("%s: video recording already started\n", __func__);
            goto exit;
        }

        if (sd_card_mount() != BK_OK)
        {
            LOGE("%s: Failed to mount SD card\n", __func__);
            goto exit;
        }

        char *file_path = "/sd0/record.avi";
        uint32_t width = 480;
        uint32_t height = 320;
        uint32_t audio_format = VIDEO_RECORDER_AUDIO_FORMAT_PCM;

        if (cmd_contain(argc, argv, "aac") || cmd_contain(argc, argv, "AAC"))
        {
            audio_format = VIDEO_RECORDER_AUDIO_FORMAT_AAC;
        }
        else if (cmd_contain(argc, argv, "g711u") || cmd_contain(argc, argv, "G711U") ||
                 cmd_contain(argc, argv, "ulaw") || cmd_contain(argc, argv, "ULAW"))
        {
            audio_format = VIDEO_RECORDER_AUDIO_FORMAT_MULAW;
        }
        else if (cmd_contain(argc, argv, "g711a") || cmd_contain(argc, argv, "G711A") ||
                 cmd_contain(argc, argv, "alaw") || cmd_contain(argc, argv, "ALAW") ||
                 cmd_contain(argc, argv, "g711") || cmd_contain(argc, argv, "G711"))
        {
            audio_format = VIDEO_RECORDER_AUDIO_FORMAT_ALAW;
        }
        else if (cmd_contain(argc, argv, "g722") || cmd_contain(argc, argv, "G722"))
        {
            audio_format = VIDEO_RECORDER_AUDIO_FORMAT_G722;
        }
        else if (cmd_contain(argc, argv, "mp3") || cmd_contain(argc, argv, "MP3"))
        {
            LOGE("%s: MP3 recording is not supported (no encoder)\n", __func__);
            goto exit;
        }

        if (argc >= 3)
        {
            file_path = argv[2];
        }

        os_memset(video_recorder_output_path, 0, sizeof(video_recorder_output_path));
        os_strncpy(video_recorder_output_path, file_path, sizeof(video_recorder_output_path) - 1);
        file_path = video_recorder_output_path;

        if (argc >= 5)
        {
            width = os_strtoul(argv[3], NULL, 10);
            height = os_strtoul(argv[4], NULL, 10);
        }

        if (cmd_contain(argc, argv, "h264") || cmd_contain(argc, argv, "H264"))
        {
            h264_mode = true;
        }

        const bool force_mp4 = cmd_contain(argc, argv, "mp4") || cmd_contain(argc, argv, "MP4");
        const bool force_avi = cmd_contain(argc, argv, "avi") || cmd_contain(argc, argv, "AVI");
        const bool path_mp4 = (os_strstr(video_recorder_output_path, ".mp4") != NULL);

        if (force_avi)
        {
            record_type = VIDEO_RECORDER_TYPE_AVI;
            char *ext = os_strstr(video_recorder_output_path, ".mp4");
            if (ext != NULL)
            {
                os_strncpy(ext, ".avi", 4);
            }
        }
        else if (force_mp4 || path_mp4 || h264_mode)
        {
            record_type = VIDEO_RECORDER_TYPE_MP4;
            char *ext = os_strstr(video_recorder_output_path, ".avi");
            if (ext != NULL)
            {
                os_strncpy(ext, ".mp4", 4);
            }
            if (h264_mode)
            {
                LOGI("%s: H264 recording uses MP4 container, output=%s (pass 'avi' to force AVI)\n",
                     __func__, video_recorder_output_path);
            }
        }
        else
        {
            record_type = VIDEO_RECORDER_TYPE_AVI;
        }

        video_recorder_h264_mode = h264_mode;
        video_recorder_update_camera_board_config(width, height);

        camera_board_config_t *camera_board = app_camera_board_config_get();
        if (camera_board == NULL)
        {
            LOGE("%s: camera board config is NULL\n", __func__);
            goto exit;
        }

        LOGI("%s: opening MIPI camera, sensor %ux%u@%u, ISP output %ux%u\n",
             __func__,
             (unsigned)camera_board->mipi.sensor_max_width,
             (unsigned)camera_board->mipi.sensor_max_height,
             (unsigned)camera_board->mipi.sensor_fps,
             (unsigned)camera_board->isp.mp_width,
             (unsigned)camera_board->isp.mp_height);

        if (app_isp_mipi_camera_turn_on(camera_board) != BK_OK)
        {
            LOGE("%s: app_isp_mipi_camera_turn_on failed\n", __func__);
            goto exit;
        }
        video_recorder_camera_started = true;

        if (h264_mode)
        {
            if (app_h264e_turn_on() != BK_OK)
            {
                LOGE("%s: app_h264e_turn_on failed\n", __func__);
                video_recorder_stop_encode_and_camera();
                goto exit;
            }
        }
        else
        {
            if (app_jpege_turn_on() != BK_OK)
            {
                LOGE("%s: app_jpege_turn_on failed\n", __func__);
                video_recorder_stop_encode_and_camera();
                goto exit;
            }
        }

        ret = video_recorder_start_isp_encode_bond();
        if (ret != AVDK_ERR_OK)
        {
            video_recorder_stop_encode_and_camera();
            goto exit;
        }

        video_recorder_frame_total_count = 0;
        video_recorder_frame_miss_count = 0;

        audio_recorder_device_cfg_t audio_recorder_cfg = {0};
        audio_recorder_cfg.audio_channels = 1;
        audio_recorder_cfg.audio_rate = (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_G722) ? 16000 : 8000;
        if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_ALAW || audio_format == VIDEO_RECORDER_AUDIO_FORMAT_MULAW)
        {
            audio_recorder_cfg.audio_bits = 8;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_PCM)
        {
            audio_recorder_cfg.audio_bits = 16;
        }
        else
        {
            audio_recorder_cfg.audio_bits = 0;
        }
        audio_recorder_cfg.audio_format = audio_format;

        ret = audio_recorder_device_init(&audio_recorder_cfg, &video_recorder_audio_recorder_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: audio_recorder_device_init failed, ret=%d\n", __func__, ret);
            video_recorder_stop_encode_and_camera();
            goto exit;
        }

        ret = audio_recorder_device_start(video_recorder_audio_recorder_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: audio_recorder_device_start failed, ret=%d\n", __func__, ret);
            audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
            video_recorder_audio_recorder_handle = NULL;
            video_recorder_stop_encode_and_camera();
            goto exit;
        }

        bk_video_recorder_config_t video_recorder_config = {0};
        video_recorder_config.record_type = record_type;
        video_recorder_config.record_format = h264_mode ? VIDEO_RECORDER_FORMAT_H264 : VIDEO_RECORDER_FORMAT_MJPEG;
        video_recorder_config.record_framerate = camera_board->mipi.sensor_fps;
        video_recorder_config.video_width = width;
        video_recorder_config.video_height = height;
        video_recorder_config.audio_channels = 1;
        video_recorder_config.audio_rate = audio_recorder_cfg.audio_rate;
        video_recorder_config.audio_bits = audio_recorder_cfg.audio_bits;
        video_recorder_config.audio_format = audio_format;
        video_recorder_config.get_frame_cb = video_recorder_get_frame_cb;
        video_recorder_config.get_audio_cb = video_recorder_get_audio_cb;
        video_recorder_config.release_frame_cb = video_recorder_release_frame_cb;
        video_recorder_config.release_audio_cb = video_recorder_release_audio_cb;

        ret = bk_video_recorder_new(&video_recorder_handle, &video_recorder_config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_recorder_new failed, ret=%d\n", __func__, ret);
            audio_recorder_device_stop(video_recorder_audio_recorder_handle);
            audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
            video_recorder_audio_recorder_handle = NULL;
            video_recorder_stop_encode_and_camera();
            goto exit;
        }

        ret = bk_video_recorder_open(video_recorder_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_recorder_open failed, ret=%d\n", __func__, ret);
            bk_video_recorder_delete(video_recorder_handle);
            video_recorder_handle = NULL;
            audio_recorder_device_stop(video_recorder_audio_recorder_handle);
            audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
            video_recorder_audio_recorder_handle = NULL;
            video_recorder_stop_encode_and_camera();
            goto exit;
        }

        ret = bk_video_recorder_start(video_recorder_handle, file_path, record_type);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_recorder_start failed, ret=%d\n", __func__, ret);
            bk_video_recorder_close(video_recorder_handle);
            bk_video_recorder_delete(video_recorder_handle);
            video_recorder_handle = NULL;
            audio_recorder_device_stop(video_recorder_audio_recorder_handle);
            audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
            video_recorder_audio_recorder_handle = NULL;
            video_recorder_stop_encode_and_camera();
            goto exit;
        }

        video_recorder_session_id++;
        if (video_recorder_start_auto_stop_timer(video_recorder_session_id) != BK_OK)
        {
            LOGE("%s: Failed to start auto stop timer\n", __func__);
            (void)video_recorder_stop_internal(false);
            goto exit;
        }

        LOGI("%s: Recording started, file=%s, %ux%u, %s\n",
             __func__, file_path, (unsigned)width, (unsigned)height,
             h264_mode ? "H264" : "MJPEG");
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        if (video_recorder_handle == NULL)
        {
            LOGE("%s: video recording not started\n", __func__);
            goto exit;
        }

        video_recorder_session_id++;
        ret = video_recorder_stop_internal(false);
        if (ret != AVDK_ERR_OK)
        {
            goto exit;
        }

        msg = CLI_CMD_RSP_SUCCEED;
    }
    else
    {
        LOGE("%s: unknown command: %s\n", __func__, argv[1]);
        goto exit;
    }

exit:
    if (msg == NULL)
    {
        msg = (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR;
    }

    if (pcWriteBuffer != NULL && xWriteBufferLen > 0)
    {
        os_strncpy(pcWriteBuffer, msg, xWriteBufferLen - 1);
        pcWriteBuffer[xWriteBufferLen - 1] = '\0';
    }
}
