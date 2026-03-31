#include "command_ate.h"

#if CONFIG_ATE_TEST
#include "sys_hal.h"
#include "aon_pmu_hal.h"
#include <driver/otp.h>
#include <driver/efuse.h>

extern UINT32 device_id;

extern void bk_delay_us(UINT32 us);
extern void ate_time_delay(volatile uint32_t times);
void rwnx_cal_dia_start();
void rwnx_cal_dia_stop();
void rwnx_cal_set_reg_mod_pa(UINT16 reg_mod, UINT16 reg_pa);

static int check_efuse_error_range(UINT8 addr, UINT8 new_byte, UINT8 error_range)
{
    UINT8 addr_efuse, data;
    UINT16 old_value;
    UINT16 new_value;
    int ret;

    addr_efuse = addr;
    data = 0;

    ret = bk_efuse_read_byte(addr_efuse, &data);
    if (ret != 0)
    {
        return -1;
    }

    new_value = new_byte;
    old_value = data;

    if ((new_value > old_value + error_range) || (old_value > new_value + error_range))
    {
        return -1;
    }

    return 0;
}

/**
 * user  format: [0x80, 0x7F]=>[-128, 127], base on vdddig 4, should adjust 
 * reg   format: vdddig [0,7], bandgap [0x00, 0x3F]
 * efuse format: xxyy yyyy, xx {00 01 10 11}=>{4 5 6 3}
 */
int sctrl_convert_vdddig_from_efuse_to_user(INT8 in_value, INT8 *out_value)
{
    if (0xC0 == (in_value & 0xC0))
    {
        /* VDDDIG=3 */
        *out_value = (in_value & 0x3F) - 0x20;
    }
    else if (0x40 == (in_value & 0xC0))
    {
        /* VDDDIG=5 */
        *out_value = (in_value & 0x3F) + 0x20;
    }
    else if (0x80 == (in_value & 0xC0))
    {
        /* VDDDIG=6 */
        *out_value = (in_value & 0x3F) + 0x40;
    }
    else
    {
        /* VDDDIG=4 */
        *out_value = in_value;
    }

    return 0;
}

int sctrl_convert_vdddig_from_user_to_efuse(INT8 in_value, INT8 *out_value)
{
    UINT32 vdddig;
    INT32 bandgap;

    /* should call sctrl_convert_vdddig_from_user_to_reg to convert bandgap to [0x10, 0x2F] */
    /* but legacy code already use vdddig=4 and bandgap=[0,63] */
#if 0
    sctrl_convert_vdddig_from_user_to_reg(in_value, &vdddig, &bandgap);
#else
    bandgap = (INT32)in_value;
    vdddig  = 4;

    while ((bandgap < 0x00) && (vdddig > 3))
    {
        vdddig -= 1;
        bandgap += 0x20;
    }
    while ((bandgap >= 0x40) && (vdddig < 6))
    {
        vdddig += 1;
        bandgap -= 0x20;
    }
#endif
    if ((vdddig < 3) || (6 < vdddig))
    {
        return -1;
    }
    else if (vdddig == 3)
    {
        *out_value = bandgap | (0x3 << 6);
    }
    else
    {
        *out_value = bandgap | ((vdddig - 4) << 6);
    }

    return 0;
}

int sctrl_convert_vdddig_from_user_to_reg(INT8 in_value, UINT32 *out_vdddig, INT32 *out_bandgap)
{
    *out_bandgap = (INT32)in_value;
    *out_vdddig  = 4;

    while ((*out_bandgap < 0x10) && (*out_vdddig > 3))
    {
        *out_vdddig -= 1;
        *out_bandgap += 0x20;
    }
    while ((*out_bandgap >= 0x30) && (*out_vdddig < 6))
    {
        *out_vdddig += 1;
        *out_bandgap -= 0x20;
    }

    return 0;
}

#define BANDGAP_BGCAL_MANUAL_MASK    (0x3FU)
#define SYS_DIG_VDD_ACTIVE_MASK      (0xF)

int sctrl_load_vdddig_from_efuse(void)
{
    bk_err_t result;
    uint8_t new_bandgap = 0xFF;

    result = bk_otp_apb_read(OTP_VDDDIG_BANDGAP, (uint8_t *)&new_bandgap, sizeof(new_bandgap));
    if ((BK_OK != result) || (0x00 == new_bandgap) || (0xFF == new_bandgap))
    {
        return -1;
    }

    sys_hal_set_bgcalm(new_bandgap);

    return 0;
}

int sctrl_set_vdddig_1voltage(void)
{
    //not support
    return 0;
}

