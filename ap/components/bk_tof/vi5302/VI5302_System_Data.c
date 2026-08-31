#include "VI5302_System_Data.h"
#include "VI5302_User_Handle.h"
#include "VI5302_API.h"

uint8_t VI5302_Write_System_Data(uint8_t offset_addr, uint8_t *buff, uint8_t len)
{
    uint8_t ret = 0;

    ret |= VI5302_IIC_Write_One_Byte(REG_USER_CMD, SUBCMD_WRITE_SYSTEM_DATA);
    ret |= VI5302_IIC_Write_One_Byte(REG_USER_LENGTH, len);
    ret |= VI5302_IIC_Write_One_Byte(REG_USER_OFFSET, offset_addr);
    ret |= VI5302_IIC_Write_X_Bytes(REG_USER_DATA, buff, len);
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_USER_CFG);

    VI5302_Delay_Ms(5);
    return ret;
}

uint8_t VI5302_Read_System_Data(uint8_t offset_addr, uint8_t *buff, uint8_t len)
{
    uint8_t ret = 0;

    ret |= VI5302_IIC_Write_One_Byte(REG_USER_CMD, SUBCMD_READ_SYSTEM_DATA);
    ret |= VI5302_IIC_Write_One_Byte(REG_USER_LENGTH, len);
    ret |= VI5302_IIC_Write_One_Byte(REG_USER_OFFSET, offset_addr);
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_USER_CFG);

    VI5302_Delay_Ms(5);
    ret |= VI5302_IIC_Read_X_Bytes(REG_USER_DATA, buff, len);

    return ret;
}

// RCO
uint8_t VI5302_Set_Sys_RCO(uint8_t value)
{
    return VI5302_Write_System_Data(0x0, &value, 1);
}

uint8_t VI5302_Get_Sys_RCO(uint8_t *value)
{
    return VI5302_Read_System_Data(0x0, value, 1);
}

// BVD
uint8_t VI5302_Set_Sys_BVD(uint8_t value)
{
    return VI5302_Write_System_Data(0x1, &value, 1);
}

uint8_t VI5302_Get_Sys_BVD(uint8_t *value)
{
    return VI5302_Read_System_Data(0x1, value, 1);
}

// TDC
uint8_t VI5302_Set_Sys_TDC(uint8_t value)
{
    return VI5302_Write_System_Data(0x2, &value, 1);
}

uint8_t VI5302_Get_Sys_TDC(uint8_t *value)
{
    return VI5302_Read_System_Data(0x2, value, 1);
}

// TDC_Delay
uint8_t VI5302_Set_Sys_TDC_Delay(uint8_t value)
{
    return VI5302_Write_System_Data(0x3, &value, 1);
}

uint8_t VI5302_Get_Sys_TDC_Delay(uint8_t *value)
{
    return VI5302_Read_System_Data(0x3, value, 1);
}

// PW_DS
uint8_t VI5302_Set_Sys_PW_DS(uint8_t value)
{
    return VI5302_Write_System_Data(0x4, &value, 1);
}

uint8_t VI5302_Get_Sys_PW_DS(uint8_t *value)
{
    return VI5302_Read_System_Data(0x4, value, 1);
}

// PW
uint8_t VI5302_Set_Sys_PW(uint8_t value)
{
    return VI5302_Write_System_Data(0x5, &value, 1);
}

uint8_t VI5302_Get_Sys_PW(uint8_t *value)
{
    return VI5302_Read_System_Data(0x5, value, 1);
}

// Integral_Time
uint8_t VI5302_Set_Sys_Integral_Time(uint32_t integral_time)
{
    uint8_t buff[3];
    buff[0] = integral_time & 0xff;
    buff[1] = (integral_time >> 8) & 0xff;
    buff[2] = (integral_time >> 16) & 0xff;
    return VI5302_Write_System_Data(0x10, buff, 3);
}

uint8_t VI5302_Get_Sys_Integral_Time(uint32_t *integral_time)
{
    uint8_t ret = 0;
    uint8_t buff[3];

    ret |= VI5302_Read_System_Data(0x10, buff, 3);
    *integral_time = (buff[2] << 16) + (buff[1] << 8) + (buff[0] << 0);

    return ret;
}

// delaytime
uint8_t VI5302_Set_Sys_DelayTime(uint16_t delay_time)
{
    return VI5302_Write_System_Data(0x13, (uint8_t *)&delay_time, 2);
}

uint8_t VI5302_Get_Sys_DelayTime(uint16_t *delay_time)
{
    return VI5302_Read_System_Data(0x13, (uint8_t *)&delay_time, 2);
}

// MA
uint8_t VI5302_Set_Sys_Histogram_MA_Window_Data(uint8_t *setting_buff)
{
    return VI5302_Write_System_Data(0x08, setting_buff, 8);
}

uint8_t VI5302_Get_Sys_Histogram_MA_Window_Data(uint8_t *getting_buff)
{
    uint8_t ret = 0, i = 0;
    ret |= VI5302_Read_System_Data(0x08, getting_buff, 8);

    getting_buff[8] = 0;
    for (i = 0; i < 8; i++)
    {
        // MA系数之和
        getting_buff[8] += ((getting_buff[i] & 0x0F) + ((getting_buff[i] >> 4) & 0x0F));
    }
    return ret;
}

// Xtalk Offset
uint8_t VI5302_Set_Sys_Xtalk_Offset(uint8_t value)
{
    return VI5302_Write_System_Data(0x15, &value, 1);
}

uint8_t VI5302_Get_Sys_Xtalk_Offset(uint8_t *value)
{
    return VI5302_Read_System_Data(0x15, value, 1);
}

// Xtalk Position
uint8_t VI5302_Set_Sys_Xtalk_Position(uint8_t value)
{
    return VI5302_Write_System_Data(0x16, &value, 1);
}

uint8_t VI5302_Get_Sys_Xtalk_Position(uint8_t *value)
{
    return VI5302_Read_System_Data(0x16, value, 1);
}

// MISC EN
uint8_t VI5302_Set_Sys_Misc(uint8_t value)
{
    return VI5302_Write_System_Data(0x17, &value, 1);
}

uint8_t VI5302_Get_Sys_Misc(uint8_t *value)
{
    return VI5302_Read_System_Data(0x17, value, 1);
}

uint8_t VI5302_Set_Sys_Temperature_Enable(uint8_t value)
{
    return VI5302_Write_System_Data(0x17, &value, 1);
}

uint8_t VI5302_Get_Sys_Temperature_Enable(uint8_t *value)
{
    return VI5302_Write_System_Data(0x17, value, 1);
}

uint8_t VI5302_Set_Integralcounts_Frame(uint8_t fps, uint32_t intecoutns)
{
    uint8_t ret = 0;

    uint32_t inte_time = 0;
    uint32_t fps_time = 0;
    int32_t delay_time = 0;
    uint16_t delay_counts = 0;

    inte_time = intecoutns * 1330 / 10;
    fps_time = 1000000000 / fps;
    delay_time = fps_time - inte_time - 1700000 - 898000;
    delay_time = (delay_time < 0) ? 0 : delay_time;
    delay_counts = (uint16_t)(delay_time / 100000);

    ret |= VI5302_Set_Sys_Integral_Time(intecoutns);
    ret |= VI5302_Set_Sys_DelayTime(delay_counts);

    return ret;
}



