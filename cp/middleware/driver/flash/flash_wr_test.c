#include <stdlib.h>
#include "cli.h"
#include "driver/flash.h"
#include <driver/flash_partition.h>
#include "sys_driver.h"
#include "flash_driver.h"
#include <driver/psram.h>
#include "bk_misc.h"
#include "driver/wdt.h"
#include "bk_wdt.h"
#include <driver/aon_rtc.h>
#include <common/bk_assert.h>

#define  TICK_PER_US    26
#define  PRINT_CNT     100

/*
 * Reference single-operation timing (us) for the on-board SPI NOR
 * (GD25WQ64E-class, JEDEC ID 0xC86517). These are coarse references used only
 * to flag grossly abnormal latency: a measurement is reported as a WARNING
 * only when it exceeds the *MAX* bound. Exceeding the typical value is normal
 * for NOR flash and must not be treated as an error. "block32" is measured as
 * 8 sequential 4KB sector erases (see flash_erase_one_unit), so its bounds are
 * ~8x the sector bounds.
 */
#define  T_PAGE_TYP     1200
#define  T_PAGE_MAX     3000
#define  T_SECTOR_TYP   60000
#define  T_SECTOR_MAX   400000
#define  T_BLK32_TYP    480000
#define  T_BLK32_MAX    3200000
#define  T_BLK64_TYP    200000
#define  T_BLK64_MAX    2000000
#define  T_CHIP_TYP     30000000
#define  T_CHIP_MAX     200000000

#define  FLASH_SECTOR_SIZE   0x1000
#define  FLASH_BLK32_SIZE    0x8000
#define  FLASH_BLK64_SIZE    0x10000
#define  FLASH_PAGE_SIZE     256

/* Defined in flash_driver.c, not exported in a public header. */
extern uint32_t bk_flash_get_crc_err_num(void);

beken_thread_t idle_read_flash_handle = NULL;
beken_thread_t idle_read_psram_handle = NULL;

typedef enum {
	FLASH_PAT_INC = 0,
	FLASH_PAT_AA,
	FLASH_PAT_55,
	FLASH_PAT_5AA5,
	FLASH_PAT_A55A,
} flash_test_pattern_t;

typedef enum {
	FLASH_ERASE_SECTOR = 0,   /* 4KB  */
	FLASH_ERASE_BLK32,        /* 32KB */
	FLASH_ERASE_BLK64,        /* 64KB */
} flash_erase_unit_t;

static const char *pattern_name(flash_test_pattern_t p)
{
	switch (p) {
	case FLASH_PAT_AA:   return "0xAA";
	case FLASH_PAT_55:   return "0x55";
	case FLASH_PAT_5AA5: return "5AA5";
	case FLASH_PAT_A55A: return "A55A";
	case FLASH_PAT_INC:
	default:             return "INC";
	}
}

static flash_test_pattern_t parse_pattern(const char *s)
{
	if (s == NULL)
		return FLASH_PAT_INC;
	if (os_strcmp(s, "0xAA") == 0 || os_strcmp(s, "AA") == 0)
		return FLASH_PAT_AA;
	if (os_strcmp(s, "0x55") == 0 || os_strcmp(s, "55") == 0)
		return FLASH_PAT_55;
	if (os_strcmp(s, "5AA5") == 0)
		return FLASH_PAT_5AA5;
	if (os_strcmp(s, "A55A") == 0)
		return FLASH_PAT_A55A;
	return FLASH_PAT_INC;
}

/* Value written/expected at absolute byte offset idx for the given pattern. */
static uint8_t pat_val(flash_test_pattern_t p, uint32_t idx)
{
	switch (p) {
	case FLASH_PAT_AA:   return 0xAA;
	case FLASH_PAT_55:   return 0x55;
	case FLASH_PAT_5AA5: return (idx & 1) ? 0xA5 : 0x5A;
	case FLASH_PAT_A55A: return (idx & 1) ? 0x5A : 0xA5;
	case FLASH_PAT_INC:
	default:             return (uint8_t)(idx & 0xFF);
	}
}

static void fill_buf(uint8_t *buf, uint32_t off, uint32_t len, flash_test_pattern_t p)
{
	for (uint32_t i = 0; i < len; i++)
		buf[i] = pat_val(p, off + i);
}

/* Range check against the actual flash size (0 means "unknown", allow). */
static bool flash_addr_valid(uint32_t addr, uint32_t len)
{
	uint32_t total = bk_flash_get_current_total_size();

	if (total == 0)
		return true;
	if (addr >= total)
		return false;
	if ((addr + len) < addr)   /* wrap */
		return false;
	if ((addr + len) > total)
		return false;
	return true;
}

