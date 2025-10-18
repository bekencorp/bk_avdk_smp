#include <os/os.h>
#include <components/avdk_utils/avdk_error.h>
#include "cli.h"
#include "components/bk_dma2d.h"
#include <driver/gpio.h>
#include "gpio_driver.h"
#include "lcd_panel_devices.h"
#include "components/bk_display.h"
#include "dma2d_test.h"
#include "driver/pwr_clk.h"
#define TAG "dma2d_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define BL_PIN  GPIO_7
#define LCD_LDO_PIN GPIO_13
static bk_dma2d_ctlr_handle_t dma2d_handle1 = NULL;
static bk_dma2d_ctlr_handle_t dma2d_handle2 = NULL;
static const lcd_device_t *lcd_device =  &lcd_device_st7701sn;
bk_display_ctlr_handle_t lcd_display_handle = NULL;


static void cli_dma2d_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    char *msg = NULL;
    // 修复open命令中的句柄赋值问题
    if (os_strcmp(argv[1], "open") == 0) {
        if (os_strcmp(argv[2], "module1") == 0) 
        {
            ret = bk_dma2d_new(&dma2d_handle1);
            AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_dma2d_new failed! ");
            LOGD("bk_dma2d_new module1 success! %p\n", dma2d_handle1);
            ret = bk_dma2d_open(dma2d_handle1);
            AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_dma2d_open failed!\n");
            LOGD("bk_dma2d_open module1 success! %p\n", dma2d_handle1);
        }
        else if (os_strcmp(argv[2], "module2") == 0)
        {
            ret = bk_dma2d_new(&dma2d_handle2);
            AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_dma2d_new failed! ");
            LOGD("bk_dma2d_new module2 success! %p\n", dma2d_handle2);
            ret = bk_dma2d_open(dma2d_handle2);
            AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_dma2d_open failed!\n");
            LOGD("bk_dma2d_open module2 success! %p\n", dma2d_handle2);
        }
        if (lcd_display_handle == NULL)
        {
            bk_display_rgb_ctlr_config_t lcd_display_config = {0};
            lcd_display_config.lcd_device = lcd_device;
            lcd_display_config.clk_pin = GPIO_0;
            lcd_display_config.cs_pin = GPIO_12;
            lcd_display_config.sda_pin = GPIO_1;
            lcd_display_config.rst_pin = GPIO_6;
            ret = bk_display_rgb_new(&lcd_display_handle, &lcd_display_config);
            AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_rgb_new failed!\n");
            LOGD("bk_display_rgb_new success!\n");


            gpio_dev_unmap(BL_PIN);
            BK_LOG_ON_ERR(bk_gpio_enable_output(BL_PIN));
            bk_gpio_set_output_high(BL_PIN);

            bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);
            ret = bk_display_open(lcd_display_handle);
            AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_open failed!\n");
        }
    }

    else if (os_strcmp(argv[1], "close") == 0) {
        if (os_strcmp(argv[2], "module1") == 0 && dma2d_handle1 != NULL)
        {
            ret = bk_dma2d_close(dma2d_handle1);
            AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_dma2d_close failed!\n");
            LOGD("bk_dma2d_close module1 success!\n");
    
            ret = bk_dma2d_delete(dma2d_handle1);
            AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_dma2d_delete failed!\n");
            dma2d_handle1 = NULL;
            LOGD("bk_dma2d_delete module1 success!\n");
        } 
        else if (os_strcmp(argv[2], "module2") == 0 && dma2d_handle2 != NULL)
        {
            ret = bk_dma2d_close(dma2d_handle2);
            AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_dma2d_close failed!\n");
            LOGD("bk_dma2d_close module2 success!\n");
    
            ret = bk_dma2d_delete(dma2d_handle2);
            AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_dma2d_delete failed!\n");
            dma2d_handle2 = NULL;
            LOGD("bk_dma2d_delete module2 success!\n");
        }
        if (lcd_display_handle != NULL)
        {
            if (dma2d_handle2 == NULL && dma2d_handle1 == NULL)
            {
                ret = bk_display_close(lcd_display_handle);
                AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_close failed!\n");
                ret = bk_display_delete(lcd_display_handle);
                AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_delete failed!\n");
                lcd_display_handle = NULL;
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                bk_gpio_set_output_low(BL_PIN);
            }
        }
    }
    
    else if (os_strcmp(argv[1], "fill") == 0) {
        if (argc < 10) {
            LOGE("Usage: dma2d fill <format> <color> <frame_width> <frame_height> <xpos> <ypos> <fill_width> <fill_height>\n");
            return;
        }
        if (dma2d_handle1 != NULL)
        {
            ret = dma2d_fill_test(dma2d_handle1, argv[2], os_strtoul(argv[3], NULL, 16),
                                    os_strtoul(argv[4], NULL, 10), os_strtoul(argv[5], NULL, 10),
                                    os_strtoul(argv[6], NULL, 10), os_strtoul(argv[7], NULL, 10),
                                    os_strtoul(argv[8], NULL, 10), os_strtoul(argv[9], NULL, 10));
            
            if (ret != AVDK_ERR_OK) {
                LOGE("dma2d_fill_test module1 failed!\n");
            }
        }
    }
    else if (os_strcmp(argv[1], "fill_module2") == 0) {
        if (argc < 10) {
            LOGE("Usage: dma2d fill <format> <color> <frame_width> <frame_height> <xpos> <ypos> <fill_width> <fill_height>\n");
            return;
        }
        if (dma2d_handle2 != NULL)
        {
            ret = dma2d_fill_test(dma2d_handle2, argv[2], os_strtoul(argv[3], NULL, 16),
                            os_strtoul(argv[4], NULL, 10), os_strtoul(argv[5], NULL, 10),
                            os_strtoul(argv[6], NULL, 10), os_strtoul(argv[7], NULL, 10),
                            os_strtoul(argv[8], NULL, 10), os_strtoul(argv[9], NULL, 10));  
        
            if (ret != AVDK_ERR_OK) {
                LOGE("dma2d_fill_test failed!\n");
            }
        }
    }
    //dma2d memcpy RGB565,0xf800 32,48,32,48,0,0,0,0,32,48
    else if (os_strcmp(argv[1], "memcpy") == 0) {
        if (argc < 14) {
            LOGE("Usage: dma2d memcpy <format> <color> <src_width> <src_height> <dst_width> <dst_height> <src_x> <src_y> <dst_x> <dst_y> <width> <height>\n");
            return;
        }
        if (dma2d_handle1 != NULL)
        {
            ret = dma2d_memcpy_test(dma2d_handle1, argv[2], os_strtoul(argv[3], NULL, 16),
                                os_strtoul(argv[4], NULL, 10), os_strtoul(argv[5], NULL, 10),
                                os_strtoul(argv[6], NULL, 10), os_strtoul(argv[7], NULL, 10),
                                os_strtoul(argv[8], NULL, 10), os_strtoul(argv[9], NULL, 10),
                                os_strtoul(argv[10], NULL, 10), os_strtoul(argv[11], NULL, 10),
                                os_strtoul(argv[12], NULL, 10), os_strtoul(argv[13], NULL, 10));
            if (ret != AVDK_ERR_OK) {
                LOGE("dma2d_memcpy_test failed!\n");
            }
        }
    }
    else if (os_strcmp(argv[1], "memcpy_module2") == 0) {
        if (argc < 14) {
            LOGE("Usage: dma2d memcpy <format> <color> <src_width> <src_height> <dst_width> <dst_height> <src_x> <src_y> <dst_x> <dst_y> <width> <height>\n");
            return;
        }   
        if  (dma2d_handle2 != NULL)
        {
            ret = dma2d_memcpy_test(dma2d_handle2, argv[2], os_strtoul(argv[3], NULL, 16),
                            os_strtoul(argv[4], NULL, 10), os_strtoul(argv[5], NULL, 10),
                            os_strtoul(argv[6], NULL, 10), os_strtoul(argv[7], NULL, 10),
                            os_strtoul(argv[8], NULL, 10), os_strtoul(argv[9], NULL, 10),
                            os_strtoul(argv[10], NULL, 10), os_strtoul(argv[11], NULL, 10),
                            os_strtoul(argv[12], NULL, 10), os_strtoul(argv[13], NULL, 10));
            if (ret != AVDK_ERR_OK) {
                LOGE("dma2d_memcpy_test failed!\n");
            }
        }
    }
    //dma2d pfc RGB565,RGB888,0xf800 32,48,32,48,0,0,0,0,32,48
    else if (os_strcmp(argv[1], "pfc") == 0) {
        if (argc < 15) {
            LOGE("Usage: dma2d pfc <input_format> <output_format> <color> <src_width> <src_height> <dst_width> <dst_height> <src_x> <src_y> <dst_x> <dst_y> <width> <height>\n");
            return;
        }
        if (dma2d_handle1 != NULL) {
            ret = dma2d_pfc_test(dma2d_handle1, argv[2], argv[3], os_strtoul(argv[4], NULL, 16),
                                os_strtoul(argv[5], NULL, 10), os_strtoul(argv[6], NULL, 10),
                                os_strtoul(argv[7], NULL, 10), os_strtoul(argv[8], NULL, 10),
                                os_strtoul(argv[9], NULL, 10), os_strtoul(argv[10], NULL, 10),
                                os_strtoul(argv[11], NULL, 10), os_strtoul(argv[12], NULL, 10),
                                os_strtoul(argv[13], NULL, 10), os_strtoul(argv[14], NULL, 10));
            
            if (ret != AVDK_ERR_OK) {
                LOGE("dma2d_pfc_test failed!\n");
            }
        }
    }
    else if (os_strcmp(argv[1], "pfc_module2") == 0) {
        if (argc < 15) {
            LOGE("Usage: dma2d pfc <input_format> <output_format> <color> <src_width> <src_height> <dst_width> <dst_height> <src_x> <src_y> <dst_x> <dst_y> <width> <height>\n");
            return;
        }
        if (dma2d_handle2 != NULL) {
            ret = dma2d_pfc_test(dma2d_handle2, argv[2], argv[3], os_strtoul(argv[4], NULL, 16),
                        os_strtoul(argv[5], NULL, 10), os_strtoul(argv[6], NULL, 10),
                        os_strtoul(argv[7], NULL, 10), os_strtoul(argv[8], NULL, 10),
                        os_strtoul(argv[9], NULL, 10), os_strtoul(argv[10], NULL, 10),
                        os_strtoul(argv[11], NULL, 10), os_strtoul(argv[12], NULL, 10),
                        os_strtoul(argv[13], NULL, 10), os_strtoul(argv[14], NULL, 10));
            
            if (ret != AVDK_ERR_OK) {
                LOGE("dma2d_pfc_test failed!\n");
            }
        }
    }
    //dma2d blend RGB565,RGB888,RGB888,0xf800,0x07e0,32,48,32,48,32,48,0,0,0,0,0,0,32,48,FF
    else if (os_strcmp(argv[1], "blend") == 0) {
        if (argc < 4) {
            LOGE("Usage: dma2d blend <fg_format> <bg_format> <output_format> <fg_color> <bg_color> <bg_width> <bg_height> <fg_width> <fg_height> <dst_width> <dst_height> <bg_x> <bg_y> <fg_x> <fg_y> <dst_x> <dst_y> <width> <height> <alpha>\n");
            return;
        }
        if (dma2d_handle1 != NULL)
        {
            ret = dma2d_blend_test(dma2d_handle1, argv[2],os_strtoul(argv[3], NULL, 16),
                                os_strtoul(argv[4], NULL, 10), os_strtoul(argv[5], NULL, 10), os_strtoul(argv[6], NULL, 10));
            
            if (ret != AVDK_ERR_OK) {
                LOGE("dma2d_blend_test failed!\n");
            }
        }
    }
    else if (os_strcmp(argv[1], "blend_module2") == 0) {
        if (argc < 4) {
            LOGE("Usage: dma2d blend <fg_format> <bg_format> <output_format> <fg_color> <bg_color> <bg_width> <bg_height> <fg_width> <fg_height> <dst_width> <dst_height> <bg_x> <bg_y> <fg_x> <fg_y> <dst_x> <dst_y> <width> <height> <alpha>\n");
            return;
        }
        if (dma2d_handle2 != NULL) {
            ret = dma2d_blend_test(dma2d_handle2, argv[2],os_strtoul(argv[3], NULL, 16),
                                os_strtoul(argv[4], NULL, 10), os_strtoul(argv[5], NULL, 10), os_strtoul(argv[6], NULL, 10));
            
            if (ret != AVDK_ERR_OK) {
                LOGE("dma2d_blend_test failed!\n");
            }
        }
    }
    else {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }

   exit: 

    if (ret != AVDK_ERR_OK) {
        msg = CLI_CMD_RSP_ERROR;
    } else {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define CMDS_COUNT  (sizeof(s_dma2d_commands) / sizeof(struct cli_command))

static const struct cli_command s_dma2d_commands[] = {
    {"dma2d", "open/close/fill/blend/memcpy/pfc ", cli_dma2d_cmd},
};

int cli_dma2d_init(void)
{
    return cli_register_commands(s_dma2d_commands, CMDS_COUNT);
}