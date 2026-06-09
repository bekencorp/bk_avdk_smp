// Copyright 2020-2021 Beken
// MIPI LCD on/off stress test: loop open -> flush(on_ms) -> close(power down) -> gap, until stopped.
// Power is handled by lcd_example_dsi_open/close (bk_lodoen_enable true/false), so each
// close in the loop powers the panel rail down; the next open powers it back up.

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/bk_display.h>
#include <common/avdk_pixel_types.h>
#include <avdk_check.h>
#include <avdk_error.h>
#include "lcd_example.h"

#define TAG "lcd_stress"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define LCD_STRESS_OFF_GAP_MS   500
#define LCD_STRESS_POLL_MS      50
#define LCD_STRESS_DEFAULT_MS   1000

static volatile uint8_t  s_stress_running = 0;
static volatile uint8_t  s_stress_abort   = 0;
static beken_thread_t    s_stress_thread  = NULL;
static beken_semaphore_t s_stress_done;
static display_ctx_t     s_stress_ctx;
static const char       *s_stress_panel   = NULL;
static uint32_t          s_stress_on_ms   = LCD_STRESS_DEFAULT_MS;

/* Sleep in small slices so lcd_stress stop can break a long on/gap window quickly. */
static void lcd_stress_delay_abortable(uint32_t ms)
{
    uint32_t waited = 0;
    while (waited < ms && s_stress_abort == 0)
    {
        rtos_delay_milliseconds(LCD_STRESS_POLL_MS);
        waited += LCD_STRESS_POLL_MS;
    }
}

static void lcd_stress_task(void *arg)
{
    uint32_t cycle = 0;

    while (s_stress_abort == 0)
    {
        os_memset(&s_stress_ctx, 0, sizeof(s_stress_ctx));

        if (lcd_example_dsi_open(&s_stress_ctx, s_stress_panel, BK_PIXEL_FORMAT_ARGB8888) != AVDK_ERR_OK)
        {
            LOGE("cycle %u: dsi_open failed, stop\r\n", (unsigned)(cycle + 1));
            break;
        }
        if (lcd_example_flush_thread_start(&s_stress_ctx) != AVDK_ERR_OK)
        {
            LOGE("cycle %u: flush_start failed, closing\r\n", (unsigned)(cycle + 1));
            lcd_example_dsi_close(&s_stress_ctx);
            break;
        }

        cycle++;
        LOGI("cycle %u: ON, flush %u ms\r\n", (unsigned)cycle, (unsigned)s_stress_on_ms);
        lcd_stress_delay_abortable(s_stress_on_ms);

        /* Tear down: stop flush, close panel (bk_lodoen_enable(false) powers the rail down). */
        lcd_example_flush_thread_stop(&s_stress_ctx);
        lcd_example_dsi_close(&s_stress_ctx);
        LOGI("cycle %u: OFF (power down)\r\n", (unsigned)cycle);

        if (s_stress_abort)
            break;
        lcd_stress_delay_abortable(LCD_STRESS_OFF_GAP_MS);
    }

    LOGI("stress stopped after %u cycle(s)\r\n", (unsigned)cycle);
    s_stress_thread  = NULL;
    s_stress_running = 0;
    rtos_set_semaphore(&s_stress_done);
    rtos_delete_thread(NULL);
}

void lcd_stress_on_off_start(const char *panel_name, uint32_t on_ms)
{
    /* Restart if already running so a new time takes effect from a clean state. */
    if (s_stress_running)
    {
        LOGI("stress already running, restarting with new params\r\n");
        lcd_stress_stop();
    }

    if (on_ms == 0)
        on_ms = LCD_STRESS_DEFAULT_MS;

    if (rtos_init_semaphore_ex(&s_stress_done, 1, 0) != BK_OK)
    {
        LOGE("init semaphore failed\r\n");
        return;
    }

    s_stress_abort   = 0;
    s_stress_running = 1;
    s_stress_panel   = panel_name;
    s_stress_on_ms   = on_ms;

    if (rtos_create_thread(&s_stress_thread,
                           BEKEN_DEFAULT_WORKER_PRIORITY,
                           "lcd_stress",
                           (beken_thread_function_t)lcd_stress_task,
                           1024 * 4,
                           NULL) != BK_OK)
    {
        LOGE("create stress thread failed\r\n");
        s_stress_thread  = NULL;
        s_stress_running = 0;
        rtos_deinit_semaphore(&s_stress_done);
        return;
    }

    LOGI("stress started: on=%u ms, off gap=%u ms (send 'lcd_stress stop' to end)\r\n",
         (unsigned)on_ms, (unsigned)LCD_STRESS_OFF_GAP_MS);
}

void lcd_stress_stop(void)
{
    if (!s_stress_running)
    {
        LOGI("stress not running\r\n");
        return;
    }

    s_stress_abort = 1;
    /* Wait for the worker to finish its current cycle and power the panel down. */
    rtos_get_semaphore(&s_stress_done, BEKEN_WAIT_FOREVER);
    rtos_deinit_semaphore(&s_stress_done);
    LOGI("stress stop done (power down ensured)\r\n");
}
