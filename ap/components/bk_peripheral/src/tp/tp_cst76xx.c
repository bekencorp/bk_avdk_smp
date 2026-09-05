// Copyright 2020-2021 Beken
//
// Hynitron CST76xx touch driver (HYT7760 / HYT7864 / CST6960 series).

#include <driver/int.h>
#include <os/mem.h>
#include <os/os.h>
#include <driver/tp.h>
#include <driver/tp_types.h>
#include "tp_i2c_ext.h"

#define TAG "cst76xx"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...)

#define CST76XX_I2C_ADDR          (0x5A)
#define CST76XX_REG_INFO          (0xD0030000U)
#define CST76XX_REG_POINT         (0xD0070000U)
#define CST76XX_REG_ACK           (0xD00002ABU)
#define CST76XX_REG_NORM_0        (0xD0000000U)
#define CST76XX_REG_NORM_1        (0xD0000100U)
#define CST76XX_REG_LP_DISABLE    (0xD0000400U)

#define CST76XX_FRM_MAGIC_HI      (0xCA)
#define CST76XX_FRM_MAGIC_LO      (0xCA)
#define CST76XX_MAX_POINTS        (5)
#define CST76XX_REPORT_HDR_BYTES  (9)
#define CST76XX_POINT_BYTES       (5)
#define CST76XX_REPORT_BUF_SIZE   \
	(CST76XX_REPORT_HDR_BYTES + CST76XX_MAX_POINTS * CST76XX_POINT_BYTES)

static uint16_t cst76xx_sum16(uint16_t seed, const uint8_t *buf, uint16_t len)
{
	uint16_t sum = seed;

	while (len-- > 0) {
		sum += *buf++;
	}
	return sum;
}

static int cst76xx_set_normal_mode(void)
{
	if (tp_i2c_wr_reg(CST76XX_I2C_ADDR, CST76XX_REG_LP_DISABLE,
	                  4, NULL, 0) != BK_OK) {
		return BK_FAIL;
	}
	if (tp_i2c_wr_reg(CST76XX_I2C_ADDR, CST76XX_REG_NORM_0,
	                  4, NULL, 0) != BK_OK) {
		return BK_FAIL;
	}
	if (tp_i2c_wr_reg(CST76XX_I2C_ADDR, CST76XX_REG_NORM_1,
	                  4, NULL, 0) != BK_OK) {
		return BK_FAIL;
	}
	return BK_OK;
}

static bool cst76xx_detect(const tp_i2c_callback_t *cb)
{
	uint8_t buf[50];

	(void)cb;
	rtos_delay_milliseconds(20);

	if (cst76xx_set_normal_mode() != BK_OK) {
		LOGD("%s, set normal mode fail\r\n", __func__);
		return false;
	}

	if (tp_i2c_wr_reg(CST76XX_I2C_ADDR, CST76XX_REG_INFO,
	                  4, buf, sizeof(buf)) != BK_OK) {
		LOGE("%s, read info reg fail\r\n", __func__);
		return false;
	}

	if (buf[2] != CST76XX_FRM_MAGIC_HI ||
	    buf[3] != CST76XX_FRM_MAGIC_LO) {
		LOGD("%s, magic mismatch: %02X %02X\r\n",
		     __func__, buf[2], buf[3]);
		return false;
	}

	LOGI("%s: chip_type=0x%02X%02X%02X%02X "
	     "fw_ver=0x%02X%02X%02X%02X res=%ux%u\r\n",
	     __func__, buf[3], buf[2], buf[1], buf[0],
	     buf[35], buf[34], buf[33], buf[32],
	     (unsigned)((buf[29] << 8) | buf[28]),
	     (unsigned)((buf[31] << 8) | buf[30]));
	return true;
}

static int cst76xx_init(const tp_i2c_callback_t *cb,
                        tp_sensor_user_config_t *config)
{
	(void)cb;
	(void)config;

	if (cst76xx_set_normal_mode() != BK_OK) {
		LOGE("%s, normal mode failed\r\n", __func__);
		return BK_FAIL;
	}
	return BK_OK;
}

static int cst76xx_send_ack(void)
{
	return tp_i2c_wr_reg(CST76XX_I2C_ADDR, CST76XX_REG_ACK,
	                     4, NULL, 0);
}

