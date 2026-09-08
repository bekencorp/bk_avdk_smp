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

#include <soc/soc.h>
#include "flash_bypass.h"
#include "driver/prro.h"

#if CONFIG_SOC_BK7236XX

/* Interrupt mask save/restore primitives, implemented in sys_driver.c */
extern uint32_t port_disable_interrupts_flag(void);
extern void port_enable_interrupts_flag(int val);

#define SPI_R_0X2(_id)  (SPI_R_BASE(_id) + 2 * 0x04)

/* Bounded spin count for the SPI TX-finish polling. Interrupts are disabled and
 * the flash is in raw SPI mode here, so a stuck transfer would otherwise hang
 * the core until the watchdog resets the chip. This bound is far larger than a
 * real single transfer needs, yet well below the watchdog timeout. */
#define FLASH_BYPASS_TX_FIN_TIMEOUT  (1000000U)

/* System clock registers (base 0x44010000) used to derive CPU/flash frequency.
 * 0x8 cpu_clk_div_mode1: clkdiv_core[3:0], cksel_core[5:4]
 * 0x9 cpu_clk_div_mode2: cksel_flash[25:24], ckdiv_flash[27:26] */
#define FLASH_BYPASS_SYS_CLK_DIV_MODE1      SYS_R_ADD_X(0x8)
#define FLASH_BYPASS_SYS_CLK_DIV_MODE2      SYS_R_ADD_X(0x9)

/* DWT cycle counter, used to spin for a precise number of CPU cycles. Raw
 * addresses are used to avoid a CMSIS header dependency, matching the
 * REG_READ/REG_WRITE style used throughout this file. */
#define FLASH_BYPASS_DEMCR                  (*(volatile uint32_t *)0xE000EDFC)
#define FLASH_BYPASS_DWT_CTRL               (*(volatile uint32_t *)0xE0001000)
#define FLASH_BYPASS_DWT_CYCCNT             (*(volatile uint32_t *)0xE0001004)
#define FLASH_BYPASS_DEMCR_TRCENA           (1U << 24)
#define FLASH_BYPASS_DWT_CYCCNTENA          (1U << 0)

/* Number of flash clock cycles to delay before driving the raw SPI bypass. */
#define FLASH_BYPASS_DELAY_FLASH_CYCLES     (256U)

/* Source clock frequencies (Hz). XTAL is 26MHz on this platform. */
#define FLASH_BYPASS_FREQ_XTAL              (26000000U)
#define FLASH_BYPASS_FREQ_320M              (320000000U)
#define FLASH_BYPASS_FREQ_480M              (480000000U)

/* CPU core clock, REG_0x8: cksel_core[5:4] 0:XTAL 1:DCO 2:320M 3:480M,
 * Fcpu = src / (clkdiv_core + 1). cksel_core==1(DCO) is never used as the core
 * clock in normal operation and its calibrated frequency is not readable here,
 * so it is treated as the conservative upper bound (320M) to never under-delay. */
__attribute__((section(".iram"))) static uint32_t flash_bypass_get_core_freq(void)
{
	uint32_t reg = REG_READ(FLASH_BYPASS_SYS_CLK_DIV_MODE1);
	uint32_t clkdiv_core = reg & 0xF;
	uint32_t cksel_core = (reg >> 4) & 0x3;
	uint32_t src;

	switch (cksel_core) {
	case 0:  src = FLASH_BYPASS_FREQ_XTAL; break;
	case 2:  src = FLASH_BYPASS_FREQ_320M; break;
	case 3:  src = FLASH_BYPASS_FREQ_480M; break;
	default: src = FLASH_BYPASS_FREQ_320M; break; /* 1:DCO, conservative */
	}

	return src / (clkdiv_core + 1);
}

/* Flash clock, REG_0x9: cksel_flash[25:24] 0:XTAL 1:APLL(480M) 2/3:120M,
 * ckdiv_flash[27:26] divides by (2*ckdiv_flash + 4): 0/1/2/3 -> /4 /6 /8 /10
 * (e.g. APLL ckdiv=1 -> 480/6 = 80M, ckdiv=3 -> 480/10 = 48M). */
__attribute__((section(".iram"))) static uint32_t flash_bypass_get_flash_freq(void)
{
	uint32_t reg = REG_READ(FLASH_BYPASS_SYS_CLK_DIV_MODE2);
	uint32_t cksel_flash = (reg >> 24) & 0x3;
	uint32_t ckdiv_flash = (reg >> 26) & 0x3;
	uint32_t src;

	switch (cksel_flash) {
	case 0:  src = FLASH_BYPASS_FREQ_XTAL; break;
	case 1:  src = FLASH_BYPASS_FREQ_480M; break;
	default: src = 120000000U; break; /* 2/3: clk_120M */
	}

	return src / (2 * ckdiv_flash + 4);
}

