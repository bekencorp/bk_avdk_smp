// Copyright 2020-2021 Beken
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

#include <driver/int.h>
#include <os/mem.h>
#include <components/tp_driver.h>
#include <components/tp_types.h>
#include "tp_sensor_devices.h"


#define TAG "cst328"
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


// external statement.
extern void bk_mem_dump_ex(const char *title, unsigned char *data, uint32_t data_len);


// macro define
#define CST328_WRITE_ADDRESS     (0x34)
#define CST328_READ_ADDRESS      (0x35)

#define CST328_POINT_INFO_NUM    (TP_SUPPORT_MAX_NUM)
#define CST328_POINT_INFO_SIZE   (5)
#define CST328_POINT_INFO_TOTAL_SIZE  (CST328_POINT_INFO_NUM * CST328_POINT_INFO_SIZE)

#define CST328_FRM_VER_CODE                 (0xCACA)
#define CST328_TP_FRM_VER_CODE_REG          (0xD1FC)
#define CST328_TP_MODE_DEBUG_REG            (0xD101)
#define CST328_TP_MODE_NORMAL_REG           (0xD109)
#define CST328_TP_SET_XY_RATE_REG           (0xD1F8)
#define CST328_TP_POINT_ADDR_START_REG      (0xD000)

#define CST328_REGS_DEBUG_EN (0)

#define SENSOR_I2C_READ(reg, buff, len)  cb->read_uint16((CST328_WRITE_ADDRESS >> 1), reg, buff, len)
#define SENSOR_I2C_WRITE(reg, buff, len)  cb->write_uint16((CST328_WRITE_ADDRESS >> 1), reg, buff, len)


static bool cst328_detect(const tp_i2c_callback_t *cb)
{
    if (NULL == cb)
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return false;
    }

    uint32_t product_id = 0;

    if (BK_OK != SENSOR_I2C_WRITE(CST328_TP_MODE_DEBUG_REG, (uint8_t *)(&product_id), 1))
    {
        LOGE("%s, write 0XD101 fail!\r\n", __func__);
        return false;
    }

    if (BK_OK != SENSOR_I2C_READ(CST328_TP_FRM_VER_CODE_REG, (uint8_t *)(&product_id), sizeof(product_id)))
    {
        LOGE("%s, read verification code reg fail!\r\n", __func__);
        return false;
    }

    LOGD("%s, product id: 0x%x\r\n", __func__, product_id);

    if (((product_id >> 16) & 0xFFFF) == CST328_FRM_VER_CODE)
    {
        LOGE("%s success\n", __func__);
        return true;
    }

    return false;
}

static int cst328_init(const tp_i2c_callback_t *cb, tp_sensor_user_config_t *config)
{
    if ((NULL == cb) || (NULL == config))
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return BK_FAIL;
    }

    //进入正常读点模式
    uint8_t mode = 0x00;
    SENSOR_I2C_WRITE(CST328_TP_MODE_NORMAL_REG, &mode, sizeof(mode));
    //设置XY分辨率
    uint32_t rate = (config->y_size) | config->x_size;
    LOGE("%s, x:%d,y:%d!\r\n", __func__, config->x_size, config->y_size);
    SENSOR_I2C_WRITE(CST328_TP_SET_XY_RATE_REG, (uint8_t*)(&rate), sizeof(rate));

    return BK_OK;
}

// cst328 get tp info.
static void cst328_read_point(uint8_t *input_buff, void *buf, uint8_t num)
{
    static uint8_t last_event = TP_EVENT_TYPE_NONE;
    uint8_t curr_event = ((input_buff[0] & 0x0f) == 0x06) ? TP_EVENT_TYPE_DOWN : TP_EVENT_TYPE_UP;
    if(curr_event == TP_EVENT_TYPE_DOWN)
    {
        if((last_event == TP_EVENT_TYPE_DOWN) || (last_event == TP_EVENT_TYPE_MOVE))
            curr_event = TP_EVENT_TYPE_MOVE;
    }
    else if(curr_event == TP_EVENT_TYPE_UP)
    {
        if(last_event == TP_EVENT_TYPE_UP)
            curr_event = TP_EVENT_TYPE_NONE;
    }
    last_event = curr_event;

    uint8_t read_id = 0;
    tp_data_t *read_data = (tp_data_t *)buf;
    read_data[read_id].event = curr_event;
    read_data[read_id].timestamp = rtos_get_time();
    read_data[read_id].width = 0;
    read_data[read_id].x_coordinate = (input_buff[1] << 4) + ((input_buff[3] >> 4) & 0x0f);
    read_data[read_id].y_coordinate = (input_buff[2] << 4) + (input_buff[3] & 0x0f);
    read_data[read_id].track_id = read_id;
    LOGD("%s, read curr_event=0x%x,x=%d,y=%d!\r\n", __func__, curr_event, read_data[read_id].x_coordinate, read_data[read_id].y_coordinate);
}

static int cst328_read_tp_info(const tp_i2c_callback_t *cb, uint8_t max_num, uint8_t *buff)
{
    uint8_t read_buff[CST328_POINT_INFO_TOTAL_SIZE];
    if ((NULL == cb) || (NULL == buff))
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return BK_FAIL;
    }

    if ((0 == max_num) || (max_num > CST328_POINT_INFO_NUM))
    {
        LOGE("%s, max_num %d is out range!\r\n", __func__, max_num);
        return BK_FAIL;
    }

    int ret = BK_OK;
    uint8_t gesture_status = 0;
    uint8_t finger_status = 0;
    uint8_t temp_status = 0;

    os_memset(read_buff, 0x00, sizeof(read_buff));
    if (BK_OK != SENSOR_I2C_READ(CST328_TP_POINT_ADDR_START_REG, (uint8_t *)read_buff, sizeof(read_buff)))
    {
        LOGE("%s, read tp info fail!\r\n", __func__);
        ret = BK_FAIL;
        goto exit_;
    }

    gesture_status = read_buff[1];
    finger_status = read_buff[2];
    temp_status = read_buff[3];

    // original registers datas.
    LOGD("%s, gesture_status=0x%02X, finger_status=0x%02X, temp_status=0x%02X\r\n", __func__, gesture_status, finger_status, temp_status >> 4);

#if (CST328_REGS_DEBUG_EN > 0)
    bk_mem_dump_ex("cst328", (unsigned char *)(read_buff), sizeof(read_buff));
#endif

    cst328_read_point(read_buff, buff, CST328_POINT_INFO_NUM);

exit_:

    return ret;
}

const tp_sensor_config_t tp_sensor_cst328 =
{
    .name = "cst328",
    .width = 412,
    .height = 960,
    .def_int_type = TP_INT_TYPE_FALLING_EDGE,
    .def_refresh_rate = 10,
    .def_tp_num = 2,
    .id = TP_ID_CST328,
    .address = (CST328_WRITE_ADDRESS >> 1),
    .detect = cst328_detect,
    .init = cst328_init,
    .read_tp_info = cst328_read_tp_info,
};

const tp_sensor_config_t *cst328_detect_sensor(const tp_i2c_callback_t *cb)
{
	if (cst328_detect(cb))
	{
		return &tp_sensor_cst328;
	}

	return NULL;
}

BK_TP_SENSOR_DETECT_SECTION(cst328_detect_sensor);
