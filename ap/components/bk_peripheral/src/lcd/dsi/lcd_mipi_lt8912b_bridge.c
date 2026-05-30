// Copyright 2024 Beken
//
// LT8912B MIPI-DSI to HDMI bridge driver.
//
// The bridge is registered as an avdk MIPI-DSI panel
// (::lcd_device_lt8912b_mipi). custom_init() programs the LT8912B
// over a private SW I2C bus owned by this driver - it is NEVER
// shared with other components, NEVER routed through the public
// ::bk_display_dsi_bus_t panel-IO channel, and NEVER carried in the
// generic vendor_config slot.
//
// Pin assignment can be overridden at runtime via
// ::bk_lcd_lt8912b_set_io_pins(); otherwise the driver falls back to
// CONFIG_LCD_LT8912B_PIN_SCL / CONFIG_LCD_LT8912B_PIN_SDA.

#include <os/os.h>
#include <os/mem.h>

#include <driver/gpio.h>
#include "gpio_driver.h"

#include <components/log.h>
#include <components/bk_lcd_panel.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>
#include <sw_i2c.h>

#include <avdk_error.h>
#include <avdk_check.h>
#include <lcd/lcd_mipi_lt8912b_bridge.h>

#if CONFIG_LCD_LT8912B_MIPI_BRIDGE

#define ENABLE_TEST_PATTERN 1

#define TAG "lt8912b_panel"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define LT8912B_I2C_ADDR_MAIN   (0x48)  /**< system control + LVDS + HDMI analog */
#define LT8912B_I2C_ADDR_CEC    (0x49)  /**< MIPI DSI digital */
#define LT8912B_I2C_ADDR_AVI    (0x4A)  /**< AVI infoframe + AUDIO */

#if defined(CONFIG_LCD_LT8912B_RES_800x600)
static const lt8912b_video_timing_t s_lt8912b_timing_current = LT8912B_VIDEO_TIMING_800x600_60();
#define LT8912B_CURRENT_TIMING  s_lt8912b_timing_current

#elif defined(CONFIG_LCD_LT8912B_RES_1024x768)
static const lt8912b_video_timing_t s_lt8912b_timing_current = LT8912B_VIDEO_TIMING_1024x768_60();
#define LT8912B_CURRENT_TIMING  s_lt8912b_timing_current

#elif defined(CONFIG_LCD_LT8912B_RES_1280x720)
static const lt8912b_video_timing_t s_lt8912b_timing_current = LT8912B_VIDEO_TIMING_1280x720_60();
#define LT8912B_CURRENT_TIMING  s_lt8912b_timing_current

#elif defined(CONFIG_LCD_LT8912B_RES_1280x800)
static const lt8912b_video_timing_t s_lt8912b_timing_current = LT8912B_VIDEO_TIMING_1280x800_60();
#define LT8912B_CURRENT_TIMING  s_lt8912b_timing_current

#elif defined(CONFIG_LCD_LT8912B_RES_1920x1080)
static const lt8912b_video_timing_t s_lt8912b_timing_current = LT8912B_VIDEO_TIMING_1920x1080_30();
#define LT8912B_CURRENT_TIMING  s_lt8912b_timing_current

#else
static const lt8912b_video_timing_t s_lt8912b_timing_current = LT8912B_VIDEO_TIMING_1280x720_60();
#define LT8912B_CURRENT_TIMING  s_lt8912b_timing_current
#endif

typedef struct {
    uint8_t cmd;
    uint8_t data;
} lt8912b_reg_t;

static const lt8912b_reg_t s_cmd_digital_clock_en[] = {
    {0x02, 0xF7},
    {0x08, 0xFF},
    {0x09, 0xFF},
    {0x0A, 0xFF},
    {0x0B, 0x7C},
    {0x0C, 0xFF},
};

// Tx analog configuration
static const lt8912b_reg_t s_cmd_tx_analog[] = {
    {0x31, 0xE1},
    {0x32, 0xE1},
    {0x33, 0x0C}, //0x17  ???????????????????check this register
    {0x37, 0x00},
    {0x38, 0x22},
    {0x60, 0x82},
};

// Cbus analog configuration
static const lt8912b_reg_t s_cmd_cbus_analog[] = {
    {0x39, 0x45},
    {0x3A, 0x00},
    {0x3B, 0x00},
};

// HDMI PLL analog configuration
static const lt8912b_reg_t s_cmd_hdmi_pll_analog[] = {
    {0x44, 0x31},
    {0x55, 0x44},
    {0x57, 0x01},
    {0x5A, 0x02},
};



/* LVDS / scaler configuration, only compiled when CONFIG_LCD_LT8912B_LVDS_ENABLE */
#if defined(CONFIG_LCD_LT8912B_AVI_ENABLE)
// Audio IIS mode configuration (HDMI mode)
static const lt8912b_reg_t s_cmd_audio_iis_mode[] = {
    {0xB2, 0x01}, // DVI mode:0x00; HDMI mode:0x01;
};

