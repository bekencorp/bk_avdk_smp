/**
 * @brief UART-based car and gimbal control module.
 *
 * Sends fixed UART command frames to control: move left/right/forward/backward, gimbal up/down.
 * UART id and baud rate are set via car_control_init(&config); pass NULL for default (UART0, 9600).
 */

#pragma once

#include <common/bk_include.h>
#include <driver/uart_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- UART command definitions (fixed frames for car protocol) ---------- */

/* Left move: 03 45 31 32 35 30 39 44 56 31 30 31 36 09 00 0c 06 00 00 00 00 00 00 00 00 00 00 01 f4 9e */
static const uint8_t CAR_CMD_MOVE_LEFT[] = {
    0x03, 0x45, 0x31, 0x32, 0x35, 0x30, 0x39, 0x44,
    0x56, 0x31, 0x30, 0x31, 0x36, 0x09, 0x00, 0x0c,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0xf4, 0x9e
};
#define CAR_CMD_MOVE_LEFT_LEN   (sizeof(CAR_CMD_MOVE_LEFT))

/* Right move: 03 45 31 32 35 30 39 44 56 31 30 31 36 09 00 0c 00 06 00 00 00 00 00 00 00 00 00 01 e2 1e (30 bytes) */
static const uint8_t CAR_CMD_MOVE_RIGHT[] = {
    0x03, 0x45, 0x31, 0x32, 0x35, 0x30, 0x39, 0x44,
    0x56, 0x31, 0x30, 0x31, 0x36, 0x09, 0x00, 0x0c,
    0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0xe2, 0x1e
};
#define CAR_CMD_MOVE_RIGHT_LEN  (sizeof(CAR_CMD_MOVE_RIGHT))

/* Forward: 03 45 31 32 35 30 39 44 56 31 30 31 36 09 00 0c 00 00 06 00 00 00 00 00 00 00 00 01 1c 89 */
static const uint8_t CAR_CMD_MOVE_FORWARD[] = {
    0x03, 0x45, 0x31, 0x32, 0x35, 0x30, 0x39, 0x44,
    0x56, 0x31, 0x30, 0x31, 0x36, 0x09, 0x00, 0x0c,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x1c, 0x89
};
#define CAR_CMD_MOVE_FORWARD_LEN  (sizeof(CAR_CMD_MOVE_FORWARD))

/* Backward: 03 45 31 32 35 30 39 44 56 31 30 31 36 09 00 0c 00 00 00 06 00 00 00 00 00 00 00 01 d7 36 */
static const uint8_t CAR_CMD_MOVE_BACKWARD[] = {
    0x03, 0x45, 0x31, 0x32, 0x35, 0x30, 0x39, 0x44,
    0x56, 0x31, 0x30, 0x31, 0x36, 0x09, 0x00, 0x0c,
    0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0xd7, 0x36
};
#define CAR_CMD_MOVE_BACKWARD_LEN (sizeof(CAR_CMD_MOVE_BACKWARD))

/* Gimbal up: 03 45 33 32 35 35 30 44 48 30 30 32 39 49 00 0c 00 00 07 00 01 00 00 00 00 00 00 00 7b 8e */
static const uint8_t CAR_CMD_GIMBAL_UP[] = {
    0x03, 0x45, 0x33, 0x32, 0x35, 0x35, 0x30, 0x44,
    0x48, 0x30, 0x30, 0x32, 0x39, 0x49, 0x00, 0x0c,
    0x00, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7b, 0x8e
};
#define CAR_CMD_GIMBAL_UP_LEN   (sizeof(CAR_CMD_GIMBAL_UP))

/* Gimbal down: 03 45 33 32 35 35 30 44 48 30 30 32 39 49 00 0c 00 00 07 00 00 01 00 00 00 00 00 00 00 aa 82 */
static const uint8_t CAR_CMD_GIMBAL_DOWN[] = {
    0x03, 0x45, 0x33, 0x32, 0x35, 0x35, 0x30, 0x44,
    0x48, 0x30, 0x30, 0x32, 0x39, 0x49, 0x00, 0x0c,
    0x00, 0x00, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xaa, 0x82
};
#define CAR_CMD_GIMBAL_DOWN_LEN (sizeof(CAR_CMD_GIMBAL_DOWN))

/**
 * @brief Car control init config: UART id and baud rate.
 */
typedef struct
{
    uart_id_t uart_id;   /**< UART port (e.g. UART_ID_0). */
    uint32_t baud_rate;  /**< Baud rate (e.g. 9600). */
} car_control_config_t;

#define DEFAULT_CAR_CONTROL_CONFIG() {  \
    .uart_id = UART_ID_1,               \
    .baud_rate = 115200,                \
}

/**
 * @brief Initialize UART for car control. Call once before using any control API.
 *
 * @param config NULL to use default (UART_ID_0, 9600); else use config->uart_id and config->baud_rate.
 * @return BK_OK on success.
 */
bk_err_t car_control_init(const car_control_config_t *config);

/**
 * @brief Send raw UART payload to MCU.
 *
 * This API is used by upper-layer modules when a custom protocol frame
 * (for example, recognition coordinates) needs to be sent directly.
 *
 * @param data Payload buffer.
 * @param len Payload length in bytes.
 * @return BK_OK on success.
 */
bk_err_t car_control_send_raw(const uint8_t *data, uint32_t len);

/**
 * @brief Send move-left command.
 * @return BK_OK on success.
 */
bk_err_t car_control_move_left(void);

/**
 * @brief Send move-right command.
 * @return BK_OK on success.
 */
bk_err_t car_control_move_right(void);

/**
 * @brief Send move-forward command.
 * @return BK_OK on success.
 */
bk_err_t car_control_move_forward(void);

/**
 * @brief Send move-backward command.
 * @return BK_OK on success.
 */
bk_err_t car_control_move_backward(void);

/**
 * @brief Send gimbal-up command.
 * @return BK_OK on success.
 */
bk_err_t car_control_gimbal_up(void);

/**
 * @brief Send gimbal-down command.
 * @return BK_OK on success.
 */
bk_err_t car_control_gimbal_down(void);

#ifdef __cplusplus
}
#endif
