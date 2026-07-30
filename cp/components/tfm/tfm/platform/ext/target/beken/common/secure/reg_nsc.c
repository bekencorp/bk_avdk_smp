// Copyright 2023-2028 Beken
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

/* reg_nsc: ARMv8-M CMSE Non-Secure-Callable register access for BK7259.
 *
 * The CP core runs TF-M as the Secure world (SPE) and the cpu0_app as the
 * Non-Secure world (NSPE). These gateways let the NS side read/write registers
 * through the secure world via SG-instruction veneers (see tfm_reg_nsc.h for the
 * NS prototypes). __tz_c_veneer expands to cmse_nonsecure_entry, so GCC emits an
 * SG stub into .gnu.sgstubs (SAU-marked NSC) and exports the symbol into the
 * CMSE import library (libtfm_s_veneers.a) linked by the NS image.
 *
 * Keep the gateway bodies minimal: they run at the S<->NS boundary and the SDK
 * log path (BK_LOGI) is NOT safe to call here (it runs through the NS driver
 * stack). When CONFIG_REG_NSC_DIAG_LOG is set we only emit bytes by raw UART
 * register access (see below). */

#include "soc/soc.h"
#include "security_defs.h"

/* --- Bring-up diagnostic ------------------------------------------------
 * Confirm the NS side actually reaches this secure gateway. Do NOT use BK_LOGI
 * (unsafe at the S<->NS boundary, see tfm_hal_platform.c). Instead push bytes
 * by raw UART register access.
 *
 * Use UART1 (0x45830000), the project's dedicated *secure* debug port: PPC
 * keeps UART1 secure (bk_tfm_ppc.c "UART1 stays secure"), so the secure world
 * writes its secure alias directly with no S/NS-alias subtlety, matching the
 * existing secure diag macros (NS_RAWMARK / BMARK / MPCMARK). Output therefore
 * lands on the UART1 console (where the boot 'N?'/'B?' markers appear), NOT on
 * the UART0 NS CLI console that shows the regshow reply.
 *
 * (UART0 is PPC-assigned Non-Secure after boot; had we logged there the secure
 * world would need UART0's NS alias 0x54820000 and would race the NS driver.) */
#ifndef CONFIG_REG_NSC_DIAG_LOG
#define CONFIG_REG_NSC_DIAG_LOG 1   /* bring-up: set 0 to strip the diag path */
#endif

#if CONFIG_REG_NSC_DIAG_LOG
#define REG_NSC_DIAG_UART   0x45830000u   /* UART1 secure alias (secure debug port) */

static void reg_nsc_diag_putc(char c)
{
	volatile uint32_t *st = (volatile uint32_t *)(REG_NSC_DIAG_UART + 0x18);
	volatile uint8_t  *tx = (volatile uint8_t  *)(REG_NSC_DIAG_UART + 0x1C);

	while (*st & (1u << 16)) { /* wait until TX FIFO is not full */ }
	*tx = (uint8_t)c;
}

static void reg_nsc_diag_kv(const char *tag, uint32_t v)
{
	static const char hex[] = "0123456789abcdef";

	for (const char *p = tag; *p; p++)
		reg_nsc_diag_putc(*p);
	for (int i = 28; i >= 0; i -= 4)
		reg_nsc_diag_putc(hex[(v >> i) & 0xf]);
	reg_nsc_diag_putc('\r');
	reg_nsc_diag_putc('\n');
}
#endif /* CONFIG_REG_NSC_DIAG_LOG */

/* Link anchor: referenced from tfm_hal_get_ns_entry_point() under
 * CONFIG_REG_ACCESS_NSC so --gc-sections keeps this translation unit (and thus
 * the gateway veneers) in tfm_s. */
void psa_reg_nsc_stub(void)
{
	return;
}

__tz_c_veneer uint32_t psa_reg_read(uint32_t addr)
{
#if CONFIG_REG_NSC_DIAG_LOG
	/* Print the entry marker BEFORE the access so we still confirm we reached
	 * the secure gateway even if REG_READ(addr) itself BusFaults. */
	reg_nsc_diag_kv("[S] psa_reg_read enter addr=", addr);
#endif
	return REG_READ(addr);
}

__tz_c_veneer void psa_reg_write(uint32_t addr, uint32_t value)
{
	REG_WRITE(addr, value);
}

__tz_c_veneer void psa_reg_dump(uint32_t addr, uint32_t size)
{
	uint32_t words = (size + 3) / 4;

	for (uint32_t i = 0; i < words; i++) {
		(void)REG_READ(addr);
		addr += 4;
	}
}
