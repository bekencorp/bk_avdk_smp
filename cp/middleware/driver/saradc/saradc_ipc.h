#ifndef _SARADC_IPC_H_
#define _SARADC_IPC_H_

#include <common/bk_include.h>
#include <driver/adc.h>

enum
{
	MB_SARADC_CMD_ACQUIRE = 0,
	MB_SARADC_CMD_INIT,
	MB_SARADC_CMD_ENABLE_BYPASS_CALIBRATION,
	MB_SARADC_CMD_START,
	MB_SARADC_CMD_READ_RAW,
	MB_SARADC_CMD_READ_RAW_DONE,
	MB_SARADC_CMD_STOP,
	MB_SARADC_CMD_DEINIT,
	MB_SARADC_CMD_RELEASE,
	MB_SARADC_CMD_SET_CONFIG,
	MB_SARADC_CMD_GET_CONFIG,
	MB_SARADC_CMD_REGISTER_ISR_CALLBACK,
	MB_SARADC_CMD_UNREGISTER_ISR_CALLBACK,
	MB_SARADC_CMD_EN,
	MB_SARADC_CMD_READ,
	MB_SARADC_CMD_READ_DONE,
	MB_SARADC_CMD_SINGLE_READ,
	MB_SARADC_CMD_SET_CHANNEL,
	MB_SARADC_CMD_SET_MODE,
	MB_SARADC_CMD_GET_MODE,
	MB_SARADC_CMD_SET_CLK,
	MB_SARADC_CMD_SET_SAMPLE_RATE,
	MB_SARADC_CMD_SET_FILTER,
	MB_SARADC_CMD_SET_STEADY_TIME,
	MB_SARADC_CMD_SET_SAMPLE_CNT,
	MB_SARADC_CMD_SET_SATURATE_MODE,
	MB_SARADC_CMD_REGISTER_ISR,
	MB_SARADC_CMD_DATA_CALCULATE,
	MB_SARADC_CMD_CALCULATE,
	MB_SARADC_CMD_INIT_GPIO,
	MB_SARADC_CMD_DEINIT_GPIO,
};

typedef struct
{
    u32                          param;
    u16                          buff[32];
    u32                          size;
    u32                          timeout;
    int16                        ret_status;
    u8                           sample_cnt;
    float                        adc_cali_data;
    u32                          crc;
    adc_config_t                 config;
    void                         *callback;
    void                         *context;
} saradc_cmd_t;

#define SARADC_IPC_READ_SIZE     0x400
#define SARADC_IPC_WRITE_SIZE    0x400

#endif //_SARADC_IPC_H_
// eof