/* Spin for the wall-clock time of flash_cycles flash clock periods, measured in
 * CPU cycles via DWT->CYCCNT: cpu_cycles = flash_cycles * Fcpu / Fflash. A guard
 * count bounds the spin so a non-counting CYCCNT cannot hang the core.
 * Reads the system clock registers, so in the SPE build it must be called only
 * after the SYS block has been promoted to secure (see flash_bypass_op_write). */
__attribute__((section(".iram"))) static void flash_bypass_delay_flash_cycles(uint32_t flash_cycles)
{
	uint32_t fcpu = flash_bypass_get_core_freq();
	uint32_t fflash = flash_bypass_get_flash_freq();
	uint32_t target = (uint32_t)(((uint64_t)flash_cycles * fcpu) / fflash);
	uint32_t start, guard, guard_max;

	if (target == 0)
		return;

	FLASH_BYPASS_DEMCR |= FLASH_BYPASS_DEMCR_TRCENA;
	FLASH_BYPASS_DWT_CTRL |= FLASH_BYPASS_DWT_CYCCNTENA;

	start = FLASH_BYPASS_DWT_CYCCNT;
	guard = 0;
	guard_max = target + 100000U;
	while (((FLASH_BYPASS_DWT_CYCCNT - start) < target) && (guard++ < guard_max))
		;
}

