// Copyright 2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <driver/isp_types.h>
#include <driver/mipi_csi.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_camera_sensor.h>
#include <components/bk_isp_camera_types.h>
#include <avdk_check.h>

#include "csi_sensor_devices.h"
#include "gpio_driver.h"
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"

#define TAG "tp2863"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define TP2863_AHD_720P25 1

#define TP2863_I2C_WRITE_ADDR_SAD0     0x88
#define TP2863_I2C_WRITE_ADDR_SAD1     0x8A
#define TP2863_MIPI_DATA_TYPE_YUV422   0x1E
#define TP2863_WIDTH                   1280
#define TP2863_HEIGHT                  720
#define TP2863_FPS_25                  25
#define TP2863_FPS_30                  30
#define TP2863_INVALID_GPIO            0xFF
#define TP2863_CH_NUM                  2
#define TP2863_MIPI_VC_MAP_DUAL_FIXED  0xE4 /* CH1->VC0, CH2->VC1 */
#define TP2863_PAGE_DECODER            0x00
#define TP2863_PAGE_MIPI               0x08
#define TP2863_REG_PAGE_SELECT         0x40

typedef struct
{
    uint8_t reg;
    uint8_t val;
} tp2863_reg_t;

typedef enum
{
    TP2863_VINP = 0,
    TP2863_VINN = 1,
    TP2863_DIFF = 2,
} tp2863_vin_t;

typedef struct
{
    bool locked;
    const char *label;
    uint8_t ch;
    uint8_t fps;
    tp2863_vin_t vin;
} tp2863_lock_info_t;

static tp2863_lock_info_t s_tp2863_locks[TP2863_CH_NUM];
static bool s_tp2863_dual_mode = false;

avdk_err_t tp2863_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);
static void tp2863_get_mipi_regs(const tp2863_reg_t **regs, uint32_t *size);

#if (!TP2863_AHD_720P25)
static const tp2863_reg_t tp2863_ch1_ahd_720p30_init_regs[] =
{
    {0x40, 0x00}, /* CH1 page */
    {0x06, 0x12},
    {0x50, 0x00}, /* VINP */
    {0x51, 0x00},
    {0x54, 0x03},
    {0x38, 0x00},
    {0x25, 0xFF},
    {0x26, 0x05},
    {0x27, 0x2D},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x54, 0x03},

    /* HD30 base timing, then STD_HDA override from the vendor script. */
    {0x42, 0x01},
    {0x43, 0x00},
    {0x02, 0x42},
    {0x07, 0xC0},
    {0x0B, 0xC0},
    {0x0C, 0x13},
    {0x0D, 0x50},
    {0x15, 0x13},
    {0x16, 0x15},
    {0x17, 0x00},
    {0x18, 0x19},
    {0x19, 0xD0},
    {0x1A, 0x25},
    {0x1C, 0x06},
    {0x1D, 0x72},
    {0x20, 0x30},
    {0x21, 0x84},
    {0x22, 0x36},
    {0x23, 0x3C},
    {0x2B, 0x60},
    {0x2C, 0x2A},
    {0x2D, 0x30},
    {0x2E, 0x70},
    {0x30, 0x48},
    {0x31, 0xBB},
    {0x32, 0x2E},
    {0x33, 0x90},
    {0x35, 0x25},
    {0x39, 0x08},

    /* STD_HDA override for HD30. */
    {0x02, 0x46},
    {0x0D, 0x70},
    {0x18, 0x1B},
    {0x20, 0x40},
    {0x21, 0x46},
    {0x25, 0xFE},
    {0x26, 0x01},
    {0x2C, 0x3A},
    {0x2D, 0x5A},
    {0x2E, 0x40},
    {0x30, 0x9D},
    {0x31, 0xCA},
    {0x32, 0x01},
    {0x33, 0xD0},

    /* Single-ended VINP path. */
    {0x3D, 0x60},
};

static const tp2863_reg_t tp2863_ch1_tvi_720p30_init_regs[] =
{
    {0x40, 0x00}, /* CH1 page */
    {0x06, 0x12},
    {0x50, 0x00}, /* VINP */
    {0x51, 0x00},
    {0x54, 0x03},
    {0x38, 0x00},
    {0x25, 0xFF},
    {0x26, 0x05},
    {0x27, 0x2D},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x54, 0x03},

    /* HD30 STD_TVI baseline from the vendor script. */
    {0x42, 0x01},
    {0x43, 0x00},
    {0x02, 0x42},
    {0x07, 0xC0},
    {0x0B, 0xC0},
    {0x0C, 0x13},
    {0x0D, 0x50},
    {0x15, 0x13},
    {0x16, 0x15},
    {0x17, 0x00},
    {0x18, 0x19},
    {0x19, 0xD0},
    {0x1A, 0x25},
    {0x1C, 0x06},
    {0x1D, 0x72},
    {0x20, 0x30},
    {0x21, 0x84},
    {0x22, 0x36},
    {0x23, 0x3C},
    {0x2B, 0x60},
    {0x2C, 0x2A},
    {0x2D, 0x30},
    {0x2E, 0x70},
    {0x30, 0x48},
    {0x31, 0xBB},
    {0x32, 0x2E},
    {0x33, 0x90},
    {0x35, 0x25},
    {0x39, 0x08},
    {0x3D, 0x60},
};

