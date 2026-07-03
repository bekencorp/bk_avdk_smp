#pragma once

#include <stdbool.h>
#include "bk_ring_buffer_node.h"
#include "bk_ring_buffer_octets.h"
#include <os/os.h>
#include <components/bluetooth/bk_dm_bap_types.h>
#include <components/bluetooth/bk_assigned_numbers.h>
#include <modules/lc3_codec.h>

#include <bk_list.h>
#include <avdk_list.h>


#define GPIO_DVP_D0     (32)
#define GPIO_DVP_D1     (33)
#define GPIO_DVP_D2     (34)
#define GPIO_DVP_D3     (35)
#define GPIO_DVP_D4     (36)
#define GPIO_DVP_D5     (37)
#define GPIO_DVP_D6     (38)
#define GPIO_DVP_D7     (39)

#define SPEAKER_GAIN_MIN    (0x0)
#define SPEAKER_GAIN_MAX    (0x3F)
#define VOLUME_STEP_MAX     (16)
#define VOLUME_STEP_MIN     (0)
#define GET_VOLUME_STEP(value) (((value) + 5) >> 3)
#define GET_VOLUME_GAIN(value) ((value) >> 1)

#define LC3_ENC_SAMPLE_RATE                     (48000)
#define LC3_ENC_CHANNELS                        (1)
#define LC3_ENC_DURATION                        (10000)
#define LC3_ENC_PER_CODE_FRAME                  (100)

#define DROP_MAX_COUNT  (10)

#if 0

#define LC3_DATA_RECV_START()       do { GPIO_DOWN(GPIO_DVP_D0); GPIO_UP(GPIO_DVP_D0); } while (0)
#define LC3_DATA_RECV_END()         do { GPIO_DOWN(GPIO_DVP_D0); } while (0)

#define RING_BUFFER_FILL_START()    do { GPIO_DOWN(GPIO_DVP_D1); GPIO_UP(GPIO_DVP_D1); } while (0)
#define RING_BUFFER_FILL_END()      do { GPIO_DOWN(GPIO_DVP_D1); } while (0)

#define DAC_ISR_START()             do { GPIO_DOWN(GPIO_DVP_D2); GPIO_UP(GPIO_DVP_D2); } while (0)
#define DAC_ISR_END()               do { GPIO_DOWN(GPIO_DVP_D2); } while (0)

#define LC3_DATA_DECODE_START()     do { GPIO_DOWN(GPIO_DVP_D3); GPIO_UP(GPIO_DVP_D3); } while (0)
#define LC3_DATA_DECODE_END()       do { GPIO_DOWN(GPIO_DVP_D3); } while (0)
#else
#define LC3_DATA_RECV_START()
#define LC3_DATA_RECV_END()

#define RING_BUFFER_FILL_START()
#define RING_BUFFER_FILL_END()

#define DAC_ISR_START()
#define DAC_ISR_END()

#define LC3_DATA_DECODE_START()
#define LC3_DATA_DECODE_END()
#endif

#define CONFIG_BOARD_AUDIO_CHANNLE_NUM     1

#define CODEC_AUDIO_LC3                      0x03U

typedef struct
{
    LIST_HEADER_T list;
    uint8_t format;
    uint16_t length;
    void *context;
    uint8_t payload[];
} bta_encoded_data_t;



typedef struct
{
    uint8_t format;
    uint8_t channels;
    uint32_t sample_rate;
    uint32_t frame_length;
    uint32_t duration;
} bta_codec_config_t;

typedef struct
{
    uint8_t encoded;
    uint8_t dac_reset;

    bta_codec_config_t dec_cfg;
    bta_codec_config_t enc_cfg;

    uint8_t volume;
} bta_audio_info_t;

int bta_audio_init(void);

void bta_audio_lc3_enc_init(bta_codec_config_t *cfg);
void bta_audio_lc3_enc_deinit(void);
void bta_audio_lc3_enc_timer_start(void);
void bta_audio_lc3_enc_timer_stop(void);
void bta_audio_lc3_dec_init(void);
void bta_audio_lc3_dec_deinit(void);
void bta_audio_set_dec_config(bta_codec_config_t *cfg);
void bta_audio_lc3_dec_data_send(void *data, uint32_t length);

void bta_audio_set_abs_volume(uint8_t per);


void bta_audio_volume_up(void);
void bta_audio_volume_down(void);
uint8_t bta_audio_calc_volume(uint8_t value, uint8_t up);
bk_err_t bta_audio_set_speaker_gain(uint8_t gain);
void bta_audio_set_speaker_gain_value(uint8_t gain);

//EXTERNAl
extern void print_hex_dump(const char *prefix, const void *buf, int len);
