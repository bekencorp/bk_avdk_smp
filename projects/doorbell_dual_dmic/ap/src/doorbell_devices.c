#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#if CONFIG_VOICE_SERVICE
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>
#endif

#include <driver/dvp_camera_types.h>
#include <driver/lcd.h>

#include "doorbell_comm.h"
#include "doorbell_transmission.h"
#include "doorbell_cmd.h"
#include "doorbell_devices.h"
#include "doorbell_cs2_service.h"

#include "wifi_transfer.h"
#include "media_app.h"
#include "camera_handle_list.h"
#include "img_service.h"

#include "uvc_pipeline_act.h"
#include "lcd_display_service.h"
#include "media_utils.h"

#define TAG "db-device"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define DB_SAMPLE_RARE_8K (8000)
#define DB_SAMPLE_RARE_16K (16000)

#define CAMERA_DEVICES_REPORT (BK_FALSE)//(BK_TRUE)

typedef enum
{
    LCD_STATUS_CLOSE,
    LCD_STATUS_OPEN,
    LCD_STATUS_UNKNOWN,
} lcd_status_t;

extern const dvp_sensor_config_t **get_sensor_config_devices_list(void);
extern int get_sensor_config_devices_num(void);

extern const doorbell_service_interface_t *doorbell_current_service;
static media_camera_device_t current_device = {0};


#define DEVICE_RESPONSE_SIZE (DOORBELL_NETWORK_MAX_SIZE - sizeof(db_evt_head_t))

#define UVC_DEVICE_ID (0xFDF6)

db_device_info_t *db_device_info = NULL;



int doorbell_get_ppis(char *ppi, int capability, int size)
{
    int ret = 0;
    strcat(ppi, "[");

    if (capability & PPI_CAP_320X240)
    {
        strcat(ppi, " \"320X240\",");
    }

    if (capability & PPI_CAP_320X480)
    {
        strcat(ppi, " \"320X480\",");
    }

    if (capability & PPI_CAP_480X272)
    {
        strcat(ppi, " \"480X272\",");
    }

    if (capability & PPI_CAP_480X320)
    {
        strcat(ppi, " \"480X320\",");
    }

    if (capability & PPI_CAP_640X480)
    {
        strcat(ppi, " \"640X480\",");
    }

    if (capability & PPI_CAP_480X800)
    {
        strcat(ppi, " \"480X800\",");
    }

    if (capability & PPI_CAP_800X480)
    {
        strcat(ppi, " \"800X480\",");
    }

    if (capability & PPI_CAP_800X600)
    {
        strcat(ppi, " \"800X600\",");
    }

    if (capability & PPI_CAP_864X480)
    {
        strcat(ppi, " \"864X480\",");
    }

    if (capability & PPI_CAP_1024X600)
    {
        strcat(ppi, " \"1024X600\",");
    }

    if (capability & PPI_CAP_1280X720)
    {
        strcat(ppi, " \"1280X720\",");
    }

    if (capability & PPI_CAP_1600X1200)
    {
        strcat(ppi, " \"1600X1200\",");
    }

    if (capability & PPI_CAP_480X480)
    {
        strcat(ppi, " \"480X480\",");
    }

    if (capability & PPI_CAP_720X288)
    {
        strcat(ppi, " \"720X288\",");
    }

    if (capability & PPI_CAP_720X576)
    {
        strcat(ppi, " \"720X576\",");
    }

    if (capability & PPI_CAP_480X854)
    {
        strcat(ppi, " \"480X854\",");
    }

    ret = strlen(ppi);

    ppi[ret - 1] = ']';

    return ret;
}


int doorbell_get_supported_camera_devices(int opcode, db_channel_t *channel, doorbell_transmission_send_t cb)
{
    db_evt_head_t *evt = os_malloc(sizeof(db_evt_head_t) + DEVICE_RESPONSE_SIZE);
    char *p = (char *)(evt + 1);

    evt->opcode = opcode;
    evt->status = EVT_STATUS_OK;
    evt->flags = EVT_FLAGS_CONTINUE;

    LOGD("DBCMD_GET_CAMERA_SUPPORTED_DEVICES\n");

#if (CAMERA_DEVICES_REPORT == BK_TRUE)

    int ret = 0;
    const dvp_sensor_config_t **sensors = get_sensor_config_devices_list();
    uint32_t i, size = get_sensor_config_devices_num();


    for (i = 0; i < size; i++)
    {
        char ppi[500] = {0};

        ret = doorbell_get_ppis(ppi, sensors[i]->ppi_cap, sizeof(ppi));

        if (ret >= sizeof(ppi))
        {
            LOGE("doorbell_camera_get_ppis overflow\n");
        }

        os_memset(p, 0, DEVICE_RESPONSE_SIZE);

        LOGV("sensor: %s, ppi: %uX%u\n", sensors[i]->name,
             ppi_to_pixel_x(sensors[i]->def_ppi),
             ppi_to_pixel_y(sensors[i]->def_ppi));
        sprintf(p, "{\"name\": \"%s\", \"id\": \"%d\", \"type\": \"DVP\", \"ppi\": %s}",
                sensors[i]->name,
                sensors[i]->id,
                ppi);

        LOGD("dump: %s\n", p);

        evt->length = CHECK_ENDIAN_UINT16(strlen(p));
        doorbell_transmission_pack_send(channel, (uint8_t *)evt, sizeof(db_evt_head_t) + evt->length, cb);
    }

#else
    os_memset(p, 0, DEVICE_RESPONSE_SIZE);

    sprintf(p, "{\"name\": \"%s\", \"id\": \"%d\", \"type\": \"DVP\", \"ppi\":[\"%uX%u\"]}",
            "DVP",
            1,
            ppi_to_pixel_x(0),
            ppi_to_pixel_y(0));
    evt->length = CHECK_ENDIAN_UINT16(strlen(p));
    doorbell_transmission_pack_send(channel, (uint8_t *)evt, sizeof(db_evt_head_t) + evt->length, cb);


#endif
    os_memset(p, 0, DEVICE_RESPONSE_SIZE);

    sprintf(p, "{\"name\": \"%s\", \"id\": \"%d\", \"type\": \"UVC\", \"ppi\":[\"%uX%u\"]}",
            "UVC",
            UVC_DEVICE_ID,
            ppi_to_pixel_x(0),
            ppi_to_pixel_y(0));
    evt->length = CHECK_ENDIAN_UINT16(strlen(p));
    evt->flags = EVT_FLAGS_COMPLETE;
    doorbell_transmission_pack_send(channel, (uint8_t *)evt, sizeof(db_evt_head_t) + evt->length, cb);

    os_free(evt);

    return 0;
}