static const tp2863_reg_t tp2863_tvi_720p25_init_regs[] =
{
    {0x40, 0x00},
    {0x06, 0x12},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x54, 0x03},
    {0x38, 0x00},
    {0x25, 0xFF},
    {0x26, 0x05},
    {0x27, 0x2D},
    {0x42, 0x01},
    {0x43, 0x00},
    {0x02, 0x42},
    {0x07, 0xC0},
    {0x0B, 0xC0},
    {0x0C, 0x13},
    {0x0D, 0x50},
    {0x15, 0x13},
    {0x16, 0x15},
    {0x17, 0x00},
    {0x18, 0x19},
    {0x19, 0xD0},
    {0x1A, 0x25},
    {0x1C, 0x07},
    {0x1D, 0xBC},
    {0x20, 0x30},
    {0x21, 0x84},
    {0x22, 0x36},
    {0x23, 0x3C},
    {0x2B, 0x60},
    {0x2C, 0x2A},
    {0x2D, 0x30},
    {0x2E, 0x70},
    {0x30, 0x48},
    {0x31, 0xBB},
    {0x32, 0x2E},
    {0x33, 0x90},
    {0x35, 0x25},
    {0x39, 0x08},
    {0x3D, 0x60},
};
#endif

static const tp2863_reg_t tp2863_ahd_720p25_init_regs[] =
{
    {0x40, 0x00},
    {0x06, 0x12},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x54, 0x03},
    {0x38, 0x00},
    {0x25, 0xFF},
    {0x26, 0x05},
    {0x27, 0x2D},
    {0x42, 0x01},
    {0x43, 0x00},
    {0x02, 0x42},
    {0x07, 0xC0},
    {0x0B, 0xC0},
    {0x0C, 0x13},
    {0x0D, 0x50},
    {0x15, 0x13},
    {0x16, 0x15},
    {0x17, 0x00},
    {0x18, 0x19},
    {0x19, 0xD0},
    {0x1A, 0x25},
    {0x1C, 0x07},
    {0x1D, 0xBC},
    {0x20, 0x30},
    {0x21, 0x84},
    {0x22, 0x36},
    {0x23, 0x3C},
    {0x2B, 0x60},
    {0x2C, 0x2A},
    {0x2D, 0x30},
    {0x2E, 0x70},
    {0x30, 0x48},
    {0x31, 0xBB},
    {0x32, 0x2E},
    {0x33, 0x90},
    {0x35, 0x25},
    {0x39, 0x08},
    {0x02, 0x46},
    {0x0D, 0x71},
    {0x18, 0x1B},
    {0x20, 0x40},
    {0x21, 0x46},
    {0x25, 0xFE},
    {0x26, 0x01},
    {0x2C, 0x3A},
    {0x2D, 0x5A},
    {0x2E, 0x40},
    {0x30, 0x9E},
    {0x31, 0x20},
    {0x32, 0x10},
    {0x33, 0x90},
    {0x3D, 0x60},
};

#if (!TP2863_AHD_720P25)
static const tp2863_reg_t tp2863_tvi_1080p30_init_regs[] =
{
    {0x40, 0x00},
    {0x06, 0x12},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x54, 0x03},
    {0x38, 0x00},
    {0x25, 0xFF},
    {0x26, 0x05},
    {0x27, 0x2D},
    {0x42, 0x00},
    {0x43, 0x00},
    {0x02, 0x40},
    {0x07, 0xC0},
    {0x0B, 0xC0},
    {0x0C, 0x03},
    {0x0D, 0x50},
    {0x15, 0x03},
    {0x16, 0xD2},
    {0x17, 0x80},
    {0x18, 0x29},
    {0x19, 0x38},
    {0x1A, 0x47},
    {0x1C, 0x08},
    {0x1D, 0x98},
    {0x20, 0x30},
    {0x21, 0x84},
    {0x22, 0x36},
    {0x23, 0x3C},
    {0x2B, 0x60},
    {0x2C, 0x2A},
    {0x2D, 0x30},
    {0x2E, 0x70},
    {0x30, 0x48},
    {0x31, 0xBB},
    {0x32, 0x2E},
    {0x33, 0x90},
    {0x35, 0x05},
    {0x39, 0x0C},
    {0x3D, 0x60},
};

