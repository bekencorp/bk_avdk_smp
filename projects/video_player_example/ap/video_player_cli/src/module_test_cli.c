#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_display.h>
#include <components/bk_camera_ctlr.h>
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>
#include <os/str.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/pwr_clk.h>
#include <bk_vfs.h>
#include <bk_posix.h>
#include <bk_partition.h>
#include <sys/stat.h>
#include "modules/avilib.h"
#include "modules/mp4lib.h"
#include "cli.h"
#include "module_test_cli.h"
#include "video_player_common.h"
#include "frame_buffer.h"
#include "lcd_panel_devices.h"

// Export voice handles for video_record_cli.c to check for conflicts
// These handles are declared without static keyword so they can be accessed externally

#define TAG "module_test_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static const lcd_device_t *lcd_device = &lcd_device_st7282;

// Static handles for LCD display, DVP camera, and voice service
static bk_display_ctlr_handle_t lcd_display_handle = NULL;
static bk_camera_ctlr_handle_t dvp_handle = NULL;
voice_handle_t voice_service_handle = NULL;  // Exported for video_record_cli.c
static voice_read_handle_t voice_read_service_handle = NULL;
static voice_write_handle_t voice_write_service_handle = NULL;
// Static handles for voice loopback (mic to speaker)
voice_handle_t voice_loopback_handle = NULL;  // Exported for video_record_cli.c
static voice_read_handle_t voice_loopback_read_handle = NULL;
static voice_write_handle_t voice_loopback_write_handle = NULL;
// Static handles for DVP display mode (DVP to LCD with mic)
static bk_display_ctlr_handle_t dvp_display_lcd_handle = NULL;
static bk_camera_ctlr_handle_t dvp_display_handle = NULL;
voice_handle_t dvp_display_voice_handle = NULL;  // Exported for video_record_cli.c
static voice_read_handle_t dvp_display_voice_read_handle = NULL;
static voice_write_handle_t dvp_display_voice_write_handle = NULL;

// Recursively scan directory and print files (used by sd_card_cmd)
static void scan_directory_recursive(const char *directory, int depth, int *file_count, int *dir_count)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char full_path[512];

    if (directory == NULL)
    {
        return;
    }

    // Open directory
    dir = opendir(directory);
    if (dir == NULL)
    {
        LOGE("%s: Failed to open directory: %s\n", __func__, directory);
        return;
    }

    // Print directory name with indentation
    for (int i = 0; i < depth; i++)
    {
        LOGI("  ");
    }
    LOGI("DIR: %s\n", directory);

    // Read all entries in directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip "." and ".."
        if (os_strcmp(entry->d_name, ".") == 0 || os_strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Build full path for entry
        os_snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        // Check if it's a directory - recursively scan it
        if (entry->d_type == DT_DIR)
        {
            (*dir_count)++;
            scan_directory_recursive(full_path, depth + 1, file_count, dir_count);
        }
        else
        {
            // It's a file
            (*file_count)++;
            for (int i = 0; i < depth + 1; i++)
            {
                LOGI("  ");
            }
            
            // Get file size
            struct stat statbuf;
            int ret = stat(full_path, &statbuf);
            if (ret == 0)
            {
                // Format file size (bytes, KB, MB)
                uint32_t file_size = (uint32_t)statbuf.st_size;
                if (file_size < 1024)
                {
                    LOGI("FILE: %s (%u bytes)\n", entry->d_name, file_size);
                }
                else if (file_size < 1024 * 1024)
                {
                    LOGI("FILE: %s (%u KB)\n", entry->d_name, file_size / 1024);
                }
                else
                {
                    LOGI("FILE: %s (%u MB)\n", entry->d_name, file_size / (1024 * 1024));
                }
            }
            else
            {
                // Failed to get file size, print without size
                LOGI("FILE: %s\n", entry->d_name);
            }
        }
    }

    // Close directory
    closedir(dir);
}

// DVP frame malloc callback
static frame_buffer_t *dvp_frame_malloc(image_format_t format, uint32_t size)
{
    frame_buffer_t *frame = NULL;
    
    // Allocate frame buffer based on format
    if (format == IMAGE_MJPEG || format == IMAGE_H264 || format == IMAGE_H265)
    {
        frame = frame_buffer_encode_malloc(size);
    }
    else if (format == IMAGE_YUV)
    {
        frame = frame_buffer_display_malloc(size);
    }
    else
    {
        LOGE("%s: unsupported format: %d\n", __func__, format);
        return NULL;
    }
    
    if (frame)
    {
        frame->sequence = 0;
        frame->length = 0;
        frame->timestamp = 0;
        frame->size = size;
    }
    
    return frame;
}

// DVP frame complete callback
static void dvp_frame_complete(image_format_t format, frame_buffer_t *frame, int result)
{
    if (frame == NULL)
    {
        return;
    }
    
    // Log frame info periodically
    if (frame->sequence % 30 == 0)
    {
        LOGD("%s: seq:%d, length:%d, format:%d, result:%d\n", __func__, 
            frame->sequence, frame->length, format, result);
    }
    
    // Free frame buffer based on format
    if (format == IMAGE_MJPEG || format == IMAGE_H264 || format == IMAGE_H265)
    {
        frame_buffer_encode_free(frame);
    }
    else if (format == IMAGE_YUV)
    {
        frame_buffer_display_free(frame);
    }
}

// DVP frame malloc callback for display mode
static frame_buffer_t *dvp_display_frame_malloc(image_format_t format, uint32_t size)
{
    frame_buffer_t *frame = NULL;
    
    // For display mode, we need YUV format
    if (format == IMAGE_YUV)
    {
        frame = frame_buffer_display_malloc(size);
    }
    else
    {
        LOGE("%s: display mode only supports YUV format, got: %d\n", __func__, format);
        return NULL;
    }
    
    if (frame)
    {
        frame->sequence = 0;
        frame->length = 0;
        frame->timestamp = 0;
        frame->size = size;
    }
    
    return frame;
}