// Audio IIS enable sequence
static const lt8912b_reg_t s_cmd_audio_iis_en[] = {
    {0x06, 0x08},
    {0x07, 0xF0},
    {0x34, 0xD2},
    {0x0F, 0x2B},
};
static const lt8912b_reg_t s_cmd_lvds[] = {
    {0x44, 0x30},
    {0x51, 0x05},
    {0x50, 0x24},
    {0x51, 0x2D},
    {0x52, 0x04},
    {0x69, 0x0E},
    {0x69, 0x8E},
    {0x6A, 0x00},
    {0x6C, 0xB8},
    {0x6B, 0x51},
    {0x04, 0xFB},
    {0x04, 0xFF},
    {0x7F, 0x00},
    {0xA8, 0x13},
};
#endif

// DDS configuration (copied from ESP reference)
static const lt8912b_reg_t s_cmd_dds_config[] = {

    // {0x4E, 0x93}, //1e config pixel clock and video ppi
    // {0x4F, 0x3E}, //7a
    // {0x50, 0x29}, //80
    // {0x51, 0x80}, //80
   
    // {0x4E, 0x1E}, //  PIXEL CLOCK 74.25Mhz, dds frequency word 0x807A1E
    // {0x4F, 0x7A}, 
    // {0x50, 0x80}, 
    // {0x51, 0x80},  

    {0x1E, 0x4F},
    {0x1F, 0x5E},
    {0x20, 0x01},
    {0x21, 0x2C},
    {0x22, 0x01},
    {0x23, 0xFA},
    {0x24, 0x00},
    {0x25, 0xC8},
    {0x26, 0x00},
    {0x27, 0x5E},
    {0x28, 0x01},
    {0x29, 0x2C},
    {0x2A, 0x01},
    {0x2B, 0xFA},
    {0x2C, 0x00},
    {0x2D, 0xC8},
    {0x2E, 0x00},
    {0x42, 0x64},
    {0x43, 0x00},
    {0x44, 0x04},
    {0x45, 0x00},
    {0x46, 0x59},
    {0x47, 0x00},
    {0x48, 0xF2},
    {0x49, 0x06},
    {0x4A, 0x00},
    {0x4B, 0x72},
    {0x4C, 0x45},
    {0x4D, 0x00},
    {0x52, 0x08},
    {0x53, 0x00},
    {0x54, 0xB2},
    {0x55, 0x00},
    {0x56, 0xE4},
    {0x57, 0x0D},
    {0x58, 0x00},
    {0x59, 0xE4},
    {0x5A, 0x8A},
    {0x5B, 0x00},
    {0x5C, 0x34},
    {0x51, 0x00},
};

static sw_i2c_handle_t *s_lt8912b_i2c = NULL;

static bk_lcd_lt8912b_io_pins_t s_lt8912b_pins = {
    .scl_pin = (int8_t)CONFIG_LCD_LT8912B_PIN_SCL,
    .sda_pin = (int8_t)CONFIG_LCD_LT8912B_PIN_SDA,
};

bk_err_t bk_lcd_lt8912b_set_io_pins(const bk_lcd_lt8912b_io_pins_t *pins)
{
    if (pins == NULL) {
        return BK_ERR_NULL_PARAM;
    }
    if (pins->scl_pin < 0 || pins->sda_pin < 0) {
        LOGE("LT8912B set_io_pins: invalid pins scl=%d sda=%d\n",
             pins->scl_pin, pins->sda_pin);
        return BK_ERR_PARAM;
    }
    if (s_lt8912b_i2c != NULL &&
        (s_lt8912b_pins.scl_pin != pins->scl_pin ||
         s_lt8912b_pins.sda_pin != pins->sda_pin)) {
        LOGW("LT8912B set_io_pins: ignored, bus already up on scl=%d sda=%d\n",
             s_lt8912b_pins.scl_pin, s_lt8912b_pins.sda_pin);
        return BK_OK;
    }
    s_lt8912b_pins = *pins;
    return BK_OK;
}

static bk_err_t lt8912b_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t value)
{
    if (s_lt8912b_i2c == NULL) {
        LOGE("LT8912B sw_i2c not initialised (custom_init must run first)\n");
        return BK_ERR_NULL_PARAM;
    }

    uint8_t buf[2] = { reg, value };
    return sw_i2c_master_write(s_lt8912b_i2c, dev_addr, buf, sizeof(buf), 1000);
}

static bk_err_t lt8912b_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *value)
{
    if (s_lt8912b_i2c == NULL) {
        LOGE("LT8912B sw_i2c not initialised (custom_init must run first)\n");
        return BK_ERR_NULL_PARAM;
    }
    if (value == NULL) {
        return BK_ERR_NULL_PARAM;
    }

    bk_err_t ret = sw_i2c_master_write(s_lt8912b_i2c, dev_addr, &reg, 1u, 1000);
    if (ret != BK_OK) {
        return ret;
    }
    return sw_i2c_master_read(s_lt8912b_i2c, dev_addr, value, 1u, 1000);
}