static const tp2863_reg_t tp2863_ahd_1080p30_init_regs[] =
{
    {0x40, 0x00},
    {0x06, 0x12},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x54, 0x03},
    {0x38, 0x00},
    {0x25, 0xFF},
    {0x26, 0x05},
    {0x27, 0x2D},
    {0x42, 0x00},
    {0x43, 0x00},
    {0x02, 0x40},
    {0x07, 0xC0},
    {0x0B, 0xC0},
    {0x0C, 0x03},
    {0x0D, 0x50},
    {0x15, 0x03},
    {0x16, 0xD2},
    {0x17, 0x80},
    {0x18, 0x29},
    {0x19, 0x38},
    {0x1A, 0x47},
    {0x1C, 0x08},
    {0x1D, 0x98},
    {0x20, 0x30},
    {0x21, 0x84},
    {0x22, 0x36},
    {0x23, 0x3C},
    {0x2B, 0x60},
    {0x2C, 0x2A},
    {0x2D, 0x30},
    {0x2E, 0x70},
    {0x30, 0x48},
    {0x31, 0xBB},
    {0x32, 0x2E},
    {0x33, 0x90},
    {0x35, 0x05},
    {0x39, 0x0C},
    {0x02, 0x44},
    {0x0D, 0x72},
    {0x15, 0x01},
    {0x16, 0xF0},
    {0x18, 0x2A},
    {0x20, 0x38},
    {0x21, 0x46},
    {0x25, 0xFE},
    {0x26, 0x0D},
    {0x2C, 0x3A},
    {0x2D, 0x54},
    {0x2E, 0x40},
    {0x30, 0xA5},
    {0x31, 0x95},
    {0x32, 0xE0},
    {0x33, 0x60},
    {0x3D, 0x60},
};

static const tp2863_reg_t tp2863_tvi_1080p25_init_regs[] =
{
    {0x40, 0x00},
    {0x06, 0x12},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x54, 0x03},
    {0x38, 0x00},
    {0x25, 0xFF},
    {0x26, 0x05},
    {0x27, 0x2D},
    {0x42, 0x00},
    {0x43, 0x00},
    {0x02, 0x40},
    {0x07, 0xC0},
    {0x0B, 0xC0},
    {0x0C, 0x03},
    {0x0D, 0x50},
    {0x15, 0x03},
    {0x16, 0xD2},
    {0x17, 0x80},
    {0x18, 0x29},
    {0x19, 0x38},
    {0x1A, 0x47},
    {0x1C, 0x0A},
    {0x1D, 0x50},
    {0x20, 0x30},
    {0x21, 0x84},
    {0x22, 0x36},
    {0x23, 0x3C},
    {0x2B, 0x60},
    {0x2C, 0x2A},
    {0x2D, 0x30},
    {0x2E, 0x70},
    {0x30, 0x48},
    {0x31, 0xBB},
    {0x32, 0x2E},
    {0x33, 0x90},
    {0x35, 0x05},
    {0x39, 0x0C},
    {0x3D, 0x60},
};

static const tp2863_reg_t tp2863_ahd_1080p25_init_regs[] =
{
    {0x40, 0x00},
    {0x06, 0x12},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x54, 0x03},
    {0x38, 0x00},
    {0x25, 0xFF},
    {0x26, 0x05},
    {0x27, 0x2D},
    {0x42, 0x00},
    {0x43, 0x00},
    {0x02, 0x40},
    {0x07, 0xC0},
    {0x0B, 0xC0},
    {0x0C, 0x03},
    {0x0D, 0x50},
    {0x15, 0x03},
    {0x16, 0xD2},
    {0x17, 0x80},
    {0x18, 0x29},
    {0x19, 0x38},
    {0x1A, 0x47},
    {0x1C, 0x0A},
    {0x1D, 0x50},
    {0x20, 0x30},
    {0x21, 0x84},
    {0x22, 0x36},
    {0x23, 0x3C},
    {0x2B, 0x60},
    {0x2C, 0x2A},
    {0x2D, 0x30},
    {0x2E, 0x70},
    {0x30, 0x48},
    {0x31, 0xBB},
    {0x32, 0x2E},
    {0x33, 0x90},
    {0x35, 0x05},
    {0x39, 0x0C},
    {0x02, 0x44},
    {0x0D, 0x73},
    {0x15, 0x01},
    {0x16, 0xF0},
    {0x18, 0x2A},
    {0x20, 0x3C},
    {0x21, 0x46},
    {0x25, 0xFE},
    {0x26, 0x0D},
    {0x2C, 0x3A},
    {0x2D, 0x54},
    {0x2E, 0x40},
    {0x30, 0xA5},
    {0x31, 0x86},
    {0x32, 0xFB},
    {0x33, 0x60},
    {0x3D, 0x60},
};
#endif

