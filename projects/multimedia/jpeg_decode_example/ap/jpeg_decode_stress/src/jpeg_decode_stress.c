#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "h264_decoder_api.h"
#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include "jpeg_data_1920_1080.h"
#include "psram_dma_stress.h"
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"


#define JPEG_DECODE_WIDTH  1920
#define JPEG_DECODE_HEIGHT 1080
#define JPEG_DECODE_SIZE   (JPEG_DECODE_WIDTH * JPEG_DECODE_HEIGHT * 2)

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

#define JPEG_DECODE_DEBUG

#ifdef JPEG_DECODE_DEBUG
#define JPEG_DECODE_DEBUG_GPIO_INIT(id)  do { gpio_dev_unmap(id); bk_gpio_enable_output(id); bk_gpio_set_output_low(id);} while (0)
#define JPEG_DECODE_FRAME_INIT()         do { JPEG_DECODE_DEBUG_GPIO_INIT(GPIO_32); } while (0)
#define JPEG_DECODE_FRAME_START()        do { bk_gpio_set_output_low(GPIO_32); bk_gpio_set_output_high(GPIO_32);} while (0)
#define JPEG_DECODE_FRAME_END()          do { bk_gpio_set_output_low(GPIO_32); } while (0)
#define JPEG_DECODE_FLEXA_INIT()         do { JPEG_DECODE_DEBUG_GPIO_INIT(GPIO_33); } while (0)
#define JPEG_DECODE_FLEXA_LINE_START()   do { bk_gpio_set_output_low(GPIO_33); bk_gpio_set_output_high(GPIO_33);} while (0)
#define JPEG_DECODE_FLEXA_LINE_END()     do { bk_gpio_set_output_low(GPIO_33); } while (0)
#else
#define JPEG_DECODE_FRAME_INIT()
#define JPEG_DECODE_FRAME_START()
#define JPEG_DECODE_FRAME_END()
#define JPEG_DECODE_FLEXA_INIT()
#define JPEG_DECODE_FLEXA_LINE_START()
#define JPEG_DECODE_FLEXA_LINE_END()
#endif

static void *s_jpeg_decode_context = NULL;
static beken_thread_t s_jpeg_decode_stress_thread = NULL;
static uint8_t s_jpeg_decode_stress_stop = 0;
static uint8_t *s_jpeg_buffer = NULL;
static uint8_t *s_output_buffer = NULL;
static uint8_t *s_dma_src_buffer = NULL;
static uint8_t *s_dma_dst_buffer = NULL;
static psram_dma_stress_handle_t s_psram_dma_handle = NULL;
static uint32_t s_jpeg_decode_mode = VCDEC_FLEXA_MODE_NONE;

static void jpeg_flexa_done_callback(uint32_t wrCnt)
{
    JPEG_DECODE_FLEXA_LINE_START();
	/*
	 * Flexa mode requires updating the PP ring-buffer read pointer,
	 * otherwise the hardware side may stall after the write pointer advances.
	 */
    // bk_printf("jpeg_flexa_done_callback: %d\n", wrCnt);

	extern void ppRbReadPointerSet(uint32_t value);
	ppRbReadPointerSet(wrCnt);
    JPEG_DECODE_FLEXA_LINE_END();
}


static void SaveStream(uint8_t* y, uint8_t* cb, uint8_t* cr, uint32_t width, uint32_t height, uint32_t type)
{
    JPEG_DECODE_FRAME_END();
    bk_printf("%s(%p, %p, %p, %d, %d, %d)\n", __func__, y, cb, cr, width, height, type);
    // extern void bk_mem_dump_ex(const char *title, unsigned char *data, uint32_t data_len);
    // bk_mem_dump_ex("y", y, width * height);
    // bk_mem_dump_ex("cb", cb, width * height / 2);
}