static flash_erase_unit_t parse_erase_unit(const char *s)
{
	if (s == NULL)
		return FLASH_ERASE_SECTOR;
	if (os_strncmp(s, "erase=", 6) == 0)
		s += 6;
	if (os_strcmp(s, "block32") == 0 || os_strcmp(s, "32k") == 0 || os_strcmp(s, "32K") == 0)
		return FLASH_ERASE_BLK32;
	if (os_strcmp(s, "block") == 0 || os_strcmp(s, "64k") == 0 || os_strcmp(s, "64K") == 0)
		return FLASH_ERASE_BLK64;
	return FLASH_ERASE_SECTOR;
}

static const char *erase_unit_name(flash_erase_unit_t u)
{
	switch (u) {
	case FLASH_ERASE_BLK32: return "block32(32KB)";
	case FLASH_ERASE_BLK64: return "block(64KB)";
	case FLASH_ERASE_SECTOR:
	default:                return "sector(4KB)";
	}
}

static uint32_t erase_unit_size(flash_erase_unit_t u)
{
	switch (u) {
	case FLASH_ERASE_BLK32: return FLASH_BLK32_SIZE;
	case FLASH_ERASE_BLK64: return FLASH_BLK64_SIZE;
	case FLASH_ERASE_SECTOR:
	default:                return FLASH_SECTOR_SIZE;
	}
}

/* Erase one unit at (aligned) addr, using the requested granularity. */
static void flash_erase_one_unit(uint32_t addr, flash_erase_unit_t u)
{
	switch (u) {
	case FLASH_ERASE_BLK32:
		for (uint32_t i = 0; i < FLASH_BLK32_SIZE; i += FLASH_SECTOR_SIZE)
			bk_flash_erase_sector(addr + i);
		break;
	case FLASH_ERASE_BLK64:
		bk_flash_erase_block(addr);
		break;
	case FLASH_ERASE_SECTOR:
	default:
		bk_flash_erase_sector(addr);
		break;
	}
}

/*
 * Full write flow used by the W/M commands:
 *   - (optional) unprotect flash
 *   - erase all covered sectors (4KB aligned)
 *   - program the requested pattern (256B page granularity)
 *   - read back and verify
 *   - (optional) restore full protection
 */
static bk_err_t test_flash_write_verify(uint32_t start_addr, uint32_t len,
					flash_test_pattern_t pat, bool manage_protect)
{
	u8 buf[256];
	uint32_t addr;
	uint32_t err_cnt = 0;
	uint32_t erase_start = start_addr & ~(FLASH_SECTOR_SIZE - 1);
	uint32_t erase_end = (start_addr + len + FLASH_SECTOR_SIZE - 1) & ~(FLASH_SECTOR_SIZE - 1);

	if (manage_protect)
		test_flash_set_protect_type_none();

	for (addr = erase_start; addr < erase_end; addr += FLASH_SECTOR_SIZE)
		bk_flash_erase_sector(addr);

	for (addr = start_addr; addr < start_addr + len; addr += 256) {
		uint32_t chunk = (start_addr + len - addr) < 256 ? (start_addr + len - addr) : 256;
		fill_buf(buf, addr - start_addr, chunk, pat);
		bk_flash_write_bytes(addr, (uint8_t *)buf, chunk);
	}

	for (addr = start_addr; addr < start_addr + len; addr += 256) {
		uint32_t chunk = (start_addr + len - addr) < 256 ? (start_addr + len - addr) : 256;
		os_memset(buf, 0, 256);
		bk_flash_read_bytes(addr, (uint8_t *)buf, chunk);
		for (uint32_t j = 0; j < chunk; j++) {
			uint8_t expected = pat_val(pat, (addr - start_addr) + j);
			if (buf[j] != expected) {
				if (err_cnt < 16)
					BK_DUMP_OUT("[w ERR] 0x%x exp=0x%02x got=0x%02x\r\n", addr + j, expected, buf[j]);
				err_cnt++;
			}
		}
	}

	if (manage_protect)
		test_flash_set_protect_type_all();

	if (err_cnt) {
		BK_DUMP_OUT("W verify FAIL %u (0xFF? try flash_test U)\r\n", err_cnt);
		return kGeneralErr;
	}
	BK_DUMP_OUT("W verify PASS pat=%s 0x%x/0x%x\r\n", pattern_name(pat), start_addr, len);
	return kNoErr;
}