// DVP frame complete callback for display mode - send YUV frames to LCD
static void dvp_display_frame_complete(image_format_t format, frame_buffer_t *frame, int result)
{
    if (frame == NULL)
    {
        return;
    }
    
    // Only process YUV frames for display
    if (format == IMAGE_YUV && result == AVDK_ERR_OK)
    {
        // Log frame info periodically
        if (frame->sequence % 30 == 0)
        {
            LOGD("%s: display frame seq:%d, width:%d, height:%d\n", __func__, 
                frame->sequence, frame->width, frame->height);
        }
        
        // Display frame to LCD if LCD is opened
        if (dvp_display_lcd_handle != NULL)
        {
            // Set pixel format for display
            frame->fmt = PIXEL_FMT_YUYV;
            
            // Flush frame to LCD display
            avdk_err_t ret = bk_display_flush(dvp_display_lcd_handle, frame, display_frame_free_cb);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_display_flush failed, ret:%d\n", __func__, ret);
                // Free frame on error
                frame_buffer_display_free(frame);
            }
            // If successful, frame will be freed by display_frame_free_cb callback
        }
        else
        {
            // LCD not opened, free frame directly
            frame_buffer_display_free(frame);
        }
    }
    else
    {
        // Free frame on error or unsupported format
        if (format == IMAGE_YUV)
        {
            frame_buffer_display_free(frame);
        }
    }
}

static const bk_dvp_callback_t dvp_display_cbs = {
    .malloc = dvp_display_frame_malloc,
    .complete = dvp_display_frame_complete,
};

static const bk_dvp_callback_t dvp_cbs = {
    .malloc = dvp_frame_malloc,
    .complete = dvp_frame_complete,
};
// Voice service send callback
static int voice_service_send_callback(unsigned char *data, unsigned int len, void *args)
{
    int ret = 0;
    if (voice_write_service_handle)
    {
        ret = bk_voice_write_frame_data(voice_write_service_handle, (char *)data, len);
        if (ret != len)
        {
            LOGV("%s: bk_voice_write_frame_data: %d != %d\n", __func__, ret, len);
        }
    }
    return ret;
}

