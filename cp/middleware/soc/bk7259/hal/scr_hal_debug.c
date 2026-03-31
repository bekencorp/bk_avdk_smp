// Copyright 2022-2023 Beken
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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#include "hal_config.h"
#include "scr_hw.h"
#include "scr_hal.h"

typedef void (*scr_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	scr_dump_fn_t fn;
} scr_reg_fn_map_t;

static void scr_dump_DeviceID(void)
{
	SOC_LOGD("DeviceID: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x0 << 2)));
}

static void scr_dump_VersionID(void)
{
	SOC_LOGD("VersionID: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x1 << 2)));
}

static void scr_dump_Global_CTRL(void)
{
	scr_Global_CTRL_t *r = (scr_Global_CTRL_t *)(SOC_SCR_REG_BASE + (0x2 << 2));

	SOC_LOGD("Global_CTRL: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x2 << 2)));
	SOC_LOGD("	soft_rst: %8x\r\n", r->soft_rst);
	SOC_LOGD("	bypass_ckg: %8x\r\n", r->bypass_ckg);
	SOC_LOGD("	reserved_bit_2_31: %8x\r\n", r->reserved_bit_2_31);
}

static void scr_dump_Global_Status(void)
{
	SOC_LOGD("Global_Status: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x3 << 2)));
}

static void scr_dump_CTRL1(void)
{
	scr_CTRL1_t *r = (scr_CTRL1_t *)(SOC_SCR_REG_BASE + (0x4 << 2));

	SOC_LOGD("CTRL1: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x4 << 2)));
	SOC_LOGD("	invlev: %8x\r\n", r->invlev);
	SOC_LOGD("	invord: %8x\r\n", r->invord);
	SOC_LOGD("	pech2fifo: %8x\r\n", r->pech2fifo);
	SOC_LOGD("	reserved_3_5: %8x\r\n", r->reserved_3_5);
	SOC_LOGD("	clkstop: %8x\r\n", r->clkstop);
	SOC_LOGD("	clkstopval: %8x\r\n", r->clkstopval);
	SOC_LOGD("	txen: %8x\r\n", r->txen);
	SOC_LOGD("	rxen: %8x\r\n", r->rxen);
	SOC_LOGD("	ts2fifo: %8x\r\n", r->ts2fifo);
	SOC_LOGD("	t0t1: %8x\r\n", r->t0t1);
	SOC_LOGD("	atrstflush: %8x\r\n", r->atrstflush);
	SOC_LOGD("	tcken: %8x\r\n", r->tcken);
	SOC_LOGD("	reserved_14_14: %8x\r\n", r->reserved_14_14);
	SOC_LOGD("	ginten: %8x\r\n", r->ginten);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_CTRL2(void)
{
	scr_CTRL2_t *r = (scr_CTRL2_t *)(SOC_SCR_REG_BASE + (0x5 << 2));

	SOC_LOGD("CTRL2: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x5 << 2)));
	SOC_LOGD("	reserved_0_0: %8x\r\n", r->reserved_0_0);
	SOC_LOGD("	reserved_1_1: %8x\r\n", r->reserved_1_1);
	SOC_LOGD("	warmrst: %8x\r\n", r->warmrst);
	SOC_LOGD("	act: %8x\r\n", r->act);
	SOC_LOGD("	deact: %8x\r\n", r->deact);
	SOC_LOGD("	vcc18: %8x\r\n", r->vcc18);
	SOC_LOGD("	vcc30: %8x\r\n", r->vcc30);
	SOC_LOGD("	vcc50: %8x\r\n", r->vcc50);
	SOC_LOGD("	act_fast: %8x\r\n", r->act_fast);
	SOC_LOGD("	reserved_9_15: %8x\r\n", r->reserved_9_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_SCPADS(void)
{
	scr_SCPADS_t *r = (scr_SCPADS_t *)(SOC_SCR_REG_BASE + (0x6 << 2));

	SOC_LOGD("SCPADS: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x6 << 2)));
	SOC_LOGD("	diraccpads: %8x\r\n", r->diraccpads);
	SOC_LOGD("	dscio: %8x\r\n", r->dscio);
	SOC_LOGD("	dscclk: %8x\r\n", r->dscclk);
	SOC_LOGD("	dscrst: %8x\r\n", r->dscrst);
	SOC_LOGD("	dscvcc: %8x\r\n", r->dscvcc);
	SOC_LOGD("	autoadeavpp: %8x\r\n", r->autoadeavpp);
	SOC_LOGD("	dscvppen: %8x\r\n", r->dscvppen);
	SOC_LOGD("	dscvpppp: %8x\r\n", r->dscvpppp);
	SOC_LOGD("	dscfcb: %8x\r\n", r->dscfcb);
	SOC_LOGD("	scpresent: %8x\r\n", r->scpresent);
	SOC_LOGD("	reserved_10_15: %8x\r\n", r->reserved_10_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_INTEN1(void)
{
	scr_INTEN1_t *r = (scr_INTEN1_t *)(SOC_SCR_REG_BASE + (0x7 << 2));

	SOC_LOGD("INTEN1: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x7 << 2)));
	SOC_LOGD("	txfidone: %8x\r\n", r->txfidone);
	SOC_LOGD("	txfiempty: %8x\r\n", r->txfiempty);
	SOC_LOGD("	rxfifull: %8x\r\n", r->rxfifull);
	SOC_LOGD("	clkstoprun: %8x\r\n", r->clkstoprun);
	SOC_LOGD("	txdone: %8x\r\n", r->txdone);
	SOC_LOGD("	rxdone: %8x\r\n", r->rxdone);
	SOC_LOGD("	txperr: %8x\r\n", r->txperr);
	SOC_LOGD("	rxperr: %8x\r\n", r->rxperr);
	SOC_LOGD("	c2cfull: %8x\r\n", r->c2cfull);
	SOC_LOGD("	rxthreshold: %8x\r\n", r->rxthreshold);
	SOC_LOGD("	atrfail: %8x\r\n", r->atrfail);
	SOC_LOGD("	atrdone: %8x\r\n", r->atrdone);
	SOC_LOGD("	screm: %8x\r\n", r->screm);
	SOC_LOGD("	scins: %8x\r\n", r->scins);
	SOC_LOGD("	scact: %8x\r\n", r->scact);
	SOC_LOGD("	scdeact: %8x\r\n", r->scdeact);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_INTSTAT1(void)
{
	scr_INTSTAT1_t *r = (scr_INTSTAT1_t *)(SOC_SCR_REG_BASE + (0x8 << 2));

	SOC_LOGD("INTSTAT1: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x8 << 2)));
	SOC_LOGD("	txfidone: %8x\r\n", r->txfidone);
	SOC_LOGD("	txfiempty: %8x\r\n", r->txfiempty);
	SOC_LOGD("	rxfifull: %8x\r\n", r->rxfifull);
	SOC_LOGD("	clkstoprun: %8x\r\n", r->clkstoprun);
	SOC_LOGD("	txdone: %8x\r\n", r->txdone);
	SOC_LOGD("	rxdone: %8x\r\n", r->rxdone);
	SOC_LOGD("	txperr: %8x\r\n", r->txperr);
	SOC_LOGD("	rxperr: %8x\r\n", r->rxperr);
	SOC_LOGD("	c2cfull: %8x\r\n", r->c2cfull);
	SOC_LOGD("	rxthreshold: %8x\r\n", r->rxthreshold);
	SOC_LOGD("	atrfail: %8x\r\n", r->atrfail);
	SOC_LOGD("	atrdone: %8x\r\n", r->atrdone);
	SOC_LOGD("	screm: %8x\r\n", r->screm);
	SOC_LOGD("	scins: %8x\r\n", r->scins);
	SOC_LOGD("	scact: %8x\r\n", r->scact);
	SOC_LOGD("	scdeact: %8x\r\n", r->scdeact);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_FIFOCTRL(void)
{
	scr_FIFOCTRL_t *r = (scr_FIFOCTRL_t *)(SOC_SCR_REG_BASE + (0x9 << 2));

	SOC_LOGD("FIFOCTRL: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x9 << 2)));
	SOC_LOGD("	txfiempty: %8x\r\n", r->txfiempty);
	SOC_LOGD("	txfifull: %8x\r\n", r->txfifull);
	SOC_LOGD("	txfiflush: %8x\r\n", r->txfiflush);
	SOC_LOGD("	reserved_3_7: %8x\r\n", r->reserved_3_7);
	SOC_LOGD("	rxfiempty: %8x\r\n", r->rxfiempty);
	SOC_LOGD("	rxfifull: %8x\r\n", r->rxfifull);
	SOC_LOGD("	rxfiflush: %8x\r\n", r->rxfiflush);
	SOC_LOGD("	reserved_11_15: %8x\r\n", r->reserved_11_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_Legacy_RX_FIFO_Counter(void)
{
	scr_Legacy_FIFO_Counter_t *r = (scr_Legacy_FIFO_Counter_t *)(SOC_SCR_REG_BASE + (0xa << 2));

	SOC_LOGD("Legacy_RX_FIFO_Counter: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0xa << 2)));
	SOC_LOGD("	legacy_rx_fifo_cnt: %8x\r\n", r->legacy_rx_fifo_cnt);
	SOC_LOGD("	legacy_tx_fifo_cnt: %8x\r\n", r->legacy_tx_fifo_cnt);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_RX_FIFO_Threshold(void)
{
	scr_RX_FIFO_Threshold_t *r = (scr_RX_FIFO_Threshold_t *)(SOC_SCR_REG_BASE + (0xb << 2));

	SOC_LOGD("RX_FIFO_Threshold: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0xb << 2)));
	SOC_LOGD("	rx_fifo_thd: %8x\r\n", r->rx_fifo_thd);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_TX_Repeat(void)
{
	scr_TX_Repeat_t *r = (scr_TX_Repeat_t *)(SOC_SCR_REG_BASE + (0xc << 2));

	SOC_LOGD("TX_Repeat: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0xc << 2)));
	SOC_LOGD("	tx_repeat: %8x\r\n", r->tx_repeat);
	SOC_LOGD("	rx_repeat: %8x\r\n", r->rx_repeat);
	SOC_LOGD("	reserved_8_15: %8x\r\n", r->reserved_8_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_Smart_Card_Clock_Divisor(void)
{
	scr_Smart_Card_Clock_Divisor_t *r = (scr_Smart_Card_Clock_Divisor_t *)(SOC_SCR_REG_BASE + (0xd << 2));

	SOC_LOGD("Smart_Card_Clock_Divisor: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0xd << 2)));
	SOC_LOGD("	smart_card_clk_div: %8x\r\n", r->smart_card_clk_div);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_Baud_Clock_Divisor(void)
{
	scr_Baud_Clock_Divisor_t *r = (scr_Baud_Clock_Divisor_t *)(SOC_SCR_REG_BASE + (0xe << 2));

	SOC_LOGD("Baud_Clock_Divisor: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0xe << 2)));
	SOC_LOGD("	band_clk_div: %8x\r\n", r->band_clk_div);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_Smart_Card_Guardtime(void)
{
	scr_Smart_Card_Guardtime_t *r = (scr_Smart_Card_Guardtime_t *)(SOC_SCR_REG_BASE + (0xf << 2));

	SOC_LOGD("Smart_Card_Guardtime: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0xf << 2)));
	SOC_LOGD("	smart_card_guardtime: %8x\r\n", r->smart_card_guardtime);
	SOC_LOGD("	reserved_8_15: %8x\r\n", r->reserved_8_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_Activation_Deactivation_Time(void)
{
	scr_Activation_Deactivation_Time_t *r = (scr_Activation_Deactivation_Time_t *)(SOC_SCR_REG_BASE + (0x10 << 2));

	SOC_LOGD("Activation_Deactivation_Time: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x10 << 2)));
	SOC_LOGD("	activation_time: %8x\r\n", r->activation_time);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_Reset_Duration(void)
{
	scr_Reset_Duration_t *r = (scr_Reset_Duration_t *)(SOC_SCR_REG_BASE + (0x11 << 2));

	SOC_LOGD("Reset_Duration: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x11 << 2)));
	SOC_LOGD("	reset_duration: %8x\r\n", r->reset_duration);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_ATR_Start_Limit(void)
{
	scr_ATR_Start_Limit_t *r = (scr_ATR_Start_Limit_t *)(SOC_SCR_REG_BASE + (0x12 << 2));

	SOC_LOGD("ATR_Start_Limit: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x12 << 2)));
	SOC_LOGD("	atr_start_limit: %8x\r\n", r->atr_start_limit);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_Two_Characters_Delay_Limit_L(void)
{
	scr_Two_Characters_Delay_Limit_L_t *r = (scr_Two_Characters_Delay_Limit_L_t *)(SOC_SCR_REG_BASE + (0x13 << 2));

	SOC_LOGD("Two_Characters_Delay_Limit_L: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x13 << 2)));
	SOC_LOGD("	two_char_delay_limit_l: %8x\r\n", r->two_char_delay_limit_l);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_INTEN2(void)
{
	scr_INTEN2_t *r = (scr_INTEN2_t *)(SOC_SCR_REG_BASE + (0x14 << 2));

	SOC_LOGD("INTEN2: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x14 << 2)));
	SOC_LOGD("	txthreshold: %8x\r\n", r->txthreshold);
	SOC_LOGD("	tckerr: %8x\r\n", r->tckerr);
	SOC_LOGD("	reserved_2_15: %8x\r\n", r->reserved_2_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_INTSTAT2(void)
{
	scr_INTSTAT2_t *r = (scr_INTSTAT2_t *)(SOC_SCR_REG_BASE + (0x15 << 2));

	SOC_LOGD("INTSTAT2: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x15 << 2)));
	SOC_LOGD("	txthreshold: %8x\r\n", r->txthreshold);
	SOC_LOGD("	tckerr: %8x\r\n", r->tckerr);
	SOC_LOGD("	reserved_2_15: %8x\r\n", r->reserved_2_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_TX_FIFO_Threshold(void)
{
	scr_TX_FIFO_Threshold_t *r = (scr_TX_FIFO_Threshold_t *)(SOC_SCR_REG_BASE + (0x16 << 2));

	SOC_LOGD("TX_FIFO_Threshold: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x16 << 2)));
	SOC_LOGD("	tx_fifo_thd: %8x\r\n", r->tx_fifo_thd);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_TX_FIFO_Counter(void)
{
	scr_TX_FIFO_Counter_t *r = (scr_TX_FIFO_Counter_t *)(SOC_SCR_REG_BASE + (0x17 << 2));

	SOC_LOGD("TX_FIFO_Counter: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x17 << 2)));
	SOC_LOGD("	tx_fifo_cnt: %8x\r\n", r->tx_fifo_cnt);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_RX_FIFO_Counter(void)
{
	scr_RX_FIFO_Counter_t *r = (scr_RX_FIFO_Counter_t *)(SOC_SCR_REG_BASE + (0x18 << 2));

	SOC_LOGD("RX_FIFO_Counter: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x18 << 2)));
	SOC_LOGD("	rx_fifo_cnt: %8x\r\n", r->rx_fifo_cnt);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_Baud_Tune_Register(void)
{
	scr_Baud_Tune_Register_t *r = (scr_Baud_Tune_Register_t *)(SOC_SCR_REG_BASE + (0x19 << 2));

	SOC_LOGD("Baud_Tune_Register: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x19 << 2)));
	SOC_LOGD("	baud_tune_reg: %8x\r\n", r->baud_tune_reg);
	SOC_LOGD("	reserved_3_15: %8x\r\n", r->reserved_3_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_Two_Characters_Delay_Limit_H(void)
{
	scr_Two_Characters_Delay_Limit_H_t *r = (scr_Two_Characters_Delay_Limit_H_t *)(SOC_SCR_REG_BASE + (0x1a << 2));

	SOC_LOGD("Two_Characters_Delay_Limit_H: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x1a << 2)));
	SOC_LOGD("	two_char_delay_limit_h: %8x\r\n", r->two_char_delay_limit_h);
	SOC_LOGD("	reserved_8_15: %8x\r\n", r->reserved_8_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void scr_dump_rsv_1b_7f(void)
{
	for (uint32_t idx = 0; idx < 101; idx++) {
		SOC_LOGD("rsv_1b_7f: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + ((0x1b + idx) << 2)));
	}
}

static void scr_dump_FIFO(void)
{
	scr_FIFO_t *r = (scr_FIFO_t *)(SOC_SCR_REG_BASE + (0x80 << 2));

	SOC_LOGD("FIFO: %8x\r\n", REG_READ(SOC_SCR_REG_BASE + (0x80 << 2)));
	SOC_LOGD("	fifo: %8x\r\n", r->fifo);
	SOC_LOGD("	reserved_8_15: %8x\r\n", r->reserved_8_15);
	SOC_LOGD("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static scr_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, scr_dump_DeviceID},
	{0x1, 0x1, scr_dump_VersionID},
	{0x2, 0x2, scr_dump_Global_CTRL},
	{0x3, 0x3, scr_dump_Global_Status},
	{0x4, 0x4, scr_dump_CTRL1},
	{0x5, 0x5, scr_dump_CTRL2},
	{0x6, 0x6, scr_dump_SCPADS},
	{0x7, 0x7, scr_dump_INTEN1},
	{0x8, 0x8, scr_dump_INTSTAT1},
	{0x9, 0x9, scr_dump_FIFOCTRL},
	{0xa, 0xa, scr_dump_Legacy_RX_FIFO_Counter},
	{0xb, 0xb, scr_dump_RX_FIFO_Threshold},
	{0xc, 0xc, scr_dump_TX_Repeat},
	{0xd, 0xd, scr_dump_Smart_Card_Clock_Divisor},
	{0xe, 0xe, scr_dump_Baud_Clock_Divisor},
	{0xf, 0xf, scr_dump_Smart_Card_Guardtime},
	{0x10, 0x10, scr_dump_Activation_Deactivation_Time},
	{0x11, 0x11, scr_dump_Reset_Duration},
	{0x12, 0x12, scr_dump_ATR_Start_Limit},
	{0x13, 0x13, scr_dump_Two_Characters_Delay_Limit_L},
	{0x14, 0x14, scr_dump_INTEN2},
	{0x15, 0x15, scr_dump_INTSTAT2},
	{0x16, 0x16, scr_dump_TX_FIFO_Threshold},
	{0x17, 0x17, scr_dump_TX_FIFO_Counter},
	{0x18, 0x18, scr_dump_RX_FIFO_Counter},
	{0x19, 0x19, scr_dump_Baud_Tune_Register},
	{0x1a, 0x1a, scr_dump_Two_Characters_Delay_Limit_H},
	{0x1b, 0x80, scr_dump_rsv_1b_7f},
	{0x80, 0x80, scr_dump_FIFO},
	{-1, -1, 0}
};

void scr_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