int doorbell_get_supported_lcd_devices(int opcode, db_channel_t *channel, doorbell_transmission_send_t cb)
{
    uint32_t i, size;
    size = get_lcd_devices_num();//media_app_get_lcd_devices_num();
    const lcd_device_t **device = get_lcd_devices_list();//media_app_get_lcd_devices_list();
    db_evt_head_t *evt = os_malloc(sizeof(db_evt_head_t) + DEVICE_RESPONSE_SIZE);
    char *p = (char *)(evt + 1);

    evt->opcode = opcode;
    evt->status = EVT_STATUS_OK;
    evt->flags = EVT_FLAGS_CONTINUE;

    LOGD("DBCMD_GET_LCD_SUPPORTED_DEVICES\n");

    if ((uint32_t)device != kGeneralErr && device != NULL)
    {
        for (i = 0; i < size; i++)
        {
            os_memset(p, 0, DEVICE_RESPONSE_SIZE);

            LOGV("lcd: %s, ppi: %uX%u\n", device[i]->name,
                 ppi_to_pixel_x(device[i]->ppi),
                 ppi_to_pixel_y(device[i]->ppi));
            sprintf(p, "{\"name\": \"%s\", \"id\": \"%d\", \"type\": \"%s\", \"ppi\":\"%uX%u\"}",
                    device[i]->name,
                    device[i]->id,
                    device[i]->type == LCD_TYPE_RGB565 ? "rgb" : "mcu",
                    ppi_to_pixel_x(device[i]->ppi),
                    ppi_to_pixel_y(device[i]->ppi));

            LOGD("dump: %s\n", p);

            evt->length = CHECK_ENDIAN_UINT16(strlen(p));

            if (i == size - 1)
            {
                evt->flags = EVT_FLAGS_COMPLETE;
            }

            doorbell_transmission_pack_send(channel, (uint8_t *)evt, sizeof(db_evt_head_t) + evt->length, cb);
        }
    }

    os_free(evt);

    return 0;
}

int doorbell_get_lcd_status(int opcode, db_channel_t *channel, doorbell_transmission_send_t cb)
{
    uint32_t lcd_status = media_app_get_lcd_status();
    db_evt_head_t *evt = os_malloc(sizeof(db_evt_head_t) + DEVICE_RESPONSE_SIZE);
    char *p = (char *)(evt + 1);

    evt->opcode = opcode;
    evt->status = EVT_STATUS_OK;
    evt->flags = EVT_FLAGS_CONTINUE;

    LOGD("DBCMD_GET_LCD_STATUS\n");
    os_memset(p, 0, DEVICE_RESPONSE_SIZE);

    if (lcd_status != LCD_STATUS_CLOSE && lcd_status != LCD_STATUS_OPEN)
    {
        lcd_status = LCD_STATUS_UNKNOWN;
    }
    sprintf(p, "{\"status\": \"%u\"}", lcd_status);
    LOGD("dump: %s\n", p);
    evt->length = CHECK_ENDIAN_UINT16(strlen(p));

    evt->flags = EVT_FLAGS_COMPLETE;

    doorbell_transmission_pack_send(channel, (uint8_t *)evt, sizeof(db_evt_head_t) + evt->length, cb);

    os_free(evt);

    return 0;
}

int doorbell_devices_set_camera_transfer_callback(void *cb)
{
    if (db_device_info == NULL)
    {
        LOGE("db_device_info null");
        return  BK_FAIL;
    }

    db_device_info->camera_transfer_cb = (media_transfer_cb_t *)cb;

    return BK_OK;
}

int doorbell_devices_set_audio_transfer_callback(const void *cb)
{
    if (db_device_info == NULL)
    {
        LOGE("db_device_info null");
        return  BK_FAIL;
    }

    db_device_info->audio_transfer_cb = (const media_transfer_cb_t *)cb;

    return BK_OK;
}