static int cst76xx_read_points(uint8_t *buff)
{
	uint8_t buf[CST76XX_REPORT_BUF_SIZE];
	tp_data_t *read_data = (tp_data_t *)buff;
	int ret;
	int retry = 2;
	uint8_t finger_num;
	uint8_t key_num;
	uint8_t report_typ;
	uint8_t total_pts;
	uint16_t index;

	while (retry-- > 0) {
		ret = tp_i2c_wr_reg(CST76XX_I2C_ADDR, CST76XX_REG_POINT,
		                    4, buf, CST76XX_REPORT_HDR_BYTES);
		if (ret != BK_OK) {
			continue;
		}

		report_typ = buf[2];
		finger_num = buf[3] & 0x0F;
		key_num = (buf[3] & 0xF0) >> 4;
		total_pts = finger_num + key_num;

		if (total_pts > CST76XX_MAX_POINTS) {
			LOGV("%s, ignore invalid point count f=%u k=%u\r\n",
			     __func__, finger_num, key_num);
			ret = BK_FAIL;
			continue;
		}

		if (total_pts > 1) {
			uint16_t extra =
				(uint16_t)((total_pts - 1) * CST76XX_POINT_BYTES);

			if ((CST76XX_REPORT_HDR_BYTES + extra) > sizeof(buf)) {
				ret = BK_FAIL;
				continue;
			}
			ret = tp_i2c_read_raw(
				CST76XX_I2C_ADDR,
				&buf[CST76XX_REPORT_HDR_BYTES], extra);
			if (ret != BK_OK) {
				continue;
			}
		}

		if (cst76xx_sum16(
			    0x55, &buf[4],
			    (uint16_t)(total_pts * CST76XX_POINT_BYTES)) !=
		    (uint16_t)(buf[0] | (buf[1] << 8))) {
			ret = BK_FAIL;
			continue;
		}

		ret = BK_OK;
		break;
	}

	if (ret != BK_OK) {
		(void)cst76xx_send_ack();
		return BK_OK;
	}

	(void)cst76xx_send_ack();

	if (report_typ != 0xFF || total_pts == 0) {
		return BK_OK;
	}

	for (uint8_t i = 0; i < finger_num; i++) {
		uint8_t pos_id;
		uint8_t event;
		uint16_t x;
		uint16_t y;

		index = (uint16_t)((key_num + i) * CST76XX_POINT_BYTES);
		if ((index + 8) >= sizeof(buf)) {
			break;
		}

		pos_id = buf[index + 8] & 0x0F;
		event = buf[index + 8] >> 4;
		x = (uint16_t)(buf[index + 4] +
		               ((buf[index + 7] & 0x0F) << 8));
		y = (uint16_t)(buf[index + 5] +
		               ((buf[index + 7] & 0xF0) << 4));

		if (pos_id >= TP_SUPPORT_MAX_NUM) {
			continue;
		}

		read_data[pos_id].event =
			(event == 0) ? TP_EVENT_TYPE_UP : TP_EVENT_TYPE_DOWN;
		read_data[pos_id].timestamp = rtos_get_time();
		read_data[pos_id].width = 0;
		read_data[pos_id].x_coordinate = x;
		read_data[pos_id].y_coordinate = y;
		read_data[pos_id].track_id = pos_id;
	}

	return BK_OK;
}

static int cst76xx_read_tp_info(const tp_i2c_callback_t *cb,
                                uint8_t max_num, uint8_t *buff)
{
	(void)cb;

	if (buff == NULL || max_num == 0) {
		return BK_FAIL;
	}

	os_memset(buff, 0, max_num * sizeof(tp_data_t));
	return cst76xx_read_points(buff);
}

const tp_sensor_config_t tp_sensor_cst76xx = {
	.name = "cst76xx",
	.def_ppi = PPI_720X1280,
	.def_int_type = TP_INT_TYPE_FALLING_EDGE,
	.def_refresh_rate = 10,
	.def_tp_num = 5,
	.id = TP_ID_CST76XX,
	.address = CST76XX_I2C_ADDR,
	.detect = cst76xx_detect,
	.init = cst76xx_init,
	.read_tp_info = cst76xx_read_tp_info,
};

const tp_sensor_config_t *cst76xx_detect_sensor(
	const tp_i2c_callback_t *cb)
{
	if (cst76xx_detect(cb)) {
		return &tp_sensor_cst76xx;
	}
	return NULL;
}

BK_TP_SENSOR_DETECT_SECTION(cst76xx_detect_sensor);
