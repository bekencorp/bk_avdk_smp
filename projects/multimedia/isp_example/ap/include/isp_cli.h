#pragma once

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/avdk_pixel_types.h>
#include <components/log.h>
#include <avdk_error.h>

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define TAG "isp-cli"

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

int cli_isp_test_init(void);
void cli_isp_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_isp_api_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_isp_tuning_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_isp_dump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

void isp_ini(uint8_t* Param);
void mipi_controller_init(uint32_t width, uint32_t height, uint8_t date_type);

/**
 * @brief DVP/ISP frame ready callback prototype.
 *
 * Invoked from the DVP capture task once per frame after a frame has been
 * dequeued from the ISP channel. The @p frame buffer is owned by the capture
 * task and is freed right after the callback returns, so the application must
 * consume (or copy out) the data before returning. Do not block for long here.
 *
 * @param frame    Pointer to the frame pixel data.
 * @param size     Frame size in bytes.
 * @param width    Frame width in pixels.
 * @param height   Frame height in pixels.
 * @param fmt      Pixel format of the frame.
 * @param user_arg User argument passed at registration time.
 */
typedef void (*isp_dvp_frame_cb_t)(uint8_t *frame, uint32_t size,
                                   uint16_t width, uint16_t height,
                                   bk_pixel_format_t fmt, void *user_arg);

/**
 * @brief Register the application frame callback used by the DVP capture task.
 *
 * @param cb       Callback to invoke for each captured frame (NULL to clear).
 * @param user_arg Opaque argument forwarded to @p cb.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t isp_dvp_register_frame_cb(isp_dvp_frame_cb_t cb, void *user_arg);

/**
 * @brief Clear the application frame callback.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t isp_dvp_unregister_frame_cb(void);

/**
 * @brief Start the DVP capture task that pushes frames to the registered callback.
 *
 * Requires the corresponding channel to be opened in frame mode beforehand
 * (e.g. "isp open dvp mp ... frame"). The task reads frames in a loop and
 * delivers them via the callback registered with isp_dvp_register_frame_cb().
 *
 * @param is_mp 1 to capture from the MP channel, 0 for the SP channel.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t isp_dvp_capture_start(uint8_t is_mp);

/**
 * @brief Stop the DVP capture task.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t isp_dvp_capture_stop(void);