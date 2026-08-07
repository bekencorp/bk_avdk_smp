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

/* From tv_corners.txt — 001.jpg (7 points, order preserved) */
static const point_t k_tv_points_001[FISHEYE_MAP_POINTS] = {
    { 75.0f, 387.0f },
    { 177.0f, 468.0f },
    { 438.0f, 583.0f },
    { 997.0f, 617.0f },
    { 1556.0f, 630.0f },
    { 1803.0f, 538.0f },
    { 1890.0f, 469.0f },
};

/* From grids.txt — sparse mesh (row, col, x, y) */
static const grid_point_t k_fisheye_grid_points[] = {
    { 1, 5, 222.0f, 267.0f },
    { 1, 6, 330.0f, 196.0f },
    { 1, 7, 562.0f, 103.0f },
    { 1, 8, 992.0f, 52.0f },
    { 1, 9, 1429.0f, 127.0f },
    { 1, 10, 1666.0f, 241.0f },
    { 1, 11, 1771.0f, 325.0f },
    { 2, 5, 300.0f, 331.0f },
    { 2, 6, 429.0f, 292.0f },
    { 2, 7, 656.0f, 252.0f },
    { 2, 8, 994.0f, 240.0f },
    { 2, 9, 1336.0f, 276.0f },
    { 2, 10, 1567.0f, 334.0f },
    { 2, 11, 1695.0f, 385.0f },
    { 3, 5, 378.0f, 388.0f },
    { 3, 6, 515.0f, 369.0f },
    { 3, 7, 724.0f, 356.0f },
    { 3, 8, 995.0f, 356.0f },
    { 3, 9, 1269.0f, 376.0f },
    { 3, 10, 1480.0f, 408.0f },
    { 3, 11, 1617.0f, 438.0f },
    { 4, 5, 449.0f, 436.0f },
    { 4, 6, 585.0f, 429.0f },
    { 4, 7, 773.0f, 427.0f },
    { 4, 8, 996.0f, 431.0f },
    { 4, 9, 1221.0f, 444.0f },
    { 4, 10, 1408.0f, 462.0f },
    { 4, 11, 1545.0f, 481.0f },
    { 5, 5, 510.0f, 474.0f },
    { 5, 6, 641.0f, 473.0f },
    { 5, 7, 807.0f, 477.0f },
    { 5, 8, 997.0f, 482.0f },
    { 5, 9, 1186.0f, 491.0f },
    { 5, 10, 1352.0f, 503.0f },
    { 5, 11, 1483.0f, 515.0f },
    { 6, 5, 563.0f, 506.0f },
    { 6, 6, 686.0f, 508.0f },
    { 6, 7, 834.0f, 512.0f },
    { 6, 8, 997.0f, 518.0f },
    { 6, 9, 1160.0f, 526.0f },
    { 6, 10, 1308.0f, 534.0f },
    { 6, 11, 1431.0f, 542.0f },
    { 7, 5, 605.0f, 530.0f },
    { 7, 6, 722.0f, 535.0f },
    { 7, 7, 854.0f, 539.0f },
    { 7, 8, 998.0f, 545.0f },
    { 7, 9, 1141.0f, 551.0f },
    { 7, 10, 1273.0f, 558.0f },
    { 7, 11, 1389.0f, 563.0f },
    { 0, 0, 76.0f, 388.0f },
    { 0, 1, 82.0f, 369.0f },
    { 0, 2, 90.0f, 346.0f },
    { 0, 3, 103.0f, 315.0f },
    { 0, 4, 124.0f, 271.0f },
    { 0, 5, 163.0f, 202.0f },
    { 0, 6, 246.0f, 93.0f },
    { 0, 10, 1747.0f, 136.0f },
    { 0, 11, 1826.0f, 260.0f },
    { 0, 12, 1859.0f, 336.0f },
    { 0, 13, 1876.0f, 383.0f },
    { 0, 14, 1886.0f, 419.0f },
    { 0, 15, 1890.0f, 447.0f },
    { 0, 16, 1894.0f, 467.0f },
    { 1, 0, 94.0f, 408.0f },
    { 1, 1, 103.0f, 394.0f },
    { 1, 2, 116.0f, 375.0f },
    { 1, 3, 135.0f, 350.0f },
    { 1, 4, 166.0f, 317.0f },
    { 1, 12, 1820.0f, 382.0f },
    { 1, 13, 1847.0f, 421.0f },
    { 1, 14, 1863.0f, 448.0f },
    { 1, 15, 1872.0f, 468.0f },
    { 1, 16, 1878.0f, 484.0f },
    { 2, 0, 119.0f, 428.0f },
    { 2, 1, 132.0f, 418.0f },
    { 2, 2, 151.0f, 404.0f },
    { 2, 3, 180.0f, 386.0f },
    { 2, 4, 224.0f, 363.0f },
    { 2, 12, 1765.0f, 425.0f },
    { 2, 13, 1806.0f, 453.0f },
    { 2, 14, 1831.0f, 474.0f },
    { 2, 15, 1848.0f, 490.0f },
    { 2, 16, 1858.0f, 502.0f },
    { 3, 0, 147.0f, 448.0f },
    { 3, 1, 166.0f, 441.0f },
    { 3, 2, 193.0f, 432.0f },
    { 3, 3, 231.0f, 420.0f },
    { 3, 4, 288.0f, 406.0f },
    { 3, 12, 1703.0f, 464.0f },
    { 3, 13, 1756.0f, 484.0f },
    { 3, 14, 1792.0f, 499.0f },
    { 3, 15, 1815.0f, 511.0f },
    { 3, 16, 1832.0f, 520.0f },
    { 4, 0, 180.0f, 469.0f },
    { 4, 1, 203.0f, 464.0f },
    { 4, 2, 238.0f, 458.0f },
    { 4, 3, 283.0f, 450.0f },
    { 4, 4, 352.0f, 444.0f },
    { 4, 12, 1639.0f, 498.0f },
    { 4, 13, 1704.0f, 512.0f },
    { 4, 14, 1750.0f, 522.0f },
    { 4, 15, 1782.0f, 531.0f },
    { 4, 16, 1804.0f, 538.0f },
    { 5, 0, 216.0f, 488.0f },
    { 5, 3, 337.0f, 479.0f },
    { 5, 4, 412.0f, 477.0f },
    { 5, 12, 1582.0f, 526.0f },
    { 5, 16, 1771.0f, 554.0f },
    { 6, 0, 249.0f, 505.0f },
    { 6, 3, 386.0f, 503.0f },
    { 6, 4, 465.0f, 504.0f },
    { 6, 12, 1530.0f, 549.0f },
    { 6, 16, 1732.0f, 568.0f },
    { 7, 0, 291.0f, 523.0f },
    { 7, 3, 435.0f, 525.0f },
    { 7, 4, 512.0f, 527.0f },
    { 7, 12, 1483.0f, 568.0f },
    { 7, 16, 1696.0f, 583.0f },
    { 8, 0, 320.0f, 537.0f },
    { 8, 1, 362.0f, 538.0f },
    { 8, 2, 415.0f, 541.0f },
    { 8, 3, 476.0f, 543.0f },
    { 8, 4, 551.0f, 546.0f },
    { 8, 5, 644.0f, 551.0f },
    { 8, 6, 750.0f, 555.0f },
    { 8, 7, 870.0f, 560.0f },
    { 8, 8, 998.0f, 566.0f },
    { 8, 9, 1125.0f, 571.0f },
    { 8, 10, 1245.0f, 576.0f },
    { 8, 11, 1351.0f, 580.0f },
    { 8, 12, 1443.0f, 583.0f },
    { 8, 13, 1516.0f, 587.0f },
    { 8, 14, 1578.0f, 590.0f },
    { 8, 15, 1627.0f, 593.0f },
    { 8, 16, 1665.0f, 595.0f },
    { 9, 0, 351.0f, 550.0f },
    { 9, 1, 396.0f, 552.0f },
    { 9, 2, 449.0f, 555.0f },
    { 9, 3, 512.0f, 559.0f },
    { 9, 4, 587.0f, 563.0f },
    { 9, 5, 674.0f, 568.0f },
    { 9, 6, 773.0f, 572.0f },
    { 9, 7, 883.0f, 578.0f },
    { 9, 8, 998.0f, 583.0f },
    { 9, 9, 1114.0f, 587.0f },
    { 9, 10, 1226.0f, 590.0f },
    { 9, 11, 1326.0f, 594.0f },
    { 9, 12, 1414.0f, 597.0f },
    { 9, 13, 1481.0f, 599.0f },
    { 9, 14, 1543.0f, 601.0f },
    { 9, 15, 1595.0f, 602.0f },
    { 9, 16, 1637.0f, 604.0f },
    { 10, 0, 380.0f, 561.0f },
    { 10, 1, 428.0f, 564.0f },
    { 10, 2, 482.0f, 568.0f },
    { 10, 3, 545.0f, 573.0f },
    { 10, 4, 616.0f, 577.0f },
    { 10, 5, 700.0f, 582.0f },
    { 10, 6, 794.0f, 587.0f },
    { 10, 7, 894.0f, 592.0f },
    { 10, 8, 998.0f, 596.0f },
    { 10, 9, 1104.0f, 599.0f },
    { 10, 10, 1207.0f, 602.0f },
    { 10, 11, 1302.0f, 605.0f },
    { 10, 12, 1380.0f, 609.0f },
    { 10, 13, 1449.0f, 610.0f },
    { 10, 14, 1510.0f, 611.0f },
    { 10, 15, 1563.0f, 612.0f },
    { 10, 16, 1608.0f, 613.0f },
    { 11, 0, 412.0f, 572.0f },
    { 11, 1, 457.0f, 576.0f },
    { 11, 2, 512.0f, 580.0f },
    { 11, 3, 574.0f, 584.0f },
    { 11, 4, 643.0f, 589.0f },
    { 11, 5, 722.0f, 594.0f },
    { 11, 6, 809.0f, 599.0f },
    { 11, 7, 902.0f, 604.0f },
    { 11, 8, 998.0f, 608.0f },
    { 11, 9, 1095.0f, 611.0f },
    { 11, 10, 1189.0f, 614.0f },
    { 11, 11, 1277.0f, 617.0f },
    { 11, 12, 1354.0f, 619.0f },
    { 11, 13, 1420.0f, 620.0f },
    { 11, 14, 1480.0f, 621.0f },
    { 11, 15, 1534.0f, 622.0f },
    { 11, 16, 1575.0f, 623.0f },
    { 12, 0, 437.0f, 583.0f },
    { 12, 1, 486.0f, 587.0f },
    { 12, 2, 538.0f, 590.0f },
    { 12, 3, 601.0f, 595.0f },
    { 12, 4, 667.0f, 600.0f },
    { 12, 5, 742.0f, 603.0f },
    { 12, 6, 822.0f, 608.0f },
    { 12, 7, 908.0f, 612.0f },
    { 12, 8, 997.0f, 616.0f },
    { 12, 9, 1087.0f, 620.0f },
    { 12, 10, 1173.0f, 623.0f },
    { 12, 11, 1256.0f, 625.0f },
    { 12, 12, 1330.0f, 627.0f },
    { 12, 13, 1395.0f, 628.0f },
    { 12, 14, 1455.0f, 629.0f },
    { 12, 15, 1504.0f, 630.0f },
    { 12, 16, 1553.0f, 630.0f },
    { 5, 1, 242.0f, 486.0f },
    { 5, 2, 283.0f, 482.0f },
    { 6, 1, 281.0f, 506.0f },
    { 6, 2, 329.0f, 504.0f },
    { 7, 1, 323.0f, 523.0f },
    { 7, 2, 375.0f, 524.0f },
    { 5, 13, 1654.0f, 536.0f },
    { 5, 14, 1703.0f, 544.0f },
    { 5, 15, 1741.0f, 551.0f },
    { 6, 13, 1604.0f, 555.0f },
    { 6, 14, 1658.0f, 561.0f },
    { 6, 15, 1703.0f, 567.0f },
    { 7, 13, 1559.0f, 571.0f },
    { 7, 14, 1620.0f, 575.0f },
    { 7, 15, 1667.0f, 580.0f },
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

    LOGI("fisheye calibration, dump: %d, output_width: %d, output_height: %d\r\n", is_dump, output_width, output_height);

    unsigned char *output_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, output_width * output_height * 2 *2);
    if (output_buffer == NULL)
    {
        LOGE("Failed to malloc output buffer\n");
        return BK_FAIL;
    }

    uint32_t t0 = rtos_get_time();
    uint32_t t1 = 0;
    uint32_t dt_ms = 0;

    fisheye_calibration(
        (grid_point_t *)k_fisheye_grid_points,
        (uint32_t)(sizeof(k_fisheye_grid_points) / sizeof(k_fisheye_grid_points[0])),
        (point_t *)k_tv_points_001,
        (point_t *)k_tv_points_001,
        DEFAULT_INPUT_WIDTH,
        DEFAULT_INPUT_HEIGHT,
        output_width,
        output_height,
        (int16_t *)output_buffer);

    t1 = rtos_get_time();
    dt_ms = t1 - t0;
    bk_printf_raw(0, NULL, "fisheye calibration execute time: %d ms\r\n", dt_ms);

    if (is_dump)
    {
        const uint32_t total = (uint32_t)output_width * (uint32_t)output_height * 4U;
        const int line_bytes = 16;
        char line[16 * 3 + 8];

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
                p += os_snprintf(p, (size_t)(line + sizeof(line) - p), "%02x ", output_buffer[base + j]);
            }
            bk_printf_raw(0, NULL, "%s\r\n", line);
            if (base % (line_bytes*8) == 0)
            {
                rtos_delay_milliseconds(10);
            }
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