/* Erase [start,start+len) by 4KB sectors, verify 0xFF; optional protect mgmt. */
static bk_err_t test_flash_erase_verify(uint32_t start_addr, uint32_t len, bool manage_protect)
{
	u8 buf[256];
	uint32_t addr;
	uint32_t err_cnt = 0;
	uint32_t erase_start = start_addr & ~(FLASH_SECTOR_SIZE - 1);
	uint32_t erase_end = (start_addr + len + FLASH_SECTOR_SIZE - 1) & ~(FLASH_SECTOR_SIZE - 1);

	if (manage_protect)
		test_flash_set_protect_type_none();

	for (addr = erase_start; addr < erase_end; addr += FLASH_SECTOR_SIZE)
		bk_flash_erase_sector(addr);

	for (addr = erase_start; addr < erase_end; addr += 256) {
		os_memset(buf, 0, 256);
		bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
		for (uint32_t j = 0; j < 256; j++) {
			if (buf[j] != 0xFF) {
				if (err_cnt < 16)
					BK_DUMP_OUT("[e ERR] 0x%x=0x%02x\r\n", addr + j, buf[j]);
				err_cnt++;
			}
		}
	}

	if (manage_protect)
		test_flash_set_protect_type_all();

	if (err_cnt) {
		BK_DUMP_OUT("E verify FAIL %u\r\n", err_cnt);
		return kGeneralErr;
	}
	BK_DUMP_OUT("E verify PASS 0x%x/0x%x\r\n", erase_start, erase_end - erase_start);
	return kNoErr;
}

static bk_err_t test_flash_read(volatile uint32_t start_addr, uint32_t len)
{
	uint32_t i, j, tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	tmp = addr + length;

	for (; addr < tmp; addr += 256) {
		os_memset(buf, 0, 256);
		bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
		int int_level = rtos_enter_critical();
		BK_DUMP_OUT("read addr:%x\r\n", addr);
		rtos_exit_critical(int_level);
		for (i = 0; i < 16; i++) {
			for (j = 0; j < 16; j++)
				BK_DUMP_OUT("%02x ", buf[i * 16 + j]);
			BK_DUMP_OUT("\r\n");
		}
	}

	return kNoErr;
}

static bk_err_t test_flash_read_without_print(volatile uint32_t start_addr, uint32_t len)
{
	uint32_t tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	tmp = addr + length;

	for (; addr < tmp; addr += 256) {
		os_memset(buf, 0, 256);
		bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
	}

	return kNoErr;
}

static bk_err_t test_flash_read_time(volatile uint32_t start_addr, uint32_t len)
{
	UINT32 time_start, time_end;
	uint32_t tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;

	tmp = addr + length;
	beken_time_get_time((beken_time_t *)&time_start);

	for (; addr < tmp; addr += 256) {
		os_memset(buf, 0, 256);
		bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
	}
	beken_time_get_time((beken_time_t *)&time_end);
	BK_DUMP_OUT("read cost: %d\r\n", time_end - time_start);

	return kNoErr;
}

/* Compact status register / protection view (RSR). */
static void flash_dump_status_analysis(void)
{
	uint16_t sr = bk_flash_read_status_reg();
	uint8_t busy = sr & 0x1;
	uint8_t wel  = (sr >> 1) & 0x1;
	uint8_t srp0 = (sr >> 7) & 0x1;
	uint8_t srp1 = (sr >> 8) & 0x1;
	uint8_t cmp  = (sr >> 14) & 0x1;
	uint8_t bp   = (sr >> 2) & 0x7;
	bool fully_unprot = (cmp == 0) ? (bp == 0x0) : (bp == 0x7);
	bool fully_prot   = (cmp == 0) ? (bp == 0x7) : (bp == 0x0);

	BK_DUMP_OUT("RSR=0x%04x BP[2:0]=0x%x CMP=%d BUSY=%d WEL=%d SRP=%d%d\r\n",
		sr, bp, cmp, busy, wel, srp0, srp1);
	BK_DUMP_OUT("Protect: %s\r\n",
		fully_unprot ? "NONE (writable)" : fully_prot ? "FULL" : "PARTIAL");
}

/* Read the JEDEC ID and print manufacturer / device id. */
static void flash_dump_id(void)
{
	uint32_t id = bk_flash_get_id();

	BK_DUMP_OUT("Flash ID=0x%06x (MID=0x%02x DID=0x%04x)\r\n",
		id & 0xFFFFFF, (id >> 16) & 0xFF, id & 0xFFFF);
}

static void flash_protect_apply(bool unprotect)
{
	uint16_t before = bk_flash_read_status_reg();
	uint16_t after;

	if (unprotect)
		test_flash_set_protect_type_none();
	else
		test_flash_set_protect_type_all();

	after = bk_flash_read_status_reg();
	BK_DUMP_OUT("Flash %s: RSR 0x%04x -> 0x%04x\r\n",
		unprotect ? "UNPROTECT" : "PROTECT", before, after);
	flash_dump_status_analysis();
}

/* Feed watchdogs and yield long enough to keep the RTOS tick advancing.
 * See the long comment in test_flash_count_time for why this yield exists. */
static void flash_test_heartbeat_yield(uint64_t iter_start)
{
	#if (CONFIG_WDT_EN)
		bk_wdt_feed();
	#endif
	#if (CONFIG_TASK_WDT)
		bk_task_wdt_feed();
	#endif
	uint32_t busy_ms = (uint32_t)((bk_aon_rtc_get_us() - iter_start) / 1000);
	uint32_t yield_ms = (busy_ms < 10) ? 10 : busy_ms;
	rtos_delay_milliseconds(yield_ms);
}

