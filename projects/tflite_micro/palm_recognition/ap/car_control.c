/**
 * @file car_control.c
 * @brief UART command implementation for car and gimbal control.
 *
 * Sends fixed-length binary frames (defined in car_control.h) to the car controller over UART.
 * UART id and baud rate are set via car_control_init(&config).
 */

#include <common/bk_include.h>
#include <driver/uart.h>
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "car_control.h"

#define TAG "car-ctrl"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define CAR_CONTROL_DEFAULT_UART_ID   UART_ID_1
#define CAR_CONTROL_DEFAULT_BAUD_RATE 115200

static uint8_t s_uart_inited = 0;
static uart_id_t s_uart_id = CAR_CONTROL_DEFAULT_UART_ID;

bk_err_t car_control_send(const uint8_t *data, uint32_t len)
{
    if (!s_uart_inited)
    {
        LOGE("car_control not inited\n");
        return BK_ERR_UART_ID_NOT_INIT;
    }
    if (data == NULL || len == 0)
    {
        return BK_ERR_PARAM;
    }
    LOGI("car_control_send data=%p len=%d\n", data, len);
    return bk_uart_write_bytes(s_uart_id, data, len);
}

bk_err_t car_control_init(const car_control_config_t *config)
{
    if (s_uart_inited)
    {
        LOGW("car_control already inited\n");
        return BK_OK;
    }

    if (config != NULL)
    {
        s_uart_id = config->uart_id;
    }
    else
    {
        s_uart_id = CAR_CONTROL_DEFAULT_UART_ID;
    }

    uint32_t baud = (config != NULL && config->baud_rate > 0)
                    ? config->baud_rate
                    : CAR_CONTROL_DEFAULT_BAUD_RATE;


    /* TODO: uart GPIO mapping */
    gpio_dev_unmap(GPIO_20);
    gpio_dev_unmap(GPIO_21);
    gpio_dev_map(GPIO_20, GPIO_DEV_UART1_TXD);
    gpio_dev_map(GPIO_21, GPIO_DEV_UART1_RXD);
    bk_gpio_pull_up(GPIO_20);
    bk_gpio_pull_up(GPIO_21);

    uart_config_t uart_cfg = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_FLOWCTRL_DISABLE,
        .src_clk = UART_SCLK_XTAL_26M,
        .rx_dma_en = UART_DMA_DISABLE,
        .tx_dma_en = UART_DMA_DISABLE,
    };

    bk_err_t ret = bk_uart_init(s_uart_id, &uart_cfg);
    if (ret != BK_OK)
    {
        LOGE("bk_uart_init failed %d\n", ret);
        return ret;
    }

    //bk_uart_enable_tx_interrupt(s_uart_id);

    s_uart_inited = 1;
    LOGI("car control UART inited, id=%d baud=%u\n", s_uart_id, (unsigned int)baud);
    return BK_OK;
}

bk_err_t car_control_move_left(void)
{
    return car_control_send(CAR_CMD_MOVE_LEFT, CAR_CMD_MOVE_LEFT_LEN);
}

bk_err_t car_control_move_right(void)
{
    return car_control_send(CAR_CMD_MOVE_RIGHT, CAR_CMD_MOVE_RIGHT_LEN);
}

bk_err_t car_control_move_forward(void)
{
    return car_control_send(CAR_CMD_MOVE_FORWARD, CAR_CMD_MOVE_FORWARD_LEN);
}

bk_err_t car_control_move_backward(void)
{
    return car_control_send(CAR_CMD_MOVE_BACKWARD, CAR_CMD_MOVE_BACKWARD_LEN);
}

bk_err_t car_control_gimbal_up(void)
{
    return car_control_send(CAR_CMD_GIMBAL_UP, CAR_CMD_GIMBAL_UP_LEN);
}

bk_err_t car_control_gimbal_down(void)
{
    return car_control_send(CAR_CMD_GIMBAL_DOWN, CAR_CMD_GIMBAL_DOWN_LEN);
}
