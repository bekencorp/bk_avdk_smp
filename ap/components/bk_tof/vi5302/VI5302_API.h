
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __VI5302_API_H
#define __VI5302_API_H


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "VI5302_User_Handle.h"

// IIC的设备地址
#define VI5302_IIC_DEV_ADDR     0xD8

#define OFFSET_CALI_DISTANCE 500


#define REG_MCU_CFG          0x00
#define REG_SYS_CFG          0x01
#define REG_DEV_STAT         0x02
#define REG_INT_STATUS       0x03
#define REG_INTR_MASK        0x04
#define REG_I2C_IDLE_TIME    0x05
#define REG_DEV_ADDR         0x06
#define REG_PW_CTRL          0x07
#define REG_SPECIAL_PURP1    0x08
#define REG_SPECIAL_PURP2    0x09
#define REG_CMD              0x0A
#define REG_SIZE             0x0B
#define REG_SCRATCH_PAD_BASE 0x0C
#define REG_USER_CMD         0x0C
#define REG_USER_LENGTH      0x0D
#define REG_USER_OFFSET      0x0E
#define REG_USER_DATA        0x0F
#define REG_DTOP_3           0x54
#define REG_DTOP_5           0x56
#define REG_CHIPID_BASE      0x4C
#define REG_POWER_CTRL       0x65

#define CMD_SET_RAM_POINTER     0x01
#define CMD_GET_RAM_POINTER     0x02
#define CMD_WRITEFW             0x03
#define CMD_WRITE_RAM           0x04
#define CMD_READ_RAM            0x05
#define CMD_USER_CFG            0x09
#define CMD_XTALK               0x0D
#define CMD_SINGLE_RANGE        0x0E
#define CMD_CONTINOUS_RANGE     0x0F
#define CMD_OPEN_KEEP_ALIVE     0x16
#define CMD_CLOSE_KEEP_ALIVE    0x17
#define CMD_STOP_RANGE          0x1F

#define SUBCMD_READ_SYSTEM_DATA     0x00
#define SUBCMD_WRITE_SYSTEM_DATA    0x01
#define SUBCMD_WRITE_EFUSE          0x02
#define SUBCMD_READ_EFUSE           0x03
#define SUBCMD_READ_FW_VERSION      0x06


//VI5302状态
typedef enum Result_Status
{
	Result_OK       = 0x00,
	Result_ERROR    = 0x90
}Result_Status;

typedef enum 
{
    VI5302_OK                    = 0x00,
    VI5302_IIC_ERROR             = 0x01, //I2C error
    VI5302_ERROR_IIC_ID          = 0x02, //IIC ID不为0xD8
    VI5302_RESULT_ERROR          = 0x04, //未获取到数据
    VI5302_ERROR_TIME_OUT        = 0x10, //获取数值超时
    VI5302_ERROR_XTALK_CALIB     = 0x20, //Xtalk标定失败
    VI5302_ERROR_OFFSET_CALIB    = 0x40, //Offset标定失败
    VI5302_ERROR_IIC_ChangeAddr  = 0x80, //修改I2C地址失败
} VI5302_Status;

typedef struct
{
    uint8_t VI5302_Chip_Version;
    uint16_t VI5302_Cali_ID;
    uint8_t VI5302_Power_Manage_Status;
    uint8_t pileup_num;
    uint8_t confidence_num;

    // 中断状态设置
    uint8_t VI5302_Interrupt_Mode_Status;
    // CG_Pos
    int8_t VI5302_Cali_CG_Pos;
    // CG_Peak
    uint16_t VI5302_Cali_CG_Peak;
    // Offset
    float VI5302_Cali_Offset;
    // MA_Sum
    uint8_t VI5302_MA_Sum;

} VI5302_Params_T;


typedef struct
{
    //校正的tof
    int16_t correction_tof;
    //置信度
    uint8_t confidence;
    //积分次数
    uint32_t intecounts;

}VI5302_MEASURE_TypeDef;

typedef struct {
    int32_t tof;
    uint8_t confidence;
} tof_data;


extern VI5302_Params_T VI5302_Cali_Data;
extern uint8_t VI5302_GPIO_Interrupt_status;

uint8_t VI5302_Get_FW_Version(uint8_t *rVersion, uint8_t *len);
uint8_t VI5302_Clear_Interrupt(void);
uint8_t VI5302_Get_And_Clear_Interrupt(uint8_t *interrupt_status);

uint8_t VI5302_Start_Single_Ranging_Cmd(void);
uint8_t VI5302_Start_Continue_Ranging_Cmd(void);
uint8_t VI5302_Stop_Continue_Ranging_Cmd(void);
uint8_t VI5302_Set_Digital_Clock_Dutycycle(void);
uint8_t VI5302_Set_Keep_Alive(uint8_t value);
uint8_t VI5302_Chip_Register_Init(void);
uint8_t VI5302_Xtalk_Calibration(uint8_t *xtalk_buff);
uint8_t VI5302_Get_Histogram_Data(uint8_t tdc,uint8_t *histogram_buff);
uint8_t VI5302_Offset_Calibration(uint16_t mili,float *ret_offset);
uint8_t VI5302_Reftof_Calibration(uint16_t *ret_reftof);
uint8_t VI5302_Get_Measure_Data(VI5302_MEASURE_TypeDef *result, uint8_t wait_mode);


#endif /* __VI5302_API_H */