static const tp2863_reg_t tp2863_mipi_1ch_2lane_297m_regs[] =
{
    {0x40, 0x08}, /* MIPI page */
    {0x02, 0x78},
    {0x03, 0x70},
    {0x04, 0x70},
    {0x05, 0x70},
    {0x06, 0x70},
    {0x13, 0xEF},
    {0x20, 0x00},
    {0x21, 0x12},
    {0x22, 0x30},
    {0x23, 0x9E}, /* CSI-2 data type 0x1E: YUV422 8-bit */

    /* MIPI_1CH2LANE_297M, matching the single locked decoder channel. */
    {0x21, 0x12},
    {0x14, 0x41},
    {0x15, 0x02},
    {0x2A, 0x04},
    {0x2B, 0x03},
    {0x2C, 0x09},
    {0x2E, 0x02},
    {0x10, 0xA0},
    {0x10, 0x20},
    {0x28, 0x02},
    {0x28, 0x00},
};

static const tp2863_reg_t tp2863_mipi_2ch_2lane_297m_regs[] =
{
    {0x40, 0x08}, /* MIPI page */
    {0x02, 0x78},
    {0x03, 0x70},
    {0x04, 0x70},
    {0x05, 0x70},
    {0x06, 0x70},
    {0x13, 0xEF},
    {0x20, 0x00},
    {0x21, 0x22},
    {0x22, 0x30},
    {0x23, 0x9E}, /* CSI-2 data type 0x1E: YUV422 8-bit */

    /* MIPI_2CH2LANE_297M, vendor-recommended mode for two 720p25/30 streams. */
    {0x21, 0x22},
    {0x14, 0x41},
    {0x15, 0x02},
    {0x2A, 0x04},
    {0x2B, 0x03},
    {0x2C, 0x09},
    {0x2E, 0x02},
    {0x10, 0xA0},
    {0x10, 0x20},
    {0x28, 0x02},
    {0x28, 0x00},
};

static avdk_err_t tp2863_write_reg(bk_camera_bus_t *bus, uint8_t reg, uint8_t val)
{
    avdk_err_t ret = bus->write8(bus, reg, val);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("write reg 0x%02x=0x%02x failed, ret=%d\n", reg, val, ret);
    }
    return ret;
}

static avdk_err_t tp2863_read_reg(bk_camera_bus_t *bus, uint8_t reg, uint8_t *val)
{
    avdk_err_t ret = bus->read8(bus, reg, val);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("read reg 0x%02x failed, ret=%d\n", reg, ret);
    }
    return ret;
}

static avdk_err_t tp2863_write_regs(bk_camera_bus_t *bus, const tp2863_reg_t *regs, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        AVDK_RETURN_ON_ERROR(tp2863_write_reg(bus, regs[i].reg, regs[i].val), TAG, "tp2863 write table failed");
    }

    return AVDK_ERR_OK;
}

static avdk_err_t tp2863_write_decoder_regs(bk_camera_bus_t *bus, uint8_t ch, const tp2863_reg_t *regs, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        uint8_t val = regs[i].val;

        if (regs[i].reg == 0x40)
        {
            val = ch;
        }
        else if (regs[i].reg == 0x42 && regs[i].val == 0x01)
        {
            uint8_t cur = 0;
            AVDK_RETURN_ON_ERROR(tp2863_read_reg(bus, regs[i].reg, &cur), TAG, "read decoder 0x42 failed");
            val = cur | ((ch == 0) ? 0x01 : 0x02);
        }
        else if (regs[i].reg == 0x42 && regs[i].val == 0x00)
        {
            uint8_t cur = 0;
            AVDK_RETURN_ON_ERROR(tp2863_read_reg(bus, regs[i].reg, &cur), TAG, "read decoder 0x42 failed");
            val = cur & ((ch == 0) ? 0xFE : 0xFD);
        }
        else if (regs[i].reg == 0x43 && regs[i].val == 0x00)
        {
            uint8_t cur = 0;
            AVDK_RETURN_ON_ERROR(tp2863_read_reg(bus, regs[i].reg, &cur), TAG, "read decoder 0x43 failed");
            val = cur & ((ch == 0) ? 0xFE : 0xFD);
        }

        AVDK_RETURN_ON_ERROR(tp2863_write_reg(bus, regs[i].reg, val), TAG, "tp2863 decoder table failed");
    }

    return AVDK_ERR_OK;
}