// Voice loopback callback - send mic data directly to speaker
static int voice_loopback_callback(unsigned char *data, unsigned int len, void *args)
{
    int ret = 0;
    if (voice_loopback_write_handle)
    {
        ret = bk_voice_write_frame_data(voice_loopback_write_handle, (char *)data, len);
        if (ret != len)
        {
            LOGV("%s: bk_voice_write_frame_data: %d != %d\n", __func__, ret, len);
        }
    }
    return ret;
}
void cli_lcd_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    char *msg = NULL;
    
    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        msg = CLI_CMD_RSP_ERROR;
        goto exit;
    }
    
    if (os_strcmp(argv[1], "open") == 0)
    {
        // Check if LCD display is already opened
        if (lcd_display_handle != NULL)
        {
            LOGE("%s: lcd_display_handle already opened\n", __func__);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        // Power on LCD LDO
        ret = bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_pm_module_vote_ctrl_external_ldo failed, ret:%d\n", __func__, ret);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        // Configure LCD display
        bk_display_rgb_ctlr_config_t lcd_display_config = {0};
        lcd_display_config.lcd_device = lcd_device;
        lcd_display_config.clk_pin = GPIO_0;
        lcd_display_config.cs_pin = GPIO_12;
        lcd_display_config.sda_pin = GPIO_1;
        lcd_display_config.rst_pin = GPIO_6;
        
        // Create LCD display controller
        ret = bk_display_rgb_new(&lcd_display_handle, &lcd_display_config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_display_rgb_new failed, ret:%d\n", __func__, ret);
            bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        LOGD("%s: bk_display_rgb_new success\n", __func__);
        
        // Open backlight
        ret = lcd_backlight_open(GPIO_7);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: lcd_backlight_open failed, ret:%d\n", __func__, ret);
            bk_display_delete(lcd_display_handle);
            lcd_display_handle = NULL;
            bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        // Open LCD display
        ret = bk_display_open(lcd_display_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_display_open failed, ret:%d\n", __func__, ret);
            lcd_backlight_close(GPIO_7);
            bk_display_delete(lcd_display_handle);
            lcd_display_handle = NULL;
            bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        LOGD("%s: LCD display opened successfully\n", __func__);
        ret = AVDK_ERR_OK;
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        if (lcd_display_handle == NULL)
        {
            LOGE("%s: lcd_display_handle is NULL\n", __func__);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        // Close LCD display
        ret = bk_display_close(lcd_display_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_display_close failed, ret:%d\n", __func__, ret);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        // Close backlight
        ret = lcd_backlight_close(GPIO_7);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: lcd_backlight_close failed, ret:%d\n", __func__, ret);
        }
        
        // Delete LCD display controller
        ret = bk_display_delete(lcd_display_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_display_delete failed, ret:%d\n", __func__, ret);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        lcd_display_handle = NULL;
        
        // Power off LCD LDO
        ret = bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_pm_module_vote_ctrl_external_ldo failed, ret:%d\n", __func__, ret);
        }
        
        LOGD("%s: LCD display closed successfully\n", __func__);
        ret = AVDK_ERR_OK;
    }
    else
    {
        LOGE("%s: unknown command: %s\n", __func__, argv[1]);
        msg = CLI_CMD_RSP_ERROR;
        goto exit;
    }
    
exit:
    if (ret != AVDK_ERR_OK && msg == NULL)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else if (ret == AVDK_ERR_OK && msg == NULL)
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }
    
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

// DVP command handler
void cli_dvp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNKNOWN;
    char *msg = NULL;
    uint16_t output_format = IMAGE_MJPEG;
    
    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        msg = CLI_CMD_RSP_ERROR;
        goto exit;
    }
    
    // Parse image format from command arguments
    if (cmd_contain(argc, argv, "jpeg") || cmd_contain(argc, argv, "MJPEG"))
    {
        output_format = IMAGE_MJPEG;
    }
    else if (cmd_contain(argc, argv, "h264") || cmd_contain(argc, argv, "H264"))
    {
        output_format = IMAGE_H264;
    }
    else if (cmd_contain(argc, argv, "h265") || cmd_contain(argc, argv, "H265"))
    {
        output_format = IMAGE_H265;
    }
    else if (cmd_contain(argc, argv, "yuv") || cmd_contain(argc, argv, "YUV"))
    {
        output_format = IMAGE_YUV;
    }
    else if (cmd_contain(argc, argv, "enc_yuv"))
    {
        output_format |= IMAGE_YUV;
    }
    
    if (os_strcmp(argv[1], "open") == 0)
    {
        // Check if DVP is already opened
        if (dvp_handle != NULL)
        {
            LOGE("%s: dvp handle already opened\n", __func__);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        if (argc < 4)
        {
            LOGE("%s: insufficient arguments for dvp open, need width and height\n", __func__);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        // Power on DVP camera
        if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
        {
            GPIO_UP(DVP_POWER_GPIO_ID);
        }
        
        // Configure DVP controller
        bk_dvp_ctlr_config_t dvp_ctrl_config = {
            .config = BK_DVP_864X480_30FPS_MJPEG_CONFIG(),
            .cbs = &dvp_cbs,
        };
        
        dvp_ctrl_config.config.img_format = output_format;
        dvp_ctrl_config.config.width = os_strtoul(argv[2], NULL, 10);
        dvp_ctrl_config.config.height = os_strtoul(argv[3], NULL, 10);
        
        // Create DVP controller
        ret = bk_camera_dvp_ctlr_new(&dvp_handle, &dvp_ctrl_config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_camera_dvp_ctlr_new failed, ret:%d\n", __func__, ret);
            if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
            {
                GPIO_DOWN(DVP_POWER_GPIO_ID);
            }
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        // Open DVP camera
        ret = bk_camera_open(dvp_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_camera_open failed, ret:%d\n", __func__, ret);
            bk_camera_delete(dvp_handle);
            dvp_handle = NULL;
            if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
            {
                GPIO_DOWN(DVP_POWER_GPIO_ID);
            }
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        LOGD("%s: DVP camera opened successfully\n", __func__);
        ret = AVDK_ERR_OK;
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        if (dvp_handle == NULL)
        {
            LOGE("%s: dvp handle is NULL\n", __func__);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        // Close DVP camera
        ret = bk_camera_close(dvp_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_camera_close failed, ret:%d\n", __func__, ret);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        // Delete DVP controller
        ret = bk_camera_delete(dvp_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_camera_delete failed, ret:%d\n", __func__, ret);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        
        dvp_handle = NULL;
        
        // Power off DVP camera
        if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
        {
            GPIO_DOWN(DVP_POWER_GPIO_ID);
        }
        
        LOGD("%s: DVP camera closed successfully\n", __func__);
        ret = AVDK_ERR_OK;
    }
    else
    {
        LOGE("%s: unknown command: %s\n", __func__, argv[1]);
        msg = CLI_CMD_RSP_ERROR;
        goto exit;
    }
    
exit:
    if (ret != AVDK_ERR_OK && msg == NULL)
    {
        msg = CLI_CMD_RSP_ERROR;
        // Power off DVP camera on error
        if (dvp_handle != NULL && DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
        {
            GPIO_DOWN(DVP_POWER_GPIO_ID);
        }
    }
    else if (ret == AVDK_ERR_OK && msg == NULL)
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }
    
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

// Voice service command handler
void cli_voice_service_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    avdk_err_t ret = AVDK_ERR_GENERIC;
    
    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        goto exit;
    }
    
    if (os_strcmp(argv[1], "start") == 0)
    {
        // Check if voice service is already started
        if (voice_service_handle != NULL)
        {
            LOGE("%s: voice service already started\n", __func__);
            goto exit;
        }
        
        // Check arguments for the specific command: voice_service start onboard 8000 1 g711a g711a onboard 8000 0
        if (argc != 10)
        {
            LOGE("%s: invalid arguments count: %d, expected 10\n", __func__, argc);
            goto exit;
        }
        
        // Parse and validate arguments
        if (os_strcmp(argv[2], "onboard") != 0 ||
            os_strtoul(argv[3], NULL, 10) != 8000 ||
            os_strtoul(argv[4], NULL, 10) != 1 ||
            os_strcmp(argv[5], "g711a") != 0 ||
            os_strcmp(argv[6], "g711a") != 0 ||
            os_strcmp(argv[7], "onboard") != 0 ||
            os_strtoul(argv[8], NULL, 10) != 8000 ||
            os_strtoul(argv[9], NULL, 10) != 0)
        {
            LOGE("%s: invalid arguments format\n", __func__);
            goto exit;
        }
        
        // Configure voice service
        voice_cfg_t voice_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_CONFIG();
        voice_cfg.spk_cfg.onboard_spk_cfg.multi_in_port_num = 2;
        
        // Initialize voice service
        voice_service_handle = bk_voice_init(&voice_cfg);
        if (voice_service_handle == NULL)
        {
            LOGE("%s: bk_voice_init failed\n", __func__);
            goto exit;
        }
        
        // Initialize voice read service
        voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
        voice_read_cfg.voice_handle = voice_service_handle;
        voice_read_cfg.voice_read_callback = voice_service_send_callback;
        voice_read_service_handle = bk_voice_read_init(&voice_read_cfg);
        if (voice_read_service_handle == NULL)
        {
            LOGE("%s: bk_voice_read_init failed\n", __func__);
            goto exit;
        }
        
        // Initialize voice write service
        voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
        voice_write_cfg.voice_handle = voice_service_handle;
        voice_write_service_handle = bk_voice_write_init(&voice_write_cfg);
        if (voice_write_service_handle == NULL)
        {
            LOGE("%s: bk_voice_write_init failed\n", __func__);
            goto exit;
        }
        
        // Start voice service
        ret = bk_voice_start(voice_service_handle);
        if (ret != BK_OK)
        {
            LOGE("%s: bk_voice_start failed, ret:%d\n", __func__, ret);
            goto exit;
        }
        
        // Start voice read service
        ret = bk_voice_read_start(voice_read_service_handle);
        if (ret != BK_OK)
        {
            LOGE("%s: bk_voice_read_start failed, ret:%d\n", __func__, ret);
            goto exit;
        }
        
        // Start voice write service
        ret = bk_voice_write_start(voice_write_service_handle);
        if (ret != BK_OK)
        {
            LOGE("%s: bk_voice_write_start failed, ret:%d\n", __func__, ret);
            goto exit;
        }
        
        LOGD("%s: Voice service started successfully\n", __func__);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        // Stop voice read service
        if (voice_read_service_handle != NULL)
        {
            bk_voice_read_stop(voice_read_service_handle);
        }
        
        // Stop voice write service
        if (voice_write_service_handle != NULL)
        {
            bk_voice_write_stop(voice_write_service_handle);
        }
        
        // Stop voice service
        if (voice_service_handle != NULL)
        {
            bk_voice_stop(voice_service_handle);
        }
        
        // Deinitialize voice read service
        if (voice_read_service_handle != NULL)
        {
            bk_voice_read_deinit(voice_read_service_handle);
            voice_read_service_handle = NULL;
        }
        
        // Deinitialize voice write service
        if (voice_write_service_handle != NULL)
        {
            bk_voice_write_deinit(voice_write_service_handle);
            voice_write_service_handle = NULL;
        }
        
        // Deinitialize voice service
        if (voice_service_handle != NULL)
        {
            bk_voice_deinit(voice_service_handle);
            voice_service_handle = NULL;
        }
        
        LOGD("%s: Voice service stopped successfully\n", __func__);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else
    {
        LOGE("%s: unknown command: %s\n", __func__, argv[1]);
        goto exit;
    }
    
exit:
    if (msg == CLI_CMD_RSP_ERROR)
    {
        // Clean up on error
        if (voice_read_service_handle != NULL)
        {
            bk_voice_read_stop(voice_read_service_handle);
            bk_voice_read_deinit(voice_read_service_handle);
            voice_read_service_handle = NULL;
        }
        
        if (voice_write_service_handle != NULL)
        {
            bk_voice_write_stop(voice_write_service_handle);
            bk_voice_write_deinit(voice_write_service_handle);
            voice_write_service_handle = NULL;
        }
        
        if (voice_service_handle != NULL)
        {
            bk_voice_stop(voice_service_handle);
            bk_voice_deinit(voice_service_handle);
            voice_service_handle = NULL;
        }
    }
    
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

// Voice loopback command handler - mic to speaker
void cli_voice_loopback_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    avdk_err_t ret = AVDK_ERR_GENERIC;
    
    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        goto exit;
    }
    
    if (os_strcmp(argv[1], "start") == 0)
    {
        // Check if voice loopback is already started
        if (voice_loopback_handle != NULL)
        {
            LOGE("%s: voice loopback already started\n", __func__);
            goto exit;
        }
        
        // Configure voice service for loopback (mic to speaker)
        voice_cfg_t voice_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_CONFIG();
        voice_cfg.spk_cfg.onboard_spk_cfg.multi_in_port_num = 2;
        
        // Initialize voice service for loopback
        voice_loopback_handle = bk_voice_init(&voice_cfg);
        if (voice_loopback_handle == NULL)
        {
            LOGE("%s: bk_voice_init failed\n", __func__);
            goto exit;
        }
        
        // Initialize voice read service (from mic)
        voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
        voice_read_cfg.voice_handle = voice_loopback_handle;
        voice_read_cfg.voice_read_callback = voice_loopback_callback;
        voice_loopback_read_handle = bk_voice_read_init(&voice_read_cfg);
        if (voice_loopback_read_handle == NULL)
        {
            LOGE("%s: bk_voice_read_init failed\n", __func__);
            goto exit;
        }
        
        // Initialize voice write service (to speaker)
        voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
        voice_write_cfg.voice_handle = voice_loopback_handle;
        voice_loopback_write_handle = bk_voice_write_init(&voice_write_cfg);
        if (voice_loopback_write_handle == NULL)
        {
            LOGE("%s: bk_voice_write_init failed\n", __func__);
            goto exit;
        }
        
        // Start voice service
        ret = bk_voice_start(voice_loopback_handle);
        if (ret != BK_OK)
        {
            LOGE("%s: bk_voice_start failed, ret:%d\n", __func__, ret);
            goto exit;
        }
        
        // Start voice read service (start capturing from mic)
        ret = bk_voice_read_start(voice_loopback_read_handle);
        if (ret != BK_OK)
        {
            LOGE("%s: bk_voice_read_start failed, ret:%d\n", __func__, ret);
            goto exit;
        }
        
        // Start voice write service (start playing to speaker)
        ret = bk_voice_write_start(voice_loopback_write_handle);
        if (ret != BK_OK)
        {
            LOGE("%s: bk_voice_write_start failed, ret:%d\n", __func__, ret);
            goto exit;
        }
        
        LOGD("%s: Voice loopback started successfully (mic -> speaker)\n", __func__);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        // Stop voice read service
        if (voice_loopback_read_handle != NULL)
        {
            bk_voice_read_stop(voice_loopback_read_handle);
        }
        
        // Stop voice write service
        if (voice_loopback_write_handle != NULL)
        {
            bk_voice_write_stop(voice_loopback_write_handle);
        }
        
        // Stop voice service
        if (voice_loopback_handle != NULL)
        {
            bk_voice_stop(voice_loopback_handle);
        }
        
        // Deinitialize voice read service
        if (voice_loopback_read_handle != NULL)
        {
            bk_voice_read_deinit(voice_loopback_read_handle);
            voice_loopback_read_handle = NULL;
        }
        
        // Deinitialize voice write service
        if (voice_loopback_write_handle != NULL)
        {
            bk_voice_write_deinit(voice_loopback_write_handle);
            voice_loopback_write_handle = NULL;
        }
        
        // Deinitialize voice service
        if (voice_loopback_handle != NULL)
        {
            bk_voice_deinit(voice_loopback_handle);
            voice_loopback_handle = NULL;
        }
        
        LOGD("%s: Voice loopback stopped successfully\n", __func__);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else
    {
        LOGE("%s: unknown command: %s\n", __func__, argv[1]);
        goto exit;
    }
    
exit:
    if (msg == CLI_CMD_RSP_ERROR)
    {
        // Clean up on error
        if (voice_loopback_read_handle != NULL)
        {
            bk_voice_read_stop(voice_loopback_read_handle);
            bk_voice_read_deinit(voice_loopback_read_handle);
            voice_loopback_read_handle = NULL;
        }
        
        if (voice_loopback_write_handle != NULL)
        {
            bk_voice_write_stop(voice_loopback_write_handle);
            bk_voice_write_deinit(voice_loopback_write_handle);
            voice_loopback_write_handle = NULL;
        }
        
        if (voice_loopback_handle != NULL)
        {
            bk_voice_stop(voice_loopback_handle);
            bk_voice_deinit(voice_loopback_handle);
            voice_loopback_handle = NULL;
        }
    }
    
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

// Voice callback for DVP display mode - send mic data to speaker
static int dvp_display_voice_callback(unsigned char *data, unsigned int len, void *args)
{
    int ret = 0;
    if (dvp_display_voice_write_handle)
    {
        ret = bk_voice_write_frame_data(dvp_display_voice_write_handle, (char *)data, len);
        if (ret != len)
        {
            LOGV("%s: bk_voice_write_frame_data: %d != %d\n", __func__, ret, len);
        }
    }
    return ret;
}

// DVP display command handler - display DVP data on LCD and open mic
void cli_dvp_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    avdk_err_t ret = AVDK_ERR_GENERIC;
    
    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        goto exit;
    }
    
    if (os_strcmp(argv[1], "start") == 0)
    {
        // Check if already started
        if (dvp_display_lcd_handle != NULL || dvp_display_handle != NULL)
        {
            LOGE("%s: DVP display already started\n", __func__);
            goto exit;
        }
        
        // Parse DVP parameters (width, height)
        uint32_t width = 480;
        uint32_t height = 320;
        if (argc >= 4)
        {
            width = os_strtoul(argv[2], NULL, 10);
            height = os_strtoul(argv[3], NULL, 10);
        }
        
        // Step 1: Open LCD display
        if (dvp_display_lcd_handle == NULL)
        {
            // Power on LCD LDO
            ret = bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_pm_module_vote_ctrl_external_ldo failed, ret:%d\n", __func__, ret);
                goto exit;
            }
            
            // Configure LCD display
            bk_display_rgb_ctlr_config_t lcd_display_config = {0};
            lcd_display_config.lcd_device = lcd_device;
            lcd_display_config.clk_pin = GPIO_0;
            lcd_display_config.cs_pin = GPIO_12;
            lcd_display_config.sda_pin = GPIO_1;
            lcd_display_config.rst_pin = GPIO_6;
            
            // Create LCD display controller
            ret = bk_display_rgb_new(&dvp_display_lcd_handle, &lcd_display_config);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_display_rgb_new failed, ret:%d\n", __func__, ret);
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                goto exit;
            }
            LOGD("%s: LCD display created successfully\n", __func__);
            
            // Open backlight
            ret = lcd_backlight_open(GPIO_7);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: lcd_backlight_open failed, ret:%d\n", __func__, ret);
                bk_display_delete(dvp_display_lcd_handle);
                dvp_display_lcd_handle = NULL;
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                goto exit;
            }
            
            // Open LCD display
            ret = bk_display_open(dvp_display_lcd_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_display_open failed, ret:%d\n", __func__, ret);
                lcd_backlight_close(GPIO_7);
                bk_display_delete(dvp_display_lcd_handle);
                dvp_display_lcd_handle = NULL;
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                goto exit;
            }
            LOGD("%s: LCD display opened successfully\n", __func__);
        }
        
        // Step 2: Open DVP camera with YUV output
        if (dvp_display_handle == NULL)
        {
            // Power on DVP camera
            if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
            {
                GPIO_UP(DVP_POWER_GPIO_ID);
            }
            
            // Configure DVP controller for YUV output
            bk_dvp_ctlr_config_t dvp_ctrl_config = {
                .config = BK_DVP_864X480_30FPS_MJPEG_CONFIG(),
                .cbs = &dvp_display_cbs,
            };
            
            dvp_ctrl_config.config.img_format = IMAGE_YUV;
            dvp_ctrl_config.config.width = width;
            dvp_ctrl_config.config.height = height;
            
            // Create DVP controller
            ret = bk_camera_dvp_ctlr_new(&dvp_display_handle, &dvp_ctrl_config);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_camera_dvp_ctlr_new failed, ret:%d\n", __func__, ret);
                if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
                {
                    GPIO_DOWN(DVP_POWER_GPIO_ID);
                }
                // Close LCD on error
                if (dvp_display_lcd_handle != NULL)
                {
                    bk_display_close(dvp_display_lcd_handle);
                    lcd_backlight_close(GPIO_7);
                    bk_display_delete(dvp_display_lcd_handle);
                    dvp_display_lcd_handle = NULL;
                    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                }
                goto exit;
            }
            
            // Open DVP camera
            ret = bk_camera_open(dvp_display_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_camera_open failed, ret:%d\n", __func__, ret);
                bk_camera_delete(dvp_display_handle);
                dvp_display_handle = NULL;
                if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
                {
                    GPIO_DOWN(DVP_POWER_GPIO_ID);
                }
                // Close LCD on error
                if (dvp_display_lcd_handle != NULL)
                {
                    bk_display_close(dvp_display_lcd_handle);
                    lcd_backlight_close(GPIO_7);
                    bk_display_delete(dvp_display_lcd_handle);
                    dvp_display_lcd_handle = NULL;
                    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                }
                goto exit;
            }
            LOGD("%s: DVP camera opened successfully\n", __func__);
        }
        
        // Step 3: Open mic (voice loopback)
        if (dvp_display_voice_handle == NULL)
        {
            // Configure voice service for loopback
            voice_cfg_t voice_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_CONFIG();
            voice_cfg.spk_cfg.onboard_spk_cfg.multi_in_port_num = 2;
            
            // Initialize voice service
            dvp_display_voice_handle = bk_voice_init(&voice_cfg);
            if (dvp_display_voice_handle == NULL)
            {
                LOGE("%s: bk_voice_init failed\n", __func__);
                // Don't fail the whole command if mic fails, just log error
                LOGW("%s: Failed to initialize mic, continuing without mic\n", __func__);
            }
            else
            {
                // Initialize voice read service (from mic)
                voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
                voice_read_cfg.voice_handle = dvp_display_voice_handle;
                voice_read_cfg.voice_read_callback = dvp_display_voice_callback;
                dvp_display_voice_read_handle = bk_voice_read_init(&voice_read_cfg);
                if (dvp_display_voice_read_handle == NULL)
                {
                    LOGE("%s: bk_voice_read_init failed\n", __func__);
                    bk_voice_deinit(dvp_display_voice_handle);
                    dvp_display_voice_handle = NULL;
                    LOGW("%s: Failed to initialize mic read, continuing without mic\n", __func__);
                }
                else
                {
                    // Initialize voice write service (to speaker)
                    voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
                    voice_write_cfg.voice_handle = dvp_display_voice_handle;
                    dvp_display_voice_write_handle = bk_voice_write_init(&voice_write_cfg);
                    if (dvp_display_voice_write_handle == NULL)
                    {
                        LOGE("%s: bk_voice_write_init failed\n", __func__);
                        bk_voice_read_deinit(dvp_display_voice_read_handle);
                        dvp_display_voice_read_handle = NULL;
                        bk_voice_deinit(dvp_display_voice_handle);
                        dvp_display_voice_handle = NULL;
                        LOGW("%s: Failed to initialize mic write, continuing without mic\n", __func__);
                    }
                    else
                    {
                        // Start voice service
                        ret = bk_voice_start(dvp_display_voice_handle);
                        if (ret != BK_OK)
                        {
                            LOGE("%s: bk_voice_start failed, ret:%d\n", __func__, ret);
                            bk_voice_write_deinit(dvp_display_voice_write_handle);
                            dvp_display_voice_write_handle = NULL;
                            bk_voice_read_deinit(dvp_display_voice_read_handle);
                            dvp_display_voice_read_handle = NULL;
                            bk_voice_deinit(dvp_display_voice_handle);
                            dvp_display_voice_handle = NULL;
                            LOGW("%s: Failed to start mic, continuing without mic\n", __func__);
                        }
                        else
                        {
                            // Start voice read service
                            ret = bk_voice_read_start(dvp_display_voice_read_handle);
                            if (ret != BK_OK)
                            {
                                LOGE("%s: bk_voice_read_start failed, ret:%d\n", __func__, ret);
                                bk_voice_stop(dvp_display_voice_handle);
                                bk_voice_write_deinit(dvp_display_voice_write_handle);
                                dvp_display_voice_write_handle = NULL;
                                bk_voice_read_deinit(dvp_display_voice_read_handle);
                                dvp_display_voice_read_handle = NULL;
                                bk_voice_deinit(dvp_display_voice_handle);
                                dvp_display_voice_handle = NULL;
                                LOGW("%s: Failed to start mic read, continuing without mic\n", __func__);
                            }
                            else
                            {
                                // Start voice write service
                                ret = bk_voice_write_start(dvp_display_voice_write_handle);
                                if (ret != BK_OK)
                                {
                                    LOGE("%s: bk_voice_write_start failed, ret:%d\n", __func__, ret);
                                    bk_voice_read_stop(dvp_display_voice_read_handle);
                                    bk_voice_stop(dvp_display_voice_handle);
                                    bk_voice_write_deinit(dvp_display_voice_write_handle);
                                    dvp_display_voice_write_handle = NULL;
                                    bk_voice_read_deinit(dvp_display_voice_read_handle);
                                    dvp_display_voice_read_handle = NULL;
                                    bk_voice_deinit(dvp_display_voice_handle);
                                    dvp_display_voice_handle = NULL;
                                    LOGW("%s: Failed to start mic write, continuing without mic\n", __func__);
                                }
                                else
                                {
                                    LOGD("%s: Mic opened successfully\n", __func__);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        LOGD("%s: DVP display mode started successfully (DVP -> LCD, mic enabled)\n", __func__);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        // Stop and close DVP camera
        if (dvp_display_handle != NULL)
        {
            ret = bk_camera_close(dvp_display_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_camera_close failed, ret:%d\n", __func__, ret);
            }
            
            ret = bk_camera_delete(dvp_display_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_camera_delete failed, ret:%d\n", __func__, ret);
            }
            
            dvp_display_handle = NULL;
            
            // Power off DVP camera
            if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
            {
                GPIO_DOWN(DVP_POWER_GPIO_ID);
            }
        }
        
        // Stop and close LCD display
        if (dvp_display_lcd_handle != NULL)
        {
            ret = bk_display_close(dvp_display_lcd_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_display_close failed, ret:%d\n", __func__, ret);
            }
            
            ret = lcd_backlight_close(GPIO_7);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: lcd_backlight_close failed, ret:%d\n", __func__, ret);
            }
            
            ret = bk_display_delete(dvp_display_lcd_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_display_delete failed, ret:%d\n", __func__, ret);
            }
            
            dvp_display_lcd_handle = NULL;
            
            // Power off LCD LDO
            ret = bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_pm_module_vote_ctrl_external_ldo failed, ret:%d\n", __func__, ret);
            }
        }
        
        // Stop and close mic
        if (dvp_display_voice_read_handle != NULL)
        {
            bk_voice_read_stop(dvp_display_voice_read_handle);
            bk_voice_read_deinit(dvp_display_voice_read_handle);
            dvp_display_voice_read_handle = NULL;
        }
        
        if (dvp_display_voice_write_handle != NULL)
        {
            bk_voice_write_stop(dvp_display_voice_write_handle);
            bk_voice_write_deinit(dvp_display_voice_write_handle);
            dvp_display_voice_write_handle = NULL;
        }
        
        if (dvp_display_voice_handle != NULL)
        {
            bk_voice_stop(dvp_display_voice_handle);
            bk_voice_deinit(dvp_display_voice_handle);
            dvp_display_voice_handle = NULL;
        }
        
        LOGD("%s: DVP display mode stopped successfully\n", __func__);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else
    {
        LOGE("%s: unknown command: %s\n", __func__, argv[1]);
        goto exit;
    }
    
exit:
    if (msg == CLI_CMD_RSP_ERROR)
    {
        // Clean up on error
        if (dvp_display_handle != NULL)
        {
            bk_camera_close(dvp_display_handle);
            bk_camera_delete(dvp_display_handle);
            dvp_display_handle = NULL;
            if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
            {
                GPIO_DOWN(DVP_POWER_GPIO_ID);
            }
        }
        
        if (dvp_display_lcd_handle != NULL)
        {
            bk_display_close(dvp_display_lcd_handle);
            lcd_backlight_close(GPIO_7);
            bk_display_delete(dvp_display_lcd_handle);
            dvp_display_lcd_handle = NULL;
            bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
        }
        
        if (dvp_display_voice_read_handle != NULL)
        {
            bk_voice_read_stop(dvp_display_voice_read_handle);
            bk_voice_read_deinit(dvp_display_voice_read_handle);
            dvp_display_voice_read_handle = NULL;
        }
        
        if (dvp_display_voice_write_handle != NULL)
        {
            bk_voice_write_stop(dvp_display_voice_write_handle);
            bk_voice_write_deinit(dvp_display_voice_write_handle);
            dvp_display_voice_write_handle = NULL;
        }
        
        if (dvp_display_voice_handle != NULL)
        {
            bk_voice_stop(dvp_display_voice_handle);
            bk_voice_deinit(dvp_display_voice_handle);
            dvp_display_voice_handle = NULL;
        }
    }
    
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

// SD card command handler
void cli_sd_card_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    int ret = BK_FAIL;
    int file_count = 0;
    int dir_count = 0;
    const char *scan_path = "/sd0";
    
    LOGD("%s: command received, argc=%d\n", __func__, argc);
    
    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        goto exit;
    }
    
    LOGD("%s: processing command: %s\n", __func__, argv[1]);
    
    if (os_strcmp(argv[1], "mount") == 0)
    {
        // Mount SD card only
        ret = sd_card_mount();
        if (ret == BK_OK)
        {
            LOGI("%s: SD card mounted successfully\n", __func__);
            msg = CLI_CMD_RSP_SUCCEED;
        }
        else
        {
            LOGE("%s: Failed to mount SD card\n", __func__);
            goto exit;
        }
    }
    else if (os_strcmp(argv[1], "unmount") == 0)
    {
        // Unmount SD card
        ret = sd_card_unmount();
        if (ret == BK_OK)
        {
            LOGI("%s: SD card unmounted successfully\n", __func__);
            msg = CLI_CMD_RSP_SUCCEED;
        }
        else
        {
            LOGE("%s: Failed to unmount SD card\n", __func__);
            goto exit;
        }
    }
    else if (os_strcmp(argv[1], "scan") == 0)
    {
        // Check if SD card is mounted
        if (!sd_card_is_mounted())
        {
            LOGE("%s: SD card not mounted, please mount first\n", __func__);
            goto exit;
        }
        
        // Get scan path if provided
        if (argc >= 3)
        {
            scan_path = argv[2];
        }
        
        LOGI("%s: Scanning SD card directory: %s\n", __func__, scan_path);
        file_count = 0;
        dir_count = 0;
        
        // Scan directory recursively
        scan_directory_recursive(scan_path, 0, &file_count, &dir_count);
        
        LOGI("%s: Scan complete - Found %d files and %d directories\n", __func__, file_count, dir_count);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "load") == 0)
    {
        // Load (mount) and scan in one command
        ret = sd_card_mount();
        if (ret != BK_OK)
        {
            LOGE("%s: Failed to mount SD card\n", __func__);
            goto exit;
        }
        
        // Get scan path if provided
        if (argc >= 3)
        {
            scan_path = argv[2];
        }
        
        LOGI("%s: Scanning SD card directory: %s\n", __func__, scan_path);
        file_count = 0;
        dir_count = 0;
        
        // Scan directory recursively
        scan_directory_recursive(scan_path, 0, &file_count, &dir_count);
        
        LOGI("%s: Scan complete - Found %d files and %d directories\n", __func__, file_count, dir_count);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else
    {
        LOGE("%s: unknown command: %s\n", __func__, argv[1]);
        goto exit;
    }
    
exit:
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

// Video file info command handler - analyze AVI file information
// Helper function to get codec name string from codec type
static const char *get_video_codec_name(uint32_t codec_type)
{
    switch (codec_type)
    {
        case MP4_CODEC_H264:
            return "H.264 (AVC1)";
        case MP4_CODEC_MJPEG:
            return "MJPEG";
        default:
            return "Unknown";
    }
}

// Helper function to get audio codec name string from codec type
static const char *get_audio_codec_name(uint32_t codec_type)
{
    switch (codec_type)
    {
        case MP4_CODEC_AAC:
            return "AAC";
        case MP4_CODEC_MP3:
            return "MP3";
        case MP4_CODEC_PCM_SOWT:
            return "PCM (Little-endian)";
        case MP4_CODEC_PCM_TWOS:
            return "PCM (Big-endian)";
        default:
            return "Unknown";
    }
}

void cli_video_file_info_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    const char *file_path = NULL;
    bool is_mp4 = false;
    
    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        goto exit;
    }
    
    // Get file path from arguments
    file_path = argv[1];
    if (file_path == NULL || file_path[0] == '\0')
    {
        LOGE("%s: file_path is required\n", __func__);
        goto exit;
    }
    
    // Detect file type by extension
    const char *ext = os_strrchr(file_path, '.');
    if (ext != NULL)
    {
        // Check if extension is .mp4 (case-insensitive)
        if ((ext[1] == 'm' || ext[1] == 'M') &&
            (ext[2] == 'p' || ext[2] == 'P') &&
            (ext[3] == '4' || ext[3] == '4') &&
            ext[4] == '\0')
        {
            is_mp4 = true;
        }
    }
    
    // Mount SD card if not mounted
    if (!sd_card_is_mounted())
    {
        int sd_ret = sd_card_mount();
        if (sd_ret != BK_OK)
        {
            LOGE("%s: sd_card_mount failed, ret=%d\n", __func__, sd_ret);
            goto exit;
        }
    }
    
    // Check if file exists
    struct stat file_stat;
    if (stat(file_path, &file_stat) != 0)
    {
        LOGE("%s: File does not exist: %s\n", __func__, file_path);
        goto exit;
    }
    
    // Get file size
    uint64_t file_size = (uint64_t)file_stat.st_size;
    
    // Format output message
    char info_buffer[2048] = {0};
    int offset = 0;
    
    if (is_mp4)
    {
        // Handle MP4 file
        mp4_t *mp4 = MP4_open_input_file(file_path, 1, MP4_MEM_PSRAM); // getIndex = 1 to read index
        if (mp4 == NULL)
        {
            LOGE("%s: Failed to open MP4 file: %s\n", __func__, file_path);
            goto exit;
        }
        
        // Get video information
        uint32_t video_frames = MP4_video_frames(mp4);
        uint32_t video_width = MP4_video_width(mp4);
        uint32_t video_height = MP4_video_height(mp4);
        double video_fps = MP4_video_frame_rate(mp4);
        uint32_t video_codec = MP4_video_codec(mp4);
        const char *video_codec_name = get_video_codec_name(video_codec);
        
        // Get audio information
        uint16_t audio_channels = MP4_audio_channels(mp4);
        uint16_t audio_bits = MP4_audio_bits(mp4);
        uint32_t audio_format = MP4_audio_format(mp4);
        uint32_t audio_rate = MP4_audio_rate(mp4);
        uint64_t audio_bytes = MP4_audio_bytes(mp4);
        const char *audio_codec_name = get_audio_codec_name(audio_format);
        
        // Calculate duration from video
        double duration_seconds = 0.0;
        if (video_fps > 0 && video_frames > 0)
        {
            duration_seconds = (double)video_frames / video_fps;
        }
        else if (audio_rate > 0 && audio_channels > 0 && audio_bits > 0 && audio_bytes > 0)
        {
            // Calculate duration from audio: bytes / (rate * channels * bytes_per_sample)
            uint32_t bytes_per_sample = (audio_bits + 7) / 8;
            duration_seconds = (double)audio_bytes / ((double)audio_rate * audio_channels * bytes_per_sample);
        }
        
        uint32_t duration_minutes = (uint32_t)(duration_seconds / 60);
        uint32_t duration_secs = (uint32_t)(duration_seconds) % 60;
        uint32_t duration_ms = (uint32_t)((duration_seconds - (uint32_t)duration_seconds) * 1000);
        
        // Format output
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "File: %s (MP4)\r\n", file_path);
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "Size: %llu bytes (%.2f MB)\r\n", 
                              file_size, (double)file_size / (1024.0 * 1024.0));
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "Duration: %u:%02u.%03u (%.2f seconds)\r\n", 
                              duration_minutes, duration_secs, duration_ms, duration_seconds);
        
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "\r\nVideo:\r\n");
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "  Resolution: %ux%u\r\n", video_width, video_height);
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "  Frame rate: %.2f fps\r\n", video_fps);
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "  Total frames: %u\r\n", video_frames);
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "  Codec: %s (0x%08X)\r\n", video_codec_name, video_codec);
        
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "\r\nAudio:\r\n");
        if (audio_channels > 0)
        {
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Channels: %u\r\n", audio_channels);
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Sample rate: %u Hz\r\n", audio_rate);
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Bits per sample: %u\r\n", audio_bits);
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Codec: %s (0x%08X)\r\n", audio_codec_name, audio_format);
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Total bytes: %llu (%.2f MB)\r\n", 
                                  audio_bytes, (double)audio_bytes / (1024.0 * 1024.0));
        }
        else
        {
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  No audio track\r\n");
        }
        
        // Close MP4 file
        MP4_close_input_file(mp4);
    }
    else
    {
        // Handle AVI file
        avi_t *avi = AVI_open_input_file(file_path, 1, AVI_MEM_PSRAM); // getIndex = 1 to read index
        if (avi == NULL)
        {
            LOGE("%s: Failed to open AVI file: %s\n", __func__, file_path);
            goto exit;
        }
        
        // Get video information
        long video_frames = AVI_video_frames(avi);
        int video_width = AVI_video_width(avi);
        int video_height = AVI_video_height(avi);
        double video_fps = AVI_video_frame_rate(avi);
        char *video_compressor = AVI_video_compressor(avi);
        
        // Get audio information
        int audio_channels = AVI_audio_channels(avi);
        int audio_bits = AVI_audio_bits(avi);
        int audio_format = AVI_audio_format(avi);
        long audio_rate = AVI_audio_rate(avi);
        long audio_bytes = AVI_audio_bytes(avi);
        
        // Calculate duration
        double duration_seconds = 0.0;
        if (video_fps > 0 && video_frames > 0)
        {
            duration_seconds = (double)video_frames / video_fps;
        }
        else if (audio_rate > 0 && audio_channels > 0 && audio_bits > 0 && audio_bytes > 0)
        {
            // Calculate duration from audio: bytes / (rate * channels * bytes_per_sample)
            uint32_t bytes_per_sample = (audio_bits + 7) / 8;
            duration_seconds = (double)audio_bytes / ((double)audio_rate * audio_channels * bytes_per_sample);
        }
        
        uint32_t duration_minutes = (uint32_t)(duration_seconds / 60);
        uint32_t duration_secs = (uint32_t)(duration_seconds) % 60;
        uint32_t duration_ms = (uint32_t)((duration_seconds - (uint32_t)duration_seconds) * 1000);
        
        // Format output
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "File: %s (AVI)\r\n", file_path);
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "Size: %llu bytes (%.2f MB)\r\n", 
                              file_size, (double)file_size / (1024.0 * 1024.0));
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "Duration: %u:%02u.%03u (%.2f seconds)\r\n", 
                              duration_minutes, duration_secs, duration_ms, duration_seconds);
        
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "\r\nVideo:\r\n");
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "  Resolution: %dx%d\r\n", video_width, video_height);
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "  Frame rate: %.2f fps\r\n", video_fps);
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "  Total frames: %ld\r\n", video_frames);
        if (video_compressor != NULL && video_compressor[0] != '\0')
        {
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Compressor: %.4s\r\n", video_compressor);
        }
        
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "\r\nAudio:\r\n");
        if (audio_channels > 0)
        {
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Channels: %d\r\n", audio_channels);
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Sample rate: %ld Hz\r\n", audio_rate);
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Bits per sample: %d\r\n", audio_bits);
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Format: 0x%04X\r\n", audio_format);
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  Total bytes: %ld (%.2f MB)\r\n", 
                                  audio_bytes, (double)audio_bytes / (1024.0 * 1024.0));
        }
        else
        {
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                                  "  No audio track\r\n");
        }
        
        // Index information
        offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset, 
                              "\r\nIndex:\r\n");
        // avi_t internal index/segment fields are private and not exposed.
        // Use public APIs as a capability probe instead.
        long first_frame_size = 0;
        if (AVI_set_video_read_index(avi, 0, &first_frame_size) == 0 && first_frame_size > 0)
        {
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset,
                                  "  Video index: Available\r\n");
        }
        else
        {
            offset += os_snprintf(info_buffer + offset, sizeof(info_buffer) - offset,
                                  "  Video index: Not available\r\n");
        }
        
        // Close AVI file
        AVI_close(avi);
    }
    
    // Copy info to output buffer
    os_strncpy(pcWriteBuffer, info_buffer, xWriteBufferLen - 1);
    pcWriteBuffer[xWriteBufferLen - 1] = '\0';
    msg = CLI_CMD_RSP_SUCCEED;
    
exit:
    if (msg == CLI_CMD_RSP_ERROR)
    {
        os_strncpy(pcWriteBuffer, msg, xWriteBufferLen - 1);
        pcWriteBuffer[xWriteBufferLen - 1] = '\0';
    }
}