/* timing: single-operation latency measurement per doc (page/sector/block32/block/chip). */
static void flash_timing_test(const char *type, uint32_t addr, flash_test_pattern_t pat)
{
	u8 buf[256];
	uint64_t t0, t1;
	uint32_t err_cnt = 0;

	test_flash_set_protect_type_none();

	BK_DUMP_OUT("timing %s addr=0x%08x\r\n", type, addr);

	if (os_strcmp(type, "page") == 0) {
		uint64_t tmin = ~0ULL, tmax = 0, tsum = 0;
		BK_DUMP_OUT("page x10 pat=%s (typ %d max %d us)\r\n", pattern_name(pat), T_PAGE_TYP, T_PAGE_MAX);
		for (int i = 0; i < 10; i++) {
			uint64_t iter = bk_aon_rtc_get_us();
			bk_flash_erase_sector(addr & ~(FLASH_SECTOR_SIZE - 1));
			fill_buf(buf, 0, FLASH_PAGE_SIZE, pat);
			t0 = bk_aon_rtc_get_us();
			bk_flash_write_bytes(addr, (uint8_t *)buf, FLASH_PAGE_SIZE);
			t1 = bk_aon_rtc_get_us();
			uint64_t dt = t1 - t0;
			if (dt < tmin) tmin = dt;
			if (dt > tmax) tmax = dt;
			tsum += dt;
			BK_DUMP_OUT("[%d] %u us%s\r\n", i + 1, (uint32_t)dt, (dt > T_PAGE_MAX) ? " WARN>max" : "");
			os_memset(buf, 0, FLASH_PAGE_SIZE);
			bk_flash_read_bytes(addr, (uint8_t *)buf, FLASH_PAGE_SIZE);
			for (uint32_t j = 0; j < FLASH_PAGE_SIZE; j++)
				if (buf[j] != pat_val(pat, j)) err_cnt++;
			flash_test_heartbeat_yield(iter);
		}
		BK_DUMP_OUT("page min/avg/max=%u/%u/%u us\r\n",
			(uint32_t)tmin, (uint32_t)(tsum / 10), (uint32_t)tmax);
	} else if (os_strcmp(type, "sector") == 0 || os_strcmp(type, "block32") == 0 || os_strcmp(type, "block") == 0) {
		flash_erase_unit_t u = (os_strcmp(type, "block32") == 0) ? FLASH_ERASE_BLK32 :
				       (os_strcmp(type, "block") == 0) ? FLASH_ERASE_BLK64 : FLASH_ERASE_SECTOR;
		uint32_t usize = erase_unit_size(u);
		uint32_t base = addr & ~(usize - 1);
		uint32_t typ = (u == FLASH_ERASE_BLK32) ? T_BLK32_TYP : (u == FLASH_ERASE_BLK64) ? T_BLK64_TYP : T_SECTOR_TYP;
		uint32_t max = (u == FLASH_ERASE_BLK32) ? T_BLK32_MAX : (u == FLASH_ERASE_BLK64) ? T_BLK64_MAX : T_SECTOR_MAX;
		uint64_t tmin = ~0ULL, tmax = 0, tsum = 0;
		BK_DUMP_OUT("erase %s x10 at 0x%08x (typ %u max %u us)\r\n", erase_unit_name(u), base, typ, max);
		for (int i = 0; i < 10; i++) {
			uint64_t iter = bk_aon_rtc_get_us();
			t0 = bk_aon_rtc_get_us();
			flash_erase_one_unit(base, u);
			t1 = bk_aon_rtc_get_us();
			uint64_t dt = t1 - t0;
			if (dt < tmin) tmin = dt;
			if (dt > tmax) tmax = dt;
			tsum += dt;
			BK_DUMP_OUT("[%d] %u us%s\r\n", i + 1, (uint32_t)dt, (dt > max) ? " WARN>max" : "");
			for (uint32_t off = 0; off < usize; off += 256) {
				os_memset(buf, 0, 256);
				bk_flash_read_bytes(base + off, (uint8_t *)buf, 256);
				for (uint32_t j = 0; j < 256; j++)
					if (buf[j] != 0xFF) err_cnt++;
			}
			flash_test_heartbeat_yield(iter);
		}
		BK_DUMP_OUT("erase min/avg/max=%u/%u/%u us\r\n",
			(uint32_t)tmin, (uint32_t)(tsum / 10), (uint32_t)tmax);
	} else if (os_strcmp(type, "chip") == 0) {
		uint32_t total = bk_flash_get_current_total_size();
		uint32_t chip_start = 0x400000;
		uint32_t chip_end = total ? total : 0x800000;
		BK_DUMP_OUT("chip erase 0x%08x-0x%08x (typ %d max %d us)\r\n", chip_start, chip_end, T_CHIP_TYP, T_CHIP_MAX);
		t0 = bk_aon_rtc_get_us();
		for (uint32_t a = chip_start; a < chip_end; a += FLASH_SECTOR_SIZE) {
			bk_flash_erase_sector(a);
			if ((a & 0xFFFFF) == 0) {
				BK_DUMP_OUT("  %08x/%08x\r\n", a, chip_end);
				#if (CONFIG_WDT_EN)
					bk_wdt_feed();
				#endif
				#if (CONFIG_TASK_WDT)
					bk_task_wdt_feed();
				#endif
			}
		}
		t1 = bk_aon_rtc_get_us();
		BK_DUMP_OUT("chip erase: %u us\r\n", (uint32_t)(t1 - t0));
		for (uint32_t a = chip_start; a < chip_end; a += 256) {
			os_memset(buf, 0, 256);
			bk_flash_read_bytes(a, (uint8_t *)buf, 256);
			for (uint32_t j = 0; j < 256; j++)
				if (buf[j] != 0xFF) err_cnt++;
		}
	} else {
		BK_DUMP_OUT("timing type: page/sector/block32/block/chip\r\n");
		return;
	}

	BK_DUMP_OUT("verify: %s %u\r\n", err_cnt ? "FAIL" : "PASS", err_cnt);
}

