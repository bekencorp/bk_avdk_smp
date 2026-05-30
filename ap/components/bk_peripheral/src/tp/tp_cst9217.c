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
#include <os/os.h>
#include <driver/tp.h>
#include <driver/tp_types.h>


#define TAG "cst9217"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


// external statement.
extern void bk_mem_dump_ex(const char *title, unsigned char *data, uint32_t data_len);


// macro define
#define CST9217_WRITE_ADDRESS    (0xB4)
#define CST9217_READ_ADDRESS     (0xB5)
#define CST9217_PRODUCT_ID_CODE  (0x9217)
#define CST9217_FRM_VER_CODE     (0xCACA)

#define CST9217_MAX_TOUCH_NUM    (5)
#define CST9217_POINT_INFO_NUM   (TP_SUPPORT_MAX_NUM)
#define CST9217_POINT_INFO_SIZE  (5)
#define CST9217_POINT_INFO_TOTAL_SIZE  (CST9217_POINT_INFO_NUM * CST9217_POINT_INFO_SIZE)
#define CST9217_READ_BUFF_SIZE   (CST9217_POINT_INFO_TOTAL_SIZE + 2)

#define CST9217_TP_SET_XY_RATE_REG       (0xD1F8)
#define CST9217_TP_FRM_VER_CODE_REG      (0xD1FC)
#define CST9217_TP_CHIP_INFO_REG         (0xD204)
#define CST9217_TP_FW_EXTRA_REG          (0xD208)
#define CST9217_TP_MODULE_ID_REG         (0xD047)
#define CST9217_TP_POINT_ADDR_START_REG  (0xD000)

#define CST9217_TP_CMD_REG               (0xD1)
#define CST9217_TP_MODE_DEBUG_CMD        (0x01)
#define CST9217_TP_MODE_NORMAL_CMD       (0x09)

#define CST9217_POINT_VALID_CODE         (0xAB)
#define CST9217_POINT_ACK_CODE           (0xAB)

#define CST9217_REGS_DEBUG_EN (0)

#define CST9217_I2C_ADDR_DEFAULT         (CST9217_WRITE_ADDRESS >> 1)

#define SENSOR_I2C_READ(reg, buff, len)       cb->read_uint16(CST9217_I2C_ADDR_DEFAULT, reg, buff, len)
#define SENSOR_I2C_WRITE(reg, buff, len)      cb->write_uint16(CST9217_I2C_ADDR_DEFAULT, reg, buff, len)
#define SENSOR_I2C_WRITE_CMD(reg, buff, len)  cb->write_uint8(CST9217_I2C_ADDR_DEFAULT, reg, buff, len)

static int cst9217_exit_debug_to_normal(const tp_i2c_callback_t *cb)
{
    uint8_t scratch[8];
    uint8_t normal_mode = CST9217_TP_MODE_NORMAL_CMD;
    int ret = SENSOR_I2C_READ(CST9217_TP_FW_EXTRA_REG, scratch, sizeof(scratch));

    if (BK_OK != ret)
    {
        LOGE("%s, read 0x%04X fail!\r\n", __func__, CST9217_TP_FW_EXTRA_REG);
        return BK_FAIL;
    }

    scratch[0] = 0;
    ret = SENSOR_I2C_READ(CST9217_TP_MODULE_ID_REG, scratch, 1);
    if (BK_OK != ret)
    {
        LOGV("%s, read optional module id 0x%04X fail, continue normal mode switch\r\n",
             __func__, CST9217_TP_MODULE_ID_REG);
    }

    ret = SENSOR_I2C_WRITE_CMD(CST9217_TP_CMD_REG, &normal_mode, sizeof(normal_mode));
    if (BK_OK != ret)
    {
        uint8_t point_probe[8] = {0};
        uint8_t debug_probe[4] = {0};
        int point_ret = SENSOR_I2C_READ(CST9217_TP_POINT_ADDR_START_REG, point_probe, 7);
        int debug_ret = SENSOR_I2C_READ(CST9217_TP_FRM_VER_CODE_REG, debug_probe, sizeof(debug_probe));

        if ((BK_OK == point_ret) && (BK_OK == debug_ret)
            && (0x00 == debug_probe[0]) && (0x00 == debug_probe[1])
            && (0x00 == debug_probe[2]) && (0x00 == debug_probe[3]))
        {
            LOGW("%s, normal mode command no ack but state probe indicates normal mode\r\n", __func__);
            return BK_OK;
        }

        LOGE("%s, i2c addr 0x%02X reg 0x%02X write 0x%02X no ack, enter normal mode fail!\r\n",
             __func__, CST9217_I2C_ADDR_DEFAULT, CST9217_TP_CMD_REG, normal_mode);
        return BK_FAIL;
    }

    return BK_OK;
}