static void jpeg_decode_output_buffer_free(void)
{
    if (s_jpeg_decode_mode == VCDEC_FLEXA_MODE_FLEXA) {
        if (s_output_buffer != NULL) {
            hsram_free(s_output_buffer);
            s_output_buffer = NULL;
        }
    } else {
        if (s_output_buffer != NULL) {
            bk_frame_buffer_free(s_output_buffer);
            s_output_buffer = NULL;
        }
    }
}

void jpeg_decode_stress_thread_entry(void *args)
{
    bk_printf("Enter %s\n", __func__);
    uint32_t outSize = JPEG_DECODE_SIZE;

    const uint8_t flexa = (s_jpeg_decode_mode == VCDEC_FLEXA_MODE_FLEXA) ? 1 : 0;
    VCDecFlexaDoneCallback fcb = flexa ? jpeg_flexa_done_callback : NULL;
    bk_err_t ret = jpeg_decoder_init(&s_jpeg_decode_context, s_jpeg_decode_mode, fcb, NULL, &SaveStream);
    if(ret < 0) {
        bk_printf("jpeg_decoder_init error with %d\n", ret);
        s_jpeg_decode_stress_stop = 1;
        return;
    }

    while (!s_jpeg_decode_stress_stop) {
        JPEG_DECODE_FRAME_START();
        ret = jpeg_decoder_decode(s_jpeg_decode_context, (uint8_t *)s_jpeg_buffer, sizeof(JPEGData_1920_1080), s_output_buffer, outSize);
        if (ret < 0) {
            bk_printf("jpeg_decoder_decode error with %d\n", ret);
            s_jpeg_decode_stress_stop = 1;
            break;
        }
        // rtos_delay_milliseconds(2);
    }

    jpeg_decoder_deinit(s_jpeg_decode_context);
    s_jpeg_decode_context = NULL;

    if (s_jpeg_buffer != NULL) {
        bk_frame_buffer_free(s_jpeg_buffer);
        s_jpeg_buffer = NULL;
    }

    jpeg_decode_output_buffer_free();

    if (s_dma_src_buffer != NULL) {
        bk_frame_buffer_free(s_dma_src_buffer);
        s_dma_src_buffer = NULL;
    }

    if (s_dma_dst_buffer != NULL) {
        bk_frame_buffer_free(s_dma_dst_buffer);
        s_dma_dst_buffer = NULL;
    }

    s_jpeg_decode_stress_thread = NULL;
    rtos_delete_thread(NULL);
}

static void jpeg_decode_dma_stress_start(void)
{
    bk_err_t ret = psram_dma_stress_create(&s_psram_dma_handle);
    if (s_psram_dma_handle == NULL || ret != BK_OK) {
        bk_printf("psram_dma_stress_create failed\n");
        return;
    }

    ret = psram_dma_stress_start(s_psram_dma_handle, s_dma_src_buffer, s_dma_dst_buffer, JPEG_DECODE_SIZE);
    if (ret != BK_OK) {
        bk_printf("psram_dma_stress_start failed\n");
        return;
    }
}

