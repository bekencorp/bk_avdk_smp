// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "bk_sensor_internal.h"

#define TEMPD_TASK_PRIO                             4
#define TEMPD_TASK_STACK_SIZE                       1536
#define TEMPD_QUEUE_LEN                             (5)

//The maximum retry number when failed to get the temperature, or
//when the temperature is out of range.
#define TEMPD_MAX_RETRY_NUM                         3
#define DEFAULT_TEMPERATURE                         25
#define TEMPD_DISPLAY_RAW_DATA                      0
#define ADC_TEMP_SENSOR_CHANNEL                     7
#define ADC_TEMP_SATURATE_MODE                      ADC_SATURATE_MODE_2

#define ADC_TEMP_BUFFER_SIZE                        (32) /* first half skip, second half for avg */
#if (ADC_TEMP_BUFFER_SIZE & 0x1)
/* odd: middle goes with skip, average latter floor(N/2) samples */
#define ADC_TEMP_BUFFER_SKIP                        ((ADC_TEMP_BUFFER_SIZE + 1) / 2)
#else
#define ADC_TEMP_BUFFER_SKIP                        (ADC_TEMP_BUFFER_SIZE / 2)
#endif
#define ADC_TEMP_CODE_DFT_25DEGREE                  6808
#define ADC_TMEP_LSB_PER_10DEGREE                   400
#define ADC_TEMP_VAL_MIN                            10
#define ADC_TEMP_VAL_MAX                            0x3FFF //for ana_reg5_adc_div=1/3

/* From 0 to ADC_TMEP_DETECT_INTERVAL_CHANGE (120s), the detect interval is
 * ADC_TMEP_DETECT_INTERVAL_INIT (1s); then the detect interval is changed
 * to ADC_TMEP_DETECT_INTERVAL (15s).
 * */
#define ADC_TMEP_DETECT_INTERVAL_INIT               (1)   // 1s
#define ADC_TMEP_DETECT_INTERVAL                    (15)  // 15s  how many second
#define ADC_TMEP_DETECT_INTERVAL_CHANGE             (30) // 30s
#define ADC_TMEP_XTAL_INIT                          (60)  // 60s

#define ADC_TMEP_DIST_INTIAL_VAL                    (0)

#define ADC_TMEP_10DEGREE_PER_DBPWR                 (1) // 7231:1,7231U:1,
#define ADC_XTAL_DIST_INTIAL_VAL                    (70)

/*pre_div = (40MHz / 2 / TEMP_DETEC_ADC_CLK - 1)*/
#define TEMP_DETEC_ADC_CLK                          0x30e035 //div 5 //1250000 //div=15
#define TEMP_DETEC_ADC_SAMPLE_RATE                  0
#define TEMP_DETEC_ADC_STEADY_CTRL                  7
#define TEMP_DETECT_ONESHOT_TIMER                   1


typedef struct {
	uint16_t detect_interval;
	uint16_t detect_threshold;
	uint32_t detect_cnt;
#if TEMP_DETECT_ONESHOT_TIMER
	beken2_timer_t detect_oneshot_timer;
#else
	beken_timer_t detect_timer;
#endif
	float         temp_last;
} temp_detect_config_t;

enum {
	TMPD_PAUSE_TIMER          = 0,
	TMPD_RESTART_TIMER,
	TMPD_CHANGE_PARAM,
	TMPD_TIMER_EXPIRED,
	VOLT_PAUSE_TIMER,
	VOLT_RESTART_TIMER,
	VOLT_TIMER_EXPIRED,
	TMPD_DEINIT,
};

typedef struct temp_message {
	uint32_t temp_msg;
} tempd_msg_t;

void temp_daemon_init(void);
void temp_daemon_deinit(void);
void temp_daemon_stop(void);
void temp_daemon_restart(void);
void temp_daemon_change_config(void);
void temp_daemon_detect_temperature(void);

int bk_sensor_send_msg(uint32_t msg_type);
