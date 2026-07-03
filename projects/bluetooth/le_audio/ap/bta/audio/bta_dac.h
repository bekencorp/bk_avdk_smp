#pragma once

enum
{
    DAC_STATE_INVALID = 0,
    DAC_STATE_START,
    DAC_STATE_STOP,
};


typedef struct
{
    uint32_t sample_rate;
    uint16_t duration; //us
} bta_dac_config_t;

int bta_dac_write(uint8_t *data, uint32_t length);
uint8_t bta_dac_get_state(void);
uint32_t bta_dac_get_fill_size(void);

int bta_dac_init(bta_dac_config_t *cfg);
int bta_dac_deinit(void);

int bta_dac_start(void);
int bta_dac_stop(void);

uint16_t bta_dac_get_pcm_length(void);
void bta_dac_set_gain_value(uint8_t gain);
bk_err_t bta_dac_set_gain(uint8_t gain);