static bk_err_t lt8912b_write_array(uint8_t dev_addr, const lt8912b_reg_t *seq, uint32_t count)
{
    bk_err_t ret = BK_OK;
    for (uint32_t i = 0; i < count; i++) {
        ret = lt8912b_write_reg(dev_addr, seq[i].cmd, seq[i].data);
        if (ret != BK_OK) {
            return ret;
        }
    }
    return BK_OK;
}

// MIPI analog settings (P/N swap and EQ)
static bk_err_t lt8912b_send_mipi_analog(bool pn_swap)
{
    bk_err_t ret;

    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x3E, pn_swap ? 0xF6 : 0xD6);
    if (ret != BK_OK) {
        return ret;
    }

    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x3F, 0xD4);
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x41, 0x3C);
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}

// MIPI basic settings: lane count and lane swap
static bk_err_t lt8912b_send_mipi_basic_set(uint8_t lane_count, bool lane_swap)
{
    bk_err_t ret;

    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x10, 0x01);
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x11, 0x10);
    if (ret != BK_OK) {
        return ret;
    }

    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x13, lane_count);
    if (ret != BK_OK) {
        return ret;
    }
    rtos_delay_milliseconds(10);
    uint8_t val = 0;
    lt8912b_read_reg(LT8912B_I2C_ADDR_CEC, 0x13, &val);
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x14, 0x00);
    if (ret != BK_OK) {
        return ret;
    }
    LOGI("%s read lane_count success, lane_count=%d, val=%d\n", __func__, lane_count, val);

    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x15, lane_swap ? 0xA8 : 0x00);
    if (ret != BK_OK) {
        return ret;
    }

    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x1A, 0x03);
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x1B, 0x03);
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}

// Program video timing into LT8912B
static bk_err_t lt8912b_send_video_setup(const lt8912b_video_timing_t *video)
{
    bk_err_t ret;

    lt8912b_reg_t timing_cfg[] = {
        {0x18, (uint8_t)(video->hs & 0xFF)},          /* HSYNC width */
        {0x19, (uint8_t)(video->vs & 0xFF)},          /* VSYNC width */
        {0x1C, (uint8_t)(video->hact & 0xFF)},        /* H active low */
        {0x1D, (uint8_t)(video->hact >> 8)},          /* H active high */
        {0x2F, 0x0C},                                 /* FIFO buffer length */
        {0x34, (uint8_t)(video->htotal & 0xFF)},      /* H total low */
        {0x35, (uint8_t)(video->htotal >> 8)},        /* H total high */
        {0x36, (uint8_t)(video->vtotal & 0xFF)},      /* V total low */
        {0x37, (uint8_t)(video->vtotal >> 8)},        /* V total high */
        {0x38, (uint8_t)(video->vbp & 0xFF)},         /* V back porch low */
        {0x39, (uint8_t)(video->vbp >> 8)},           /* V back porch high */
        {0x3A, (uint8_t)(video->vfp & 0xFF)},         /* V front porch low */
        {0x3B, (uint8_t)(video->vfp >> 8)},           /* V front porch high */
        {0x3C, (uint8_t)(video->hbp & 0xFF)},         /* H back porch low */
        {0x3D, (uint8_t)(video->hbp >> 8)},           /* H back porch high */
        {0x3E, (uint8_t)(video->hfp & 0xFF)},         /* H front porch low */
        {0x3F, (uint8_t)(video->hfp >> 8)},           /* H front porch high */
    };

    ret = lt8912b_write_array(LT8912B_I2C_ADDR_CEC,
                              timing_cfg,
                              sizeof(timing_cfg) / sizeof(timing_cfg[0]));
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}
// Recalculate DDS after video_timing is programmed, based on pclk_mhz
static bk_err_t lt8912b_update_dds(uint32_t pclk_mhz)
{
    // DDS = (pclk_mhz * 32768) / 24
    uint32_t dds = (pclk_mhz * 32768) / 24;
    bk_err_t ret;

    LOGI("Updating DDS for %lu MHz: 0x%06lX \n", pclk_mhz, dds);

    /* Configure 0x4E~0x51 using an array */
    lt8912b_reg_t dds_cfg[] = {
        {0x4E, (uint8_t)(dds & 0xFF)},
        {0x4F, (uint8_t)((dds >> 8) & 0xFF)},
        {0x50, (uint8_t)((dds >> 16) & 0xFF)},
        {0x51, 0x80},  /* enable bit / highest byte (bit 24) */
    };

    ret = lt8912b_write_array(LT8912B_I2C_ADDR_CEC,
                              dds_cfg,
                              sizeof(dds_cfg) / sizeof(dds_cfg[0]));
    if (ret != BK_OK) {
        return ret;
    }

    /* Read back 0x4E~0x51 using an array for verification */
    const uint8_t dds_regs[] = {0x4E, 0x4F, 0x50, 0x51};
    uint8_t check[4] = {0};

    for (int i = 0; i < 4; i++) {
        ret = lt8912b_read_reg(LT8912B_I2C_ADDR_CEC, dds_regs[i], &check[i]);
        if (ret != BK_OK) {
            return ret;
        }
    }

    LOGI("DDS set to: 0x%02X%02X%02X%02X \n", check[3], check[2], check[1], check[0]);

    return BK_OK;
}