__attribute__((section(".iram"))) int flash_bypass_op_write(uint8_t *op_code, uint8_t *tx_buf, uint32_t tx_len)
{
	uint32_t reg;
	uint32_t reg_0x2, reg_ctrl, reg_dat;
	uint32_t reg_stat, reg_cfg;
	uint32_t int_status = 0;
	int exceptional_flag = 0;
	uint32_t reg_sys_clk_en_0xc, reg_sys_clk_sel_0xa;
#if CONFIG_TFM_BK7236_V5 && CONFIG_TFM_BUILDING_SPE
	prro_secure_type_t spi0_sec_bak, sys_sec_bak;
#endif

	int_status = port_disable_interrupts_flag();

#if CONFIG_TFM_BK7236_V5 && CONFIG_TFM_BUILDING_SPE
	/* Secure (SPE) runtime only. The raw SPI bypass drives the SPI0 master
	 * and the system-clock registers (0x44010000) directly. Both peripherals
	 * are provisioned as non-secure (see project ppc.csv: SPI0=FALSE,
	 * SYS=FALSE), so a secure-world access to them faults. Temporarily
	 * promote SPI0 and the system block to secure for the duration of this
	 * transfer, then restore the original attributes before re-enabling
	 * interrupts. The promotion stays entirely inside this interrupts-disabled
	 * critical section, so the non-secure world never observes SPI0 as secure.
	 * The PRRO register writes are done while the flash is still mapped
	 * normally (the switch to raw SPI happens in step 5 below and is reverted
	 * in step 8), so calling these flash-resident helpers here is safe.
	 *
	 * Gate on CONFIG_TFM_BUILDING_SPE (set only for the secure SPE image):
	 * this same file is also compiled into the platform_ns and platform_bl2
	 * targets, where -DBL2 is also defined, so BL2/DOMAIN_NS cannot be used to
	 * distinguish the SPE. In NS the peripherals are already accessible and
	 * writing the PRRO control registers is itself a secure-only operation,
	 * and BL2 runs fully secure before the NS provisioning is applied. */
	spi0_sec_bak = bk_prro_get_secure(PRRO_DEV_SPI0);
	sys_sec_bak  = bk_prro_get_secure(PRRO_DEV_SYS);
	bk_prro_set_secure(PRRO_DEV_SPI0, PRRO_SECURE);
	bk_prro_set_secure(PRRO_DEV_SYS, PRRO_SECURE);
#endif

	/* Delay 256 flash clock cycles before driving the raw SPI bypass. The CPU
	 * and flash frequencies are derived from the system clock registers so the
	 * delay holds regardless of the current DVFS operating point. Placed after
	 * the SPE promotion above so the SYS register reads do not fault. */
	flash_bypass_delay_flash_cycles(FLASH_BYPASS_DELAY_FLASH_CYCLES);

	/*step 1, save spi register configuration*/
	reg_0x2 = REG_READ(SPI_R_0X2(0));
	reg_ctrl = REG_READ(SPI_R_CTRL(0));
	reg_stat = REG_READ(SPI_R_INT_STATUS(0));
	reg_dat  = REG_READ(SPI_R_DATA(0));
	reg_cfg  = REG_READ(SPI_R_CFG(0));

	/*step 2, en software reset bit, bk7236/58 should enable this bit*/
	reg = REG_READ(SPI_R_0X2(0));
	reg |= (1 << 0);
	REG_WRITE(SPI_R_0X2(0), reg);

	/*step 3, config spi master*/
	/*     3.1 clear spi fifo content (bounded to avoid a hang)*/
	reg = REG_READ(SPI_R_INT_STATUS(0));
	for (uint32_t drain = 0; (reg & SPI_STATUS_RXFIFO_RD_READY) && (drain <= FLASH_BYPASS_TX_FIN_TIMEOUT); drain++) {
		REG_READ(SPI_R_DATA(0));
		reg = REG_READ(SPI_R_INT_STATUS(0));
	}
	/*     3.2 disable spi block, and backup */
	reg_sys_clk_en_0xc = reg = REG_READ(SYS_R_ADD_X(0xc));
	reg &= ~(1 << 1);
	REG_WRITE(SYS_R_ADD_X(0xc), reg);

	/*     3.3 clear spi status*/
	REG_WRITE(SPI_R_CTRL(0), 0);
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);
	REG_WRITE(SPI_R_CFG(0), 0);

	/*     3.5 set the spi master mode*/
	// open clock, and backup
	reg = REG_READ(SYS_R_ADD_X(0xc));
	reg |= (1 << 1);
	REG_WRITE(SYS_R_ADD_X(0xc), reg);

	// select 26M
	reg_sys_clk_sel_0xa = reg = REG_READ(SYS_R_ADD_X(0xa));
	reg &= ~(1 << 4);
	REG_WRITE(SYS_R_ADD_X(0xa), reg);

	// set to spi config directly
	reg = 0xC00100; // spien  msten  spi_clk=1---13M
	REG_WRITE(SPI_R_CTRL(0), reg);

	/*step 5, switch flash interface to spi
	 *        Pay attention to prefetch instruction destination, the text can not
	 *        fetch from flash space after this timepoint.
	 */
	reg = REG_READ(SYS_R_ADD_X(0x2));
	reg |= (1 << 9);
	REG_WRITE(SYS_R_ADD_X(0x2), reg);

	if(op_code != NULL)
	{
		/*step 6, write enable for volatile status register: 50H*/
		/*      6.1:take cs*/
		reg = REG_READ(SPI_R_CFG(0));
		reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
		reg |= (1 << SPI_CFG_TX_TRAHS_LEN_POSI);
		reg |= (SPI_CFG_TX_EN | SPI_CFG_TX_FIN_INT_EN);
		REG_WRITE(SPI_R_CFG(0), reg);

		/*      6.2:write tx fifo (wait for TXFIFO ready, bounded)
		 * The SPI master was just re-configured above, so on the first
		 * invocation after boot the TX FIFO may not be write-ready on the
		 * very first read. Poll instead of failing immediately, otherwise
		 * the volatile SR write spuriously returns -1. */
		for (uint32_t to = 0; ; to++) {
			reg = REG_READ(SPI_R_INT_STATUS(0));
			if (reg & SPI_STATUS_TXFIFO_WR_READY)
				break;
			if (to > FLASH_BYPASS_TX_FIN_TIMEOUT) {
				exceptional_flag = -1;
				goto wr_exceptional;
			}
		}
		REG_WRITE(SPI_R_DATA(0), *op_code);

		/*      6.3:waiting for TXFIFO_EMPTY interrupt (bounded to avoid a hang)*/
		for (uint32_t to = 0; ; to++) {
			reg = REG_READ(SPI_R_INT_STATUS(0));
			if (reg & SPI_STATUS_TX_FINISH_INT)
				break;
			if (to > FLASH_BYPASS_TX_FIN_TIMEOUT) {
				exceptional_flag = -3;
				goto wr_exceptional;
			}
		}

		/*      6.4:release cs*/
		reg = REG_READ(SPI_R_CFG(0));
		reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
		reg &= ~(SPI_CFG_TX_EN | SPI_CFG_TX_FIN_INT_EN);
		REG_WRITE(SPI_R_CFG(0), reg);

		// cler stat and fifo (bounded to avoid a hang)
		reg = REG_READ(SPI_R_INT_STATUS(0));
		for (uint32_t drain = 0; (reg & SPI_STATUS_RXFIFO_RD_READY) && (drain <= FLASH_BYPASS_TX_FIN_TIMEOUT); drain++) {
			REG_READ(SPI_R_DATA(0));
			reg = REG_READ(SPI_R_INT_STATUS(0));
		}
		reg = REG_READ(SPI_R_INT_STATUS(0));
		REG_WRITE(SPI_R_INT_STATUS(0), reg);
	}

	if((tx_len == 0) || (tx_buf == NULL))
	{
		exceptional_flag = 0;
		goto wr_exceptional;
	}

	/*step 7, for tx_buf */
	/*      7.1:take cs*/
	reg = REG_READ(SPI_R_CFG(0));
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_RX_TRAHS_LEN_POSI);
	reg |= ((tx_len & SPI_CFG_TRX_LEN_MASK) << SPI_CFG_TX_TRAHS_LEN_POSI);
	reg |= (SPI_CFG_TX_EN | SPI_CFG_TX_FIN_INT_EN);
	REG_WRITE(SPI_R_CFG(0), reg);

	/*      7.2:write tx fifo*/
	// write tx first
	for (int i = 0, wait = 0; i < tx_len; ) {
		reg = REG_READ(SPI_R_INT_STATUS(0));
		if ((reg & SPI_STATUS_TXFIFO_WR_READY) == 0) {
			for(volatile int j=0; j<500; j++);
			wait++;
			if(wait > 100) {
				exceptional_flag = -2;
				goto wr_exceptional;
			}
		} else {
			wait = 0;
			REG_WRITE(SPI_R_DATA(0), tx_buf[i]);
			i++;
		}
	}

	/*      7.3:waiting for TXFIFO_EMPTY interrupt (bounded to avoid a hang)*/
	for (uint32_t to = 0; ; to++) {
		reg = REG_READ(SPI_R_INT_STATUS(0));
		if (reg & SPI_STATUS_TX_FINISH_INT)
			break;
		if (to > FLASH_BYPASS_TX_FIN_TIMEOUT) {
			exceptional_flag = -4;
			goto wr_exceptional;
		}
	}

	/*      7.4:release cs*/
	reg = REG_READ(SPI_R_CFG(0));
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_RX_TRAHS_LEN_POSI);
	reg &= (SPI_CFG_RX_EN | SPI_CFG_TX_EN | SPI_CFG_TX_FIN_INT_EN);
	REG_WRITE(SPI_R_CFG(0), reg);

	// cler stat and fifo (bounded to avoid a hang)
	reg = REG_READ(SPI_R_INT_STATUS(0));
	for (uint32_t drain = 0; (reg & SPI_STATUS_RXFIFO_RD_READY) && (drain <= FLASH_BYPASS_TX_FIN_TIMEOUT); drain++) {
		REG_READ(SPI_R_DATA(0));
		reg = REG_READ(SPI_R_INT_STATUS(0));
	}
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);
	exceptional_flag = 0;