int sctrl_set_vdddig_to_reg(const unsigned char *content, int cnt, UINT8 *tx_buffer)
{
    tx_buffer[0] = 0x55;

    if (cnt < 3)
    {
        /* 55 54 xx - xx: vdddig */
        tx_buffer[1] = 0xCC;
    }
    else
    {
        UINT32 vdddig = content[2];

        if (vdddig > SYS_DIG_VDD_ACTIVE_MASK)
        {
            /* failed */
            tx_buffer[1] = 0xCC;
        }
        else
        {
            /* success */
            sys_hal_set_vdd_value(vdddig);
            tx_buffer[1] = 0x33;
        }
    }
    uart_send_bytes_for_ate(tx_buffer, 2);

	return 0;
}

int sctrl_set_bandgap_to_efuse(const unsigned char *content, int cnt, UINT8 *tx_buffer)
{
    UINT8 bandgap = 0;

	tx_buffer[0] = 0x55;
    if (cnt < 3)
    {
        if (bk_otp_apb_read(OTP_VDDDIG_BANDGAP, &bandgap, sizeof(bandgap)) == 0)
        {
            /* success */
            tx_buffer[0] = 0x0E;
            tx_buffer[1] = bandgap;
            uart_send_bytes_for_ate(tx_buffer, 2);
            return 0;
        }
        else
        {
            tx_buffer[1] = 0xAA;
            uart_send_bytes_for_ate(tx_buffer, 2);
            return -1;
        }
    }
    bandgap = content[2];
    if (bandgap > BANDGAP_BGCAL_MANUAL_MASK)
    {
        tx_buffer[1] = 0xAA;
        uart_send_bytes_for_ate(tx_buffer, 2);
        return -1;
    }

    if (bk_otp_apb_update(OTP_VDDDIG_BANDGAP, &bandgap, sizeof(bandgap)) == 0)
    {
        /* success */
        sys_hal_set_bgcalm(bandgap);
        tx_buffer[1] = 0x33;
    }
    else
    {
        tx_buffer[1] = 0xCC;
    }

    uart_send_bytes_for_ate(tx_buffer, 2);

    return 0;
}

int sctrl_set_bandgap_to_reg(const unsigned char *content, int cnt, UINT8 *tx_buffer)
{
    UINT8 bandgap = 0;

    if (cnt > 2) {
		//set bandgap
		bandgap = content[2];
    } else {
		bandgap = sys_hal_cali_bgcalm();
    }

    sys_hal_set_bgcalm(bandgap);

	tx_buffer[0] = 0xE;
	tx_buffer[1] = bandgap;
    uart_send_bytes_for_ate(tx_buffer, 2);

    return 0;
}

int sctrl_check_dpll_unlock(UINT8 bandgap, UINT8 *tx_buffer)
{
    //not support
    return 0;
}

int sctrl_set_dia_to_efuse(const unsigned char *content, int cnt, UINT8 *tx_buffer)
{
    UINT8 dia = 0;

	tx_buffer[0] = 0x55;
    if (cnt < 3)
    {
        if (bk_otp_apb_read(OTP_DIA, &dia, sizeof(dia)) == 0)
        {
            /* success */
            rwnx_cal_set_reg_mod_pa(0, dia);
            tx_buffer[0] = 0x0E;
            tx_buffer[1] = dia;
            uart_send_bytes_for_ate(tx_buffer, 2);
            return 0;
        }
        else
        {
            tx_buffer[1] = 0xAA;
            uart_send_bytes_for_ate(tx_buffer, 2);
            return -1;
        }
    }
    dia = content[2];
    if (dia > 0x1F)
    {
        tx_buffer[1] = 0xDD;
        uart_send_bytes_for_ate(tx_buffer, 2);
        return -1;
    }

    if (bk_otp_apb_update(OTP_DIA, &dia, sizeof(dia)) == 0)
    {
        /* success */
        tx_buffer[1] = 0x33;
    }
    else
    {
        tx_buffer[1] = 0xCC;
    }

	rwnx_cal_dia_stop();
    uart_send_bytes_for_ate(tx_buffer, 2);

    return 0;
}

int sctrl_set_dia_to_reg(const unsigned char *content, int cnt, UINT8 *tx_buffer)
{
    UINT8 dia = 0;

	rwnx_cal_dia_start();
    if (cnt > 2) {
        //set dia
		dia = content[2];
        tx_buffer[0] = 0xE;
        tx_buffer[1] = dia;
        rwnx_cal_set_reg_mod_pa(0, dia);
    } else {
        tx_buffer[0] = 0x55;
        tx_buffer[1] = 0xCC;
    }

    uart_send_bytes_for_ate(tx_buffer, 2);

    return 0;
}

#endif // CFG_ATE_TEST
