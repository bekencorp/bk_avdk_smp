#ifndef VND_CAL_H
#define VND_CAL_H

#include "bk_arm_arch.h"
#include "bk_misc.h"

typedef struct tmp_pwr_st {
	unsigned trx0x0c_12_15 : 1;
	signed p_index_delta : 7;
	signed p_index_delta_g : 7;
	signed p_index_delta_ble : 7;
	signed p_index_delta_thread : 7;
	signed xtal_c_dlta : 10;
} TMP_PWR_ST, *TMP_PWR_PTR;

typedef struct txpwr_cal_st {
	UINT8 channel;
	UINT8 value;
} TXPWR_CAL_ST, *TXPWR_CAL_PTR;

typedef struct {
	UINT32 cali_mode;
	INT32 gtx_tssi_thred_chan1_b;
	INT32 gtx_tssi_thred_chan7_b;
	INT32 gtx_tssi_thred_chan13_b;
	INT32 gtx_tssi_thred_chan1_g;
	INT32 gtx_tssi_thred_chan7_g;
	INT32 gtx_tssi_thred_chan13_g;
} AUTO_PWR_CALI_CONTEXT;

typedef struct {
	unsigned short pregain : 12;
	unsigned short unuse : 4;
} PWR_REGS;

void vnd_cal_overlay(void);

#endif