// Program AVI infoframe according to video timing
static bk_err_t lt8912b_send_avi_infoframe(const lt8912b_video_timing_t *video)
{
    bk_err_t ret;

    uint8_t vic = (uint8_t)video->vic;
    uint8_t aspect_ratio = video->aspect_ratio;
    uint8_t pb0;
    uint8_t pb2;
    uint8_t pb4;
    uint8_t sync_polarity;

    // enable null package
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_AVI, 0x3C, 0x41);
    if (ret != BK_OK) {
        return ret;
    }

    sync_polarity = (video->h_polarity ? 0x02 : 0x00) + (video->v_polarity ? 0x01 : 0x00);
    pb2 = (uint8_t)((aspect_ratio << 4) + 0x08);
    pb4 = vic;
    if ((uint16_t)(pb2 + pb4) <= 0x5F) {
        pb0 = (uint8_t)(0x5F - pb2 - pb4);
    } else {
        pb0 = (uint8_t)(0x15F - pb2 - pb4);
    }

    // sync polarity
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0xAB, sync_polarity);
    if (ret != BK_OK) {
        return ret;
    }

    // PB0 checksum
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_AVI, 0x43, pb0);
    if (ret != BK_OK) {
        return ret;
    }
    // PB1: RGB888
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_AVI, 0x44, 0x10);
    if (ret != BK_OK) {
        return ret;
    }
    // PB2: aspect ratio
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_AVI, 0x45, pb2);
    if (ret != BK_OK) {
        return ret;
    }
    // PB3
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_AVI, 0x46, 0x00);
    if (ret != BK_OK) {
        return ret;
    }
    // PB4: VIC
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_AVI, 0x47, pb4);
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}

// MIPI RX logic reset
static bk_err_t lt8912b_mipi_rx_logic_reset(void)
{
    bk_err_t ret;

    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x03, 0x7F);
    if (ret != BK_OK) {
        return ret;
    }
    rtos_delay_milliseconds(100);
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x03, 0xFF);
    if (ret != BK_OK) {
        return ret;
    }

    // DDS reset
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x05, 0xFB);
    if (ret != BK_OK) {
        return ret;
    }
    rtos_delay_milliseconds(100);
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x05, 0xFF);
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}

// Debug helper: read MIPI input detection registers and log values
static bk_err_t lt8912b_debug_detect_input_mipi(void)
{
    bk_err_t ret;
    uint8_t hsync_l = 0;
    uint8_t hsync_h = 0;
    uint8_t vsync_l = 0;
    uint8_t vsync_h = 0;

    // These registers mirror _panel_lt8912b_detect_input_mipi() in ESP code
    ret = lt8912b_read_reg(LT8912B_I2C_ADDR_MAIN, 0x9C, &hsync_l);
    if (ret != BK_OK) {
        LOGE("read MIPI hsync_l failed, err=%d\n", ret);
        return ret;
    }
    ret = lt8912b_read_reg(LT8912B_I2C_ADDR_MAIN, 0x9D, &hsync_h);
    if (ret != BK_OK) {
        LOGE("read MIPI hsync_h failed, err=%d\n", ret);
        return ret;
    }
    ret = lt8912b_read_reg(LT8912B_I2C_ADDR_MAIN, 0x9E, &vsync_l);
    if (ret != BK_OK) {
        LOGE("read MIPI vsync_l failed, err=%d\n", ret);
        return ret;
    }
    ret = lt8912b_read_reg(LT8912B_I2C_ADDR_MAIN, 0x9F, &vsync_h);
    if (ret != BK_OK) {
        LOGE("read MIPI vsync_h failed, err=%d\n", ret);
        return ret;
    }

    LOGI("LT8912B MIPI input(0x9C~0x9F): H sync=0x%02X 0x%02X, V sync=0x%02X 0x%02X \n",
          hsync_l,hsync_h, vsync_l, vsync_h);

    return BK_OK;
}