static avdk_err_t tp2863_enable_decoder(bk_camera_bus_t *bus)
{
    uint8_t reg06 = 0;

    AVDK_RETURN_ON_ERROR(tp2863_read_reg(bus, 0x06, &reg06), TAG, "read decoder enable failed");
    AVDK_RETURN_ON_ERROR(tp2863_write_reg(bus, 0x06, reg06 | 0x80), TAG, "enable decoder failed");

    return AVDK_ERR_OK;
}

static avdk_err_t tp2863_apply_mipi_dual_vc_map_on_bus(bk_camera_bus_t *bus)
{
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, "bus is NULL");

    AVDK_RETURN_ON_ERROR(tp2863_write_reg(bus, 0x40, 0x08), TAG, "select mipi page failed");
    AVDK_RETURN_ON_ERROR(tp2863_write_reg(bus, 0x26, TP2863_MIPI_VC_MAP_DUAL_FIXED),
                         TAG, "set fixed dual vc map failed");
    AVDK_RETURN_ON_ERROR(tp2863_write_reg(bus, 0x40, 0x00), TAG, "restore decoder page failed");
    LOGI("fixed dual vc map applied: ch1->vc0, ch2->vc1\n");

    return AVDK_ERR_OK;
}

static bool tp2863_decoder_is_locked(bk_camera_bus_t *bus, uint8_t ch)
{
    uint8_t status = 0;
    uint8_t detected = 0;

    tp2863_write_reg(bus, 0x40, ch);
    if (tp2863_read_reg(bus, 0x01, &status) != AVDK_ERR_OK)
    {
        return false;
    }

    if (tp2863_read_reg(bus, 0x03, &detected) != AVDK_ERR_OK)
    {
        return false;
    }

    LOGI("decoder ch%d status 0x01=0x%02x, 0x03=0x%02x\n", ch + 1, status, detected);

    /*
     * Some HD modes report a stable decoder with HLOCK/SLOCK/VDET set while
     * VLOCK is not asserted. Treat carrier present, no video loss, HLOCK,
     * SLOCK, and VDET as the practical lock condition.
     */
    return ((status & 0x80) == 0) && ((status & 0x38) == 0x38) && ((status & 0x01) == 0);
}

static void tp2863_select_input(bk_camera_bus_t *bus, uint8_t ch, tp2863_vin_t vin)
{
    uint8_t input = 0;

    tp2863_write_reg(bus, 0x40, ch);
    if (tp2863_read_reg(bus, 0x50, &input) == AVDK_ERR_OK)
    {
        input = (ch == 0) ? ((input & 0xFC) | (uint8_t)vin) : ((input & 0xF3) | (uint8_t)(vin << 2));
        tp2863_write_reg(bus, 0x50, input);
    }

    if (vin == TP2863_DIFF)
    {
        tp2863_write_reg(bus, 0x38, 0x4E);
        tp2863_write_reg(bus, 0x3D, 0x40);
    }
    else
    {
        tp2863_write_reg(bus, 0x38, 0x00);
        tp2863_write_reg(bus, 0x3D, 0x60);
    }
}

static bool tp2863_try_decoder_mode(bk_camera_bus_t *bus,
                                    const char *label,
                                    uint8_t ch,
                                    const tp2863_reg_t *regs,
                                    uint32_t size,
                                    tp2863_vin_t vin)
{
    LOGI("try decoder mode %s, ch=%d, vin=%d\n", label, ch + 1, vin);

    if (tp2863_write_decoder_regs(bus, ch, regs, size) != AVDK_ERR_OK)
    {
        return false;
    }

    tp2863_select_input(bus, ch, vin);

    if (tp2863_enable_decoder(bus) != AVDK_ERR_OK)
    {
        return false;
    }

    rtos_delay_milliseconds(150);

    return tp2863_decoder_is_locked(bus, ch);
}

