#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct bk_24g_osi_funcs_t
{
    uint32_t _version;
    uint32_t size;

    int (*_int_isr_register_wrapper)(void *isr, void *arg);
    int (*_int_isr_unregister_wrapper)(void);
    int (*_int_ctrl_wrapper)(bool en);
    int (*_power_ctrl_wrapper)(uint8_t power_state);
    int (*_clock_ctrl_wrapper)(uint8_t clock_state);
    void (*_vote_rf_ctrl_wrapper)(uint8_t cmd);
    void (*_set_pwr_table_wrapper)(void);

    int (*_delay_milliseconds)(uint32_t num_ms);
    void (*_delay_us)(uint32_t us);
    void (*_log)(int level, char *tag, const char *fmt, ...);
    void (*_reset)(void);


    void *(*_malloc)(unsigned int size);
    void (*_free)(void *p);


    uint32_t (*_disable_int)(void);
    void (*_enable_int)(uint32_t int_level);
    void (*_flush_dcache)(void);
};

int bk_24g_os_adapter_init(void);
int32_t bk24_os_adapter_init(void *osi_funcs);
