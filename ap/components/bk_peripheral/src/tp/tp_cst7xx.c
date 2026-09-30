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

#include <driver/tp.h>
#include <driver/tp_types.h>
#include <os/mem.h>

#define TAG "cst7xx"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CST7XX_I2C_ADDRESS        (0x15)
#define CST7XX_STATUS_REG         (0x00)
#define CST7XX_INFO_REG           (0xA6)
#define CST7XX_NORMAL_MODE_REG    (0xFE)
#define CST7XX_NORMAL_MODE_VALUE  (0x00)
#define CST7XX_EXPECTED_CHIP_TYPE (0x13)
#define CST7XX_MAX_TOUCH_NUM      (2)
#define CST7XX_POINT_SIZE         (6)
#define CST7XX_REPORT_SIZE        (3 + CST7XX_POINT_SIZE * CST7XX_MAX_TOUCH_NUM)

#define SENSOR_I2C_READ(reg, buff, len) \
	cb->read_uint8(CST7XX_I2C_ADDRESS, reg, buff, len)
#define SENSOR_I2C_WRITE(reg, buff, len) \
	cb->write_uint8(CST7XX_I2C_ADDRESS, reg, buff, len)

static bool s_tp_down[CST7XX_MAX_TOUCH_NUM];
static uint16_t s_last_x[CST7XX_MAX_TOUCH_NUM];
static uint16_t s_last_y[CST7XX_MAX_TOUCH_NUM];

static void cst7xx_touch_up(tp_data_t *data, uint8_t slot)
{
	if (slot >= CST7XX_MAX_TOUCH_NUM || !s_tp_down[slot])
	{
		return;
	}

	data[slot].event = TP_EVENT_TYPE_UP;
	data[slot].track_id = slot;
	data[slot].x_coordinate = s_last_x[slot];
	data[slot].y_coordinate = s_last_y[slot];
	data[slot].timestamp = rtos_get_time();
	s_tp_down[slot] = false;
}

static void cst7xx_touch_down(tp_data_t *data, uint8_t slot,
	uint16_t x, uint16_t y, uint8_t event)
{
	if (slot >= CST7XX_MAX_TOUCH_NUM)
	{
		return;
	}

	data[slot].event = s_tp_down[slot] ? TP_EVENT_TYPE_MOVE : event;
	data[slot].track_id = slot;
	data[slot].x_coordinate = x;
	data[slot].y_coordinate = y;
	data[slot].width = 0;
	data[slot].timestamp = rtos_get_time();
	s_last_x[slot] = x;
	s_last_y[slot] = y;
	s_tp_down[slot] = true;
}

static bool cst7xx_detect(const tp_i2c_callback_t *cb)
{
	uint8_t info[6] = {0};

	if (cb == NULL)
	{
		LOGE("%s, pointer is null!\r\n", __func__);
		return false;
	}

	if (BK_OK != SENSOR_I2C_READ(CST7XX_INFO_REG, info, sizeof(info)))
	{
		LOGE("%s, read reg 0xA6 fail!\r\n", __func__);
		return false;
	}

	LOGI("%s, fw=0x%02X project=0x%02X chip=0x%02X\r\n",
		__func__, info[0], info[3], info[4]);

	if (info[4] != CST7XX_EXPECTED_CHIP_TYPE)
	{
		LOGE("%s, chip type 0x%02X != 0x%02X\r\n",
			__func__, info[4], CST7XX_EXPECTED_CHIP_TYPE);
		return false;
	}

	return true;
}

static int cst7xx_init(const tp_i2c_callback_t *cb,
	tp_sensor_user_config_t *config)
{
	uint8_t value = CST7XX_NORMAL_MODE_VALUE;

	if (cb == NULL || config == NULL)
	{
		LOGE("%s, pointer is null!\r\n", __func__);
		return BK_FAIL;
	}

	os_memset(s_tp_down, 0, sizeof(s_tp_down));
	os_memset(s_last_x, 0, sizeof(s_last_x));
	os_memset(s_last_y, 0, sizeof(s_last_y));

	if (BK_OK != SENSOR_I2C_WRITE(CST7XX_NORMAL_MODE_REG, &value, 1))
	{
		LOGE("%s, enter normal mode fail!\r\n", __func__);
		return BK_FAIL;
	}

	return BK_OK;
}

