#include "VI5302_Efuse.h"
#include "VI5302_User_Handle.h"
#include "VI5302_API.h"

#define EFUSE_MAX_SIZE 61

uint8_t VI5302_Efuse_Write_Less_Than_61Bytes(uint8_t base, uint8_t *write_buff, uint8_t len)
{
    uint16_t timeout = 1000;
    uint8_t ret = 0;
    uint8_t value = 0;

    if (len > EFUSE_MAX_SIZE || len == 0)
    {
        return 2;
    }

    ret |= VI5302_IIC_Read_One_Byte(REG_MCU_CFG, &value);
    ret |= VI5302_IIC_Write_One_Byte(REG_MCU_CFG, value & (~(1 << 7))); // disable efuse_rd switch
    ret |= VI5302_IIC_Read_One_Byte(REG_POWER_CTRL, &value);
    ret |= VI5302_IIC_Write_One_Byte(REG_POWER_CTRL, value | (1 << 0)); // enable level shift

    VI5302_7V5_Enable(1);

    ret |= VI5302_IIC_Read_One_Byte(REG_MCU_CFG, &value);
    ret |= VI5302_IIC_Write_One_Byte(REG_MCU_CFG, value | (1 << 6));    // enable efuse_w switch
    
    ret |= VI5302_IIC_Write_One_Byte(REG_SCRATCH_PAD_BASE, 0x02);
    ret |= VI5302_IIC_Write_One_Byte(REG_SCRATCH_PAD_BASE + 1, len);
    ret |= VI5302_IIC_Write_One_Byte(REG_SCRATCH_PAD_BASE + 2, base);
    ret |= VI5302_IIC_Write_X_Bytes(REG_SCRATCH_PAD_BASE + 3, write_buff, len);
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_USER_CFG);

    while (timeout != 0)
    {
        ret |= VI5302_IIC_Read_One_Byte(REG_SPECIAL_PURP2, &value);
        if (value == 0x13)
            break;

        timeout--;
        VI5302_Delay_Ms(1);
    }

    VI5302_7V5_Enable(0);

    if (timeout == 0)
    {
        return 1;
    }

    return ret;
}

uint8_t VI5302_Efuse_Write(uint8_t base, uint8_t *write_buff, uint8_t len)
{
    uint8_t i = 0;
    uint8_t ret = 0;

    while (len > EFUSE_MAX_SIZE)
    {
        ret |= VI5302_Efuse_Write_Less_Than_61Bytes(base + i * EFUSE_MAX_SIZE, &write_buff[i * EFUSE_MAX_SIZE], EFUSE_MAX_SIZE);
        len -= EFUSE_MAX_SIZE;
        i++;
        VI5302_Delay_Ms(100);
    }
    if (len > 0)
    {
        ret |= VI5302_Efuse_Write_Less_Than_61Bytes(base + i * EFUSE_MAX_SIZE, &write_buff[i * EFUSE_MAX_SIZE], len);
    }

    return ret;
}

uint8_t VI5302_Efuse_Read_Less_Than_61Bytes(uint8_t base, uint8_t *read_buff, uint8_t len)
{
    uint8_t ret = 0;
    if (len > EFUSE_MAX_SIZE || len == 0)
    {
        return 2;
    }
    ret |= VI5302_IIC_Write_One_Byte(REG_SCRATCH_PAD_BASE + 0, 0x03);
    ret |= VI5302_IIC_Write_One_Byte(REG_SCRATCH_PAD_BASE + 1, len);
    ret |= VI5302_IIC_Write_One_Byte(REG_SCRATCH_PAD_BASE + 2, base);
    ret |= VI5302_IIC_Write_One_Byte(REG_CMD, CMD_USER_CFG);

    VI5302_Delay_Ms(100);

    ret |= VI5302_IIC_Read_X_Bytes(0x0F, read_buff, len);

    return ret;
}

uint8_t VI5302_Efuse_Read(uint8_t base, uint8_t *read_buff, uint8_t len)
{
    uint8_t ret = 0;
    uint8_t i = 0;

    while (len > EFUSE_MAX_SIZE)
    {
        ret |= VI5302_Efuse_Read_Less_Than_61Bytes(base + i * EFUSE_MAX_SIZE, &read_buff[i * EFUSE_MAX_SIZE], EFUSE_MAX_SIZE);

        len -= EFUSE_MAX_SIZE;
        i++;
    }
    if (len > 0)
    {
        ret |= VI5302_Efuse_Read_Less_Than_61Bytes(base + i * EFUSE_MAX_SIZE, &read_buff[i * EFUSE_MAX_SIZE], len);
    }
    return ret;
}