// Check HDMI ready status via HPD bit (0xC1[7])
 bk_err_t lt8912b_is_ready(bool *ready)
{
    bk_err_t ret;
    uint8_t val = 0;

    if (ready == NULL) {
        return BK_ERR_NULL_PARAM;
    }

    // Optional: log current MIPI input detection registers
    ret = lt8912b_debug_detect_input_mipi();
    if (ret != BK_OK) {
        // For debug we still continue to read HPD, but return error to caller
        LOGW("lt8912b_debug_detect_input_mipi failed, err=%d\n", ret);
    }

    ret = lt8912b_read_reg(LT8912B_I2C_ADDR_MAIN, 0xC1, &val);
    if (ret != BK_OK) {
        LOGE("LT8912B read HPD failed, err=%d\n", ret);
        return ret;
    }

    *ready = ((val & 0x80) == 0x80) ? true : false;
    LOGI("LT8912B HPD reg=0x%02X, ready=%d \n", val, *ready);

    /* Read video timing related registers into an array for debug/verification */
    const uint8_t timing_regs[] = {
        0x18, 0x19, 0x1C, 0x1D, 0x2F,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    };
    uint8_t timing_vals[sizeof(timing_regs)] = {0};

    for (size_t i = 0; i < sizeof(timing_regs); i++) {
        ret = lt8912b_read_reg(LT8912B_I2C_ADDR_CEC, timing_regs[i], &timing_vals[i]);
        if (ret != BK_OK) {
            return ret;
        }
    }

    /* Decode timing fields from registers to human-readable values for debug */
    uint8_t reg_18 = timing_vals[0];
    uint8_t reg_19 = timing_vals[1];
    uint8_t reg_1c = timing_vals[2];
    uint8_t reg_1d = timing_vals[3];
    uint8_t reg_34 = timing_vals[5];
    uint8_t reg_35 = timing_vals[6];
    uint8_t reg_36 = timing_vals[7];
    uint8_t reg_37 = timing_vals[8];
    uint8_t reg_38 = timing_vals[9];
    uint8_t reg_39 = timing_vals[10];
    uint8_t reg_3a = timing_vals[11];
    uint8_t reg_3b = timing_vals[12];
    uint8_t reg_3c = timing_vals[13];
    uint8_t reg_3d = timing_vals[14];
    uint8_t reg_3e = timing_vals[15];
    uint8_t reg_3f = timing_vals[16];

    uint16_t hs_reg     = reg_18;
    uint16_t vs_reg     = reg_19;
    uint16_t hact_reg   = (uint16_t)((reg_1d << 8) | reg_1c);
    uint16_t htotal_reg = (uint16_t)((reg_35 << 8) | reg_34);
    uint16_t vtotal_reg = (uint16_t)((reg_37 << 8) | reg_36);
    uint16_t vbp_reg    = (uint16_t)((reg_39 << 8) | reg_38);
    uint16_t vfp_reg    = (uint16_t)((reg_3b << 8) | reg_3a);
    uint16_t hbp_reg    = (uint16_t)((reg_3d << 8) | reg_3c);
    uint16_t hfp_reg    = (uint16_t)((reg_3f << 8) | reg_3e);

    LOGI("%s, video timing from regs: hs=%u, vs=%u, hact=%u, htotal=%u, vtotal=%u, "
         "hfp=%u, hbp=%u, vfp=%u, vbp=%u\n",
         __func__, hs_reg, vs_reg, hact_reg, htotal_reg, vtotal_reg,
         hfp_reg, hbp_reg, vfp_reg, vbp_reg);
    return BK_OK;
}

// LVDS output enable / disable
static bk_err_t lt8912b_lvds_output(bool on)
{
    bk_err_t ret;

    if (on) {
        ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x02, 0xF7);
        if (ret != BK_OK) {
            return ret;
        }
        ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x02, 0xFF);
        if (ret != BK_OK) {
            return ret;
        }
        ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x03, 0xCB);
        if (ret != BK_OK) {
            return ret;
        }
        ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x03, 0xFB);
        if (ret != BK_OK) {
            return ret;
        }
        ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x03, 0xFF);
        if (ret != BK_OK) {
            return ret;
        }
        ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x44, 0x30);
        if (ret != BK_OK) {
            return ret;
        }
        LOGI("LT8912B LVDS output enabled\n");
    } else {
        ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x44, 0x31);
        if (ret != BK_OK) {
            return ret;
        }
        LOGI("LT8912B LVDS output disabled\n");
    }

    return BK_OK;
}

// HDMI output enable / disable
static bk_err_t lt8912b_hdmi_output(bool on)
{
    bk_err_t ret;

    if (on) {
        ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x33, 0x0E);
        if (ret != BK_OK) {
            return ret;
        }
        LOGI("LT8912B HDMI output enabled\n");
    } else {
        ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x33, 0x0C);
        if (ret != BK_OK) {
            return ret;
        }
        LOGI("LT8912B HDMI output disabled\n");
    }

    return BK_OK;
}

#if ENABLE_TEST_PATTERN
/**
 * Send LT8912B internal test pattern config (CEC/DSI regs 0x70-0x7d, 0x42, 0x1e, 0x4e-0x51).
 * Pattern resolution and pixel clock are set from video timing; pattern is then enabled.
 * For debug only: HDMI shows chip test pattern without MIPI input.
 */
