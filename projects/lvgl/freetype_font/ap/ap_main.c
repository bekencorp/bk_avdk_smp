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

#define TAG "freetype_font"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);
extern const lcd_device_t lcd_device_st77903_h0165y008t;

bk_display_qspi_ctlr_config_t qspi_ctlr_config = {
    .lcd_device = &lcd_device_st77903_h0165y008t,
    .qspi_id = 0,
    .reset_pin = GPIO_40,
    .te_pin = 0,
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

static void lv_example_freetype(void)
{
    bk_err_t ret = lv_vfs_init();
    if (ret != BK_OK) {
        LOGE("lv_vfs_init failed\r\n");
        return;
    }

    int fd = open(PATH_INTERNAL_FLASH_FILE("Lato-Regular.ttf"), O_RDONLY);
    if (fd < 0) {
        LOGE("file_content open failed\r\n");
        lv_vfs_deinit();
        return;
    }

    int file_len = lv_img_get_filelen(PATH_INTERNAL_FLASH_FILE("Lato-Regular.ttf"));
    if (file_len <= 0) {
        LOGE("file len read failed\r\n");
        close(fd);
        lv_vfs_deinit();
        return;
    }

    uint32_t *file_content = psram_malloc(file_len);
    if (file_content == NULL) {
        LOGE("file_content malloc failed\r\n");
        close(fd);
        lv_vfs_deinit();
        return;
    }

    uint32_t read_len = read(fd, file_content, file_len);
    LOGD("read_len = %d \r\n", read_len);
    close(fd);
    ret = lv_vfs_deinit();
    if (ret != BK_OK) {
        LOGE("lv_vfs_deinit failed\r\n");
        return;
    }

    lv_vendor_disp_lock();
    /*Create a font*/
    static lv_ft_info_t info;
    /*FreeType uses C standard file system, so no driver letter is required.*/
    info.name = PATH_INTERNAL_FLASH_FILE("Lato-Regular.ttf");
    info.weight = 24;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = file_content;
    info.mem_size = file_len;
    if(!lv_ft_font_init(&info)) {
        LV_LOG_ERROR("create failed.");
    }

    /*Create style with the new font*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_font(&style, info.font);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);

    /*Create a label with the new style*/
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_obj_add_style(label, &style, 0);
    lv_label_set_text(label, "Hello world\nI'm a font created with FreeType");
    lv_obj_center(label);
    lv_vendor_disp_unlock();
}

bk_err_t lvgl_app_freetype_font_init(void)
{
    lv_vnd_config_t lv_vnd_config = {0};

    lv_vnd_config.width = qspi_ctlr_config.lcd_device->width;
    lv_vnd_config.height = qspi_ctlr_config.lcd_device->height;
    lv_vnd_config.render_mode = RENDER_PARTIAL_MODE;
    lv_vnd_config.rotation = ROTATE_NONE;
    for (int i = 0; i < CONFIG_LVGL_FRAME_BUFFER_NUM; i++) {
        lv_vnd_config.frame_buffer[i] = frame_buffer_display_malloc(lv_vnd_config.width * lv_vnd_config.height * sizeof(bk_color_t));
        if (lv_vnd_config.frame_buffer[i] == NULL) {
            LOGE("lv_frame_buffer[%d] malloc failed\r\n", i);
            return BK_FAIL;
        }
    }
    bk_display_qspi_new(&lv_vnd_config.handle, &qspi_ctlr_config);
    lv_vendor_init(&lv_vnd_config);

    bk_display_open(lv_vnd_config.handle);
    lcd_backlight_open(GPIO_7);

#if (CONFIG_TP)
    drv_tp_open(lv_vnd_config.width, lv_vnd_config.height, TP_MIRROR_NONE);
#endif

    lv_example_freetype();

    lv_vendor_start();

    return BK_OK;
}

#define CMDS_COUNT  (sizeof(s_freetype_font_commands) / sizeof(struct cli_command))

void cli_freetype_font_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s %d\r\n", __func__, __LINE__);
}

static const struct cli_command s_freetype_font_commands[] =
{
    {"freetype_font", "freetype_font", cli_freetype_font_cmd},
};

int cli_freetype_font_init(void)
{
    return cli_register_commands(s_freetype_font_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();

    media_service_init();

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_LVGL_CODE_RUN, PM_POWER_MODULE_STATE_ON);

    lvgl_app_freetype_font_init();

    return 0;
}
