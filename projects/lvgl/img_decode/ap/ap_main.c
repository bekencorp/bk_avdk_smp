#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "components/media_types.h"
#include "driver/drv_tp.h"
#if CONFIG_LVGL
#include "lvgl.h"
#include "lv_vendor.h"
#include "lv_img_utility.h"
#endif
#include "media_service.h"
#if (CONFIG_VFS)
#include "driver/flash_partition.h"
#include "bk_posix.h"
#endif
#include "components/bk_display.h"
#include "driver/gpio.h"
#include "gpio_driver.h"

#define TAG "img_decode"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define LCD_LDO_PIN         (GPIO_13)

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);
extern const lcd_device_t lcd_device_st7701s;

bk_display_rgb_ctlr_config_t rgb_ctlr_config = {
    .lcd_device = &lcd_device_st7701s,
    .clk_pin = GPIO_0,
    .cs_pin = GPIO_12,
    .sda_pin = GPIO_1,
    .rst_pin = GPIO_6,
};

static avdk_err_t lcd_backlight_open(uint8_t bl_io)
{
    gpio_dev_unmap(bl_io);
    BK_LOG_ON_ERR(bk_gpio_enable_output(bl_io));
    BK_LOG_ON_ERR(bk_gpio_pull_up(bl_io));
    bk_gpio_set_output_high(bl_io);
    return AVDK_ERR_OK;
}

static avdk_err_t lcd_backlight_close(uint8_t bl_io)
{
    BK_LOG_ON_ERR(bk_gpio_pull_down(bl_io));
    bk_gpio_set_output_low(bl_io);
    return AVDK_ERR_OK;
}

static int _fs_mount_lfs(void)
{
    int ret;

    struct bk_little_fs_partition partition;
    char *fs_name = NULL;
    bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);

    fs_name = "littlefs";
    partition.part_type = LFS_FLASH;
    partition.part_flash.start_addr = pt->partition_start_addr;
    partition.part_flash.size = pt->partition_length;
    partition.mount_path = VFS_INTERNAL_FLASH_PATITION_0;

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

    return ret;
}

static bk_err_t lv_vfs_init(void)
{
    bk_err_t ret = BK_FAIL;

    do {
        ret = _fs_mount_lfs();
        if (BK_OK != ret) {
            LOGD("[%s][%d] mount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }
        LOGD("[%s][%d] mount success\r\n", __FUNCTION__, __LINE__);
    } while(0);

    return ret;
}

static bk_err_t lv_vfs_deinit(void)
{
    bk_err_t ret = BK_FAIL;

    ret = umount(VFS_INTERNAL_FLASH_PATITION_0);
    if (BK_OK != ret) {
        LOGD(NULL, "[%s][%d] unmount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    LOGD("[%s][%d] unmount success\r\n", __FUNCTION__, __LINE__);

    return ret;
}

#if CONFIG_LVGL_V8
static lv_img_dsc_t img_dsc = {0};
#else
static lv_image_dsc_t img_dsc = {0};
#endif

bk_err_t lvgl_app_img_decode_init(void)
{
    lv_vnd_config_t lv_vnd_config = {0};

    lv_vnd_config.width = rgb_ctlr_config.lcd_device->width;
    lv_vnd_config.height = rgb_ctlr_config.lcd_device->height;
    lv_vnd_config.render_mode = RENDER_PARTIAL_MODE;
    lv_vnd_config.rotation = ROTATE_NONE;
    for (int i = 0; i < CONFIG_LVGL_FRAME_BUFFER_NUM; i++) {
        lv_vnd_config.frame_buffer[i] = frame_buffer_display_malloc(lv_vnd_config.width * lv_vnd_config.height * sizeof(bk_color_t));
        if (lv_vnd_config.frame_buffer[i] == NULL) {
            LOGE("lv_frame_buffer[%d] malloc failed\r\n", i);
            return BK_FAIL;
        }
    }
    bk_display_rgb_new(&lv_vnd_config.handle, &rgb_ctlr_config);
    lv_vendor_init(&lv_vnd_config);

    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);
    bk_display_open(lv_vnd_config.handle);
    lcd_backlight_open(GPIO_7);

#if (CONFIG_TP)
    drv_tp_open(lv_vnd_config.width, lv_vnd_config.height, TP_MIRROR_NONE);
#endif

    lv_vfs_init();

    // lv_jpeg_img_load_with_sw_dec(PATH_INTERNAL_FLASH_FILE("/img/bg.jpg"), &img_dsc, false);
    // lv_png_img_load(PATH_INTERNAL_FLASH_FILE("/img/bg_wifi_icon.png"), &img_dsc);  //need to open LV_USE_PNG
    lv_jpeg_img_load_with_hw_dec(PATH_INTERNAL_FLASH_FILE("/img/anim/anim-0.jpg"), &img_dsc, false);
#if CONFIG_LVGL_V8
    lv_obj_t * img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_dsc);
#else
    lv_obj_t * img = lv_image_create(lv_screen_active());
    lv_image_set_src(img, &img_dsc);
#endif
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    lv_vendor_start();

    return BK_OK;
}

#define CMDS_COUNT  (sizeof(s_img_decode_commands) / sizeof(struct cli_command))

void cli_img_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s %d\r\n", __func__, __LINE__);
}

static const struct cli_command s_img_decode_commands[] =
{
    {"img_decode", "img_decode", cli_img_decode_cmd},
};

int cli_freetype_font_init(void)
{
    return cli_register_commands(s_img_decode_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();

    media_service_init();

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_LVGL_CODE_RUN, PM_POWER_MODULE_STATE_ON);

    lvgl_app_img_decode_init();

    return 0;
}