static bk_err_t lt8912b_send_test_pattern(const lt8912b_video_timing_t *video)
{
    bk_err_t ret;
    uint32_t dds_initial_value;

    if (video == NULL) {
        return BK_ERR_NULL_PARAM;
    }

    /* Pattern resolution set */
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x72, 0x12);
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x73, (uint8_t)((video->hs + video->hbp) & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x74, (uint8_t)((video->hs + video->hbp) >> 8));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x75, (uint8_t)((video->vs + video->vbp) & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x76, (uint8_t)(video->hact & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x77, (uint8_t)(video->vact & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x78,
                           (uint8_t)(((video->vact >> 8) << 4) | (video->hact >> 8)));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x79, (uint8_t)(video->htotal & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x7A, (uint8_t)(video->vtotal & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x7B,
                           (uint8_t)(((video->vtotal >> 8) << 4) | (video->htotal >> 8)));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x7C, (uint8_t)(video->hs & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x7D,
                           (uint8_t)(((video->hs >> 8) << 6) | (video->vs & 0xFF)));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x70, 0x80); /* pattern enable */
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x71, 0x51);
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x42, 0x12);
    if (ret != BK_OK) {
        return ret;
    }

    /* h v d pol hdmi sel pll sel */
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x1E, 0x67);
    if (ret != BK_OK) {
        return ret;
    }

    /* Pattern pixel clock set: DDS = pclk_mhz * 0x16C16 */
    dds_initial_value = (uint32_t)(video->pclk_mhz * 0x16C16);
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x4E, (uint8_t)(dds_initial_value & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x4F, (uint8_t)((dds_initial_value >> 8) & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x50, (uint8_t)((dds_initial_value >> 16) & 0xFF));
    if (ret != BK_OK) {
        return ret;
    }
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_CEC, 0x51, 0x80);
    if (ret != BK_OK) {
        return ret;
    }

    LOGI("LT8912B test pattern enabled\n");
    return BK_OK;
}

/**
 * Public helper API: enable LT8912B internal test pattern using current Kconfig resolution.
 * This can be called from CLI or other modules to trigger HDMI test pattern on demand.
 */
bk_err_t bk_lcd_lt8912b_send_test_pattern(void)
{
    /* Use currently selected timing (LT8912B_CURRENT_TIMING) */
    return lt8912b_send_test_pattern(&LT8912B_CURRENT_TIMING);
}
#endif /* ENABLE_TEST_PATTERN */

static bk_err_t lt8912b_enable_i2c(void)
{
    bk_err_t ret;
    rtos_delay_milliseconds(100);
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0xFF, 0x80);  //SW RESET enter config mode
    if (ret != BK_OK) {
        return ret;
    }
    rtos_delay_milliseconds(100);
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0xEE, 0x01);  //ENABLE I2C
    if (ret != BK_OK) {
        return ret;
    }
    rtos_delay_milliseconds(50);
    ret = lt8912b_write_reg(LT8912B_I2C_ADDR_MAIN, 0x05, 0x01);  //RESET REG
    if (ret != BK_OK) {
        return ret;
    }

    return BK_OK;
}
// Full LT8912B initialization for 1280x720@60Hz
static bk_err_t lt8912b_init_sequence(void)
{
    bk_err_t ret;

    ret = lt8912b_enable_i2c();
    if (ret != BK_OK) {
        return ret;
    }

    // Digital clock enable
    ret = lt8912b_write_array(LT8912B_I2C_ADDR_MAIN,
                              s_cmd_digital_clock_en,
                              sizeof(s_cmd_digital_clock_en) / sizeof(s_cmd_digital_clock_en[0]));
    if (ret != BK_OK) {
        return ret;
    }

    // Tx analog
    ret = lt8912b_write_array(LT8912B_I2C_ADDR_MAIN,
                              s_cmd_tx_analog,
                              sizeof(s_cmd_tx_analog) / sizeof(s_cmd_tx_analog[0]));
    if (ret != BK_OK) {
        return ret;
    }

    // Cbus analog
    ret = lt8912b_write_array(LT8912B_I2C_ADDR_MAIN,
                              s_cmd_cbus_analog,
                              sizeof(s_cmd_cbus_analog) / sizeof(s_cmd_cbus_analog[0]));
    if (ret != BK_OK) {
        return ret;
    }

    // HDMI PLL analog
    ret = lt8912b_write_array(LT8912B_I2C_ADDR_MAIN,
                              s_cmd_hdmi_pll_analog,
                              sizeof(s_cmd_hdmi_pll_analog) / sizeof(s_cmd_hdmi_pll_analog[0]));
    if (ret != BK_OK) {
        return ret;
    }

    // MIPI analog, no P/N swap
    ret = lt8912b_send_mipi_analog(false);
    if (ret != BK_OK) {
        return ret;
    }

    // MIPI basic settings: 2 lanes, no lane swap
    ret = lt8912b_send_mipi_basic_set(2, false); //LT8912B_I2C_ADDR_CEC
    if (ret != BK_OK) {
        return ret;
    }

    // Update DDS based on video timing
    ret = lt8912b_update_dds(LT8912B_CURRENT_TIMING.pclk_mhz);
    if (ret != BK_OK) {
        return ret;
    }

    // DDS config
    ret = lt8912b_write_array(LT8912B_I2C_ADDR_CEC,
                              s_cmd_dds_config,
                              sizeof(s_cmd_dds_config) / sizeof(s_cmd_dds_config[0]));
    if (ret != BK_OK) {
        return ret;
    }



    // Video timing configuration (resolution selected by Kconfig macro)
    ret = lt8912b_send_video_setup(&LT8912B_CURRENT_TIMING);
    if (ret != BK_OK) {
        return ret;
    }

    ret = lt8912b_debug_detect_input_mipi(); //witing for mipi input lock  
    if (ret != BK_OK) {
        return ret;
    }

    // MIPI RX logic reset
    ret = lt8912b_mipi_rx_logic_reset();  
    if (ret != BK_OK) {
        return ret;
    }

#if defined(CONFIG_LCD_LT8912B_AVI_ENABLE)
    // AVI infoframe configuration
    ret = lt8912b_send_avi_infoframe(&LT8912B_CURRENT_TIMING);
    if (ret != BK_OK) {
        return ret;
    }

    // Audio IIS mode (HDMI)
    ret = lt8912b_write_array(LT8912B_I2C_ADDR_MAIN,
                              s_cmd_audio_iis_mode,
                              sizeof(s_cmd_audio_iis_mode) / sizeof(s_cmd_audio_iis_mode[0]));
    if (ret != BK_OK) {
        return ret;
    }

    // Audio IIS enable (AVI device)
    ret = lt8912b_write_array(LT8912B_I2C_ADDR_AVI,
                              s_cmd_audio_iis_en,
                              sizeof(s_cmd_audio_iis_en) / sizeof(s_cmd_audio_iis_en[0]));
    if (ret != BK_OK) {
        return ret;
    }
#endif

#if defined(CONFIG_LCD_LT8912B_LVDS_ENABLE)
    // LVDS / scaler configuration
    ret = lt8912b_write_array(LT8912B_I2C_ADDR_MAIN,
                              s_cmd_lvds,
                              sizeof(s_cmd_lvds) / sizeof(s_cmd_lvds[0]));
    if (ret != BK_OK) {
        return ret;
    }

    // Disable LVDS output (HDMI only)
    ret = lt8912b_lvds_output(false);
    if (ret != BK_OK) {
        return ret;
    }
#endif

    // Enable HDMI output
    ret = lt8912b_hdmi_output(true);
    if (ret != BK_OK) {
        return ret;
    }

    LOGI("LT8912B init sequence completed (resolution from Kconfig)\n");
    return BK_OK;
}