static bool cst9217_detect(const tp_i2c_callback_t *cb)
{
    if (NULL == cb)
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return false;
    }

    uint8_t info[4] = {0};
    uint32_t check_code = 0;
    uint16_t product_id = 0;

    uint8_t enter_debug = CST9217_TP_MODE_DEBUG_CMD;
    int ret = SENSOR_I2C_WRITE_CMD(CST9217_TP_CMD_REG, &enter_debug, sizeof(enter_debug));
    if (BK_OK != ret)
    {
        LOGE("%s, i2c addr 0x%02X reg 0x%02X write 0x%02X no ack, enter debug mode fail!\r\n",
                __func__, CST9217_I2C_ADDR_DEFAULT, CST9217_TP_CMD_REG, enter_debug);
        return false;
    }
    LOGV("%s, i2c addr 0x%02X ack\r\n", __func__, CST9217_I2C_ADDR_DEFAULT);

    ret = SENSOR_I2C_READ(CST9217_TP_FRM_VER_CODE_REG, info, sizeof(info));
    if (BK_OK != ret)
    {
        LOGE("%s, read verification code reg fail!\r\n", __func__);
        return false;
    }

    check_code = ((uint32_t)info[3] << 24) | ((uint32_t)info[2] << 16) | ((uint32_t)info[1] << 8) | info[0];
    LOGV("%s, check code: 0x%08X\r\n", __func__, check_code);

    if (((check_code >> 16) & 0xFFFF) != CST9217_FRM_VER_CODE)
    {
        return false;
    }

    ret = SENSOR_I2C_READ(CST9217_TP_CHIP_INFO_REG, info, sizeof(info));
    if (BK_OK != ret)
    {
        LOGE("%s, read chip info reg fail!\r\n", __func__);
        return false;
    }

    product_id = ((uint16_t)info[3] << 8) | info[2];
    LOGI("%s, product id: 0x%04X\r\n", __func__, product_id);

    if (CST9217_PRODUCT_ID_CODE == product_id)
    {
        LOGI("%s success\r\n", __func__);
        return true;
    }

    return false;
}

static int cst9217_init(const tp_i2c_callback_t *cb, tp_sensor_user_config_t *config)
{
    if ((NULL == cb) || (NULL == config))
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return BK_FAIL;
    }

    uint8_t rate[4];

    if (BK_OK != cst9217_exit_debug_to_normal(cb))
    {
        return BK_FAIL;
    }

    rate[0] = config->x_size & 0xFF;
    rate[1] = (config->x_size >> 8) & 0xFF;
    rate[2] = config->y_size & 0xFF;
    rate[3] = (config->y_size >> 8) & 0xFF;

    if (BK_OK != SENSOR_I2C_WRITE(CST9217_TP_SET_XY_RATE_REG, rate, sizeof(rate)))
    {
        LOGE("%s, set xy resolution fail!\r\n", __func__);
        return BK_FAIL;
    }

    return BK_OK;
}