static bk_err_t test_flash_count_time(volatile uint32_t start_addr, uint32_t len, uint32_t test_times,
				      flash_test_pattern_t pat, flash_erase_unit_t eunit, uint32_t print_cnt)
{
	uint32_t tmp;
	u8 buf[256];
	uint32_t addr = start_addr;
	uint32_t length = len;
	int32_t tick_cnt = 0;
	uint64_t time_start = 0;
	uint64_t time_end = 0;
	uint32_t total_erase_err = 0;
	uint32_t total_write_err = 0;
	uint32_t usize = erase_unit_size(eunit);
	uint32_t erase_start = start_addr & ~(usize - 1);
	uint32_t erase_end = (start_addr + len + usize - 1) & ~(usize - 1);

	tmp = addr + length;

	BK_DUMP_OUT("C test: addr=0x%08x len=0x%08x times=%d pat=%s unit=%s erase=0x%08x-0x%08x\r\n",
		start_addr, len, test_times, pattern_name(pat), erase_unit_name(eunit), erase_start, erase_end);

	test_flash_set_protect_type_none();

	/* Single-operation latency measured once up-front. */
	{
		fill_buf(buf, 0, FLASH_PAGE_SIZE, pat);
		bk_flash_erase_sector(erase_start);
		time_start = bk_aon_rtc_get_us();
		bk_flash_write_bytes(erase_start, (uint8_t *)buf, FLASH_PAGE_SIZE);
		time_end = bk_aon_rtc_get_us();
		BK_DUMP_OUT("single page prog: %u us (typ %d max %d)%s\r\n",
			(uint32_t)(time_end - time_start), T_PAGE_TYP, T_PAGE_MAX,
			((time_end - time_start) > T_PAGE_MAX) ? " WARN>max" : "");

		time_start = bk_aon_rtc_get_us();
		flash_erase_one_unit(erase_start, eunit);
		time_end = bk_aon_rtc_get_us();
		BK_DUMP_OUT("single %s erase: %u us\r\n", erase_unit_name(eunit), (uint32_t)(time_end - time_start));
	}

	for (int i = 0; i <= (int)test_times; i++) {
		uint64_t iter_start = bk_aon_rtc_get_us();

		time_start = bk_aon_rtc_get_us();
		for (addr = start_addr; addr < tmp; addr += 256) {
			os_memset(buf, 0, 256);
			bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
		}
		time_end = bk_aon_rtc_get_us();
		tick_cnt = (int32_t)(time_end - time_start);
		if (i % print_cnt == 0)
			BK_DUMP_OUT("[r %d] %d us\r\n", i, tick_cnt);

		time_start = bk_aon_rtc_get_us();
		for (addr = erase_start; addr < erase_end; addr += usize)
			flash_erase_one_unit(addr, eunit);
		time_end = bk_aon_rtc_get_us();
		tick_cnt = (int32_t)(time_end - time_start);
		if (i % print_cnt == 0)
			BK_DUMP_OUT("[e %d] %d us\r\n", i, tick_cnt);

		for (addr = erase_start; addr < erase_end; addr += 256) {
			os_memset(buf, 0, 256);
			bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
			for (int j = 0; j < 256; j++) {
				if (buf[j] != 0xff) {
					total_erase_err++;
					if (i % print_cnt == 0 && total_erase_err < 16)
						BK_DUMP_OUT("[e ERR] 0x%x=0x%02x\r\n", addr + j, buf[j]);
				}
			}
		}

		time_start = bk_aon_rtc_get_us();
		for (addr = start_addr; addr < tmp; addr += 256) {
			fill_buf(buf, addr - start_addr, 256, pat);
			bk_flash_write_bytes(addr, (uint8_t *)buf, 256);
		}
		time_end = bk_aon_rtc_get_us();
		tick_cnt = (int32_t)(time_end - time_start);
		if (i % print_cnt == 0)
			BK_DUMP_OUT("[w %d] %d us\r\n", i, tick_cnt);

		for (addr = start_addr; addr < tmp; addr += 256) {
			os_memset(buf, 0, 256);
			bk_flash_read_bytes(addr, (uint8_t *)buf, 256);
			for (int j = 0; j < 256; j++) {
				uint8_t expected = pat_val(pat, (addr - start_addr) + j);
				if (buf[j] != expected) {
					total_write_err++;
					if (i % print_cnt == 0 && total_write_err < 16)
						BK_DUMP_OUT("[w ERR] 0x%x exp=0x%02x got=0x%02x\r\n", addr + j, expected, buf[j]);
				}
			}
		}

		flash_test_heartbeat_yield(iter_start);
	}

	BK_DUMP_OUT("C summary: cycles=%d unit=%s erase=%s(%u) write=%s(%u)\r\n",
		test_times + 1, erase_unit_name(eunit),
		total_erase_err ? "FAIL" : "PASS", total_erase_err,
		total_write_err ? "FAIL" : "PASS", total_write_err);

	return (total_erase_err || total_write_err) ? kGeneralErr : kNoErr;
}