static bk_err_t lt8912b_custom_init(bk_avdk_lcd_panel_t *panel, void *priv)
{
    AVDK_RETURN_ON_FALSE(panel, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    if (s_lt8912b_i2c == NULL) {
        if (s_lt8912b_pins.scl_pin < 0 || s_lt8912b_pins.sda_pin < 0) {
            LOGE("LT8912B I2C pins not configured; call bk_lcd_lt8912b_set_io_pins() "
                 "or set CONFIG_LCD_LT8912B_PIN_SCL/_SDA\n");
            return BK_ERR_NULL_PARAM;
        }
        sw_i2c_config_t cfg = {
            .scl_pin = (gpio_id_t)s_lt8912b_pins.scl_pin,
            .sda_pin = (gpio_id_t)s_lt8912b_pins.sda_pin,
        };
        s_lt8912b_i2c = sw_i2c_init(&cfg);
        if (s_lt8912b_i2c == NULL) {
            LOGE("sw_i2c_init(scl=%d, sda=%d) failed\n",
                 s_lt8912b_pins.scl_pin, s_lt8912b_pins.sda_pin);
            return BK_ERR_NO_MEM;
        }
    }

    bk_err_t ret = lt8912b_init_sequence();
    if (ret != BK_OK) {
        LOGE("LT8912B init sequence failed, err=%d\n", ret);
        return ret;
    }
    bool ready = false;
    ret = lt8912b_is_ready(&ready);
    if (ret != BK_OK) {
        LOGE("LT8912B is_ready check failed, err=%d\n", ret);
        return ret;
    }
    if (!ready) {
        LOGW("LT8912B HDMI not ready (HPD low), please check HDMI cable/monitor\n");
    } else {
        LOGI("LT8912B HDMI reports ready (HPD high)\n");
    }
    return BK_OK;
}

static bk_err_t lt8912b_custom_reset(bk_avdk_lcd_panel_t *panel, void *priv)
{
    return BK_OK;
}


#if defined(CONFIG_LCD_LT8912B_RES_800x600)
const bk_display_dsi_panel_t lcd_device_lt8912b_mipi = {
    .id = 0x8912600,
    .name = "lt8912b_mipi_800x600",
    .n_lanes = DSI_ACTIVE_LANES_2,
    .fps = 60,
    .timing = {
        .h_size = 800,
        .v_size = 600,
        .hsync_pulse_width = 32,
        .vsync_pulse_width = 4,
        .hsync_back_porch = 88,
        .hsync_front_porch = 48,
        .vsync_back_porch = 23,
        .vsync_front_porch = 4,
    },
    .init_cmds = NULL,
    .read_id_regs = NULL,
    .read_id_bytes = 0,
    .custom_reset = lt8912b_custom_reset,
    .custom_init = lt8912b_custom_init,
};
BK_LCD_PANEL_DEVICE_SECTION(lcd_device_lt8912b_mipi, "lt8912b_mipi_800x600", BK_LCD_PANEL_BUS_DSI);

#elif defined(CONFIG_LCD_LT8912B_RES_1024x768)
const bk_display_dsi_panel_t lcd_device_lt8912b_mipi = {
    .id = 0x8912768,
    .name = "lt8912b_mipi_1024x768",
    .n_lanes = DSI_ACTIVE_LANES_2,
    .fps = 60,
    .timing = {
        .h_size = 1024,
        .v_size = 768,
        .hsync_pulse_width = 32,
        .vsync_pulse_width = 4,
        .hsync_back_porch = 80,
        .hsync_front_porch = 48,
        .vsync_back_porch = 15,
        .vsync_front_porch = 3,
    },
    .init_cmds = NULL,
    .read_id_regs = NULL,
    .read_id_bytes = 0,
    .custom_reset = lt8912b_custom_reset,
    .custom_init = lt8912b_custom_init,
};
BK_LCD_PANEL_DEVICE_SECTION(lcd_device_lt8912b_mipi, "lt8912b_mipi_1024x768", BK_LCD_PANEL_BUS_DSI);

#elif defined(CONFIG_LCD_LT8912B_RES_1280x720)
const bk_display_dsi_panel_t lcd_device_lt8912b_mipi = {
    .id = 0x8912720,
    .name = "lt8912b_mipi_1280x720",
    .n_lanes = DSI_ACTIVE_LANES_2,
    .fps = 60,
    .timing = {
        .h_size = 1280,
        .v_size = 720,
        .hsync_pulse_width = 32,
        .vsync_pulse_width = 5,
        .hsync_back_porch = 80,
        .hsync_front_porch = 48,
        .vsync_back_porch = 13,
        .vsync_front_porch = 3,
    },
    .init_cmds = NULL,
    .read_id_regs = NULL,
    .read_id_bytes = 0,
    .custom_reset = lt8912b_custom_reset,
    .custom_init = lt8912b_custom_init,
};
BK_LCD_PANEL_DEVICE_SECTION(lcd_device_lt8912b_mipi, "lt8912b_mipi_1280x720", BK_LCD_PANEL_BUS_DSI);

#elif defined(CONFIG_LCD_LT8912B_RES_1280x800)
const bk_display_dsi_panel_t lcd_device_lt8912b_mipi = {
    .id = 0x8912800,
    .name = "lt8912b_mipi_1280x800",
    .n_lanes = DSI_ACTIVE_LANES_2,
    .fps = 60,
    .timing = {
        .h_size = 1280,
        .v_size = 800,
        .hsync_pulse_width = 32,
        .vsync_pulse_width = 6,
        .hsync_back_porch = 80,
        .hsync_front_porch = 48,
        .vsync_back_porch = 14,
        .vsync_front_porch = 3,
    },
    .init_cmds = NULL,
    .read_id_regs = NULL,
    .read_id_bytes = 0,
    .custom_reset = lt8912b_custom_reset,
    .custom_init = lt8912b_custom_init,
};
BK_LCD_PANEL_DEVICE_SECTION(lcd_device_lt8912b_mipi, "lt8912b_mipi_1280x800", BK_LCD_PANEL_BUS_DSI);

#elif defined(CONFIG_LCD_LT8912B_RES_1920x1080)
const bk_display_dsi_panel_t lcd_device_lt8912b_mipi = {
    .id = 0x89121080,
    .name = "lt8912b_mipi_1920x1080",
    .n_lanes = DSI_ACTIVE_LANES_2,
    .fps = 35,
    .timing = {
        .h_size = 1920,
        .v_size = 1080,
        .hsync_pulse_width = 32,
        .vsync_pulse_width = 5,
        .hsync_back_porch = 80,
        .hsync_front_porch = 48,
        .vsync_back_porch = 8,
        .vsync_front_porch = 3,
    },
    .init_cmds = NULL,
    .read_id_regs = NULL,
    .read_id_bytes = 0,
    .custom_reset = lt8912b_custom_reset,
    .custom_init = lt8912b_custom_init,
};
BK_LCD_PANEL_DEVICE_SECTION(lcd_device_lt8912b_mipi, "lt8912b_mipi_1920x1080", BK_LCD_PANEL_BUS_DSI);

#else
const bk_display_dsi_panel_t lcd_device_lt8912b_mipi = {
    .id = 0x8912720,
    .name = "lt8912b_mipi_1280x720",
    .n_lanes = DSI_ACTIVE_LANES_2,
    .fps = 60,
    .timing = {
        .h_size = 1280,
        .v_size = 720,
        .hsync_pulse_width = 32,
        .vsync_pulse_width = 5,
        .hsync_back_porch = 80,
        .hsync_front_porch = 48,
        .vsync_back_porch = 13,
        .vsync_front_porch = 3,
    },
    .init_cmds = NULL,
    .read_id_regs = NULL,
    .read_id_bytes = 0,
    .custom_reset = lt8912b_custom_reset,
    .custom_init = lt8912b_custom_init,
};
BK_LCD_PANEL_DEVICE_SECTION(lcd_device_lt8912b_mipi, "lt8912b_mipi_1280x720", BK_LCD_PANEL_BUS_DSI);
#endif

#endif /* CONFIG_LCD_LT8912B_MIPI_BRIDGE */