static int cst7xx_read_tp_info(const tp_i2c_callback_t *cb,
	uint8_t max_num, uint8_t *buff)
{
	uint8_t report[CST7XX_REPORT_SIZE] = {0};
	uint8_t reported_ids = 0;
	tp_data_t *data = (tp_data_t *)buff;

	if (cb == NULL || buff == NULL)
	{
		LOGE("%s, pointer is null!\r\n", __func__);
		return BK_FAIL;
	}

	if (max_num == 0)
	{
		LOGE("%s, max_num is 0!\r\n", __func__);
		return BK_FAIL;
	}

	if (BK_OK != SENSOR_I2C_READ(CST7XX_STATUS_REG, report, sizeof(report)))
	{
		LOGE("%s, read report fail!\r\n", __func__);
		return BK_FAIL;
	}

	uint8_t touch_num = report[2];
	if (touch_num > CST7XX_MAX_TOUCH_NUM)
	{
		LOGE("%s, invalid touch num %u\r\n", __func__, touch_num);
		return BK_FAIL;
	}

	for (uint8_t i = 0; i < touch_num; i++)
	{
		uint8_t offset = 3 + i * CST7XX_POINT_SIZE;
		uint8_t id = (report[offset + 2] >> 4) & 0x0F;
		uint8_t slot = (max_num == 1) ? 0 : id;
		uint8_t event = (report[offset] >> 6) & 0x03;
		uint16_t x = ((uint16_t)(report[offset] & 0x0F) << 8)
			| report[offset + 1];
		uint16_t y = ((uint16_t)(report[offset + 2] & 0x0F) << 8)
			| report[offset + 3];

		if (id >= CST7XX_MAX_TOUCH_NUM
			|| slot >= CST7XX_MAX_TOUCH_NUM
			|| slot >= max_num)
		{
			LOGE("%s, invalid touch id %u\r\n", __func__, id);
			continue;
		}

		LOGV("%s, id=%u slot=%u event=%u x=%u y=%u\r\n",
			__func__, id, slot, event, x, y);

		reported_ids |= (1U << slot);
		if (event == 1)
		{
			cst7xx_touch_up(data, slot);
		}
		else if (event == 0 || event == 2)
		{
			cst7xx_touch_down(data, slot, x, y,
				event == 0 ? TP_EVENT_TYPE_DOWN : TP_EVENT_TYPE_MOVE);
		}
		else
		{
			LOGD("%s, ignore event %u for id %u\r\n", __func__, event, id);
		}
	}

	uint8_t slot_num = (max_num < CST7XX_MAX_TOUCH_NUM)
		? max_num : CST7XX_MAX_TOUCH_NUM;
	for (uint8_t slot = 0; slot < slot_num; slot++)
	{
		if ((reported_ids & (1U << slot)) == 0)
		{
			cst7xx_touch_up(data, slot);
		}
	}

	LOGV("%s, touch_num=%u\r\n", __func__, touch_num);
	return BK_OK;
}

const tp_sensor_config_t tp_sensor_cst7xx =
{
	.name = "cst7xx",
	.def_ppi = PPI_400X400,
	.def_int_type = TP_INT_TYPE_FALLING_EDGE,
	.def_refresh_rate = 20,
	.def_tp_num = CST7XX_MAX_TOUCH_NUM,
	.id = TP_ID_CST7XX,
	.address = CST7XX_I2C_ADDRESS,
	.detect = cst7xx_detect,
	.init = cst7xx_init,
	.read_tp_info = cst7xx_read_tp_info,
};

const tp_sensor_config_t *cst7xx_detect_sensor(const tp_i2c_callback_t *cb)
{
	if (cst7xx_detect(cb))
	{
		return &tp_sensor_cst7xx;
	}

	return NULL;
}

BK_TP_SENSOR_DETECT_SECTION(cst7xx_detect_sensor);