void cli_jpeg_decode_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    const char *msg = CLI_CMD_RSP_SUCCEED;

    if (argc < 2) {
        bk_printf("usage: jpeg_decode_stress <start|stop> [none|flexa]\n");
        ret = BK_FAIL;
        msg = CLI_CMD_RSP_ERROR;
        goto exit;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        if (s_jpeg_decode_stress_thread != NULL) {
            bk_printf("jpeg decode stress thread already running\n");
            ret = BK_FAIL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }

        s_jpeg_decode_mode = VCDEC_FLEXA_MODE_NONE;
        if (argc >= 3) {
            if (os_strcmp(argv[2], "none") == 0) {
                s_jpeg_decode_mode = VCDEC_FLEXA_MODE_NONE;
            } else if (os_strcmp(argv[2], "flexa") == 0) {
                s_jpeg_decode_mode = VCDEC_FLEXA_MODE_FLEXA;
            } else {
                bk_printf("invalid mode: %s\n", argv[2]);
                bk_printf("usage: jpeg_decode_stress <start|stop> [none|flexa]\n");
                ret = BK_FAIL;
                msg = CLI_CMD_RSP_ERROR;
                goto exit;
            }
        }

        s_jpeg_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, sizeof(JPEGData_1920_1080));
        if (s_jpeg_buffer == NULL) {
            bk_printf("s_jpeg_buffer malloc failed\n");
            ret = BK_FAIL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        os_memcpy(s_jpeg_buffer, JPEGData_1920_1080, sizeof(JPEGData_1920_1080));

        if (s_jpeg_decode_mode == VCDEC_FLEXA_MODE_FLEXA) {
            s_output_buffer = hsram_malloc(bk_image_size_get(JPEG_DECODE_WIDTH, 16 * 2, BK_PIXEL_FORMAT_NV12));
        } else {
            s_output_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, JPEG_DECODE_SIZE);
        }
        if (s_output_buffer == NULL) {
            bk_printf("s_output_buffer malloc failed\n");
            bk_frame_buffer_free(s_jpeg_buffer);
            s_jpeg_buffer = NULL;
            ret = BK_FAIL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }

        s_dma_src_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, JPEG_DECODE_SIZE);
        if (s_dma_src_buffer == NULL) {
            bk_printf("s_dma_src_buffer malloc failed\n");
            bk_frame_buffer_free(s_jpeg_buffer);
            s_jpeg_buffer = NULL;
            jpeg_decode_output_buffer_free();
            ret = BK_FAIL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }

        s_dma_dst_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, JPEG_DECODE_SIZE);
        if (s_dma_dst_buffer == NULL) {
            bk_printf("s_dma_dst_buffer malloc failed\n");
            bk_frame_buffer_free(s_jpeg_buffer);
            s_jpeg_buffer = NULL;
            jpeg_decode_output_buffer_free();
            bk_frame_buffer_free(s_dma_src_buffer);
            s_dma_src_buffer = NULL;
            ret = BK_FAIL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }

        bk_printf("s_jpeg_buffer: %p, s_output_buffer: %p, s_dma_src_buffer: %p, s_dma_dst_buffer: %p\n", s_jpeg_buffer, s_output_buffer, s_dma_src_buffer, s_dma_dst_buffer);
        bk_printf("jpeg decode mode: %s\n", (s_jpeg_decode_mode == VCDEC_FLEXA_MODE_FLEXA) ? "flexa" : "none");

        jpeg_decode_dma_stress_start();

        JPEG_DECODE_FRAME_INIT();
        JPEG_DECODE_FLEXA_INIT();

        s_jpeg_decode_stress_stop = 0;

        ret = rtos_create_thread(&s_jpeg_decode_stress_thread,
                                 BEKEN_DEFAULT_WORKER_PRIORITY,
                                 "jpeg_decode_stress",
                                 (beken_thread_function_t)jpeg_decode_stress_thread_entry,
                                 10 * 1024,
                                 NULL);
        if (ret != BK_OK) {
            bk_printf("create jpeg decode stress thread failed, ret=%d\n", (int)ret);
            s_jpeg_decode_stress_thread = NULL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }

    } else if (os_strcmp(argv[1], "stop") == 0) {
        if (s_jpeg_decode_stress_thread == NULL) {
            bk_printf("jpeg decode stress thread not running\n");
            ret = BK_FAIL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        s_jpeg_decode_stress_stop = 1;
    } else {
        bk_printf("usage: jpeg_decode_stress <start|stop> [none|flexa]\n");
        ret = BK_FAIL;
        msg = CLI_CMD_RSP_ERROR;
        goto exit;
    }

exit:
    if (pcWriteBuffer && xWriteBufferLen > 0) {
        int len = os_strlen(msg);
        if (len >= xWriteBufferLen) {
            len = xWriteBufferLen - 1;
        }
        os_memcpy(pcWriteBuffer, msg, len);
        pcWriteBuffer[len] = '\0';
    }
}