int doorbell_camera_turn_on(camera_parameters_t *parameters)
{
    bk_err_t ret = BK_FAIL;
    uint8_t rot_angle = 0;
    media_camera_device_t device = {0};

    LOGD("%s, id: %d, %d X %d, format: %d, Protocol: %d\n", __func__,
         parameters->id, parameters->width, parameters->height,
         parameters->format, parameters->protocol);

    if (db_device_info->video_handle != NULL)
    {
        LOGD("%s, id: %d already open\n", __func__, parameters->id);
        return EVT_STATUS_ALREADY;
    }

    if (parameters->id == UVC_DEVICE_ID)
    {
        device.type = UVC_CAMERA;
        device.port = 1;
        db_device_info->camera_id = 1;
    }
    else
    {
        device.type = DVP_CAMERA;
        device.port = 0;
        db_device_info->camera_id = 0;
    }

    if (parameters->format == 0) // wifi transfer format 0/1:mjpeg/h264
    {
        device.format = IMAGE_MJPEG;
        if (device.type == DVP_CAMERA)
        {
            device.format = IMAGE_YUV | IMAGE_MJPEG;
        }

        db_device_info->h264_transfer = false;
    }
    else
    {
        if (device.type == DVP_CAMERA)
        {
            device.format = IMAGE_YUV | IMAGE_H264;
            db_device_info->pipeline_enable = false;
        }
        else
        {
            device.format = IMAGE_MJPEG;// uvc output mjpeg(not h264 stream)
            db_device_info->pipeline_enable = true;
        }
        db_device_info->h264_transfer = true;
    }

    LOGD("%s, device:fmt:%d, transfer:%s\n", __func__, device.format, db_device_info->h264_transfer ? "h264" : "mjpeg");
    device.width = parameters->width;
    device.height = parameters->height;
    device.fps = FPS30;

    ret = media_app_camera_open(&db_device_info->video_handle, &device);

    if (ret != BK_OK)
    {
        LOGE("%s failed\n", __func__);
        if (db_device_info->pipeline_enable)
        {
            int ret_val = 0;
            ret_val = h264_jdec_pipeline_regenerate_idr_frame();
            if (ret_val != BK_OK)
            {
                LOGE("%s h264_jdec_pipeline_regenerate_idr_frame failed\n", __func__);
                return ret_val;
            }
        }
        return ret;
    }

    // check, output image need rotate or not (for soft jpegdec)
    switch (parameters->rotate)
    {
        case 90:
            rot_angle = ROTATE_90;
            break;
        case 180:
            rot_angle = ROTATE_180;
            break;
        case 270:
            rot_angle = ROTATE_270;
            break;
        case 0:
            rot_angle = ROTATE_NONE;
            break;
        default:
            rot_angle = ROTATE_90;
            break;
    }
    media_app_set_rotate(rot_angle);

    if (db_device_info->pipeline_enable)
    {
        ret = h264_jdec_pipeline_open();
        if (ret != BK_OK)
        {
            LOGE("%s h264_pipeline_open failed\n", __func__);
            return ret;
        }
    }

    if (check_lcd_task_is_open())
    {
        if (device.type == UVC_CAMERA)
        {
            lcd_jdec_pipeline_open();
        }
        else if (device.type == DVP_CAMERA)
        {
            img_service_open();
        }
    }
    else
    {
        current_device.type = device.type;
    }

    return ret;
}

int doorbell_camera_turn_off(void)
{
    if (db_device_info->video_handle == NULL)
    {
        LOGD("%s, %d already close\n", __func__);
        return EVT_STATUS_ALREADY;
    }

    //if (db_device_info->pipeline_enable)
    {
        h264_jdec_pipeline_close();
        LOGD("%s h264_pipeline close\n", __func__);
    }

    do
    {
        db_device_info->video_handle = bk_camera_handle_node_pop();
        if (db_device_info->video_handle)
        {
            LOGD("%s, %d, %p\n", __func__, __LINE__, db_device_info->video_handle);
            media_app_camera_close(&db_device_info->video_handle);
        }
        else
        {
            break;
        }
    }
    while (1);

    db_device_info->video_handle = NULL;
    db_device_info->camera_id = CAMERA_MAX_NUM;

    db_device_info->pipeline_enable = false;

    db_device_info->h264_transfer = false;

    return 0;
}

int doorbell_video_transfer_turn_on(void)
{
    int ret = -1;

    if (db_device_info->transfer_enable)
    {
        LOGD("%s, id: %d already open\n", __func__, db_device_info->transfer_enable);
        return EVT_STATUS_ALREADY;
    }

    if (db_device_info->camera_transfer_cb)
    {
        if (db_device_info->h264_transfer)
        {
            ret = bk_wifi_transfer_frame_open(db_device_info->camera_transfer_cb, IMAGE_H264);
        }
        else
        {
            ret = bk_wifi_transfer_frame_open(db_device_info->camera_transfer_cb, IMAGE_MJPEG);
        }
    }
    else
    {
        LOGE("media_transfer_cb: NULL\n");
    }

    if (ret == BK_OK)
    {
        db_device_info->transfer_enable = 1;
    }

    return ret;
}

int doorbell_video_transfer_turn_off(void)
{
    int ret = -1;

    if (db_device_info->transfer_enable == false)
    {
        LOGD("%s, id: %d already close\n", __func__, db_device_info->transfer_enable);
        return EVT_STATUS_ALREADY;
    }

    ret = bk_wifi_transfer_frame_close();

#if (CONFIG_INTEGRATION_DOORBELL_CS2)
    doorbell_cs2_img_timer_deinit();
#endif

    db_device_info->transfer_enable = false;

    return ret;
}