static bool tp2863_scan_decoder_modes(bk_camera_bus_t *bus)
{
    typedef struct
    {
        const char *label;
        const tp2863_reg_t *regs;
        uint32_t size;
        uint8_t fps;
    } tp2863_decoder_mode_t;

    const tp2863_decoder_mode_t modes[] =
    {
        {"AHD 720p25", tp2863_ahd_720p25_init_regs, ARRAY_SIZE(tp2863_ahd_720p25_init_regs), TP2863_FPS_25},
#if (!TP2863_AHD_720P25)
        {"TVI 720p25", tp2863_tvi_720p25_init_regs, ARRAY_SIZE(tp2863_tvi_720p25_init_regs), TP2863_FPS_25},
        {"AHD 720p30", tp2863_ch1_ahd_720p30_init_regs, ARRAY_SIZE(tp2863_ch1_ahd_720p30_init_regs), TP2863_FPS_30},
        {"TVI 720p30", tp2863_ch1_tvi_720p30_init_regs, ARRAY_SIZE(tp2863_ch1_tvi_720p30_init_regs), TP2863_FPS_30},
        {"AHD 1080p30", tp2863_ahd_1080p30_init_regs, ARRAY_SIZE(tp2863_ahd_1080p30_init_regs), TP2863_FPS_30},
        {"TVI 1080p30", tp2863_tvi_1080p30_init_regs, ARRAY_SIZE(tp2863_tvi_1080p30_init_regs), TP2863_FPS_30},
        {"AHD 1080p25", tp2863_ahd_1080p25_init_regs, ARRAY_SIZE(tp2863_ahd_1080p25_init_regs), TP2863_FPS_25},
        {"TVI 1080p25", tp2863_tvi_1080p25_init_regs, ARRAY_SIZE(tp2863_tvi_1080p25_init_regs), TP2863_FPS_25},
#endif
    };

    os_memset(s_tp2863_locks, 0, sizeof(s_tp2863_locks));

    for (uint8_t ch = 0; ch < 2; ch++)
    {
        for (uint32_t mode = 0; mode < ARRAY_SIZE(modes); mode++)
        {
            for (tp2863_vin_t vin = TP2863_VINP; vin <= TP2863_DIFF; vin++)
            {
                if (tp2863_try_decoder_mode(bus, modes[mode].label, ch, modes[mode].regs, modes[mode].size, vin))
                {
                    LOGI("locked decoder mode %s, ch=%d, vin=%d\n", modes[mode].label, ch + 1, vin);
                    s_tp2863_locks[ch].locked = true;
                    s_tp2863_locks[ch].label = modes[mode].label;
                    s_tp2863_locks[ch].ch = ch;
                    s_tp2863_locks[ch].fps = modes[mode].fps;
                    s_tp2863_locks[ch].vin = vin;
                    break;
                }
            }

            if (s_tp2863_locks[ch].locked)
            {
                break;
            }
        }

        if (!s_tp2863_locks[ch].locked)
        {
            LOGW("decoder ch%d not locked\n", ch + 1);
        }
    }

    s_tp2863_dual_mode = s_tp2863_locks[0].locked && s_tp2863_locks[1].locked;
    LOGI("decoder lock summary: ch1=%d(%s), ch2=%d(%s), dual=%d\n",
         s_tp2863_locks[0].locked, s_tp2863_locks[0].label ? s_tp2863_locks[0].label : "none",
         s_tp2863_locks[1].locked, s_tp2863_locks[1].label ? s_tp2863_locks[1].label : "none",
         s_tp2863_dual_mode);

    return s_tp2863_locks[0].locked || s_tp2863_locks[1].locked;
}

static void tp2863_get_mipi_regs(const tp2863_reg_t **regs, uint32_t *size)
{
    if (s_tp2863_dual_mode)
    {
        *regs = tp2863_mipi_2ch_2lane_297m_regs;
        *size = ARRAY_SIZE(tp2863_mipi_2ch_2lane_297m_regs);
        return;
    }

    *regs = tp2863_mipi_1ch_2lane_297m_regs;
    *size = ARRAY_SIZE(tp2863_mipi_1ch_2lane_297m_regs);
}

static avdk_err_t tp2863_reset_gpio(bk_camera_sensor_config_t *config)
{
    if (config->pin_pwdn != TP2863_INVALID_GPIO)
    {
        BK_LOG_ON_ERR(bk_gpio_enable_output(config->pin_pwdn));
        bk_gpio_set_capacity(config->pin_pwdn, GPIO_DRIVER_CAPACITY_3);
        bk_gpio_set_output_low(config->pin_pwdn);
    }

    if (config->pin_reset != TP2863_INVALID_GPIO)
    {
        BK_LOG_ON_ERR(bk_gpio_enable_output(config->pin_reset));
        bk_gpio_set_capacity(config->pin_reset, GPIO_DRIVER_CAPACITY_3);
        bk_gpio_set_output_low(config->pin_reset);
        rtos_delay_milliseconds(5);
        bk_gpio_set_output_high(config->pin_reset);
        rtos_delay_milliseconds(20);
    }

    return AVDK_ERR_OK;
}