wr_exceptional:
	/*step 8, switch flash interface to flash controller */
	reg = REG_READ(SYS_R_ADD_X(0x2));
	reg &= ~(1 << 9);
	REG_WRITE(SYS_R_ADD_X(0x2), reg);

	// recover icu and powerdown bits for spi
	REG_WRITE(SYS_R_ADD_X(0xc), reg_sys_clk_en_0xc);
	REG_WRITE(SYS_R_ADD_X(0xa), reg_sys_clk_sel_0xa);

	/*step 10, restore spi register configuration*/
	REG_WRITE(SPI_R_CTRL(0), reg_ctrl);
	REG_WRITE(SPI_R_INT_STATUS(0), reg_stat);
	REG_WRITE(SPI_R_DATA(0), reg_dat);
	REG_WRITE(SPI_R_CFG(0), reg_cfg);
	REG_WRITE(SPI_R_0X2(0), reg_0x2);

#if CONFIG_TFM_BK7236_V5 && CONFIG_TFM_BUILDING_SPE
	/* restore the original secure attributes of SPI0 / system block.
	 * Flash is already switched back to the controller (step 8), so these
	 * flash-resident PRRO helpers are safe to call here. */
	bk_prro_set_secure(PRRO_DEV_SYS, sys_sec_bak);
	bk_prro_set_secure(PRRO_DEV_SPI0, spi0_sec_bak);
#endif

	/*step 11, enable interrupt*/
	port_enable_interrupts_flag(int_status);

	return exceptional_flag;
}

#endif /*CONFIG_SOC_BK7236XX*/
