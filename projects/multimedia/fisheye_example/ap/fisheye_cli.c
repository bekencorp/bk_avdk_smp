#include <os/os.h>
#include <os/str.h>
#include <common/bk_err.h>
#include <components/system.h>
#include "cli.h"
#include "modules/fisheye_calibration.h"
#include <components/bk_frame_buffer.h>

#define TAG "fisheye"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define DEFAULT_INPUT_WIDTH 1920
#define DEFAULT_INPUT_HEIGHT 1080

#define DEFAULT_OUTPUT_WIDTH 320
#define DEFAULT_OUTPUT_HEIGHT 180

/* From Temp/fisheye2/my_camera_new.json. */
static const fisheye_camera_params_t k_demo_camera_params = {
    .camera_matrix = {
        792.7519934341502, 0.0, 960.0,
        0.0, 791.9044820171898, 540.0,
        0.0, 0.0, 1.0,
    },
    .distortion_coeffs = {
        -0.10392960533096862,
        0.0002735835104686153,
        0.003989249589054039,
        -0.0016220066209364516,
    },
    .distortion_coeffs_fallback = {
        -0.1092884,
        0.02436084,
        0.003989249589054039,
        -0.0016220066209364516,
    },
};

/* From Temp/fisheye2/tv_corners.txt — 001.jpg (7 points, order preserved). */
static const point_t k_tv_points_001[FISHEYE_MAP_POINTS] = {
    {75.0f, 387.0f},
    {262.0f, 511.0f},
    {438.0f, 583.0f},
    {997.0f, 617.0f},
    {1556.0f, 630.0f},
    {1732.0f, 569.0f},
    {1890.0f, 469.0f},
};

static int fisheye_run_calibration(uint8_t is_dump, uint16_t output_width, uint16_t output_height)
{
    if (output_width == 0 || output_width > DEFAULT_INPUT_WIDTH)
    {
        output_width = DEFAULT_OUTPUT_WIDTH;
        output_height = DEFAULT_OUTPUT_HEIGHT;
    }
    if (output_height == 0 || output_height > DEFAULT_INPUT_HEIGHT)
    {
        output_width = DEFAULT_OUTPUT_WIDTH;
        output_height = DEFAULT_OUTPUT_HEIGHT;
    }

    LOGI("fisheye calibration v19, dump: %d, output_width: %d, output_height: %d\r\n",
         is_dump, output_width, output_height);

    unsigned char *output_buffer = bk_frame_buffer_malloc(
        MEM_SLAB_HEAP_UNCODED,
        (uint32_t)output_width * (uint32_t)output_height * 2U * sizeof(int16_t));
    if (output_buffer == NULL)
    {
        LOGE("Failed to malloc output buffer\n");
        return BK_FAIL;
    }

    fisheye_calibration_result_t result;
    uint32_t t0 = rtos_get_time();
    int ret = fisheye_calibration(
        &k_demo_camera_params,
        k_tv_points_001,
        DEFAULT_INPUT_WIDTH,
        DEFAULT_INPUT_HEIGHT,
        output_width,
        output_height,
        (int16_t *)output_buffer,
        &result);
    uint32_t t1 = rtos_get_time();
    uint32_t dt_ms = t1 - t0;

    if (ret != 0)
    {
        LOGE("fisheye calibration failed: %d\r\n", ret);
        bk_frame_buffer_free(output_buffer);
        return BK_FAIL;
    }

    bk_printf_raw(0, NULL,
                  "fisheye calibration execute time: %d ms, fallback: %d, max_error: %.3f, coverage: %.6f\r\n",
                  dt_ms,
                  result.used_fallback,
                  result.source_max_error_px,
                  result.valid_coverage);

    if (is_dump)
    {
        const uint32_t total = (uint32_t)output_width * (uint32_t)output_height * 2U * sizeof(int16_t);
        const int line_bytes = 8;
        char line[8 * 6 + 8];

        int prev_sync = bk_get_printf_sync();
        bk_set_printf_sync(1);
        t1 = rtos_get_time();
        dt_ms = t1 - t0;
        for (uint32_t base = 0; base < total; base += (uint32_t)line_bytes)
        {
            uint32_t n = (uint32_t)line_bytes;
            if (base + n > total)
            {
                n = total - base;
            }
            char *p = line;
            for (uint32_t j = 0; j < n; j++)
            {
                p += os_snprintf(p, (size_t)(line + sizeof(line) - p), "0x%02x ", output_buffer[base + j]);
            }
            bk_printf_raw(0, NULL, "%s\r\n", line);
            rtos_delay_milliseconds(10);
        }
        t1 = rtos_get_time();
        dt_ms = t1 - t0;
        LOGI("fisheye calibration dump execute time: %d ms\r\n", dt_ms);
        bk_set_printf_sync((uint8_t)prev_sync);
    }
    bk_frame_buffer_free(output_buffer);
    return BK_OK;
}

int fisheye_run_default_it_test(void)
{
    return fisheye_run_calibration(0, DEFAULT_OUTPUT_WIDTH, DEFAULT_OUTPUT_HEIGHT);
}

static void cli_fisheye_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2 || argv[1] == NULL)
    {
        LOGE("Usage: fisheye cal <dump> <width> <height>\n");
        LOGE("  dump: 0 or 1, default is 0\n");
        LOGE("  width: output width, default is %d\n", DEFAULT_OUTPUT_WIDTH);
        LOGE("  height: output height, default is %d\n", DEFAULT_OUTPUT_HEIGHT);
        return;
    }

    if (os_strcmp(argv[1], "cal") == 0)
    {
        uint8_t is_dump = 0;
        uint16_t output_width = 0;
        uint16_t output_height = 0;

        if (argc > 2)
        {
            is_dump = (uint8_t)os_strtoul(argv[2], NULL, 10);
        }

        if (argc > 4)
        {
            output_width = (uint16_t)os_strtoul(argv[3], NULL, 10);
            output_height = (uint16_t)os_strtoul(argv[4], NULL, 10);
        }

        (void)fisheye_run_calibration(is_dump, output_width, output_height);
    }
}

static const struct cli_command s_fisheye_cli_commands[] =
{
    {"fisheye", "fisheye cal <dump> <w> <h>", cli_fisheye_cmd},
};

void fisheye_cli_init(void)
{
    cli_register_commands(s_fisheye_cli_commands,
                          sizeof(s_fisheye_cli_commands) / sizeof(s_fisheye_cli_commands[0]));
}