static avdk_err_t tp2863_try_address(bk_camera_bus_t *bus, uint16_t write_addr)
{
    uint8_t val = 0;

    bus->write_address = write_addr;
    return tp2863_read_reg(bus, 0x40, &val);
}

static int TP2863_Init(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    return 0;
}

static int TP2863_Exit(ISP_PORT IspPort)
{
    return 0;
}

static int TP2863_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
    return 0;
}

static int TP2863_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    return 0;
}

static int TP2863_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    return 0;
}

static int TP2863_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    return 0;
}

static int TP2863_SetIspDefault(ISP_PORT IspPort)
{
    return 0;
}

static int TP2863_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = TP2863_Init;
    pIspSnsFunc->pfnSensorExit    = TP2863_Exit;
    pIspSnsFunc->pfnWriteReg      = TP2863_WriteReg;
    pIspSnsFunc->pfnReadReg       = TP2863_ReadReg;
    pIspSnsFunc->pfnSetMode       = TP2863_SetMode;
    pIspSnsFunc->pfnSetStream     = TP2863_SetStream;
    pIspSnsFunc->pfnSetIspDefault = TP2863_SetIspDefault;
    return 0;
}

static int TP2863_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    return 0;
}

const ISP_SNS_OBJ_S snsTP2863Obj = {
    .pfnInitIspSnsFunc = TP2863_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = TP2863_InitAeSnsFunc,
};

static avdk_err_t tp2863_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");

    bk_camera_bus_t *bus = csi_sensor->config.bus;
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, "bus is NULL");
    const tp2863_reg_t *mipi_regs = NULL;
    uint32_t mipi_regs_size = 0;

    LOGI("%s\n", __func__);

    if (!tp2863_scan_decoder_modes(bus))
    {
        LOGW("no decoder mode locked\n");
    }

    tp2863_get_mipi_regs(&mipi_regs, &mipi_regs_size);
    LOGI("start %s mipi output\n", s_tp2863_dual_mode ? "2ch" : "1ch");
    AVDK_RETURN_ON_ERROR(tp2863_write_regs(bus, mipi_regs, mipi_regs_size), TAG, "mipi init failed");
    if (s_tp2863_dual_mode)
    {
        AVDK_RETURN_ON_ERROR(tp2863_apply_mipi_dual_vc_map_on_bus(bus),
                             TAG, "apply fixed dual vc map failed");
    }
    bk_mipi_csi_ext_set_enable(0);
    bk_mipi_csi_phy_term_set(0x303, 0x808);

    return AVDK_ERR_OK;
}

static avdk_err_t tp2863_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    if (width != TP2863_WIDTH || height != TP2863_HEIGHT)
    {
        LOGE("unsupported ppi %ux%u\n", width, height);
        return AVDK_ERR_UNSUPPORTED;
    }

    bk_mipi_csi_controller_init(width, height, TP2863_MIPI_DATA_TYPE_YUV422);

    return AVDK_ERR_OK;
}

static avdk_err_t tp2863_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    if (fps != TP2863_FPS_25 && fps != TP2863_FPS_30)
    {
        LOGE("unsupported fps %u\n", fps);
        return AVDK_ERR_UNSUPPORTED;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t tp2863_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, "bus is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    const tp2863_reg_t *mipi_regs = NULL;
    uint32_t mipi_regs_size = 0;

    LOGI("%s, width: %d, height: %d, fps: %d\n", __func__, format->width, format->height, format->fps);

    AVDK_RETURN_ON_ERROR(tp2863_set_ppi(controller, format->width, format->height), TAG, "set ppi failed");
    AVDK_RETURN_ON_ERROR(tp2863_set_fps(controller, format->fps), TAG, "set fps failed");

    bk_mipi_csi_controller_reset();

    *((volatile uint32_t *)(0x4c040000 + 0x0000155C)) = (6 << 1);
    *((volatile uint32_t *)(0x4c050000 + 0x000000AC)) = ((1 << 0) | (0x2A << 8));

    tp2863_get_mipi_regs(&mipi_regs, &mipi_regs_size);
    LOGI("restart %s mipi output\n", s_tp2863_dual_mode ? "2ch" : "1ch");
    AVDK_RETURN_ON_ERROR(tp2863_write_regs(bus, mipi_regs, mipi_regs_size), TAG, "restart mipi failed");
    return AVDK_ERR_OK;
}