int doorbell_display_turn_on(uint16_t id, uint16_t rotate, uint16_t fmt)
{
    LOGD("%s, id: %d, rotate: %d fmt: %d\n", __func__, id, rotate, fmt);

    if (db_device_info->lcd_id != 0)
    {
        LOGD("%s, id: %d already open\n", __func__, id);
        return EVT_STATUS_ALREADY;
    }
    const lcd_device_t *device = (const lcd_device_t *)get_lcd_device_by_id(id);
    if ((uint32_t)device == BK_FAIL || device == NULL)
    {
        LOGD("%s, could not find device id: %d\n", __func__, id);
        return EVT_STATUS_ERROR;
    }

    lcd_open_t lcd_open = {0};
    lcd_open.device_ppi = device->ppi;
    lcd_open.device_name = device->name;

    uint8_t rot_angle = 0;
    if (fmt == 0)
    {
        lcd_set_fmt(PIXEL_FMT_RGB565_LE);
    }
    else if (fmt == 1)
    {
        lcd_set_fmt(PIXEL_FMT_RGB888);
    }

    switch (rotate)
    {
        case 90:
            rot_angle = ROTATE_90;
            break;
        case 180:
            rot_angle = ROTATE_180;
            break;
        case 270:
            rot_angle = ROTATE_270;
            break;
        case 0:
        default:
            rot_angle = ROTATE_NONE;
            break;
    }
    media_app_set_rotate(rot_angle);

    if (current_device.type == UVC_CAMERA)
    {
        lcd_jdec_pipeline_open();
    }
    else if (current_device.type == DVP_CAMERA)
    {
        img_service_open();
    }

    if (media_app_lcd_disp_open(&lcd_open) != BK_OK)
    {
        lcd_jdec_pipeline_close();
        img_service_close();
    }

    db_device_info->lcd_id = id;
    return 0;
}

int doorbell_display_turn_off(void)
{
    LOGD("%s, id: %d\n", __func__, db_device_info->lcd_id);

    if (db_device_info->lcd_id == 0)
    {
        LOGD("%s, %d already close\n", __func__);
        return EVT_STATUS_ALREADY;
    }

    lcd_jdec_pipeline_close();
    img_service_close();
    media_app_lcd_disp_close();

    db_device_info->lcd_id = 0;
    return 0;
}

#if CONFIG_VOICE_SERVICE
int doorbell_udp_voice_send_callback(unsigned char *data, unsigned int len, void *args)
{
    if (db_device_info == NULL)
    {
        LOGE("%s, db_device_info NULL\n", __func__);
        return BK_FAIL;
    }

    if (db_device_info->audio_transfer_cb == NULL)
    {
        LOGE("%s, audio_transfer_cb NULL\n", __func__);
        return BK_FAIL;
    }

    if (len > db_device_info->audio_transfer_cb->get_tx_size())
    {
        LOGE("%s, buffer over flow %d %d\n", __func__, len, db_device_info->audio_transfer_cb->get_tx_size());
        return BK_FAIL;
    }

    uint8_t *buffer = db_device_info->audio_transfer_cb->get_tx_buf();

    if (db_device_info->audio_transfer_cb->prepare)
    {
        db_device_info->audio_transfer_cb->prepare(data, len);
    }

    return db_device_info->audio_transfer_cb->send(buffer, len);
}

int doorbell_audio_turn_off(void)
{
    if (db_device_info->audio_enable == BK_FALSE)
    {
        LOGD("%s already turn off\n", __func__);

        return BK_FAIL;
    }

    LOGD("%s entry\n", __func__);

    db_device_info->audio_enable = BK_FALSE;

    if (doorbell_current_service
        && doorbell_current_service->audio_state_changed)
    {
        doorbell_current_service->audio_state_changed(DB_TURN_OFF);
    }

    if (db_device_info->voice_read_handle)
    {
        bk_voice_read_stop(db_device_info->voice_read_handle);
    }

    if (db_device_info->voice_write_handle)
    {
        bk_voice_write_stop(db_device_info->voice_write_handle);
    }

    if (db_device_info->voice_handle)
    {
        bk_voice_stop(db_device_info->voice_handle);
    }

    if (db_device_info->voice_read_handle)
    {
        bk_voice_read_deinit(db_device_info->voice_read_handle);
    }

    if (db_device_info->voice_write_handle)
    {
        bk_voice_write_deinit(db_device_info->voice_write_handle);
    }

    if (db_device_info->voice_handle)
    {
        bk_voice_deinit(db_device_info->voice_handle);
    }
    db_device_info->voice_read_handle = NULL;
    db_device_info->voice_write_handle = NULL;
    db_device_info->voice_handle  = NULL;

    LOGD("%s out\n", __func__);
    return BK_OK;
}

bk_err_t doorbell_audio_event_handle(vioce_evt_t event, void *param, void *args)
{
    doorbell_msg_t msg;

    switch (event)
    {
        case VOC_EVT_MIC_NOT_SUPPORT:
        case VOC_EVT_SPK_NOT_SUPPORT:
        case VOC_EVT_ERROR_UNKNOW:
        case VOC_EVT_STOP:
            LOGD("%s, -->>event: %d\n", __func__, event);
            msg.event = DBEVT_VOICE_EVENT;
            msg.param = event;
            doorbell_send_msg(&msg);
            break;

        default:
            break;
    }

    return BK_OK;
}