static int16_t pre_x[CST9217_MAX_TOUCH_NUM] = {-1, -1, -1, -1, -1};
static int16_t pre_y[CST9217_MAX_TOUCH_NUM] = {-1, -1, -1, -1, -1};
static int16_t pre_w[CST9217_MAX_TOUCH_NUM] = {-1, -1, -1, -1, -1};
static uint8_t s_tp_down[CST9217_MAX_TOUCH_NUM];

static void cst9217_touch_up(void *buf, uint8_t id)
{
    tp_data_t *read_data = (tp_data_t *)buf;

    if (s_tp_down[id])
    {
        s_tp_down[id] = 0;
        read_data[id].event = TP_EVENT_TYPE_UP;
    }
    else
    {
        read_data[id].event = TP_EVENT_TYPE_NONE;
    }

    read_data[id].timestamp = rtos_get_time();
    read_data[id].width = (pre_w[id] < 0) ? 0 : pre_w[id];
    read_data[id].x_coordinate = (pre_x[id] < 0) ? 0 : pre_x[id];
    read_data[id].y_coordinate = (pre_y[id] < 0) ? 0 : pre_y[id];
    read_data[id].track_id = id;

    pre_x[id] = -1;
    pre_y[id] = -1;
    pre_w[id] = -1;
}

static void cst9217_touch_down(void *buf, uint8_t id, int16_t x, int16_t y, int16_t w)
{
    tp_data_t *read_data = (tp_data_t *)buf;

    if (s_tp_down[id])
    {
        read_data[id].event = TP_EVENT_TYPE_MOVE;
    }
    else
    {
        read_data[id].event = TP_EVENT_TYPE_DOWN;
        s_tp_down[id] = 1;
    }

    read_data[id].timestamp = rtos_get_time();
    read_data[id].width = w;
    read_data[id].x_coordinate = x;
    read_data[id].y_coordinate = y;
    read_data[id].track_id = id;

    pre_x[id] = x;
    pre_y[id] = y;
    pre_w[id] = w;
}

static void cst9217_release_all_points(void *buf)
{
    for (uint8_t id = 0; id < CST9217_MAX_TOUCH_NUM; id++)
    {
        if (s_tp_down[id])
        {
            cst9217_touch_up(buf, id);
        }
    }
}

static void cst9217_read_point(uint8_t *input_buff, void *buf, uint8_t num)
{
    uint8_t *read_buf = input_buff;
    uint8_t touch_num = num;
    uint8_t reported_ids = 0;

    if (touch_num == 0)
    {
        cst9217_release_all_points(buf);
        return;
    }

    for (uint8_t read_index = 0; read_index < touch_num; read_index++)
    {
        uint8_t off_set = read_index * CST9217_POINT_INFO_SIZE;
        uint8_t event_flag = (read_buf[off_set] & 0x0F) >> 1;
        uint8_t read_id = (read_buf[off_set] >> 4) & 0x0F;
        int16_t input_x = ((uint16_t)read_buf[off_set + 1] << 4) | ((read_buf[off_set + 3] >> 4) & 0x0F);
        int16_t input_y = ((uint16_t)read_buf[off_set + 2] << 4) | (read_buf[off_set + 3] & 0x0F);
        int16_t input_w = read_buf[off_set + 4];

        if (read_id >= CST9217_MAX_TOUCH_NUM)
        {
            LOGE("%s, touch ID %d is out range!\r\n", __func__, read_id);
            continue;
        }

        if ((event_flag != 0x03) && (event_flag != 0x00))
        {
            LOGE("%s, invalid touch event 0x%02X, id=%d\r\n", __func__, event_flag, read_id);
            continue;
        }

        reported_ids |= (1U << read_id);

        if (event_flag == 0x03)
        {
            cst9217_touch_down(buf, read_id, input_x, input_y, input_w);
        }
        else
        {
            cst9217_touch_up(buf, read_id);
        }
    }

    for (uint8_t id = 0; id < CST9217_MAX_TOUCH_NUM; id++)
    {
        if (s_tp_down[id] && ((reported_ids & (1U << id)) == 0))
        {
            cst9217_touch_up(buf, id);
        }
    }
}