/* Read/write/erase fixed 4KB on the USR_CONFIG partition (flash_test T <W/R/E>). */
static void test_flash_usr_config(char op)
{
	bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
	uint32_t addr;

	if (pt == NULL) {
		BK_DUMP_OUT("USR_CONFIG not found\r\n");
		return;
	}
	addr = pt->partition_start_addr;
	BK_DUMP_OUT("USR_CONFIG 0x%08x/0x%08x op=%c\r\n", addr, pt->partition_length, op);

	switch (op) {
	case 'W':
		test_flash_write_verify(addr, 0x1000, FLASH_PAT_INC, true);
		break;
	case 'R':
		test_flash_read(addr, 0x1000);
		break;
	case 'E':
		test_flash_erase_verify(addr, 0x1000, true);
		break;
	default:
		BK_DUMP_OUT("flash_test T <W/R/E>\r\n");
		break;
	}
}

static void test_idle_read_flash(void *arg) {
	while (1) {
		test_flash_read_without_print(0x1000, 1000);
		test_flash_read_without_print(0x100000, 1000);
		test_flash_read_without_print(0x200000, 1000);
		test_flash_read_without_print(0x300000, 0x1000);
	}
	rtos_delete_thread(&idle_read_flash_handle);
}

#define write_data(addr,val)                    *((volatile unsigned long *)(addr)) = val
#define read_data(addr,val)                     val = *((volatile unsigned long *)(addr))
#define get_addr_data(addr)                     *((volatile unsigned long *)(addr))