void doorbell_audio_codec_type_mapping(voice_cfg_t* voice_cfg, audio_parameters_t *parameters)
{
    switch (parameters->rmt_recoder_fmt)
    {
        case CODEC_FORMAT_G711A:
        {
            voice_cfg->enc_type = AUDIO_ENC_TYPE_G711A;
            break;
        }
        case CODEC_FORMAT_G711U:
        {
            voice_cfg->enc_type = AUDIO_ENC_TYPE_G711U;
            break;
        }
        case CODEC_FORMAT_PCM:
        {
            /* pcm encoder config */
            voice_cfg->enc_type = AUDIO_ENC_TYPE_PCM;
            break;
        }
        case CODEC_FORMAT_AAC:
        {
            /* pcm encoder config */
            voice_cfg->enc_type = AUDIO_ENC_TYPE_AAC;
            break;
        }
        case CODEC_FORMAT_G722:
        {
            /* pcm encoder config */
            voice_cfg->enc_type = AUDIO_ENC_TYPE_G722;
            break;
        }
        case CODEC_FORMAT_OPUS:
        {
            /* pcm encoder config */
            voice_cfg->enc_type = AUDIO_ENC_TYPE_OPUS;
            break;
        }
        default:
        {
            LOGE("not support encoder format%d\n",parameters->rmt_recoder_fmt);
            break;
        }
    }

    switch (parameters->rmt_player_fmt)
    {
        case CODEC_FORMAT_G711A:
        {
            voice_cfg->dec_type = AUDIO_DEC_TYPE_G711A;
            break;
        }
        case CODEC_FORMAT_G711U:
        {
            voice_cfg->dec_type = AUDIO_DEC_TYPE_G711U;
            break;
        }
        case CODEC_FORMAT_PCM:
        {
            /* pcm encoder config */
            voice_cfg->dec_type = AUDIO_DEC_TYPE_PCM;
            break;
        }
        case CODEC_FORMAT_AAC:
        {
            /* pcm encoder config */
            voice_cfg->dec_type = AUDIO_DEC_TYPE_AAC;
            break;
        }
        case CODEC_FORMAT_G722:
        {
            /* pcm encoder config */
            voice_cfg->dec_type = AUDIO_DEC_TYPE_G722;
            break;
        }
        case CODEC_FORMAT_OPUS:
        {
            /* pcm encoder config */
            voice_cfg->dec_type = AUDIO_DEC_TYPE_OPUS;
            break;
        }
        default:
        {
            LOGE("not support decoder format%d\n",parameters->rmt_player_fmt);
            break;
        }
    }
}