static uint8_t read_buff[CST9217_READ_BUFF_SIZE];
static int cst9217_read_tp_info(const tp_i2c_callback_t *cb, uint8_t max_num, uint8_t *buff)
{
    if ((NULL == cb) || (NULL == buff))
    {
        LOGE("%s, pointer is null!\r\n", __func__);
        return BK_FAIL;
    }

    if ((0 == max_num) || (max_num > CST9217_POINT_INFO_NUM))
    {
        LOGE("%s, max_num %d is out range!\r\n", __func__, max_num);
        return BK_FAIL;
    }

    int ret = BK_OK;
    uint8_t point_num = 0;
    uint8_t report_num = 0;
    uint8_t ack = CST9217_POINT_ACK_CODE;

    os_memset(read_buff, 0x00, sizeof(read_buff));
    if (BK_OK != SENSOR_I2C_READ(CST9217_TP_POINT_ADDR_START_REG, read_buff, 7))
    {
        LOGE("%s, read tp info fail!\r\n", __func__);
        return BK_FAIL;
    }

    if ((read_buff[6] != CST9217_POINT_VALID_CODE) || (read_buff[0] == CST9217_POINT_VALID_CODE))
    {
        LOGD("%s, invalid point data: head=0x%02X, tail=0x%02X\r\n", __func__, read_buff[0], read_buff[6]);
        goto exit_;
    }

    point_num = read_buff[5] & 0x7F;
    if (point_num > CST9217_MAX_TOUCH_NUM)
    {
        LOGE("%s, point num %d is out range!\r\n", __func__, point_num);
        ret = BK_FAIL;
        goto exit_;
    }

    report_num = (point_num > max_num) ? max_num : point_num;

    if (point_num > 1)
    {
        uint8_t read_len = (point_num - 1) * CST9217_POINT_INFO_SIZE + 1;

        if (BK_OK != SENSOR_I2C_READ(CST9217_TP_POINT_ADDR_START_REG + 7, &read_buff[5], read_len))
        {
            LOGE("%s, read multi-point info fail!\r\n", __func__);
            ret = BK_FAIL;
            goto exit_;
        }

        if (read_buff[point_num * CST9217_POINT_INFO_SIZE] != CST9217_POINT_VALID_CODE)
        {
            LOGD("%s, invalid multi-point tail=0x%02X\r\n",
                 __func__, read_buff[point_num * CST9217_POINT_INFO_SIZE]);
            goto exit_;
        }
    }

#if (CST9217_REGS_DEBUG_EN > 0)
    bk_mem_dump_ex("cst9217", (unsigned char *)read_buff, sizeof(read_buff));
#endif

    cst9217_read_point(read_buff, buff, report_num);

exit_:
    if (BK_OK != SENSOR_I2C_WRITE(CST9217_TP_POINT_ADDR_START_REG, &ack, sizeof(ack)))
    {
        LOGE("%s, write point ack fail!\r\n", __func__);
        ret = BK_FAIL;
    }

    return ret;
}

const tp_sensor_config_t tp_sensor_cst9217 =
{
    .name = "cst9217",
    .def_ppi = PPI_400X400,
    .def_int_type = TP_INT_TYPE_FALLING_EDGE,
    .def_refresh_rate = 10,
    .def_tp_num = 5,
    .id = TP_ID_CST9217,
    .address = CST9217_I2C_ADDR_DEFAULT,
    .detect = cst9217_detect,
    .init = cst9217_init,
    .read_tp_info = cst9217_read_tp_info,
};

// Detection function wrapper for section registration
const tp_sensor_config_t *cst9217_detect_sensor(const tp_i2c_callback_t *cb)
{
    if (cst9217_detect(cb))
    {
        return &tp_sensor_cst9217;
    }

    return NULL;
}

BK_TP_SENSOR_DETECT_SECTION(cst9217_detect_sensor);