#if (CONFIG_PSRAM_AS_SYS_MEMORY)
bool s_is_cacheable = false;
bool s_task_stop = false;
#define PSRAM_DATA_ADDR     0x60000000
static void test_idle_read_psram(void *arg) {
	uint32_t i, val = 0;
	uint32_t buf_len = 512;

	uint32_t *test_data = (uint32_t *)psram_malloc(buf_len);
	if (test_data == NULL) {
		CLI_LOGE("psram malloc fail\r\n");
		return;
	}

	while (1) {
		if(s_is_cacheable == false) {
			for(i=0;i<1024;i++){
				write_data((PSRAM_DATA_ADDR+i*0x4),0x11+i);
				write_data((PSRAM_DATA_ADDR + 0x1000 +i*0x4),0x22+i);
				write_data((PSRAM_DATA_ADDR + 0x2000 +i*0x4),0x33+i);
				write_data((PSRAM_DATA_ADDR + 0x3000 +i*0x4),0x44+i);
			}
			for(i=0;i<1024;i++){
				write_data((PSRAM_DATA_ADDR + 0x4000 +i*0x4),0x55+i);
				write_data((PSRAM_DATA_ADDR + 0x5000 +i*0x4),0x66+i);
				write_data((PSRAM_DATA_ADDR + 0x6000 +i*0x4),0x77+i);
				write_data((PSRAM_DATA_ADDR + 0x7000 +i*0x4),0x88+i);
			}
			for(i=0;i<4*1024;i++){
				get_addr_data(PSRAM_DATA_ADDR+i*0x4);
			}
			for(i=0;i<4*1024;i++){
				get_addr_data(PSRAM_DATA_ADDR + 0x4000 +i*0x4);
			}
			for(i=0;i<4*1024;i++){
				get_addr_data(PSRAM_DATA_ADDR + i*0x4);
			}
			for(i=0;i<4*1024;i++){
				get_addr_data(PSRAM_DATA_ADDR + 0x4000 +i*0x4);
			}

			if(s_task_stop == true) {
				BK_DUMP_OUT("exit psram non-cacheable\r\n");
				break;
			}
		} else {
			for (i = 0; i < buf_len / 4; i++) {
				test_data[i] = 0x11223344 + i;
			}
			for (i = 0; i < buf_len / 4; i++) {
				val = test_data[i];
			}
			if(s_task_stop == true) {
				BK_DUMP_OUT("exit psram cacheable val=0x%x\r\n", val);
				break;
			}
		}
	}

	if (test_data) {
		os_free(test_data);
	}
	test_data = NULL;

	if (idle_read_psram_handle) {
		BK_DUMP_OUT("idle_read_psram stopped\n");
		rtos_delete_thread(&idle_read_psram_handle);
		idle_read_psram_handle = NULL;
	}
}
#endif
static void flash_command_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	char cmd = 0;
	uint32_t len = 0;
	uint32_t addr = 0;

	if (argc < 2) {
		BK_DUMP_OUT("flash_test R/W/E/M/N/T <addr> <len> [pat] | C <addr> <len> <times> [pat] [erase] | timing <page/sector/block32/block/chip> <addr> | ID/RSR/U/P/WSR/CRC_ERR\r\n");
		msg = CLI_CMD_RSP_ERROR;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

#if (CONFIG_SYSTEM_CTRL)
	if (os_strcmp(argv[1], "config") == 0) {
		uint32_t flash_src_clk = os_strtoul(argv[2], NULL, 10);
		uint32_t flash_div_clk = os_strtoul(argv[3], NULL, 10);
		uint32_t flash_line_mode = os_strtoul(argv[4], NULL, 10);

		if (FLASH_CLK_XTAL == flash_src_clk) {
			sys_drv_flash_cksel(flash_src_clk);
		}

		if((FLASH_CLK_XTAL != sys_drv_flash_get_clk_sel()) && (0 == flash_div_clk)) {
			BK_DUMP_OUT("cfg fail: set src=26M or div>0 first\n");
			return;
		}

		sys_drv_flash_set_clk_div(flash_div_clk);
		sys_drv_flash_cksel(flash_src_clk);
		if (flash_line_mode == 2)
			bk_flash_power_saving_enter();
		else
			bk_flash_power_saving_exit();

		BK_DUMP_OUT("flash cfg: src=%u div=%u line=%u\n", flash_src_clk, flash_div_clk, flash_line_mode);

		return;
	}

#endif
	if (os_strcmp(argv[1], "idle_read_start") == 0) {
		uint32_t task_prio = os_strtoul(argv[2], NULL, 10);
		BK_DUMP_OUT("idle_read_flash start prio=%u\n", task_prio);
		rtos_create_thread(&idle_read_flash_handle, task_prio,
			"idle_read_flash",
			(beken_thread_function_t) test_idle_read_flash,
			CONFIG_APP_MAIN_TASK_STACK_SIZE,
			(beken_thread_arg_t)0);

		return;
	} else if (os_strcmp(argv[1], "idle_read_stop") == 0) {
		if (idle_read_flash_handle) {
			rtos_delete_thread(&idle_read_flash_handle);
			idle_read_flash_handle = NULL;
			BK_DUMP_OUT("idle_read_flash stop\n");
		}
		return;
	}

#if (CONFIG_PSRAM_AS_SYS_MEMORY)
	if (os_strcmp(argv[1], "idle_read_psram_start") == 0) {
		s_task_stop = false;
		uint32_t task_prio = os_strtoul(argv[2], NULL, 10);
		BK_DUMP_OUT("idle_read_psram start prio=%u\n", task_prio);
		rtos_create_thread(&idle_read_psram_handle, task_prio,
			"idle_read_psram",
			(beken_thread_function_t) test_idle_read_psram,
			CONFIG_APP_MAIN_TASK_STACK_SIZE,
			(beken_thread_arg_t)0);

		return;
	} else if (os_strcmp(argv[1], "idle_read_psram_stop") == 0) {
		s_task_stop = true;
		BK_DUMP_OUT("idle_read_psram stopping\n");
		return;
	} else if (os_strcmp(argv[1], "psram_cache") == 0) {
		uint32_t psram_cache_flag = os_strtoul(argv[2], NULL, 10);
		s_is_cacheable = (psram_cache_flag == 1);
		BK_DUMP_OUT("idle_read_psram cacheable=%d\n", s_is_cacheable);
		return;
	}
#endif

	if (os_strcmp(argv[1], "ID") == 0) {
		flash_dump_id();
		return;
	} else if (os_strcmp(argv[1], "U") == 0 || os_strcmp(argv[1], "UNPROTECT") == 0 || os_strcmp(argv[1], "UNPROT") == 0) {
		flash_protect_apply(true);
		return;
	} else if (os_strcmp(argv[1], "P") == 0 || os_strcmp(argv[1], "PROT") == 0 || os_strcmp(argv[1], "PROTECT") == 0) {
		flash_protect_apply(false);
		return;
	} else if (os_strcmp(argv[1], "RSR") == 0) {
		flash_dump_status_analysis();
		return;
	} else if (os_strcmp(argv[1], "WSR") == 0) {
		uint16_t sts_val = os_strtoul(argv[2], NULL, 16);
		bk_flash_write_status_reg(sts_val);
		BK_DUMP_OUT("WSR 0x%04x done, read back = 0x%04x\r\n", sts_val, bk_flash_read_status_reg());
		return;
	} else if (os_strcmp(argv[1], "CRC_ERR") == 0) {
		BK_DUMP_OUT("Flash CRC error count: %u\r\n", bk_flash_get_crc_err_num());
		return;
	} else if (os_strcmp(argv[1], "timing") == 0) {
		if (argc < 3) {
			BK_DUMP_OUT("flash_test timing <page/sector/block32/block/chip> <addr> [pattern]\r\n");
			return;
		}
		addr = (argc >= 4) ? os_strtoul(argv[3], NULL, 16) : 0x400000;
		flash_test_pattern_t pat = (argc >= 5) ? parse_pattern(argv[4]) : FLASH_PAT_INC;
		flash_timing_test(argv[2], addr, pat);
		return;
	} else if (os_strcmp(argv[1], "C") == 0) {
		addr = os_strtoul(argv[2], NULL, 16);
		len = os_strtoul(argv[3], NULL, 16);
		uint32_t test_times = os_strtoul(argv[4], NULL, 10);
		flash_test_pattern_t pat = FLASH_PAT_INC;
		flash_erase_unit_t eunit = FLASH_ERASE_SECTOR;
		if (argc >= 6) {
			if (os_strncmp(argv[5], "pattern=", 8) == 0)
				pat = parse_pattern(argv[5] + 8);
			else
				pat = parse_pattern(argv[5]);
		}
		if (argc >= 7)
			eunit = parse_erase_unit(argv[6]);
		if (!flash_addr_valid(addr, len)) {
			BK_DUMP_OUT("addr/len out of flash range (total=0x%x)\r\n", bk_flash_get_current_total_size());
			return;
		}
		test_flash_count_time(addr, len, test_times, pat, eunit, PRINT_CNT);
		return;
	}

	/* flash_test T <W/R/E> : USR_CONFIG partition op (fixed 4KB). */
	if (os_strcmp(argv[1], "T") == 0 && argc == 3) {
		test_flash_usr_config(argv[2][0]);
		return;
	}

	if (argc >= 4) {
		cmd = argv[1][0];
		addr = os_strtoul(argv[2], NULL, 16);
		len = os_strtoul(argv[3], NULL, 16);
		flash_test_pattern_t pat = (argc >= 5) ? parse_pattern(argv[4]) : FLASH_PAT_INC;

		if (!flash_addr_valid(addr, len)) {
			BK_DUMP_OUT("addr/len out of flash range (total=0x%x)\r\n", bk_flash_get_current_total_size());
			os_memcpy(pcWriteBuffer, CLI_CMD_RSP_ERROR, os_strlen(CLI_CMD_RSP_ERROR));
			return;
		}

		switch (cmd) {
		case 'E':
			test_flash_erase_verify(addr, len, true);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		case 'R':
			test_flash_read(addr, len);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		case 'W':
			test_flash_write_verify(addr, len, pat, true);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		case 'N':
			test_flash_erase_verify(addr, len, false);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		case 'M':
			test_flash_write_verify(addr, len, pat, false);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		case 'T':
			test_flash_read_time(addr, len);
			msg = CLI_CMD_RSP_SUCCEED;
			break;
		default:
			BK_DUMP_OUT("bad cmd (run 'flash_test')\r\n");
			msg = CLI_CMD_RSP_ERROR;
			break;
		}
	} else {
		BK_DUMP_OUT("bad args (run 'flash_test')\r\n");
		msg = CLI_CMD_RSP_ERROR;
	}
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static void partShow_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_partition_t i;
	bk_logic_partition_t *partition;

	for (i = BK_PARTITION_BOOTLOADER; i <= BK_PARTITIONS_TABLE_SIZE; i++) {
		partition = bk_flash_partition_get_info(i);
		if (partition == NULL)
			continue;

		BK_DUMP_OUT("%4d | %11s |  Dev:%d  | 0x%08lx | 0x%08lx |\r\n", i,
				  partition->partition_description, partition->partition_owner,
				  partition->partition_start_addr, partition->partition_length);
	};

}

#define FLASH_CMD_CNT (sizeof(s_flash_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_flash_commands[] = {
	{"fmap_test",    "flash_test memory map",      partShow_Command},
	{"flash_test",   "flash_test <cmd(R/W/E/N)>", flash_command_test},
};

int bk_flash_wr_register_cli_test_feature(void)
{
	return cli_register_module_test_feature(s_flash_commands, FLASH_CMD_CNT);
}
// eof