bk_err_t doorbell_buf_len_cal(voice_cfg_t *cfg)
{
    if (NULL == cfg) 
    {
        LOGE("%s, %d, voice_cfg is NULL!\n", __func__, __LINE__);
        return BK_FAIL;
    }
    
    cfg->enc_common.frame_in_size = cfg->enc_common.samp_rate*cfg->enc_common.frame_in_ms/1000*cfg->enc_common.bits/8;
    cfg->enc_common.frame_in_size *= cfg->enc_common.channels;
    
    switch (cfg->enc_type)
    {
        case AUDIO_ENC_TYPE_G711A:
        case AUDIO_ENC_TYPE_G711U:
        {
            cfg->enc_common.bitrate = cfg->enc_common.samp_rate*cfg->enc_common.bits/2;
            cfg->enc_common.frame_out_size = cfg->enc_common.bitrate*cfg->enc_common.frame_in_ms/1000/8;
            cfg->enc_common.frame_out_size *= cfg->enc_common.channels;
            break;
        }
        case AUDIO_ENC_TYPE_PCM:
        {
            cfg->enc_common.frame_out_size = cfg->enc_common.frame_in_size;
            break;
        }
        #if CONFIG_VOICE_SERVICE_AAC_ENCODER
        case AUDIO_ENC_TYPE_AAC:
        {    
            cfg->enc_common.bitrate = cfg->enc_cfg.aac_enc_cfg.bitrate;
            cfg->enc_common.frame_out_size = cfg->enc_common.bitrate*cfg->enc_common.frame_in_ms/1000/8;
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_G722_ENCODER
        case AUDIO_ENC_TYPE_G722:
        {
            cfg->enc_common.bitrate = 64000;
            cfg->enc_common.frame_out_size = cfg->enc_common.bitrate*cfg->enc_common.frame_in_ms/1000/8;
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_OPUS_ENCODER
        case AUDIO_ENC_TYPE_OPUS:
        {
            cfg->enc_common.bitrate = 16000;
            cfg->enc_common->vbr = 1;
            cfg->enc_common.frame_out_size = cfg->enc_common.bitrate*cfg->enc_common.frame_in_ms/1000/8;
            cfg->enc_common.frame_out_size *= 2;//enable VBR,output buffer size = CBR output buffer size * 2
            break;
        }
        #endif
        default:
            BK_LOGE(TAG, "%s, %d, enc_type: %d is not support\n", __func__, __LINE__, cfg->enc_type);
            return BK_FAIL;
            break;
    }

    cfg->dec_common.frame_out_size = cfg->dec_common.samp_rate*cfg->dec_common.frame_in_ms/1000*cfg->dec_common.bits/8;
    cfg->dec_common.frame_out_size *= cfg->dec_common.channels;
    cfg->dec_common.frame_in_size = cfg->dec_common.frame_out_size;
    switch (cfg->dec_type)
    {
        case AUDIO_DEC_TYPE_G711A:
        case AUDIO_DEC_TYPE_G711U:
        {
            cfg->dec_common.frame_in_size >>= 1;
            break;
        }
        case AUDIO_DEC_TYPE_PCM:
        {
            break;
        }
        #if CONFIG_VOICE_SERVICE_AAC_DECODER
        case AUDIO_DEC_TYPE_AAC:
        {
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_G722_DECODER
        case AUDIO_DEC_TYPE_G722:
        {
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_OPUS_DECODER
        case AUDIO_DEC_TYPE_OPUS:
        {
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_MP3_DECODER
        case AUDIO_DEC_TYPE_MP3:
        {
            cfg->dec_common.frame_out_size = bk_voice_get_mp3_frame_sample_cnt(cfg->dec_common.samp_rate,cfg->dec_common.channels)*cfg->dec_common.bits/8;
            cfg->dec_common.frame_in_size = MAINBUF_SIZE;
            break;
        }
        #endif
        default:
        {
            BK_LOGE(TAG, "%s, %d, enc_type: %d is not support\n", __func__, __LINE__, cfg->dec_type);
            return BK_FAIL;
            break;
        }
    }

    if(cfg->aec_en)
    {
        if(3 == cfg->aec_ver)
        {
            cfg->aec_cfg.aec_v3_alg_cfg.out_block_size = cfg->enc_common.frame_in_size;
            cfg->aec_cfg.aec_v3_alg_cfg.out_block_num = 2;
        }
    }
    
    BK_LOGI(TAG, "%s, %d, aud enc samp:%d,bitrate:%d,frame (ms):%d,bits:%d,vbr:%d,chs:%d\n", __func__, __LINE__,
            cfg->enc_common.samp_rate,
            cfg->enc_common.bitrate,
            cfg->enc_common.frame_in_ms,
            cfg->enc_common.bits,
            cfg->enc_common.vbr,
            cfg->enc_common.channels);

    BK_LOGI(TAG, "aud dec samp:%d,frame (ms):%d,bits:%d,chs:%d\n",
            cfg->dec_common.samp_rate,
            cfg->dec_common.frame_in_ms,
            cfg->dec_common.bits,
            cfg->dec_common.channels);

    BK_LOGI(TAG, "enc in s:%d,out s:%d,dec in s:%d,out s:%d\n",
            cfg->enc_common.frame_in_size,
            cfg->enc_common.frame_out_size,
            cfg->dec_common.frame_in_size,
            cfg->dec_common.frame_out_size);
    return BK_OK;
}


void doorbell_voice_cfg_update(voice_cfg_t* voice_cfg)
{
    voice_cfg->read_pool_size = voice_cfg->enc_common.frame_out_size*2;
    voice_cfg->write_pool_size = voice_cfg->dec_common.frame_in_size*2;
    
    switch (voice_cfg->enc_type)
    {
        case AUDIO_ENC_TYPE_G711A:
        case AUDIO_ENC_TYPE_G711U:
        {
            /* g711 encoder config */
            g711_encoder_cfg_t g711_encoder_cfg = DEFAULT_G711_ENCODER_CONFIG();
            voice_cfg->enc_cfg.g711_enc_cfg = g711_encoder_cfg;
            if (AUDIO_ENC_TYPE_G711A == voice_cfg->enc_type)
            {
                voice_cfg->enc_cfg.g711_enc_cfg.enc_mode = G711_ENC_MODE_A_LOW;
            }
            else
            {
                voice_cfg->enc_cfg.g711_enc_cfg.enc_mode = G711_ENC_MODE_U_LOW;
            }
            voice_cfg->enc_cfg.g711_enc_cfg.buf_sz = voice_cfg->enc_common.frame_in_size;
            voice_cfg->enc_cfg.g711_enc_cfg.out_block_size = voice_cfg->enc_common.frame_out_size;
            break;
        }
        case AUDIO_ENC_TYPE_PCM:
        {
            voice_cfg->enc_cfg.pcm_enc_cfg = 0;      // not used
            break;
        }
        #if CONFIG_VOICE_SERVICE_AAC_ENCODER
        case AUDIO_ENC_TYPE_AAC:
        {
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_G722_ENCODER
        case AUDIO_ENC_TYPE_G722:
        {
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_OPUS_ENCODER
        case AUDIO_ENC_TYPE_OPUS:
        {
            break;
        }
        #endif
        default:
        {
            LOGE("%s:%d,not support encoder type:%d\n",__func__,__LINE__,voice_cfg->enc_type);
            break;
        }
    }

    switch (voice_cfg->dec_type)
    {
        case AUDIO_DEC_TYPE_G711A:
        case AUDIO_DEC_TYPE_G711U:
        {
            /* g711 decoder config */
            g711_decoder_cfg_t g711_decoder_cfg = DEFAULT_G711_DECODER_CONFIG();
            voice_cfg->dec_cfg.g711_dec_cfg = g711_decoder_cfg;
            if (AUDIO_DEC_TYPE_G711A == voice_cfg->dec_type)
            {
                voice_cfg->dec_cfg.g711_dec_cfg.dec_mode = G711_DEC_MODE_A_LOW;
            }
            else
            {
                voice_cfg->dec_cfg.g711_dec_cfg.dec_mode = G711_DEC_MODE_U_LOW;
            }
            voice_cfg->dec_cfg.g711_dec_cfg.buf_sz = voice_cfg->dec_common.frame_in_size;
            voice_cfg->dec_cfg.g711_dec_cfg.out_block_size = voice_cfg->dec_common.frame_out_size;
            break;
        }
        case AUDIO_DEC_TYPE_PCM:
        {
            voice_cfg->dec_cfg.pcm_dec_cfg = 0;      // not used
            break;
        }
        #if CONFIG_VOICE_SERVICE_AAC_DECODER
        case AUDIO_DEC_TYPE_AAC:
        {
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_G722_DECODER
        case AUDIO_DEC_TYPE_G722:
        {
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_OPUS_DECODER
        case AUDIO_DEC_TYPE_OPUS:
        {
            break;
        }
        #endif
        #if CONFIG_VOICE_SERVICE_MP3_DECODER
        case AUDIO_DEC_TYPE_MP3:
        {
            break;
        }
        #endif
        default:
        {
            LOGE("%s:%d,not support decoder type:%d\n",__func__,__LINE__,voice_cfg->dec_type);
            break;
        }
    }
}

int doorbell_audio_turn_on(audio_parameters_t *parameters)
{
    voice_cfg_t voice_cfg = {0};

    if (db_device_info->audio_enable == BK_TRUE)
    {
        LOGD("%s already turn on\n", __func__);

        return BK_FAIL;
    }

    LOGD("%s, AEC: %d, UAC: %d, sample rate: %d, %d, fmt: %d, %d\n", __func__,
         parameters->aec, parameters->uac, parameters->rmt_recorder_sample_rate,
         parameters->rmt_player_sample_rate, parameters->rmt_recoder_fmt, parameters->rmt_player_fmt);

    uint32_t mic_sample_rate = parameters->rmt_recorder_sample_rate;
    uint32_t spk_sample_rate = parameters->rmt_player_sample_rate;

    //AEC only support 8k/16k sample rate/20ms frame
    switch (mic_sample_rate)
    {
        case DB_SAMPLE_RARE_8K:
            mic_sample_rate = 8000;
            break;

        case DB_SAMPLE_RARE_16K:
            mic_sample_rate = 16000;
            break;

        default:
            mic_sample_rate = 8000;
            break;
    }
   
    if((20 != parameters->rmt_recorder_frame_in_ms)
        && (40 != parameters->rmt_recorder_frame_in_ms)
        && (60 != parameters->rmt_recorder_frame_in_ms))
    {
        parameters->rmt_recorder_frame_in_ms = 20;
    }

    if((20 != parameters->rmt_player_frame_in_ms)
        && (40 != parameters->rmt_player_frame_in_ms)
        && (60 != parameters->rmt_player_frame_in_ms))
    {
        parameters->rmt_player_frame_in_ms = 20;
    }
    LOGD("para->rec_f_ms:%d,pl_f_ms:%d\n",parameters->rmt_recorder_frame_in_ms,parameters->rmt_player_frame_in_ms);

    if (parameters->uac == 1)
    {
        voice_cfg_t voice_uac_cfg = VOICE_BY_UAC_MIC_SPK_CFG_DEFAULT();
        voice_cfg = voice_uac_cfg;

        if (parameters->aec == 1)
        {
            voice_cfg.aec_en = true;
            voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.fs = mic_sample_rate;
        }
        else
        {
            voice_cfg.aec_en = false;
        }
        voice_cfg.enc_common.samp_rate = mic_sample_rate;
        voice_cfg.dec_common.samp_rate = spk_sample_rate;
        voice_cfg.enc_common.frame_in_ms = parameters->rmt_recorder_frame_in_ms;
        voice_cfg.dec_common.frame_in_ms = parameters->rmt_player_frame_in_ms;
        voice_cfg.enc_common.channels = 1;
        voice_cfg.dec_common.channels = 1;
        voice_cfg.enc_common.bits = voice_cfg.mic_cfg.uac_mic_cfg.bits;
        voice_cfg.dec_common.bits = voice_cfg.spk_cfg.onboard_spk_cfg.bits; 

        doorbell_audio_codec_type_mapping(&voice_cfg, parameters);
        doorbell_buf_len_cal(&voice_cfg);
        voice_cfg.mic_cfg.uac_mic_cfg.samp_rate = mic_sample_rate;
        voice_cfg.mic_cfg.uac_mic_cfg.frame_size = mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)

        if(voice_cfg.enc_en)
        {
            voice_cfg.mic_cfg.uac_mic_cfg.out_block_size = voice_cfg.enc_common.frame_in_size;
            voice_cfg.mic_cfg.uac_mic_cfg.out_block_num = 2;
        }
        else
        {
            voice_cfg.mic_cfg.uac_mic_cfg.out_block_size = voice_cfg.mic_cfg.uac_mic_cfg.frame_size;
            voice_cfg.mic_cfg.uac_mic_cfg.out_block_num = 2;
        }

        voice_cfg.spk_cfg.uac_spk_cfg.samp_rate = spk_sample_rate;
        voice_cfg.spk_cfg.uac_spk_cfg.frame_size = voice_cfg.dec_common.frame_out_size;
    }
    else//DUAL DMIC
    {        
        voice_cfg_t voice_onboard_cfg = VOICE_BY_ONBOARD_DUAL_DMIC_MIC_SPK_CFG_DEFAULT();
        voice_cfg = voice_onboard_cfg;

        if (parameters->aec == 1)
        {
            voice_cfg.aec_en = true;
            if(3 == voice_cfg.aec_ver)
            {
                voice_cfg.aec_cfg.aec_v3_alg_cfg.aec_cfg.fs = mic_sample_rate;
            }
            else
            {
                voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.fs = mic_sample_rate;
            }
            
        }
        else
        {
            voice_cfg.aec_en = false;
        }
        
        voice_cfg.enc_common.samp_rate = mic_sample_rate;
        voice_cfg.dec_common.samp_rate = spk_sample_rate;
        voice_cfg.enc_common.frame_in_ms = parameters->rmt_recorder_frame_in_ms;
        voice_cfg.dec_common.frame_in_ms = parameters->rmt_player_frame_in_ms;
        voice_cfg.enc_common.channels = 1;
        voice_cfg.dec_common.channels = 1;
        voice_cfg.enc_common.bits = voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.bits;
        voice_cfg.dec_common.bits = voice_cfg.spk_cfg.onboard_spk_cfg.bits; 
        
        doorbell_audio_codec_type_mapping(&voice_cfg, parameters);
        doorbell_buf_len_cal(&voice_cfg);

        voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg.adc_cfg.sample_rate = mic_sample_rate;
        voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg.frame_size = mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)

        if(voice_cfg.enc_en)
        {
            voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg.out_block_size = voice_cfg.enc_common.frame_in_size;
            voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg.out_block_num = 2;
        }
        else

        {
            voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg.out_block_size = voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg.frame_size;
            voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg.out_block_num = 2;
        }

        voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate = spk_sample_rate;
        voice_cfg.spk_cfg.onboard_spk_cfg.frame_size = voice_cfg.dec_common.frame_out_size;
    }

    doorbell_voice_cfg_update(&voice_cfg);

    //voice_cfg.event_handle = doorbell_audio_event_handle; /* close audio event, because sram is not enough */
    voice_cfg.event_handle = NULL;
    voice_cfg.args = NULL;
    db_device_info->voice_handle = bk_voice_init(&voice_cfg);
    if (!db_device_info->voice_handle)
    {
        LOGE("voice init fail\n");
        goto error;
    }

    voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
    voice_read_cfg.voice_handle = db_device_info->voice_handle;
    //voice_read_cfg.max_read_size = mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
    voice_read_cfg.max_read_size = 1280;//mic_sample_rate * 2 * 20 * 10 / 1000; //one frame size(200ms)
    voice_read_cfg.voice_read_callback = doorbell_udp_voice_send_callback;
    voice_read_cfg.args = NULL;
    voice_read_cfg.task_stack = 1024 * 4;
    voice_read_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
    db_device_info->voice_read_handle = bk_voice_read_init(&voice_read_cfg);
    if (!db_device_info->voice_read_handle)
    {
        LOGE("voice read init fail\n");
        goto error;
    }

    voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
    voice_write_cfg.voice_handle = db_device_info->voice_handle;
    voice_write_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
    db_device_info->voice_write_handle = bk_voice_write_init(&voice_write_cfg);
    if (!db_device_info->voice_write_handle)
    {
        LOGE("voice write init fail\n");
        goto error;
    }

    if (BK_OK != bk_voice_start(db_device_info->voice_handle))
    {
        LOGE("voice start fail\n");
        goto error;
    }

    if (BK_OK != bk_voice_read_start(db_device_info->voice_read_handle))
    {
        LOGE("voice read start fail\n");
        goto error;
    }

    if (BK_OK != bk_voice_write_start(db_device_info->voice_write_handle))
    {
        LOGE("voice write start fail\n");
        goto error;
    }

    db_device_info->audio_enable = BK_TRUE;

    if (doorbell_current_service
        && doorbell_current_service->audio_state_changed)
    {
        doorbell_current_service->audio_state_changed(DB_TURN_ON);
    }

    return BK_OK;
error:
    if (db_device_info->voice_read_handle)
    {
        bk_voice_read_stop(db_device_info->voice_read_handle);
    }

    if (db_device_info->voice_write_handle)
    {
        bk_voice_write_stop(db_device_info->voice_write_handle);
    }

    if (db_device_info->voice_handle)
    {
        bk_voice_stop(db_device_info->voice_handle);
    }

    if (db_device_info->voice_read_handle)
    {
        bk_voice_read_deinit(db_device_info->voice_read_handle);
    }

    if (db_device_info->voice_write_handle)
    {
        bk_voice_write_deinit(db_device_info->voice_write_handle);
    }

    if (db_device_info->voice_handle)
    {
        bk_voice_deinit(db_device_info->voice_handle);
    }
    db_device_info->voice_read_handle = NULL;
    db_device_info->voice_write_handle  = NULL;
    db_device_info->voice_handle  = NULL;

    return BK_FAIL;
}

int doorbell_audio_acoustics(uint32_t index, uint32_t param)
{
    LOGD("%s, %u, %u\n", __func__, index, param);
#if 0
    bk_err_t ret = BK_FAIL;

    switch (index)
    {
        case AA_ECHO_DEPTH:
            ret = bk_aud_intf_set_aec_para(AUD_INTF_VOC_AEC_EC_DEPTH, param);
            break;
        case AA_MAX_AMPLITUDE:
            ret = bk_aud_intf_set_aec_para(AUD_INTF_VOC_AEC_TXRX_THR, param);
            break;
        case AA_MIN_AMPLITUDE:
            ret = bk_aud_intf_set_aec_para(AUD_INTF_VOC_AEC_TXRX_FLR, param);
            break;
        case AA_NOISE_LEVEL:
            ret = bk_aud_intf_set_aec_para(AUD_INTF_VOC_AEC_NS_LEVEL, param);
            break;
        case AA_NOISE_PARAM:
            ret = bk_aud_intf_set_aec_para(AUD_INTF_VOC_AEC_NS_PARA, param);
            break;
    }

    return ret;
#endif
    return -1;
}

void doorbell_audio_data_callback(uint8_t *data, uint32_t length)
{
    bk_err_t ret = BK_OK;

    if (db_device_info->audio_enable)
    {
        ret = bk_voice_write_frame_data(db_device_info->voice_write_handle, (char *)data, length);
        if (ret != length)
        {
            LOGV("write speaker data fail, need_write: %d, ret: %d\n", length, ret);
        }
    }
}
#endif

int doorbell_devices_init(void)
{
    if (db_device_info == NULL)
    {
        db_device_info = os_malloc(sizeof(db_device_info_t));
    }

    if (db_device_info == NULL)
    {
        LOGE("malloc db_device_info failed");
        return  BK_FAIL;
    }

    os_memset(db_device_info, 0, sizeof(db_device_info_t));

    db_device_info->camera_id = CAMERA_MAX_NUM;

    return BK_OK;
}

void doorbell_devices_deinit(void)
{
    if (db_device_info)
    {
        os_free(db_device_info);
        db_device_info = NULL;
    }
}