static avdk_err_t tp2863_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");

    bk_camera_bus_t *bus = csi_sensor->config.bus;
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, "bus is NULL");

    if (cmd == CSI_SNS_READ)
    {
        uint8_t dump_val = 0;
        AVDK_RETURN_ON_ERROR(tp2863_read_reg(bus, (uint8_t)addr, &dump_val), TAG, "read reg failed");
        LOGI("tp2863 {0x%02x, 0x%02x}\n", addr, dump_val);
    }
    else if (cmd == CSI_SNS_WRITE)
    {
        LOGI("tp2863 {0x%02x, 0x%02x}\n", addr, val);
        AVDK_RETURN_ON_ERROR(tp2863_write_reg(bus, (uint8_t)addr, val), TAG, "write reg failed");
    }

    return AVDK_ERR_OK;
}

static avdk_err_t tp2863_set_hmirror(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    return AVDK_ERR_UNSUPPORTED;
}

static avdk_err_t tp2863_set_vflip(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    return AVDK_ERR_UNSUPPORTED;
}

static const csi_sensor_config_t csi_sensor_tp2863 =
{
    .name = "tp2863",
    .clk = MCLK_24M,
    .mipi_data_type = TP2863_MIPI_DATA_TYPE_YUV422,
    .default_width = TP2863_WIDTH,
    .default_height = TP2863_HEIGHT,
    .default_fps = TP2863_FPS_30,
    .id = 0,
    .address = (TP2863_I2C_WRITE_ADDR_SAD0 >> 1),
    .init = tp2863_init,
    .detect = tp2863_detect,
    .set_ppi = tp2863_set_ppi,
    .set_fps = tp2863_set_fps,
    .reg_ctrl = tp2863_ctrl,
};

static const ISP_PUB_ATTR_S tp2863_mipi_linear_attr = {
    .pSnsObj      = (void *)&snsTP2863Obj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_YUYV_SWAP,
    .snsFps       = TP2863_FPS_30 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t tp2863_format_array[] = {
    {
        .width = TP2863_WIDTH,
        .height = TP2863_HEIGHT,
        .fps = TP2863_FPS_30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_YUYV_SWAP,
    },
    {
        .width = TP2863_WIDTH,
        .height = TP2863_HEIGHT,
        .fps = TP2863_FPS_25,
        .output_pixel_fmt = BK_PIXEL_FORMAT_YUYV_SWAP,
    },
};

static avdk_err_t tp2863_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &tp2863_format_array[0];
    format_array->size = ARRAY_SIZE(tp2863_format_array);
    return AVDK_ERR_OK;
}

static void *tp2863_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void *)&snsTP2863Obj;
}

static void *tp2863_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void *)csi_sensor->sensor_config;
}

avdk_err_t tp2863_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");
    AVDK_RETURN_ON_FALSE(config->bus, AVDK_ERR_GENERIC, TAG, "bus is NULL");

    LOGI("%s, reset_pin: %d, pwdn_pin: %d\n", __func__, config->pin_reset, config->pin_pwdn);

    AVDK_RETURN_ON_ERROR(tp2863_reset_gpio(config), TAG, "reset failed");

    uint16_t detected_addr = TP2863_I2C_WRITE_ADDR_SAD0;
    if (tp2863_try_address(config->bus, TP2863_I2C_WRITE_ADDR_SAD0) != AVDK_ERR_OK)
    {
        if (tp2863_try_address(config->bus, TP2863_I2C_WRITE_ADDR_SAD1) != AVDK_ERR_OK)
        {
            if (config->pin_reset != TP2863_INVALID_GPIO)
            {
                bk_gpio_set_output_low(config->pin_reset);
            }
            return AVDK_ERR_GENERIC;
        }
        detected_addr = TP2863_I2C_WRITE_ADDR_SAD1;
    }

    LOGI("%s detect success, write_addr=0x%02x\n", TAG, detected_addr);

    bk_camera_csi_sensor_t *csi_sensor = os_malloc(sizeof(bk_camera_csi_sensor_t));
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(csi_sensor, 0, sizeof(bk_camera_csi_sensor_t));

    config->bus->write_address = detected_addr;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    csi_sensor->ops.init = tp2863_init;
    csi_sensor->ops.set_format = tp2863_set_format;
    csi_sensor->ops.reg_ctrl = tp2863_ctrl;
    csi_sensor->ops.set_hmirror = tp2863_set_hmirror;
    csi_sensor->ops.set_vflip = tp2863_set_vflip;
    csi_sensor->ops.get_sensor_object = tp2863_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = tp2863_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = tp2863_query_support_formats;

    csi_sensor->isp_pub_attr = &tp2863_mipi_linear_attr;
    csi_sensor->sensor_config = &csi_sensor_tp2863;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return AVDK_ERR_OK;
}

BK_CAMERA_SENSOR_DETECT_SECTION(tp2863_detect, CSI_CAMERA_PORT);
